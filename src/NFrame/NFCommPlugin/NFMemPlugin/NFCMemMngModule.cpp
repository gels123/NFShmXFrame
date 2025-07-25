// -------------------------------------------------------------------------
//    @FileName         :    NFCMemMngModule.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFKernelPlugin
//
// -------------------------------------------------------------------------

#include "NFCMemMngModule.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"

#include <cassert>
#include <errno.h>
#include "NFComm/NFCore/NFStringUtility.h"
#include "NFComm/NFCore/NFFileUtility.h"
#include "NFMemGlobalId.h"
#include "NFMemObjSeg.h"
#include "NFComm/NFObjCommon/NFShmMgr.h"
#include "NFComm/NFObjCommon/NFTypeDefines.h"
#include "NFComm/NFObjCommon/NFRawObject.h"
#include "NFMemIdx.h"
#include "NFMemTimerMng.h"
#include "NFMemTransMng.h"
#include "NFComm/NFCore/NFTime.h"
#include "NFComm/NFPluginModule/NFIPlugin.h"
#include "NFMemEventMgr.h"

NFCMemMngModule::NFCMemMngModule(NFIPluginManager* p) : NFIMemMngModule(p)
{
    m_pObjPluginManager = p;
    m_enRunMode = EN_OBJ_MODE_INIT;
    m_enCreateMode = EN_OBJ_MODE_INIT;
    m_siShmSize = 0;

    m_iObjSegSizeTotal = 0;
    m_iTotalObjCount = 0;
    m_nObjSegSwapCounter.resize(EOT_MAX_TYPE);
    m_pGlobalId = nullptr;
}

NFCMemMngModule::~NFCMemMngModule()
{
}

bool NFCMemMngModule::AfterLoadAllPlugin()
{
    std::list<NFIPlugin*> listPlugin = m_pObjPluginManager->GetListPlugin();

    for (auto iter = listPlugin.begin(); iter != listPlugin.end(); ++iter)
    {
        NFIPlugin* pPlugin = *iter;
        if (pPlugin)
        {
            pPlugin->InitShmObjectRegister();
        }
    }

    /*
        初始化共享内存里的对象
    */
    InitializeAllObj();

    /*
        创建一些全局性对象
    */
    InitMemObjectGlobal();

    return true;
}

bool NFCMemMngModule::Execute()
{
    auto pTimerMng = dynamic_cast<NFMemTimerMng*>(GetHeadObj(EOT_TYPE_TIMER_MNG));
    if (pTimerMng)
    {
        pTimerMng->OnTick(NF_ADJUST_TIMENOW_MS());
    }
    auto pTransManager = dynamic_cast<NFMemTransMng*>(GetHeadObj(EOT_TRANS_MNG));
    if (pTransManager)
    {
        pTransManager->TickNow(m_pObjPluginManager->GetCurFrameCount());
    }
    return true;
}

bool NFCMemMngModule::Finalize()
{
    return true;
}

/**
* 共享内存模式
*/
EN_OBJ_MODE NFCMemMngModule::GetInitMode()
{
    return EN_OBJ_MODE_INIT;
}

void NFCMemMngModule::SetInitMode(EN_OBJ_MODE mode)
{
}

/**
* 共享内存创建对象模式
*/
EN_OBJ_MODE NFCMemMngModule::GetCreateMode()
{
    return m_enCreateMode;
}

/**
* 共享内存创建对象模式
*/
void NFCMemMngModule::SetCreateMode(EN_OBJ_MODE mode)
{
    m_enCreateMode = mode;
}

EN_OBJ_MODE NFCMemMngModule::GetRunMode()
{
    return m_enRunMode;
}

/**
* @brief  对象seq自增
*/
int NFCMemMngModule::IncreaseObjSeqNum()
{
    return 0;
}

/**
* @brief  NFMemGlobalId
*/
NFMemGlobalId* NFCMemMngModule::GetGlobalId() const
{
    return m_pGlobalId;
}

/**
* @brief  设置功能内存初始化成功
*/
void NFCMemMngModule::SetShmInitSuccessFlag()
{

}

std::string NFCMemMngModule::GetClassName(int bType)
{
    NF_ASSERT_MSG((int) bType < (int) m_nObjSegSwapCounter.size(), "bType < (int)m_nObjSegSwapCounter.size()");
    return m_nObjSegSwapCounter[bType].m_szClassName;
}

int NFCMemMngModule::GetClassType(int bType)
{
    NF_ASSERT_MSG((int) bType < (int) m_nObjSegSwapCounter.size(), "bType < (int)m_nObjSegSwapCounter.size()");
    return m_nObjSegSwapCounter[bType].m_iObjType;
}

NFMemObjSegSwapCounter* NFCMemMngModule::CreateCounterObj(int bType)
{
    NF_ASSERT_MSG((int) bType < (int) m_nObjSegSwapCounter.size(), "bType < (int)m_nObjSegSwapCounter.size()");
    return &m_nObjSegSwapCounter[bType];
}

