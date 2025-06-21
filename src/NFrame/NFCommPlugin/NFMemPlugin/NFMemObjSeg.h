// -------------------------------------------------------------------------
//    @FileName         :    NFObjSeg.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------
#pragma once

#include <NFComm/NFPluginModule/NFChunkPool.h>

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFShmStl/NFShmDyList.h"
#include "NFComm/NFShmStl/NFShmDyVector.h"
#include "NFComm/NFShmStl/NFShmDyHashMapWithList.h"
#include "NFMemIdx.h"

class NFObject;
class NFCMemMngModule;
class NFMemObjSeg;

class NFMemObjSeg
{
    friend class NFCMemMngModule;
public:
    template <typename T>
    T* FindModule() const
    {
        return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<T>();
    }
public:
    void *AllocMemForObject();

    int FreeMemForObject(void *pMem);
public:
    int HashInsert(NFObjectHashKey key, int objId);

    NFObject* HashFind(NFObjectHashKey key);

    int HashErase(NFObjectHashKey key);
public:
    NFObject* GetObj(int iIdx);
    int GetObjId(void *pstObj) const;
public:
    NFObject* GetHeadObj();

    NFObject* GetNextObj(NFObject* pObj);

    bool IsEnd(int iIndex);
public:
    int GetItemCount() const { return m_pObjectPool->GetAllCount(); }

    int GetUsedCount() const { return m_pObjectPool->GetUsedCount(); }

    int GetFreeCount() const { return m_pObjectPool->GetFreeCount(); }

    int GetObjSize() const { return m_nObjSize; }

    int GetMemSize() const { return GetItemCount() * m_nObjSize; }

    static int GetHashSize(int itemCount) { return NFShmDyHashMapWithList<uint64_t, int>::CountSize(itemCount); }
public:
    /**
     * @brief ShmObj类链表迭代器+1
     * @param iPos
     * @return
     */
    virtual size_t IterIncr(size_t iPos)
    {
        ++iPos;
        return iPos;
    }

    /**
     * @brief ShmObj类链表迭代器-1
     * @param iPos
     * @return
     */
    virtual size_t IterDecr(size_t iPos)
    {
        --iPos;
        return iPos;
    }

    virtual size_t IterBegin()
    {
        return 0;
    }

    virtual size_t IterEnd()
    {
        return m_objIndexVec.size();
    }

    virtual size_t IterNext(size_t iPos)
    {
        ++iPos;
        return iPos;
    }

    virtual NFObject* GetIterObj(size_t iPos)
    {
        if (iPos < m_objIndexVec.size())
        {
            return m_objIndexVec[iPos].GetAttachedObj();
        }
        return nullptr;
    }

    virtual const NFObject* GetIterObj(size_t iPos) const
    {
        if (iPos < m_objIndexVec.size())
        {
            return m_objIndexVec[iPos].GetAttachedObj();
        }
        return nullptr;
    }

    std::vector<NFMemIdx>& GetObjIndexVec() { return m_objIndexVec; }
    const std::vector<NFMemIdx>& GetObjIndexVec() const { return m_objIndexVec; }
private:
    NFMemObjSeg();

    ~NFMemObjSeg();

    int SetAndInitObj(NFCMemMngModule* pShmModule, size_t nObjSize, int iItemCount, NFObject*(*pfCreateObj)(void*), bool iUseHash = false);

    size_t m_nObjSize;
    std::unordered_map<NFObjectHashKey, int> m_hashMgr;
    bool m_iUseHash;

    NFObject*(*m_pCreateFn)(void*);
    NFCMemMngModule* m_pShmModule;

    NFChunkPool* m_pObjectPool;
    std::vector<NFMemIdx> m_objIndexVec;
};


