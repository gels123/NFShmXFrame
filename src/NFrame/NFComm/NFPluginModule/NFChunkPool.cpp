// -------------------------------------------------------------------------
//    @FileName         :    NFChunkPool.cpp
//    @Author           :    gaoyi
//    @Date             :    2022/10/10
//    @Email			:    445267987@qq.com
//    @Module           :    NFChunkPool
//
// -------------------------------------------------------------------------

#include "NFChunkPool.h"

#include "NFStackTrace.h"
#include "NFComm/NFCore/NFLikely.h"
#include "NFComm/NFPluginModule/NFCheck.h"

NFChunkPool::NFChunkPool(uint32_t reserveSize,
                         uint32_t chunkSize,
                         uint32_t chunkCount,
                         bool freeToSys)
{
    m_reserveSize = reserveSize;
    m_freeCount = 0;
    m_chunkSize = (chunkSize < sizeof(void*)) ? sizeof(void*) : chunkSize;
    m_chunkCount = (chunkCount < 2) ? 2 : chunkCount;
    m_freeToSys = freeToSys;
    m_freeThreshold = (m_chunkCount * 2);
    m_memSize = static_cast<size_t>(m_chunkSize + m_reserveSize) * m_chunkCount;
    m_allCount = 0;
}

NFChunkPool::~NFChunkPool()
{
    FreeBlockList(m_blockList);
    FreeBlockList(m_fullList);
    FreeBlockList(m_emptyList);
}

void* NFChunkPool::AllocChunk()
{
    void* chunk = nullptr;
    NFMemBlock* block = nullptr;
    if (!m_blockList.IsEmpty())
    {
        block = static_cast<NFMemBlock*>(m_blockList.Head()->GetHost());
        chunk = AllocChunk(block);
        NF_ASSERT(chunk);
        if (block->m_freeNum == 0)
        {
            m_blockList.Delete(&block->m_blockList);
            m_fullList.InsertHead(&block->m_blockList);
        }
    }
    else if (!m_emptyList.IsEmpty())
    {
        block = static_cast<NFMemBlock*>(m_emptyList.Head()->GetHost());
        chunk = AllocChunk(block);
        NF_ASSERT(chunk);
        m_emptyList.Delete(&block->m_blockList);
        m_blockList.InsertHead(&block->m_blockList);
    }
    else
    {
        block = AllocBlock();
        if (block)
        {
            chunk = AllocChunk(block);
            NF_ASSERT(chunk);
            m_blockList.InsertHead(&block->m_blockList);
        }
    }
    if (chunk)
    {
        m_freeCount--;
    }
    return chunk;
}

bool NFChunkPool::FreeChunk(void* chunk)
{
    NFDoubleNode* curr = m_blockList.Head();
    while (curr)
    {
        auto block = static_cast<NFMemBlock*>(curr->GetHost());
        if (FreeChunk(block, chunk))
        {
            m_freeCount++;
            if (UNLIKELY(block->m_freeNum == m_chunkCount))
            {
                if (m_freeToSys && (m_freeCount > m_freeThreshold))
                {
                    // free one block to system
                    m_blockList.Delete(curr);
                    FreeBlock(block);
                }
                else
                {
                    m_blockList.Delete(curr);
                    m_emptyList.InsertHead(curr);
                }
            }
            return true;
        }
        else
        {
            curr = m_blockList.Next(curr);
        }
    }
    curr = m_fullList.Head();
    while (curr)
    {
        auto block = static_cast<NFMemBlock*>(curr->GetHost());
        if (FreeChunk(block, chunk))
        {
            m_freeCount++;
            m_fullList.Delete(curr);
            m_blockList.InsertHead(curr);
            return true;
        }
        else
        {
            curr = m_fullList.Next(curr);
        }
    }
    return false;
}

uint32_t NFChunkPool::ChunkSize() const
{
    return m_chunkSize;
}

int NFChunkPool::GetChunkIndex(void* chunk) const
{
    for (size_t i = 0; i < m_vecBlock.size(); ++i)
    {
        NFMemBlock *pBlock = m_vecBlock[i];
        if (pBlock)
        {
            int iChunkIndex = GetChunkIndex(pBlock, chunk);
            if (iChunkIndex >= 0)
            {
                return i * m_chunkCount + iChunkIndex;
            }
        }
    }
    return -1;
}