int NFCMemMngModule::InitAllObjSeg()
{
    int iRet = 0;

    for (int i = 0; i < static_cast<int>(m_nObjSegSwapCounter.size()); i++)
    {
        m_nObjSegSwapCounter[i].m_iObjType = i;
        if (m_nObjSegSwapCounter[i].m_nObjSize > 0 && m_nObjSegSwapCounter[i].m_iItemCount > 0)
        {
            auto pObjSeg = new NFMemObjSeg();
            NFMemObjSegSwapCounter* pObjSegSwapCounter = &m_nObjSegSwapCounter[i];
            pObjSegSwapCounter->SetObjSeg(pObjSeg);
            NFShmMgr::Instance()->m_iType = i;
            iRet = pObjSeg->SetAndInitObj(this, pObjSegSwapCounter->m_nObjSize,
                                          pObjSegSwapCounter->m_iItemCount,
                                          pObjSegSwapCounter->m_pResumeFn, pObjSegSwapCounter->m_iUseHash);

            NFShmMgr::Instance()->m_iType = INVALID_ID;

            if (iRet)
            {
                NFLogInfo(NF_LOG_DEFAULT, 0, "NFMemObjSeg::InitAllObj failed!");
                return iRet;
            }
        }
    }

    return 0;
}

void
NFCMemMngModule::RegisterClassToObjSeg(int bType, size_t nObjSize, int iItemCount, NFObject*(*pfResumeObj)(void*),
                                       NFObject*(*pCreateFn)(),
                                       void (*pDestroy)(NFObject*), int parentType, const std::string& pszClassName,
                                       bool useHash, bool singleton)
{
    NFMemObjSegSwapCounter* pCounter = CreateCounterObj(bType);
    if (pCounter->m_nObjSize > 0)
    {
        NF_ASSERT(pCounter->m_nObjSize == nObjSize);
        NF_ASSERT(pCounter->m_iObjType == bType);
        NF_ASSERT(pCounter->m_singleton == singleton);
        NF_ASSERT(pCounter->m_pResumeFn == pfResumeObj);
        NF_ASSERT(pCounter->m_szClassName == pszClassName);
        NF_ASSERT(pCounter->m_pCreateFn == pCreateFn);
        NF_ASSERT(pCounter->m_pDestroyFn == pDestroy);

        NF_ASSERT(pCounter->m_iUseHash == useHash);
        if (singleton)
        {
            return;
        }
    }
    pCounter->m_nObjSize = nObjSize;
    pCounter->m_iItemCount += iItemCount;
    pCounter->m_iObjType = bType;
    pCounter->m_singleton = singleton;

    if (pCounter->m_iItemCount < 0)
    {
        pCounter->m_iItemCount = 0;
    }

    if (singleton)
    {
        pCounter->m_iItemCount = 1;
    }

    pCounter->m_pResumeFn = pfResumeObj;
    pCounter->m_szClassName = pszClassName;

    pCounter->m_pCreateFn = pCreateFn;
    pCounter->m_pDestroyFn = pDestroy;
    if (parentType < 0)
    {
        pCounter->m_pParent = nullptr;
    }
    else
    {
        pCounter->m_pParent = GetObjSegSwapCounter(parentType);
        NFMemObjSegSwapCounter* pParentClass = pCounter->m_pParent;
        while (pParentClass)
        {
            pParentClass->m_childrenObjType.insert(bType);
            if (pCounter->m_parentObjType.find(pParentClass->m_iObjType) != pCounter->m_parentObjType.end())
            {
                NFLogError(NF_LOG_DEFAULT, 0, "class {} has the parent class:{}, circle dead", pszClassName, pParentClass->m_iObjType);
                NF_ASSERT(false);
            }
            pCounter->m_parentObjType.insert(pParentClass->m_iObjType);
            pParentClass = pParentClass->m_pParent;
        }
    }
    pCounter->m_pObjSeg = nullptr;
    pCounter->m_szClassName = pszClassName;
    pCounter->m_iUseHash = useHash;
}

void NFCMemMngModule::UnRegisterClassToObjSeg(int bType)
{
    // 获取对象类型对应的计数器
    NFMemObjSegSwapCounter* pCounter = CreateCounterObj(bType);

    // 如果存在父类关系，需要清理继承链
    if (pCounter->m_pParent != nullptr)
    {
        NFMemObjSegSwapCounter* pParentClass = pCounter->m_pParent;
        while (pParentClass)
        {
            // 从父类的子类列表中移除当前类型
            pParentClass->m_childrenObjType.erase(bType);
            // 从当前类型的父类列表中移除父类
            pCounter->m_parentObjType.erase(pParentClass->m_iObjType);
            pParentClass = pParentClass->m_pParent;
        }
    }
    
    // 清理计数器对象，释放相关资源
    pCounter->clear();
}

void NFMemObjSegSwapCounter::SetObjSeg(NFMemObjSeg* pObjSeg)
{
    m_pObjSeg = pObjSeg;
}

//////////////////////////////////////////////////////////////////////////
int NFCMemMngModule::InitializeAllObj()
{
    int iRet = InitAllObjSeg();

    if (iRet)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "failed!");
        return -1;
    }

    //对象内存分配完毕后，统一把创建方式改为Init.
    NFShmMgr::Instance()->SetCreateMode(EN_OBJ_MODE_INIT);
    SetCreateMode(EN_OBJ_MODE_INIT);
    return 0;
}

size_t NFCMemMngModule::GetAllObjSize() const
{
    return m_iObjSegSizeTotal;
}

