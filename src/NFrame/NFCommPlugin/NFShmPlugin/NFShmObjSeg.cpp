// -------------------------------------------------------------------------
//    @FileName         :    NFShmObjSeg.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------

#include "NFShmObjSeg.h"
#include "NFComm/NFObjCommon/NFShmMgr.h"
#include "NFShmIdx.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include "NFComm/NFObjCommon/NFObject.h"
#include "NFCShmMngModule.h"

void* NFShmObjSeg::operator new(size_t nSize, void* pBuffer) throw()
{
    return pBuffer;
}

void NFShmObjSeg::operator delete(void* pMem, void* ptr)
{
}

NFShmObjSeg* NFShmObjSeg::CreateObjSeg(NFCShmMngModule* pShmModule)
{
    void* pVoid = pShmModule->CreateSegment(sizeof(NFShmObjSeg));
    NFShmObjSeg* pTmp = new(pVoid) NFShmObjSeg();
    return pTmp;
}

NFShmObjSeg::NFShmObjSeg()
{
    m_nObjSize = 0;
    m_nMemSize = 0;
    m_pObjs = nullptr;
    m_iItemCount = 0;
    m_iUseHash = false;
    m_pCreateFn = nullptr;
}

int NFShmObjSeg::SetAndInitObj(NFCShmMngModule* pShmModule, size_t nObjSize, int iItemCount, NFObject*(*pfCreateObj)(void*), bool iUseHash)
{
    m_pShmModule = pShmModule;
    m_nObjSize = nObjSize;
    m_iItemCount = iItemCount;
    m_iUseHash = iUseHash;
    m_nMemSize = 0;
    m_pCreateFn = pfCreateObj;

    /**
     * @brief idx mem
     */
    size_t idxSize = 0;
    char* pIdxBuffer = nullptr;
#ifdef SHM_OBJ_SEQ_USE_VECTOR_INDEX
    size_t idxQueeuSize = 0;
    char* pIdxQueueBuff = nullptr;
    idxSize = NFShmDyVector<NFShmIdx>::CountSize(m_iItemCount);
    pIdxBuffer = static_cast<char*>(m_pShmModule->CreateSegment(idxSize));
    if (!pIdxBuffer)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "CreateSegment Failed, Allocated share mem not enough Program exit(1)\n");
        NFSLEEP(1000);
        exit(1);
    }
    m_nMemSize += idxSize;

    idxQueeuSize = NFShmDyList<int>::CountSize(m_iItemCount);
    pIdxQueueBuff = static_cast<char*>(m_pShmModule->CreateSegment(idxQueeuSize));
    if (!pIdxQueueBuff)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "CreateSegment Failed, Allocated share mem not enough Program exit(1)\n");
        NFSLEEP(1000);
        exit(1);
    }
    m_nMemSize += idxQueeuSize;
#else
    idxSize = NFShmDyList<NFShmIdx>::CountSize(m_iItemCount);
    pIdxBuffer = static_cast<char*>(m_pShmModule->CreateSegment(idxSize));
    if (!pIdxBuffer)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "CreateSegment Failed, Allocated share mem not enough Program exit(1)\n");
        NFSLEEP(1000);
        exit(1);
    }

    m_nMemSize += idxSize;
#endif

    /**
     * @brief use hash mem
     */
    size_t hashSize = 0;
    char* pHashBuffer = nullptr;
    if (m_iUseHash)
    {
        hashSize = m_hashMgr.CountSize(m_iItemCount);
        pHashBuffer = static_cast<char*>(m_pShmModule->CreateSegment(hashSize));
        if (!pHashBuffer)
        {
            NFLogError(NF_LOG_DEFAULT, 0, "CreateSegment Failed, Allocated share mem not enough Program exit(1)\n");
            NFSLEEP(1000);
            exit(1);
        }

        m_nMemSize += hashSize;
    }
#ifdef SHM_OBJ_SEQ_USE_VECTOR_INDEX
    size_t objSize = m_iItemCount * (sizeof(int) + m_nObjSize);
