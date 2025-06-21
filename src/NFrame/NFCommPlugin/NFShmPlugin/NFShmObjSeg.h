// -------------------------------------------------------------------------
//    @FileName         :    NFObjSeg.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------
#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFShmStl/NFShmDyList.h"
#include "NFComm/NFShmStl/NFShmDyVector.h"
#include "NFComm/NFShmStl/NFShmDyHashMapWithList.h"
#include "NFShmIdx.h"

class NFObject;
class NFShmObjSeg;
class NFCShmMngModule;

/**
    vec  m_createUseTime:9304 ms m_deleteUseTime:3557 ms m_getUseTime:38 ms
    vec m_createUseTime:9457 ms m_deleteUseTime:3496 ms m_getUseTime:35 ms
    list m_createUseTime:9627 ms m_deleteUseTime:3483 ms m_getUseTime:32 ms
    list m_createUseTime:9441 ms m_deleteUseTime:3449 ms m_getUseTime:31 ms
    分别测试了vector索引和list索引在10万次CreateObj,DestroyObj,GetObjByObjId下的效率，发现他们的效率几乎差不多,
    但在随机删除轮询tick的情况下，vector更加的平均，所以优先选择最新的vector索引方案
 */
#define SHM_OBJ_SEQ_USE_VECTOR_INDEX

class NFShmObjSeg
{
    friend class NFCShmMngModule;
public:
    void *operator new(size_t nSize, void *pBuffer) throw();

    static NFShmObjSeg* CreateObjSeg(NFCShmMngModule* pShmModule);

    void operator delete(void* pMem, void* ptr);
public:
    template <typename T>
    T* FindModule() const
    {
        return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<T>();
    }
public:
    int FormatObj();                        //格式化对象区

    void *AllocMemForObject();

    int FreeMemForObject(void *pMem);

    int CreateObject();
public:
    int HashInsert(NFObjectHashKey key, int objId);

    NFObject *HashFind(NFObjectHashKey key);

    int HashErase(NFObjectHashKey key);
public:
    NFObject *GetObj(int iIdx);

    NFShmIdx *GetIdx(int iIdx);

    int GetObjId(void *pstObj) const;
public:
    NFObject *GetHeadObj();

    NFObject *GetNextObj(NFObject *pObj);

    bool IsEnd(int iIndex);
public:
    int GetItemCount() const { return m_iItemCount; }
#ifdef SHM_OBJ_SEQ_USE_VECTOR_INDEX
    int GetUsedCount() const { return m_idxVec.size(); }
#else
    int GetUsedCount() const { return m_idxLst.size(); }
#endif

    int GetFreeCount() const { return m_iItemCount - GetUsedCount(); }

    int GetObjSize() const { return m_nObjSize; }

    int GetMemSize() const { return m_nMemSize; }

    int GetAllSize() const { return m_nMemSize + sizeof(NFShmObjSeg); }

    static int GetHashSize(int itemCount) { return NFShmDyHashMapWithList<uint64_t, int>::CountSize(itemCount); }
public:
#ifdef SHM_OBJ_SEQ_USE_VECTOR_INDEX
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
        return m_idxVec.size();
    }

    virtual size_t IterNext(size_t iPos)
    {
        ++iPos;
        return iPos;
    }

    virtual NFObject* GetIterObj(size_t iPos)
    {
        if (iPos < m_idxVec.size())
        {
            return m_idxVec[iPos].GetAttachedObj();
        }
        return nullptr;
    }

    virtual const NFObject* GetIterObj(size_t iPos) const
    {
        if (iPos < m_idxVec.size())
        {
            return m_idxVec[iPos].GetAttachedObj();
        }
        return nullptr;
    }
#else
    /**
     * @brief ShmObj类链表迭代器+1
     * @param iPos
     * @return
     */
    virtual size_t IterIncr(size_t iPos)
    {
        auto iter = m_idxLst.GetIterator(iPos);
        ++iter;
        return iter.m_node->m_self;
    }

    /**
     * @brief ShmObj类链表迭代器-1
     * @param iPos
     * @return
     */
    virtual size_t IterDecr(size_t iPos)
    {
        auto iter = m_idxLst.GetIterator(iPos);
        --iter;
        return iter.m_node->m_self;
    }

    virtual size_t IterBegin()
    {
        auto iter = m_idxLst.begin();
        return iter.m_node->m_self;
    }

    virtual size_t IterEnd()
    {
        auto iter = m_idxLst.end();
        return iter.m_node->m_self;
    }

    virtual size_t IterNext(size_t iPos)
    {
        auto iter = m_idxLst.GetIterator(iPos);
        ++iter;
        return iter.m_node->m_self;
    }

    virtual NFObject* GetIterObj(size_t iPos)
    {
        auto iter = m_idxLst.GetIterator(iPos);
        return iter->GetAttachedObj();
    }

    virtual const NFObject* GetIterObj(size_t iPos) const
    {
        auto iter = m_idxLst.GetIterator(iPos);
        return iter->GetAttachedObj();
    }
#endif

protected:
    NFShmIdx *CreateIdx();

    int DestroyIdx(int iIdx);
private:
    NFShmObjSeg();

    ~NFShmObjSeg();

    int SetAndInitObj(NFCShmMngModule* pShmModule, size_t nObjSize, int iItemCount, NFObject*(*pfCreateObj)(void*), bool iUseHash = false);

    size_t m_nObjSize;
    size_t m_nMemSize;
#ifdef SHM_OBJ_SEQ_USE_VECTOR_INDEX
    NFShmDyVector<NFShmIdx> m_idxVec;
    NFShmDyList<int> m_idxQueue;
#else
    NFShmDyList<NFShmIdx> m_idxLst;
#endif
    char* m_pObjs;
    NFShmDyHashMapWithList<NFObjectHashKey, int> m_hashMgr;
    int m_iItemCount;
    bool m_iUseHash;

    NFObject *(*m_pCreateFn)(void *);
    NFCShmMngModule* m_pShmModule;
};