int NFCMemMngModule::InitMemObjectGlobal()
{
    m_pGlobalId = dynamic_cast<NFMemGlobalId*>(NFMemGlobalId::CreateObject());
    NF_ASSERT(m_pGlobalId != NULL);
    for (int i = EOT_GLOBAL_ID + 1; i < static_cast<int>(m_nObjSegSwapCounter.size()); i++)
    {
        if (m_nObjSegSwapCounter[i].m_nObjSize > 0 && m_nObjSegSwapCounter[i].m_iItemCount > 0)
        {
            NFMemObjSegSwapCounter* pObjSegSwapCounter = &m_nObjSegSwapCounter[i];
            if (pObjSegSwapCounter->m_singleton)
            {
                CreateObj(i);
                NFLogTrace(NF_LOG_DEFAULT, 0, "Create Mem Global Obj:{}", pObjSegSwapCounter->m_szClassName);
            }
        }
    }

    InitSpecialMemObj();

    return 0;
}

int NFCMemMngModule::InitSpecialMemObj()
{
    auto pManager = dynamic_cast<NFMemTransMng*>(GetHeadObj(EOT_TRANS_MNG));
    if (pManager)
    {
        /**
         * @brief 平衡处理，大概一帧处理200个trans
         */
        pManager->Init(1, 200);
    }

    return 0;
}

NFMemObjSeg* NFCMemMngModule::GetObjSeg(int iType) const
{
    if (iType >= 0 && iType < static_cast<int>(m_nObjSegSwapCounter.size())
        && m_nObjSegSwapCounter[iType].m_pObjSeg)
    {
        return m_nObjSegSwapCounter[iType].m_pObjSeg;
    }

    return nullptr;
}

NFMemObjSegSwapCounter* NFCMemMngModule::GetObjSegSwapCounter(int iType)
{
    if (iType >= 0 && iType < static_cast<int>(m_nObjSegSwapCounter.size()))
    {
        return &m_nObjSegSwapCounter[iType];
    }

    return nullptr;
}

int NFCMemMngModule::GetItemCount(int iType)
{
    NFMemObjSeg* pObjSeg = GetObjSeg(iType);
    if (pObjSeg)
    {
        return pObjSeg->GetItemCount();
    }
    NFLogError(NF_LOG_DEFAULT, 0, "now GetItemCount iType:{} null objseg", iType);
    return 0;
}

int NFCMemMngModule::GetUsedCount(int iType)
{
    NFMemObjSeg* pObjSeg = GetObjSeg(iType);
    if (pObjSeg)
    {
        return pObjSeg->GetUsedCount();
    }
    NFLogError(NF_LOG_DEFAULT, 0, "now GetUsedCount iType:{} null objseg", iType);
    return 0;
}

int NFCMemMngModule::GetFreeCount(int iType)
{
    NFMemObjSeg* pObjSeg = GetObjSeg(iType);
    if (pObjSeg)
    {
        return pObjSeg->GetFreeCount();
    }
    NFLogError(NF_LOG_DEFAULT, 0, "now GetFreeCount iType:{} null objseg", iType);
    return 0;
}

void* NFCMemMngModule::AllocMemForObject(int iType)
{
    NFMemObjSeg* pObjSeg = GetObjSeg(iType);
    if (pObjSeg)
    {
        void* p = pObjSeg->AllocMemForObject();

        NFLogTrace(NF_LOG_DEFAULT, 0, "{} type:{} objsize:{} AllocMem:{} usedCount:{} allCount:{}", m_nObjSegSwapCounter[iType].m_szClassName,
                   m_nObjSegSwapCounter[iType].m_iObjType, m_nObjSegSwapCounter[iType].m_nObjSize, p, pObjSeg->GetUsedCount(), pObjSeg->GetItemCount());
        return p;
    }
    NFLogError(NF_LOG_DEFAULT, 0, "now AllocMemForObject iType:{} null objseg", iType);
    return nullptr;
}

void NFCMemMngModule::FreeMemForObject(int iType, void* pMem)
{
    NFMemObjSeg* pObjSeg = GetObjSeg(iType);
    if (pObjSeg)
    {
        int iRet = pObjSeg->FreeMemForObject(pMem);
        if (iRet != 0)
        {
            NFLogError(NF_LOG_DEFAULT, 0, "nFreeMemForObject Failed, the pMem is no the class data, classType:{}, className:{}", iType, m_nObjSegSwapCounter[iType].m_szClassName);
        }
        return;
    }
    NFLogError(NF_LOG_DEFAULT, 0, "now FreeMemForObject iType:{} null objseg", iType);
}

NFObject* NFCMemMngModule::GetObjByObjId(int iType, int iIndex)
{
    NFMemObjSeg* pObjSeg = GetObjSeg(iType);
    if (pObjSeg)
    {
        return pObjSeg->GetObj(iIndex);
    }
    NFLogError(NF_LOG_DEFAULT, 0, "now GetObjByObjId iType:{} null objseg", iType);
    return nullptr;
}

bool NFCMemMngModule::IsEnd(int iType, int iIndex)
{
    assert(IsTypeValid(iType));

    if (m_nObjSegSwapCounter[iType].m_pObjSeg)
    {
        return m_nObjSegSwapCounter[iType].m_pObjSeg->IsEnd(iIndex);
    }

    return true;
}

void NFCMemMngModule::SetSecOffSet(int iOffset)
{
    if (m_pGlobalId)
    {
        m_pGlobalId->SetSecOffSet(iOffset);
    }
}

int NFCMemMngModule::GetSecOffSet() const
{
    if (m_pGlobalId)
    {
        return m_pGlobalId->GetSecOffSet();
    }
    return 0;
}

size_t NFCMemMngModule::IterIncr(int iType, size_t iPos)
{
    assert(IsTypeValid(iType));

    return m_nObjSegSwapCounter[iType].m_pObjSeg->IterIncr(iPos);
}