#else
    size_t objSize = m_iItemCount * m_nObjSize;
#endif
    m_pObjs = static_cast<char*>(m_pShmModule->CreateSegment(objSize));
    if (!m_pObjs)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "CreateSegment Failed, Allocated share mem not enough Program exit(1)\n");
        NFSLEEP(1000);
        exit(1);
    }

    m_nMemSize += objSize;

    if (m_pShmModule->GetInitMode() == EN_OBJ_MODE_INIT)
    {
        NFShmMgr::Instance()->SetCreateMode(EN_OBJ_MODE_INIT);
        m_pShmModule->SetCreateMode(EN_OBJ_MODE_INIT);
        NFShmMgr::Instance()->SetAddrOffset(m_pShmModule->GetAddrOffset());
#ifdef SHM_OBJ_SEQ_USE_VECTOR_INDEX
        int iRet = m_idxVec.Init(pIdxBuffer, idxSize, m_iItemCount, true);
        if (iRet != 0)
        {
            NFLogError(NF_LOG_DEFAULT, 0, "m_idxVec.Init Program exit(1)\n");
            NFSLEEP(1000);
            exit(1);
        }

        iRet = m_idxQueue.Init(pIdxQueueBuff, idxQueeuSize, m_iItemCount, true);
        if (iRet != 0)
        {
            NFLogError(NF_LOG_DEFAULT, 0, "m_idxQueue.Init Program exit(1)\n");
            NFSLEEP(1000);
            exit(1);
        }
        for (int i = 0; i < m_iItemCount; i++)
        {
            m_idxQueue.push_back(i);
        }

        for (int i = 0; i < m_iItemCount; i++)
        {
            char* p = m_pObjs + i * (sizeof(int) + m_nObjSize);
            auto pInt = reinterpret_cast<int*>(p);
            *pInt = INVALID_ID;
        }
#else
        int iRet = m_idxLst.Init(pIdxBuffer, idxSize, m_iItemCount, true);
        if (iRet != 0)
        {
            NFLogError(NF_LOG_DEFAULT, 0, "m_idxLst.Init Program exit(1)\n");
            NFSLEEP(1000);
            exit(1);
        }
#endif


        if (m_iUseHash)
        {
            iRet = m_hashMgr.Init(pHashBuffer, hashSize, m_iItemCount, true);
            if (iRet != 0)
            {
                NFLogError(NF_LOG_DEFAULT, 0, "m_hashMgr.Init Program exit(1)\n");
                NFSLEEP(1000);
                exit(1);
            }
        }
    }
    else
    {
        NFShmMgr::Instance()->SetCreateMode(EN_OBJ_MODE_RECOVER);
        m_pShmModule->SetCreateMode(EN_OBJ_MODE_RECOVER);
        NFShmMgr::Instance()->SetAddrOffset(m_pShmModule->GetAddrOffset());
#ifdef SHM_OBJ_SEQ_USE_VECTOR_INDEX
        int iRet = m_idxVec.Init(pIdxBuffer, idxSize, m_iItemCount, false);
        if (iRet != 0)
        {
            NFLogError(NF_LOG_DEFAULT, 0, "m_idxVec.Init Program exit(1)\n");
            NFSLEEP(1000);
            exit(1);
        }

        iRet = m_idxQueue.Init(pIdxQueueBuff, idxQueeuSize, m_iItemCount, false);
        if (iRet != 0)
        {
            NFLogError(NF_LOG_DEFAULT, 0, "m_idxQueue.Init Program exit(1)\n");
            NFSLEEP(1000);
            exit(1);
        }
#else
        int iRet = m_idxLst.Init(pIdxBuffer, idxSize, m_iItemCount, false);
        if (iRet != 0)
        {
            NFLogError(NF_LOG_DEFAULT, 0, "m_idxLst.Init Program exit(1)\n");
            NFSLEEP(1000);
            exit(1);
        }
#endif

        FormatObj();

        if (m_iUseHash)
        {
            iRet = m_hashMgr.Init(pHashBuffer, hashSize, m_iItemCount, false);
            if (iRet != 0)
            {
                NFLogError(NF_LOG_DEFAULT, 0, "m_hashMgr.Init Program exit(1)\n");
                NFSLEEP(1000);
                exit(1);
            }
        }
    }

    return 0;
}

