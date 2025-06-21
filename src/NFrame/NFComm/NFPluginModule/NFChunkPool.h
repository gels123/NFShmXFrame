// -------------------------------------------------------------------------
//    @FileName         :    NFChunkPool.h
//    @Author           :    gaoyi
//    @Date             :    2022/10/10
//    @Email			:    445267987@qq.com
//    @Module           :    NFChunkPool
//
// -------------------------------------------------------------------------

#pragma once

#include <vector>

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFCore/NFDoubleList.h"

struct NFMemBlock {
    uint32_t m_freeIdx;
    uint32_t m_freeNum;
    void *m_freeList;
    NFDoubleNode m_blockList;

    NFMemBlock()
        : m_freeIdx(0),
          m_freeNum(0),
          m_freeList(nullptr),
          m_blockList(reinterpret_cast<void *>(this)) {
    }
};

class NFChunkPool {
public:
    NFChunkPool(uint32_t reserveSize,
                uint32_t chunkSize,
                uint32_t chunkCount,
                bool freeToSys);

    virtual ~NFChunkPool();

    void *AllocChunk();

    bool FreeChunk(void *chunk);

    uint32_t ChunkSize() const;

    int GetChunkIndex(void *chunk) const;

    void *GetChunkByIndex(int iIndex) const;

    uint32_t GetAllCount() const { return m_allCount; }
    uint32_t GetFreeCount() const { return m_freeCount; }
    uint32_t GetUsedCount() const { return m_allCount - m_freeCount; }
protected:
    NFMemBlock *AllocBlock();
    void FreeBlock(NFMemBlock *pBlock);

    void *AllocChunk(NFMemBlock *block);

    bool FreeChunk(NFMemBlock *block, void *chunk);

    int GetChunkIndex(NFMemBlock *block, void *chunk) const;

    void FreeBlockList(NFDoubleList &list);

    void *&NextOf(void * chunk);

protected:
    bool m_freeToSys;
    uint32_t m_reserveSize;
    uint32_t m_chunkSize;
    uint32_t m_chunkCount;
    uint32_t m_freeCount;
    uint32_t m_allCount;
    uint32_t m_freeThreshold;
    size_t m_memSize;
    NFDoubleList m_blockList;
    NFDoubleList m_emptyList;
    NFDoubleList m_fullList;
    std::vector<NFMemBlock *> m_vecBlock;
};

#ifndef  CHECK_MEM_LEAK
#define  CHECK_MEM_LEAK
#endif // CHECK_MEM_LEAK