size_t NFCMemMngModule::IterDecr(int iType, size_t iPos)
{
    assert(IsTypeValid(iType));

    return m_nObjSegSwapCounter[iType].m_pObjSeg->IterDecr(iPos);
}

NFCMemMngModule::iterator NFCMemMngModule::IterBegin(int iType)
{
    assert(IsTypeValid(iType));

    return iterator(this, iType, m_nObjSegSwapCounter[iType].m_pObjSeg->IterBegin());
}

NFCMemMngModule::iterator NFCMemMngModule::IterEnd(int iType)
{
    assert(IsTypeValid(iType));

    return iterator(this, iType, m_nObjSegSwapCounter[iType].m_pObjSeg->IterEnd());
}

NFCMemMngModule::const_iterator NFCMemMngModule::IterBegin(int iType) const
{
    assert(IsTypeValid(iType));

    return const_iterator(this, iType, m_nObjSegSwapCounter[iType].m_pObjSeg->IterBegin());
}

NFCMemMngModule::const_iterator NFCMemMngModule::IterEnd(int iType) const
{
    assert(IsTypeValid(iType));

    return const_iterator(this, iType, m_nObjSegSwapCounter[iType].m_pObjSeg->IterEnd());
}

/**
 * @brief vector删除一个，但是要保持顺序不变，这里只是把最后一个调换到当前位置，然后删除最后一个,所以仍然返回当前位置的迭代器
 * @param iter
 * @return
 */
NFCMemMngModule::iterator NFCMemMngModule::Erase(iterator iter)
{
    assert(IsTypeValid(iter.m_type));
    if (iter != IterEnd(iter.m_type))
    {
        DestroyObj(iter.GetObj());
    }
    return iter;
}

bool NFCMemMngModule::IsValid(iterator iter)
{
    return GetIterObj(iter.m_type, iter.m_pos) != nullptr;
}

NFObject* NFCMemMngModule::GetIterObj(int iType, size_t iPos)
{
    assert(IsTypeValid(iType));

    return m_nObjSegSwapCounter[iType].m_pObjSeg->GetIterObj(iPos);
}


const NFObject* NFCMemMngModule::GetIterObj(int iType, size_t iPos) const
{
    assert(IsTypeValid(iType));

    return m_nObjSegSwapCounter[iType].m_pObjSeg->GetIterObj(iPos);
}

bool NFCMemMngModule::IsTypeValid(int iType) const
{
    if (iType < 0 || iType >= static_cast<int>(m_nObjSegSwapCounter.size()))
    {
        return false;
    }

    return true;
}

NFTransBase* NFCMemMngModule::CreateTrans(int iType)
{
    CHECK_EXPR(IsTypeValid(iType), nullptr, "iType:{} is not valid", iType);

    auto pManager = dynamic_cast<NFMemTransMng*>(GetHeadObj(EOT_TRANS_MNG));
    if (pManager)
    {
        return pManager->CreateTrans(iType);
    }
    return nullptr;
}

NFTransBase* NFCMemMngModule::GetTrans(uint64_t ullTransId)
{
    auto* pManager = dynamic_cast<NFMemTransMng*>(GetHeadObj(EOT_TRANS_MNG));
    if (pManager)
    {
        return pManager->GetTransBase(ullTransId);
    }
    return nullptr;
}

NFObject* NFCMemMngModule::CreateObjByHashKey(int iType, NFObjectHashKey hashKey)
{
    assert(IsTypeValid(iType));

    NFObject* pObj = nullptr;
    if (!m_nObjSegSwapCounter[iType].m_iUseHash)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "the obj can't use hash");
        return nullptr;
    }

    if (m_nObjSegSwapCounter[iType].m_pObjSeg->HashFind(hashKey))
    {
        NFLogError(NF_LOG_DEFAULT, 0, "the hash key:{} exist.........", hashKey);
        return nullptr;
    }

    NFShmMgr::Instance()->m_iType = iType;
    pObj = m_nObjSegSwapCounter[iType].m_pCreateFn();
    NFShmMgr::Instance()->m_iType = INVALID_ID;
    if (pObj)
    {
        int iGlobalId = pObj->GetGlobalId();
        int iObjId = pObj->GetObjId();

        if (iGlobalId >= 0 && iObjId >= 0)
        {
            if (m_nObjSegSwapCounter[iType].m_pObjSeg)
            {
                int hashRet = m_nObjSegSwapCounter[iType].m_pObjSeg->HashInsert(hashKey, iObjId);
                if (hashRet < 0)
                {
                    NFLogDebug(NF_LOG_DEFAULT, hashKey, "CreateObjByHashKey Fail! hashKey:{} type:{} className:{} GlobalID:{} UsedCount:{} AllCount:{}", hashKey, iType, m_nObjSegSwapCounter[iType].m_szClassName, iGlobalId, m_nObjSegSwapCounter[iType].m_pObjSeg->GetUsedCount(),
                               m_nObjSegSwapCounter[iType].m_pObjSeg->GetItemCount());
                    m_nObjSegSwapCounter[iType].m_pDestroyFn(pObj);
                    pObj = nullptr;
                    NF_ASSERT(false);
                }
                else
                {
                    pObj->SetHashKey(hashKey);
                }

                NFLogDebug(NF_LOG_DEFAULT, hashKey, "CreateObjByHashKey Success! hashKey:{} type:{} className:{} GlobalID:{} objId:{} UsedCount:{} AllCount:{}", hashKey, iType, m_nObjSegSwapCounter[iType].m_szClassName, iGlobalId, iObjId, m_nObjSegSwapCounter[iType].m_pObjSeg->GetUsedCount(),
                           m_nObjSegSwapCounter[iType].m_pObjSeg->GetItemCount());
            }
            else
            {
                NFLogDebug(NF_LOG_DEFAULT, hashKey, "CreateObjByHashKey Fail! hashKey:{} type:{} className:{} GlobalID:{} UsedCount:{} AllCount:{}", hashKey, iType, m_nObjSegSwapCounter[iType].m_szClassName, iGlobalId, m_nObjSegSwapCounter[iType].m_pObjSeg->GetUsedCount(),
                           m_nObjSegSwapCounter[iType].m_pObjSeg->GetItemCount());
            }
        }
        else
        {
            NFLogDebug(NF_LOG_DEFAULT, hashKey, "CreateObjByHashKey Fail! hashKey:{} type:{} className:{} GlobalID:{} UsedCount:{} AllCount:{}", hashKey, iType, m_nObjSegSwapCounter[iType].m_szClassName, iGlobalId, m_nObjSegSwapCounter[iType].m_pObjSeg->GetUsedCount(),
                       m_nObjSegSwapCounter[iType].m_pObjSeg->GetItemCount());
            m_nObjSegSwapCounter[iType].m_pDestroyFn(pObj);
            pObj = nullptr;
            NF_ASSERT(false);
        }
    }
    else
    {
        NFLogDebug(NF_LOG_DEFAULT, hashKey, "CreateObjByHashKey Fail! hashKey:{} type:{} className:{} UsedCount:{} AllCount:{}", hashKey, iType, m_nObjSegSwapCounter[iType].m_szClassName, m_nObjSegSwapCounter[iType].m_pObjSeg->GetUsedCount(), m_nObjSegSwapCounter[iType].m_iItemCount);
    }

    return pObj;
}

