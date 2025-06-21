// -------------------------------------------------------------------------
//    @FileName         :    NFMemObjSeg.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------

#include "NFMemObjSeg.h"
#include "NFComm/NFObjCommon/NFShmMgr.h"
#include "NFMemIdx.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include "NFComm/NFObjCommon/NFObject.h"
#include "NFCMemMngModule.h"

NFMemObjSeg::NFMemObjSeg()
{
    m_nObjSize = 0;
    m_iUseHash = false;
    m_pCreateFn = nullptr;
    m_pObjectPool = nullptr;
}

int NFMemObjSeg::SetAndInitObj(NFCMemMngModule* pShmModule, size_t nObjSize, int iItemCount, NFObject*(*pfCreateObj)(void*), bool iUseHash)
{
    m_pShmModule = pShmModule;
    m_nObjSize = nObjSize;
    m_iUseHash = iUseHash;
    m_pCreateFn = pfCreateObj;

    int chunkCount = iItemCount > 500 ? 500 : iItemCount;
    chunkCount = chunkCount <= 0 ? 1 : chunkCount;
    m_pObjectPool = new NFChunkPool(sizeof(int), nObjSize, chunkCount, true);

    NFShmMgr::Instance()->SetCreateMode(EN_OBJ_MODE_INIT);
    m_pShmModule->SetCreateMode(EN_OBJ_MODE_INIT);

    return 0;
}

NFMemObjSeg::~NFMemObjSeg()
{
    NF_SAFE_DELETE(m_pObjectPool);
}

NFObject* NFMemObjSeg::GetObj(int iIdx)
{
    CHECK_NULL_RE_NULL(0, m_pObjectPool, "");
    void *p = m_pObjectPool->GetChunkByIndex(iIdx);
    auto pHead = static_cast<int*>(p);
    CHECK_EXPR(pHead, nullptr, "pHead == NULL, iIdx:{} stack:{}", iIdx, TRACE_STACK());
    int iHeadIndex = *pHead;
    CHECK_EXPR(iHeadIndex >= 0 && iHeadIndex < (int)m_objIndexVec.size(), nullptr, "iHeadIndex:{} not valid, stack:{}", iHeadIndex, TRACE_STACK());
    NFMemIdx& idx = m_objIndexVec[iHeadIndex];
    CHECK_EXPR(iIdx == idx.GetIndex(), nullptr, "iIndex == idx.GetIndex(), stack:{}", TRACE_STACK());
    return idx.GetAttachedObj();
}

int NFMemObjSeg::HashInsert(NFObjectHashKey key, int objId)
{
    auto iter = m_hashMgr.find(key);
    if (iter == m_hashMgr.end())
    {
        m_hashMgr.insert(std::make_pair(key, objId));
        return 0;
    }

    return INVALID_ID;
}

NFObject* NFMemObjSeg::HashFind(NFObjectHashKey key)
{
    auto iter = m_hashMgr.find(key);
    if (iter != m_hashMgr.end())
    {
        int objId = iter->second;
        return GetObj(objId);
    }

    return nullptr;
}

int NFMemObjSeg::HashErase(NFObjectHashKey key)
{
    size_t count = m_hashMgr.erase(key);
    if (count > 0)
    {
        return 0;
    }

    return -1;
}

void* NFMemObjSeg::AllocMemForObject()
{
    CHECK_NULL_RE_NULL(0, m_pObjectPool, "m_pObjectPool == NULL");
    void* p = m_pObjectPool->AllocChunk();
    CHECK_EXPR(p, nullptr, "p == NULL, statck:{}", TRACE_STACK());
    int iIndex = m_pObjectPool->GetChunkIndex(p);
    CHECK_EXPR(iIndex >= 0, nullptr, "iIndex:{} not valid, stack:{}", iIndex, TRACE_STACK());
    NFMemIdx idx;
    idx.SetObjBuf(reinterpret_cast<void*>(reinterpret_cast<size_t>(p)+sizeof(int)));
    idx.SetIndex(iIndex);
    m_objIndexVec.push_back(idx);
    auto pHead = static_cast<int*>(p);
    CHECK_EXPR(pHead, nullptr, "pHead == NULL, statck:{}", TRACE_STACK());
    *pHead = static_cast<int>(m_objIndexVec.size()) - 1;
    return idx.GetObjBuf();
}