NFShmObjSeg::~NFShmObjSeg()
{
    //no-op
}

NFObject* NFShmObjSeg::GetObj(int iIdx)
{
#ifdef SHM_OBJ_SEQ_USE_VECTOR_INDEX
    CHECK_EXPR(iIdx >= 0 && iIdx < m_iItemCount, nullptr, "iIdx:{} error stack:{}", iIdx, TRACE_STACK());
    void* p = m_pObjs + (sizeof(int) + m_nObjSize) * iIdx;
    auto pHead = static_cast<int*>(p);
    CHECK_EXPR(pHead, nullptr, "pHead == NULL, iIdx:{} stack:{}", iIdx, TRACE_STACK());
    int iHeadIndex = *pHead;
    if (iHeadIndex >= 0 && iHeadIndex < static_cast<int>(m_idxVec.size()))
    {
        NFShmIdx& idx = m_idxVec[iHeadIndex];
        CHECK_EXPR(iIdx == idx.GetIndex() && idx.GetObjBuf() == static_cast<char*>(p) + sizeof(int), nullptr, "iIndex:{} == idx.GetIndex():{}, stack:{}", iIdx, idx.GetIndex(), TRACE_STACK());
        return idx.GetAttachedObj();
    }
    return nullptr;
#else
    auto pNode = m_idxLst.GetNode(iIdx);
    return likely(pNode && pNode->m_valid) ? pNode->m_data.GetAttachedObj() : nullptr;
#endif
}

int NFShmObjSeg::FormatObj()
{
#ifdef SHM_OBJ_SEQ_USE_VECTOR_INDEX
    for (auto iter = m_idxVec.begin(); iter != m_idxVec.end(); ++iter)
    {
        CHECK_EXPR(iter->GetIndex() >= 0 && iter->GetIndex() < m_iItemCount, -1, "iter->GetIndex():{} error stack:{}", iter->GetIndex(), TRACE_STACK());
        char* p = m_pObjs + (sizeof(int) + m_nObjSize) * iter->GetIndex();
        char* buf = p + sizeof(int);
        iter->SetObjBuf(buf);
        m_pCreateFn(buf);
    }
#else
    for (auto iter = m_idxLst.begin(); iter != m_idxLst.end(); ++iter)
    {
        void* p = m_pObjs + m_nObjSize * iter.m_node->m_self;
        iter->SetObjBuf(p);
        m_pCreateFn(p);
    }
#endif

    return 0;
}

int NFShmObjSeg::HashInsert(NFObjectHashKey key, int objId)
{
    auto iter = m_hashMgr.find(key);
    if (iter == m_hashMgr.end())
    {
        iter = m_hashMgr.emplace_hint(key, objId);
        if (iter != m_hashMgr.end())
        {
            return 0;
        }
    }

    return INVALID_ID;
}

NFObject* NFShmObjSeg::HashFind(NFObjectHashKey key)
{
    auto iter = m_hashMgr.find(key);
    if (iter != m_hashMgr.end())
    {
        int objId = iter->second;
        return GetObj(objId);
    }

    return nullptr;
}

int NFShmObjSeg::HashErase(NFObjectHashKey key)
{
    size_t count = m_hashMgr.erase(key);
    if (count > 0)
    {
        return 0;
    }

    return -1;
}

void* NFShmObjSeg::AllocMemForObject()
{
    NFShmIdx* pIdx = CreateIdx();

    if (pIdx == nullptr)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "AllocMemForObject failed! NFObjSeg alloc error ");
        return nullptr;
    }

    return pIdx->GetObjBuf();
}