NFObject* NFCMemMngModule::CreateObj(int iType)
{
    assert(IsTypeValid(iType));

    NFObject* pObj = nullptr;
    if (m_nObjSegSwapCounter[iType].m_iUseHash)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "the obj use hash, create obj use CreateObjByHashKey(uint64_t hashKey, int iType)");
        return nullptr;
    }

    NFShmMgr::Instance()->m_iType = iType;
    pObj = m_nObjSegSwapCounter[iType].m_pCreateFn();
    NFShmMgr::Instance()->m_iType = INVALID_ID;
    if (pObj)
    {
        int iId = pObj->GetGlobalId();

        if (iId >= 0)
        {
            NFLogDebug(NF_LOG_DEFAULT, 0, "CreateObj Success! type:{} className:{} GlobalID:{} UsedCount:{} AllCount:{}", iType, m_nObjSegSwapCounter[iType].m_szClassName, iId, m_nObjSegSwapCounter[iType].m_pObjSeg->GetUsedCount(), m_nObjSegSwapCounter[iType].m_pObjSeg->GetItemCount());
        }
        else
        {
            m_nObjSegSwapCounter[iType].m_pDestroyFn(pObj);
            pObj = nullptr;
            NFLogError(NF_LOG_DEFAULT, 0, "No global id available, CreateObj Failed! type:{} className:{}", iType, m_nObjSegSwapCounter[iType].m_szClassName);
            NF_ASSERT(false);
        }
    }
    else
    {
        NFLogError(NF_LOG_DEFAULT, 0, "CreateObj Failed! type:{} className:{}", iType, m_nObjSegSwapCounter[iType].m_szClassName);
    }

    return pObj;
}

NFObject* NFCMemMngModule::GetObjByHashKey(int iType, NFObjectHashKey hashKey)
{
    NF_ASSERT(IsTypeValid(iType));
    if (!m_nObjSegSwapCounter[iType].m_iUseHash)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "the obj not use hash, get obj use GetObjByGlobalId");
        return nullptr;
    }

    return m_nObjSegSwapCounter[iType].m_pObjSeg->HashFind(hashKey);
}

NFObject* NFCMemMngModule::GetObjByGlobalIdWithNoCheck(int iGlobalId)
{
    NFObject* pObj = m_pGlobalId->GetObj(iGlobalId);
    return pObj;
}

const std::unordered_set<int>& NFCMemMngModule::GetChildrenType(int iType)
{
    NF_ASSERT(IsTypeValid(iType));

    return m_nObjSegSwapCounter[iType].m_childrenObjType;
}

NFObject* NFCMemMngModule::GetObjByGlobalId(int iType, int iGlobalId, bool withChildrenType)
{
    NFObject* pObj = m_pGlobalId->GetObj(iGlobalId);
    if (!pObj)
    {
        return nullptr;
    }

    int iRealType;

#if defined(_DEBUG) | defined(_DEBUG_)
    pObj->CheckMemMagicNum();
#endif

    if (iType < 0)
    {
        return pObj;
    }

    iRealType = pObj->GetClassType();

    if (iRealType != iType)
    {
        if (!withChildrenType)
        {
            return nullptr;
        }
    }

    auto pObjSeqSwapCounter = iRealType >= 0 && iRealType < static_cast<int>(m_nObjSegSwapCounter.size()) ? &m_nObjSegSwapCounter[iRealType] : nullptr;

    if (!pObjSeqSwapCounter)
    {
        return nullptr;
    }

    if (pObjSeqSwapCounter->m_iObjType == iType)
    {
        return pObj;
    }

    if (pObjSeqSwapCounter->m_parentObjType.find(iType) != pObjSeqSwapCounter->m_parentObjType.end())
    {
        return pObj;
    }

    return nullptr;
}