int NFMemObjSeg::FreeMemForObject(void* pMem)
{
    CHECK_NULL(0, pMem);
    CHECK_NULL(0, m_pObjectPool);
    auto p = reinterpret_cast<void*>(reinterpret_cast<size_t>(pMem) - sizeof(int));
    CHECK_EXPR(p, -1, "p == NULL, stack:{}", TRACE_STACK());
    int iIndex = m_pObjectPool->GetChunkIndex(p);
    CHECK_EXPR(iIndex >= 0, -1, "iIndex:{} not valid, stack:{}", iIndex, TRACE_STACK());
    auto pHead = static_cast<int*>(p);
    CHECK_EXPR(pHead, -1, "pHead == NULL, stack:{}", TRACE_STACK());
    int iHeadIndex = *pHead;
    CHECK_EXPR(iHeadIndex >= 0 && iHeadIndex < (int)m_objIndexVec.size(), -1, "iHeadIndex:{} not valid, stack:{}", iHeadIndex, TRACE_STACK());
    NFMemIdx& idx = m_objIndexVec[iHeadIndex];
    CHECK_EXPR(pMem == idx.GetObjBuf(), -1, "pMem != idx.GetObjBuf(), stack:{}", TRACE_STACK());
    CHECK_EXPR(iIndex == idx.GetIndex(), -1, "iIndex == idx.GetIndex(), stack:{}", TRACE_STACK());
    if (iHeadIndex == static_cast<int>(m_objIndexVec.size()) - 1)
    {
        m_objIndexVec.pop_back();
    }
    else
    {
        NFMemIdx& backIdx = m_objIndexVec.back();
        void* pBack = backIdx.GetObjBuf();
        CHECK_EXPR(pBack, -1, "pBack == NULL, stack:{}", TRACE_STACK());
        auto pBackHead = reinterpret_cast<void*>(reinterpret_cast<size_t>(pBack) - sizeof(int));
        CHECK_EXPR(pBackHead, -1, "pBackHead == NULL, stack:{}", TRACE_STACK());
        int iBackIndex = m_pObjectPool->GetChunkIndex(pBackHead);
        CHECK_EXPR(iBackIndex >= 0 && iBackIndex == backIdx.GetIndex(), -1, "iBackIndex:{} not valid, stack:{}", iBackIndex, TRACE_STACK());
        auto pIntBackHead = static_cast<int*>(pBackHead);
        CHECK_EXPR(pIntBackHead, -1, "pBackHead == NULL, stack:{}", TRACE_STACK());
        CHECK_EXPR(*pIntBackHead == static_cast<int>(m_objIndexVec.size()) -1, -1, "stack:{}", TRACE_STACK());
        *pIntBackHead = iHeadIndex;

        m_objIndexVec[iHeadIndex] = backIdx;
        m_objIndexVec.pop_back();
    }

    m_pObjectPool->FreeChunk(p);
    return 0;
}

NFObject* NFMemObjSeg::GetHeadObj()
{
    if (m_objIndexVec.empty())
    {
        return nullptr;
    }

    return m_objIndexVec.front().GetAttachedObj();
}

NFObject* NFMemObjSeg::GetNextObj(NFObject* pObj)
{
    CHECK_NULL_RET_VAL(0, m_pObjectPool, nullptr);
    auto p = reinterpret_cast<void*>(reinterpret_cast<size_t>(pObj) - sizeof(int));
    CHECK_EXPR(p, nullptr, "p == NULL, stack:{}", TRACE_STACK());
    int iIndex = m_pObjectPool->GetChunkIndex(p);
    CHECK_EXPR(iIndex >= 0, nullptr, "iIndex:{} not valid, stack:{}", iIndex, TRACE_STACK());
    auto pHead = static_cast<int*>(p);
    CHECK_EXPR(pHead, nullptr, "pHead == NULL, stack:{}", TRACE_STACK());
    int iHeadIndex = *pHead;
    CHECK_EXPR(iHeadIndex >= 0 && iHeadIndex < (int)m_objIndexVec.size(), nullptr, "iHeadIndex:{} not valid, stack:{}", iHeadIndex, TRACE_STACK());
    NFMemIdx& idx = m_objIndexVec[iHeadIndex];
    CHECK_EXPR(pObj == idx.GetAttachedObj(), pObj, "pMem != idx.GetObjBuf(), stack:{}", TRACE_STACK());
    CHECK_EXPR(iIndex == idx.GetIndex(), pObj, "iIndex == idx.GetIndex(), stack:{}", TRACE_STACK());
    ++iHeadIndex;
    if (iHeadIndex >= 0 && iHeadIndex < static_cast<int>(m_objIndexVec.size()))
    {
        NFMemIdx& idxNext = m_objIndexVec[iHeadIndex];
        return idxNext.GetAttachedObj();
    }
    return nullptr;
}

bool NFMemObjSeg::IsEnd(int iIndex)
{
    if (iIndex >= static_cast<int>(m_objIndexVec.size()))
        return true;
    return false;
}

int NFMemObjSeg::GetObjId(void* pstObj) const
{
    CHECK_NULL_RET_VAL(0, m_pObjectPool, INVALID_ID);
    auto p = reinterpret_cast<void*>(reinterpret_cast<size_t>(pstObj) - sizeof(int));
    CHECK_EXPR(p, -1, "p == NULL, stack:{}", TRACE_STACK());
    return m_pObjectPool->GetChunkIndex(p);
}