int NFShmObjSeg::FreeMemForObject(void* pMem)
{
#ifdef SHM_OBJ_SEQ_USE_VECTOR_INDEX
    char* p = static_cast<char*>(pMem) - sizeof(int);
    CHECK_EXPR(p, -1, "p == NULL, stack:{}", TRACE_STACK());

    if (!p || p < m_pObjs || (p > m_pObjs + m_iItemCount * (sizeof(int) + m_nObjSize)))
    {
        return -1;
    }

    int iMemIndex = (p - m_pObjs) / (sizeof(int) + m_nObjSize);
    if ((p - m_pObjs) % (sizeof(int) + m_nObjSize) != 0)
    {
        return -1;
    }
    CHECK_EXPR(iMemIndex >= 0 && iMemIndex < m_iItemCount, -1, "iMemIndex:{} not valid, stack:{}", iMemIndex, TRACE_STACK());

    auto pHead = static_cast<int*>(static_cast<void*>(p));
    CHECK_EXPR(pHead, -1, "pHead == NULL, stack:{}", TRACE_STACK());

    int iIdx = *pHead;
    CHECK_EXPR(iIdx >= 0 && iIdx < static_cast<int>(m_idxVec.size()), -1, "iIdx:{} not valid, stack:{}", iIdx, TRACE_STACK());
    NFShmIdx& idx = m_idxVec[iIdx];
    CHECK_EXPR(iMemIndex == idx.GetIndex() && pMem == idx.GetObjBuf(), -1, "iMemIndex:{}, idx.GetIndex():{}, stack:{}", iMemIndex, idx.GetIndex(), TRACE_STACK());

    *pHead = INVALID_ID;
    m_idxQueue.push_back(iMemIndex);
    if (iIdx != static_cast<int>(m_idxVec.size()) - 1)
    {
        NFShmIdx& backIdx = m_idxVec.back();
        void* pBack = backIdx.GetObjBuf();
        CHECK_EXPR(pBack, -1, "pBack == NULL, stack:{}", TRACE_STACK());
        auto pBackHead = static_cast<char*>(pBack) - sizeof(int);
        CHECK_EXPR(pBackHead, -1, "pBackHead == NULL, stack:{}", TRACE_STACK());
        CHECK_EXPR(pBackHead >= m_pObjs && pBackHead < m_pObjs + m_iItemCount * (sizeof(int)+m_nObjSize), -1, "pBackHead not valid, stack:{}", TRACE_STACK());
        CHECK_EXPR((pBackHead - m_pObjs) % (sizeof(int)+m_nObjSize) == 0, -1, "pBackHead not valid, stack:{}", TRACE_STACK());
        int iBackIndex = (pBackHead - m_pObjs) / (sizeof(int) + m_nObjSize);
        CHECK_EXPR(iBackIndex >= 0 && iBackIndex == backIdx.GetIndex(), -1, "iBackIndex:{} not valid, stack:{}", iBackIndex, TRACE_STACK());
        auto pIntBackHead = static_cast<int*>(static_cast<void*>(pBackHead));
        CHECK_EXPR(pIntBackHead, -1, "pBackHead == NULL, stack:{}", TRACE_STACK());
        CHECK_EXPR(*pIntBackHead == static_cast<int>(m_idxVec.size()) -1, -1, "stack:{}", TRACE_STACK());
        *pIntBackHead = iIdx;
        m_idxVec[iIdx] = backIdx;
    }
    m_idxVec.pop_back();

    return 0;
#else
    int iIdx = 0;
    iIdx = GetObjId(pMem);
    CHECK_EXPR(iIdx >= 0, -1,"FreeMemForObject Failed, the pMem is no the class data");
    return DestroyIdx(iIdx);
#endif
}