/*static */
NFObject* NFCMemMngModule::GetObjByMiscId(int iMiscId, int iType)
{
    if (iMiscId == -1)
    {
        return nullptr;
    }

    //MiscID is globalid
    if (iMiscId >= 0)
    {
        //带globalId的对象尽量不要通过GetObjFromMiscID来获取对象
#if defined( _DEBUG_) | defined(_DEBUG)
        NFLogError(NF_LOG_DEFAULT, 0, "advice:don't use GetObjByMiscId get object with globalId. {} ,type {}", iMiscId, iType);
#endif
        return GetObjByGlobalId(iType, iMiscId);
    }
    int iTypeInId = (iMiscId & 0x7f800000) >> 23;
    int iIndexInId = iMiscId & 0x007fffff;
    NFObject* pObj = GetObjByObjId(iTypeInId, iIndexInId);

    if (!pObj)
    {
        return nullptr;
    }

#if defined(_DEBUG) | defined(_DEBUG_)
    pObj->CheckMemMagicNum();
#endif

    int iRealType = pObj->GetClassType();

    //不需要GlobalId的扩展类请不要通过CIDRuntimeClass创建对象
    assert(iRealType == iTypeInId);

    if (iRealType == iType)
    {
        return pObj;
    }

    if (iType >= 0)
    {
        auto pObjSegSwapCounter = iRealType >= 0 && iRealType < static_cast<int>(m_nObjSegSwapCounter.size()) ? &m_nObjSegSwapCounter[iType] : nullptr;

        if (!pObjSegSwapCounter)
        {
            assert(0);
            return nullptr;
        }

        if (pObjSegSwapCounter->m_iObjType == iType)
        {
            return pObj;
        }

        while (pObjSegSwapCounter->m_pParent)
        {
            if (pObjSegSwapCounter->m_pParent->m_iObjType == iType)
            {
                return pObj;
            }

            pObjSegSwapCounter = pObjSegSwapCounter->m_pParent;
        }

#if defined( _DEBUG_) | defined(_DEBUG)
        assert(0);
#endif
        NFLogError(NF_LOG_DEFAULT, 0, "Want Type:{} Real Type:{}", iType, iRealType)

        return nullptr;
    }

    return pObj;
}

NFObject* NFCMemMngModule::GetHeadObj(int iType)
{
    if (!IsTypeValid(iType))
    {
        return nullptr;
    }

    if (m_nObjSegSwapCounter[iType].m_pObjSeg)
    {
        return m_nObjSegSwapCounter[iType].m_pObjSeg->GetHeadObj();
    }

    return nullptr;
}

NFObject* NFCMemMngModule::GetNextObj(int iType, NFObject* pObj)
{
    if (!IsTypeValid(iType) || !pObj)
    {
        return nullptr;
    }

    if (m_nObjSegSwapCounter[iType].m_pObjSeg)
    {
        return m_nObjSegSwapCounter[iType].m_pObjSeg->GetNextObj(pObj);
    }

    return nullptr;
}

int NFCMemMngModule::GetGlobalId(int iType, int iIndex, NFObject* pObj)
{
    if (m_pGlobalId)
    {
        return m_pGlobalId->GetGlobalId(iType, iIndex, pObj);
    }
    return INVALID_ID;
}

int NFCMemMngModule::GetObjId(int iType, NFObject* pObj)
{
    NFMemObjSeg* pObjSeg = GetObjSeg(iType);
    if (pObjSeg)
    {
        return pObjSeg->GetObjId(pObj);
    }
    NFLogError(NF_LOG_DEFAULT, 0, "now GetObjID iType:{} null objseg", iType);
    return -1;
}