void* NFChunkPool::GetChunkByIndex(int iIndex) const
{
    int iBlockIndex = iIndex / m_chunkCount;
    int iObjIndex = iIndex % m_chunkCount;
    if (iBlockIndex < 0 || iBlockIndex >= static_cast<int>(m_vecBlock.size()))
    {
        return nullptr;
    }
    void* pBlock = m_vecBlock[iBlockIndex];
    CHECK_NULL_RE_NULL(0, pBlock, "iIndex:{} stack:{}", iIndex, TRACE_STACK());

    size_t start = reinterpret_cast<size_t>(pBlock) + sizeof(NFMemBlock);
    size_t pos = start + iObjIndex * (m_chunkSize + m_reserveSize);
    if ((pos < start) || (pos > (start + m_memSize - m_chunkSize - m_reserveSize)))
    {
        return nullptr;
    }

    return reinterpret_cast<void*>(pos);
}

NFMemBlock* NFChunkPool::AllocBlock()
{
    void* ptr = malloc(m_memSize + sizeof(NFMemBlock));
    auto block = static_cast<NFMemBlock*>(ptr);
    if (block)
    {
        block->m_freeIdx = 0;
        block->m_freeList = nullptr;
        block->m_freeNum = m_chunkCount;
        block->m_blockList.Reset();
        block->m_blockList.SetHost(block);
        m_freeCount += m_chunkCount;
        m_allCount += m_chunkCount;
        bool flag = false;
        for (size_t i = 0; i < m_vecBlock.size(); ++i)
        {
            if (m_vecBlock[i] == nullptr)
            {
                m_vecBlock[i] = block;
                flag = true;
                break;
            }
        }
        if (!flag)
        {
            m_vecBlock.push_back(block);
        }
    }
    return block;
}

void NFChunkPool::FreeBlock(NFMemBlock* pBlock)
{
    NF_ASSERT(pBlock);
    bool flag = false;
    for (size_t i = 0; i < m_vecBlock.size(); ++i)
    {
        if (m_vecBlock[i] == pBlock)
        {
            m_vecBlock[i] = nullptr;
            flag = true;
            break;
        }
    }

    if (!flag)
    {
        LOG_ERR(0, -1, "not find the block int m_vecBlock, stack:{}", TRACE_STACK());
    }

    free(pBlock);
    m_freeCount -= m_chunkCount;
    m_allCount -= m_chunkCount;
}

void* NFChunkPool::AllocChunk(NFMemBlock* block)
{
    NF_ASSERT(block);
    void* chunk = nullptr;
    char* start = (reinterpret_cast<char*>(block)) + sizeof(NFMemBlock);
    // free list first
    if (block->m_freeList)
    {
        chunk = block->m_freeList;
        block->m_freeList = NextOf(block->m_freeList);
    }
    // unused chunk
    else if (block->m_freeIdx < m_chunkCount)
    {
        chunk = (start + block->m_freeIdx * (m_chunkSize + m_reserveSize));
        block->m_freeIdx++;
    }
    else
    {
        chunk = nullptr;
    }
    if (chunk)
    {
        NF_ASSERT(block->m_freeNum > 0);
        block->m_freeNum--;
    }
    return chunk;
}

int NFChunkPool::GetChunkIndex(NFMemBlock* block, void* chunk) const
{
    NF_ASSERT(block);
    NF_ASSERT(chunk);
    size_t pos = reinterpret_cast<size_t>(chunk);
    size_t start = reinterpret_cast<size_t>(block) + sizeof(NFMemBlock);
    if ((pos < start) || (pos > (start + m_memSize - m_chunkSize - m_reserveSize)))
    {
        return -1;
    }

    if ((pos - start) % (m_chunkSize + m_reserveSize) != 0)
    {
        return -1;
    }

    return static_cast<int>((pos - start) / (m_chunkSize + m_reserveSize));
}

bool NFChunkPool::FreeChunk(NFMemBlock* block, void* chunk)
{
    NF_ASSERT(block);
    NF_ASSERT(chunk);
    size_t pos = reinterpret_cast<size_t>(chunk);
    size_t start = reinterpret_cast<size_t>(block) + sizeof(NFMemBlock);
    if ((pos < start) || (pos > (start + m_memSize - m_chunkSize - m_reserveSize)))
    {
        return false;
    }
    NextOf(chunk) = block->m_freeList;
    block->m_freeList = chunk;
    block->m_freeNum++;
    NF_ASSERT(block->m_freeNum <= m_chunkCount);
    return true;
}

void NFChunkPool::FreeBlockList(NFDoubleList& list)
{
    NFDoubleNode* curr = nullptr;
    NFMemBlock* block = nullptr;
    curr = list.Head();
    while (curr)
    {
        block = static_cast<NFMemBlock*>(curr->GetHost());
        curr = list.Next(curr);
        free(block);
    }
}

void*& NFChunkPool::NextOf(void* const chunk)
{
    return *(static_cast<void**>(chunk));
}