NFShmIdx* NFShmObjSeg::CreateIdx()
{
#ifdef SHM_OBJ_SEQ_USE_VECTOR_INDEX
    if (m_idxQueue.empty())
    {
        NFLogError(NF_LOG_DEFAULT, 0, "CreateIdx failed! usedCount:{} allCount:{}", m_idxVec.size(), m_idxVec.max_size());
        return nullptr;
    }

    int iIndex = m_idxQueue.front();
    m_idxQueue.pop_front();
    CHECK_EXPR(iIndex >= 0 && iIndex < m_iItemCount, nullptr, "iIndex:{} not valid, stack:{}", iIndex, TRACE_STACK());
    void* p = m_pObjs + (sizeof(int) + m_nObjSize) * iIndex;

    m_idxVec.push_back(NFShmIdx());
    NFShmIdx& idx = m_idxVec.back();
    idx.SetObjBuf(static_cast<char*>(p) + sizeof(int));
    idx.SetIndex(iIndex);

    auto pHead = static_cast<int*>(p);
    CHECK_EXPR(pHead, nullptr, "pHead == NULL, statck:{}", TRACE_STACK());
    CHECK_EXPR(*pHead == INVALID_ID, nullptr, "*pHead not valid, statck:{}", TRACE_STACK());
    *pHead = static_cast<int>(m_idxVec.size()) - 1;
    return &idx;
#else
    auto iter = m_idxLst.insert(m_idxLst.end());
    if (iter == m_idxLst.end())
    {
        NFLogError(NF_LOG_DEFAULT, 0, "CreateIdx failed! usedCount:{} allCount:{}", m_idxLst.size(), m_idxLst.max_size());
        return nullptr;
    }

    iter->SetObjBuf(reinterpret_cast<char*>(m_pObjs) + m_nObjSize * iter.m_node->m_self);
    return &*iter;
#endif
}

int NFShmObjSeg::DestroyIdx(int iIdx)
{
#ifdef SHM_OBJ_SEQ_USE_VECTOR_INDEX
    CHECK_EXPR(iIdx >= 0 && iIdx < static_cast<int>(m_idxVec.size()), -1, "iIdx:{} not valid, stack:{}", iIdx, TRACE_STACK());
    if (iIdx != static_cast<int>(m_idxVec.size()) - 1)
    {
        m_idxVec[iIdx] = m_idxVec[m_idxVec.size() - 1];
    }
    m_idxVec.pop_back();

    return 0;
#else
    auto pNode = m_idxLst.GetNode(iIdx);
    if (pNode == nullptr)
    {
        return -1;
    }

    if (!pNode->m_valid)
    {
        return -2;
    }

    m_idxLst.erase(m_idxLst.GetIterator(pNode));
    return 0;
#endif
}

NFShmIdx* NFShmObjSeg::GetIdx(int iIdx)
{
#ifdef SHM_OBJ_SEQ_USE_VECTOR_INDEX
    CHECK_EXPR(iIdx >= 0 && iIdx < static_cast<int>(m_idxVec.size()), nullptr, "iIdx:{} not valid, stack:{}", iIdx, TRACE_STACK());
    return &m_idxVec[iIdx];
#else
    auto pNode = m_idxLst.GetNode(iIdx);
    if (pNode == nullptr)
    {
        return nullptr;
    }

    if (!pNode->m_valid)
    {
        return nullptr;
    }

    return &pNode->m_data;
#endif
}

NFObject* NFShmObjSeg::GetHeadObj()
{
#ifdef SHM_OBJ_SEQ_USE_VECTOR_INDEX
    auto iter = m_idxVec.begin();
    if (iter == m_idxVec.end())
    {
        return nullptr;
    }

    return iter->GetAttachedObj();
#else
    auto iter = m_idxLst.begin();
    if (iter == m_idxLst.end())
    {
        return nullptr;
    }

    return iter->GetAttachedObj();
#endif
}