int NFCMemMngModule::DestroyObjAutoErase(int iType, int maxNum, const DESTROY_OBJECT_AUTO_ERASE_FUNCTION& func)
{
    if (m_nObjSegSwapCounter[iType].m_pObjSeg)
    {
        if (m_nObjSegSwapCounter[iType].m_iUseHash)
        {
            std::vector<NFObject*> vecObj;
            for (auto iter = m_nObjSegSwapCounter[iType].m_pObjSeg->m_hashMgr.begin(); iter != m_nObjSegSwapCounter[iType].m_pObjSeg->m_hashMgr.end(); ++iter)
            {
                NFObject* pObj = m_nObjSegSwapCounter[iType].m_pObjSeg->GetObj(iter->second);
                if (func)
                {
                    if (func(pObj))
                    {
                        vecObj.push_back(pObj);
                        NFLogInfo(NF_LOG_DEFAULT, 0, "DestroyObjAutoErase Data, key:{} objId:{} type:{}", iter->first, iter->second, iType);
                    }
                }
                else
                {
                    vecObj.push_back(pObj);
                    NFLogInfo(NF_LOG_DEFAULT, 0, "DestroyObjAutoErase Data, key:{} objId:{} type:{}", iter->first, iter->second, iType);
                }

                if (maxNum > 0 && static_cast<int>(vecObj.size()) >= maxNum)
                {
                    break;
                }
            }

            for (int i = 0; i < static_cast<int>(vecObj.size()); i++)
            {
                DestroyObj(vecObj[i]);
            }
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }

    return 0;
}

void NFCMemMngModule::ClearAllObj(int iType)
{
    if (!IsTypeValid(iType)) return;

    if (m_nObjSegSwapCounter[iType].m_pObjSeg)
    {
        std::vector<NFObject*> vec;
        for (auto iter = IterBegin(iType); iter != IterEnd(iType); ++iter)
        {
            vec.push_back(&*iter);
        }

        for (auto iter = vec.begin(); iter != vec.end(); ++iter)
        {
            DestroyObj(*iter);
        }
    }
}

void NFCMemMngModule::DestroyObj(NFObject* pObj)
{
    int iType = pObj->GetClassType();
    int iIndex = pObj->GetObjId();
    int iGlobalId = pObj->GetGlobalId();
    int64_t iHashId = pObj->GetHashKey();
    std::string className = pObj->GetClassName();

    if (iType < 0 || iType >= static_cast<int>(m_nObjSegSwapCounter.size()))
    {
        return;
    }

    if (iIndex < 0)
    {
        return;
    }

    if (m_nObjSegSwapCounter[iType].m_pObjSeg)
    {
        if (m_nObjSegSwapCounter[iType].m_iUseHash)
        {
            NFObject* pTempObj = m_nObjSegSwapCounter[iType].m_pObjSeg->HashFind(iHashId);
            if (pTempObj != pObj)
            {
                NFLogError(NF_LOG_DEFAULT, iHashId, "DestroyObj {} Error, key:{} globalId:{} type:{} index:{}", className, iHashId,
                           iGlobalId, iType, iIndex, iHashId);
            }
            int ret = m_nObjSegSwapCounter[iType].m_pObjSeg->HashErase(iHashId);
            if (ret != 0)
            {
                NFLogError(NF_LOG_DEFAULT, 0, "HashErase:{} Failed!", iHashId);
            }
        }

        m_pGlobalId->ReleaseId(iGlobalId);

        m_nObjSegSwapCounter[iType].m_pDestroyFn(pObj);

        if (m_nObjSegSwapCounter[iType].m_iUseHash)
        {
            NFLogDebug(NF_LOG_DEFAULT, iHashId, "DestroyObj {}, key:{} globalId:{} type:{} index:{} iHashID:{} UsedNum:{} AllNum:{}", className, iHashId, iGlobalId, iType, iIndex,
                       iHashId, m_nObjSegSwapCounter[iType].m_pObjSeg->GetUsedCount(), m_nObjSegSwapCounter[iType].m_pObjSeg->GetItemCount());
        }
        else
        {
            NFLogDebug(NF_LOG_DEFAULT, 0, "DestroyObj {}, globalId:{} type:{} index:{} UsedNum:{} AllNum:{}", className, iGlobalId, iType, iIndex, m_nObjSegSwapCounter[iType].m_pObjSeg->GetUsedCount(), m_nObjSegSwapCounter[iType].m_pObjSeg->GetItemCount());
        }
    }
}

int NFCMemMngModule::DeleteAllTimer(NFObject* pObj)
{
    return NFMemTimerMng::Instance()->ClearAllTimer(pObj);
}

int NFCMemMngModule::DeleteAllTimer(NFObject* pObj, NFRawObject* pRawShmObj)
{
    return NFMemTimerMng::Instance()->ClearAllTimer(pObj, pRawShmObj);
}

int NFCMemMngModule::DeleteTimer(NFObject* pObj, int timeObjId)
{
    auto pManager = dynamic_cast<NFMemTimerMng*>(GetHeadObj(EOT_TYPE_TIMER_MNG));
    if (pManager)
    {
        NFMemTimer* pShmTimer = pManager->GetTimer(timeObjId);
        if (pShmTimer)
        {
            if (pShmTimer->GetTimerShmObj() == pObj)
            {
                return pManager->Delete(timeObjId);
            }
            NFLogError(NF_LOG_DEFAULT, 0, "timeObjId:{} pShmTimer->GetTimerShmObj:{} != pObj:{} is not the obj timer..............", timeObjId,
                       static_cast<void*>(pShmTimer->GetTimerShmObj()), static_cast<void*>(pObj));
            NFSLEEP(1)
            NF_ASSERT(false);
        }
    }
    return -1;
}

//注册距离现在多少时间执行一次的定时器(hour  minutes  second  microSec为第一次执行距离现在的时分秒毫秒, 只执行一次)
int NFCMemMngModule::SetTimer(NFObject* pObj, int hour, int minutes, int second, int microSec, NFRawObject* pRawShmObj/* = nullptr*/)
{
    auto pManager = dynamic_cast<NFMemTimerMng*>(GetHeadObj(EOT_TYPE_TIMER_MNG));
    if (pManager)
    {
        return pManager->SetTimer(pObj, hour, minutes, second, microSec, pRawShmObj);
    }
    return -1;
}

//注册某一个时间点执行一次的定时器(hour  minutes  second为第一次执行的时间点时分秒, 只执行一次)
int NFCMemMngModule::SetCalender(NFObject* pObj, int hour, int minutes, int second, NFRawObject* pRawShmObj/* = nullptr*/)
{
    auto pManager = dynamic_cast<NFMemTimerMng*>(GetHeadObj(EOT_TYPE_TIMER_MNG));
    if (pManager)
    {
        return pManager->SetCalender(pObj, hour, minutes, second, pRawShmObj);
    }
    return -1;
}

//注册某一个时间点执行一次的定时器(timestamp为第一次执行的时间点的时间戳,单位是秒, 只执行一次)
int NFCMemMngModule::SetCalender(NFObject* pObj, uint64_t timestamp, NFRawObject* pRawShmObj/* = nullptr*/)
{
    auto pManager = dynamic_cast<NFMemTimerMng*>(GetHeadObj(EOT_TYPE_TIMER_MNG));
    if (pManager)
    {
        return pManager->SetCalender(pObj, timestamp, pRawShmObj);
    }
    return -1;
}

//注册循环执行定时器（hour  minutes  second  microSec为第一次执行距离现在的时分秒毫秒,  interval 为循环间隔时间，为毫秒）
int NFCMemMngModule::SetTimer(NFObject* pObj, int interval, int callCount, int hour, int minutes, int second, int microSec, NFRawObject* pRawShmObj/* = nullptr*/)
{
    auto pManager = dynamic_cast<NFMemTimerMng*>(GetHeadObj(EOT_TYPE_TIMER_MNG));
    if (pManager)
    {
        return pManager->SetTimer(pObj, interval, callCount, hour, minutes, second, microSec, pRawShmObj);
    }
    return -1;
}

//注册循环执行定时器（hour  minutes  second  microSec为第一次执行距离现在的时分秒毫秒）
int NFCMemMngModule::SetDayTime(NFObject* pObj, int callCount, int hour, int minutes, int second, int microSec, NFRawObject* pRawShmObj/* = nullptr*/)
{
    auto pManager = dynamic_cast<NFMemTimerMng*>(GetHeadObj(EOT_TYPE_TIMER_MNG));
    if (pManager)
    {
        return pManager->SetDayTime(pObj, callCount, hour, minutes, second, microSec, pRawShmObj);
    }
    return -1;
}

//注册某一个时间点日循环执行定时器（hour  minutes  second为一天中开始执行的时间点，    23：23：23     每天23点23分23秒执行）
int NFCMemMngModule::SetDayCalender(NFObject* pObj, int callCount, int hour, int minutes, int second, NFRawObject* pRawShmObj/* = nullptr*/)
{
    auto pManager = dynamic_cast<NFMemTimerMng*>(GetHeadObj(EOT_TYPE_TIMER_MNG));
    if (pManager)
    {
        return pManager->SetDayCalender(pObj, callCount, hour, minutes, second, pRawShmObj);
    }
    return -1;
}

//周循环（hour  minutes  second  microSec为第一次执行距离现在的时分秒毫秒）
int NFCMemMngModule::SetWeekTime(NFObject* pObj, int callCount, int hour, int minutes, int second, int microSec, NFRawObject* pRawShmObj/* = nullptr*/)
{
    auto pManager = dynamic_cast<NFMemTimerMng*>(GetHeadObj(EOT_TYPE_TIMER_MNG));
    if (pManager)
    {
        return pManager->SetWeekTime(pObj, callCount, hour, minutes, second, microSec, pRawShmObj);
    }
    return -1;
}

//注册某一个时间点周循环执行定时器（ weekDay  hour  minutes  second 为一周中某一天开始执行的时间点）
int NFCMemMngModule::SetWeekCalender(NFObject* pObj, int callCount, int weekDay, int hour, int minutes, int second, NFRawObject* pRawShmObj/* = nullptr*/)
{
    auto pManager = dynamic_cast<NFMemTimerMng*>(GetHeadObj(EOT_TYPE_TIMER_MNG));
    if (pManager)
    {
        return pManager->SetWeekCalender(pObj, callCount, weekDay, hour, minutes, second, pRawShmObj);
    }
    return -1;
}

//月循环（hour  minutes  second  microSec为第一次执行距离现在的时分秒毫秒,最好是同一天）
int NFCMemMngModule::SetMonthTime(NFObject* pObj, int callCount, int hour, int minutes, int second, int microSec, NFRawObject* pRawShmObj/* = nullptr*/)
{
    auto pManager = dynamic_cast<NFMemTimerMng*>(GetHeadObj(EOT_TYPE_TIMER_MNG));
    if (pManager)
    {
        return pManager->SetMonthTime(pObj, callCount, hour, minutes, second, microSec, pRawShmObj);
    }
    return -1;
}

//注册某一个时间点月循环执行定时器（ day  hour  minutes  second 为一月中某一天开始执行的时间点）
int NFCMemMngModule::SetMonthCalender(NFObject* pObj, int callCount, int day, int hour, int minutes, int second, NFRawObject* pRawShmObj/* = nullptr*/)
{
    auto pManager = dynamic_cast<NFMemTimerMng*>(GetHeadObj(EOT_TYPE_TIMER_MNG));
    if (pManager)
    {
        return pManager->SetMonthCalender(pObj, callCount, day, hour, minutes, second, pRawShmObj);
    }
    return -1;
}

int NFCMemMngModule::FireExecute(NF_SERVER_TYPE serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const google::protobuf::Message& message)
{
    return NFMemEventMgr::Instance()->Fire(serverType, eventId, srcType, srcId, message);
}

int NFCMemMngModule::Subscribe(NFObject* pObj, NF_SERVER_TYPE serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const string& desc)
{
    return NFMemEventMgr::Instance()->Subscribe(pObj, serverType, eventId, srcType, srcId, desc);
}

int NFCMemMngModule::UnSubscribe(NFObject* pObj, NF_SERVER_TYPE serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId)
{
    return NFMemEventMgr::Instance()->UnSubscribe(pObj, serverType, eventId, srcType, srcId);
}

int NFCMemMngModule::UnSubscribeAll(NFObject* pObj)
{
    return NFMemEventMgr::Instance()->UnSubscribeAll(pObj);
}