NFObject* NFShmObjSeg::GetNextObj(NFObject* pObj)
{
#ifdef SHM_OBJ_SEQ_USE_VECTOR_INDEX
    char* p = static_cast<char*>(static_cast<void*>(pObj)) - sizeof(int);
    CHECK_EXPR(p, nullptr, "p == NULL, stack:{}", TRACE_STACK());

    if (!p || p < m_pObjs || (p > m_pObjs + m_iItemCount * (sizeof(int) + m_nObjSize)))
    {
        return nullptr;
    }

    int iMemIndex = (p - m_pObjs) / (sizeof(int) + m_nObjSize);
    if ((p - m_pObjs) % (sizeof(int) + m_nObjSize) != 0)
    {
        return nullptr;
    }

    auto pHead = static_cast<int*>(static_cast<void*>(p));
    CHECK_EXPR(pHead, nullptr, "pHead == NULL, stack:{}", TRACE_STACK());

    int iIdx = *pHead;
    CHECK_EXPR(iIdx >= 0 && iIdx < static_cast<int>(m_idxVec.size()), nullptr, "iIdx:{} not valid, stack:{}", iIdx, TRACE_STACK());
    NFShmIdx& idx = m_idxVec[iIdx];
    CHECK_EXPR(iMemIndex == idx.GetIndex() && pObj == idx.GetAttachedObj(), nullptr, "iMemIndex:{}, idx.GetIndex():{}, stack:{}", iMemIndex, idx.GetIndex(), TRACE_STACK());
    ++iIdx;
    if (iIdx >= 0 && iIdx < static_cast<int>(m_idxVec.size()))
    {
        return m_idxVec[iIdx].GetAttachedObj();
    }

    return nullptr;
#else
    int iIdx = -1;
    iIdx = GetObjId(pObj);
    if (iIdx < 0)
    {
        return nullptr;
    }

    auto pNode = m_idxLst.GetNode(iIdx);
    if (pNode == nullptr || !pNode->m_valid)
    {
        return nullptr;
    }

    auto pNextNode = m_idxLst.GetNode(pNode->m_next);
    if (pNextNode == nullptr || !pNextNode->m_valid)
    {
        return nullptr;
    }

    return pNextNode->m_data.GetAttachedObj();
#endif
}

bool NFShmObjSeg::IsEnd(int iIndex)
{
    if (iIndex >= m_iItemCount)
        return true;
    return false;
}

int NFShmObjSeg::GetObjId(void* pstObj) const
{
#ifdef SHM_OBJ_SEQ_USE_VECTOR_INDEX
    char* p = static_cast<char*>(pstObj) - sizeof(int);
    CHECK_EXPR(p, -1, "p == NULL, stack:{}", TRACE_STACK());

    if (!p || p < m_pObjs || (p > m_pObjs + m_iItemCount * (sizeof(int) + m_nObjSize)))
    {
        return -1;
    }

    int iIdx = (p - m_pObjs) / (sizeof(int) + m_nObjSize);

    if ((p - m_pObjs) % (sizeof(int) + m_nObjSize) != 0)
    {
        return -1;
    }

    return iIdx;
#else
    if (!pstObj || static_cast<char*>(pstObj) < reinterpret_cast<char*>(m_pObjs) || (static_cast<char*>(pstObj) > reinterpret_cast<char*>(m_pObjs) + m_iItemCount * m_nObjSize))
    {
        return -1;
    }

    int iIdx = (static_cast<char*>(pstObj) - reinterpret_cast<char*>(m_pObjs)) / m_nObjSize;

    if ((static_cast<char*>(pstObj) - reinterpret_cast<char*>(m_pObjs)) % m_nObjSize != 0)
    {
        return -1;
    }

    return iIdx;
#endif
}

int NFShmObjSeg::CreateObject()
{
    void* pBuff = nullptr;
    int iTempIdx = -1;
    pBuff = AllocMemForObject();

    if (!pBuff)
    {
        return iTempIdx;
    }

    m_pCreateFn(pBuff);
    iTempIdx = GetObjId(pBuff);
    return iTempIdx;
}

