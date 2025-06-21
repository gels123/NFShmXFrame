// -------------------------------------------------------------------------
//    @FileName         :    NFMemEventMgr.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFMemEventMgr
//
// -------------------------------------------------------------------------

#include "NFMemEventMgr.h"
#include "NFComm/NFPluginModule/NFIMemMngModule.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"

NFMemEventMgr::NFMemEventMgr()
{
    if (EN_OBJ_MODE_INIT == NFShmMgr::Instance()->GetCreateMode())
    {
        CreateInit();
    }
    else
    {
        ResumeInit();
    }
}

NFMemEventMgr::~NFMemEventMgr()
{
}

int NFMemEventMgr::CreateInit()
{
    m_nFireLayer = 0;
    return 0;
}

int NFMemEventMgr::ResumeInit()
{
    return 0;
}

int NFMemEventMgr::Subscribe(NFObject* pSink, NF_SERVER_TYPE serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const std::string& desc)
{
    CHECK_NULL(0, pSink);
    CHECK_EXPR_ASSERT(pSink->GetGlobalId() != INVALID_ID, -1, "NFObject GetGlobalID == INVALID_ID, desc:{}", desc);

    NFMemEventKey skey;
    skey.m_serverType = serverType;
    skey.m_eventId = eventId;
    skey.m_srcType = srcType;
    skey.m_srcId = srcId;

    /**
    *@brief 先判断指针pSink对象有没有注册，然后把skey放入
    *       这个指针的的集合里，如果skey已经存在，
    *       说明已经存入，直接退出
    */
    auto objListIter = m_shmObjAllSubscribe.find(pSink->GetGlobalId());
    if (objListIter == m_shmObjAllSubscribe.end())
    {
        objListIter = m_shmObjAllSubscribe.emplace(pSink->GetGlobalId(), NFNodeObjMultiList<NFMemSubscribeInfo>()).first;
        CHECK_EXPR(objListIter != m_shmObjAllSubscribe.end(), -1, "m_shmObjAllSubscribe Insert Failed, Space Not Enough, desc:{}", desc);
        objListIter->second.InitShmObj(this);
    }

    auto pObjList = &objListIter->second;

    auto eventKeyListIter = m_eventKeyAllSubscribe.find(skey);
    if (eventKeyListIter == m_eventKeyAllSubscribe.end())
    {
        eventKeyListIter = m_eventKeyAllSubscribe.emplace(skey, NFNodeObjMultiList<NFMemSubscribeInfo>()).first;
        CHECK_EXPR(eventKeyListIter != m_eventKeyAllSubscribe.end(), -1, "m_eventKeyAllSubscribe Insert Failed, Space Not Enough, desc:{}", desc);
        eventKeyListIter->second.InitShmObj(this);
    }

    auto pEventKeyList = &eventKeyListIter->second;

    /**
    *@brief 判断skey有没有存在，把对象存入skey的链表里
    */
    auto pInfo = NFMemSubscribeInfo::CreateObj();
    CHECK_EXPR(pInfo, -1, "CreateObj NFMemSubscribeInfo Failed, desc:{}", desc);
    pInfo->m_pSink = pSink;
    pInfo->m_szDesc = desc;
    pInfo->m_eventKey = skey;
    pInfo->m_shmObjId = pSink->GetGlobalId();

    int ret = pObjList->AddNode(NF_SHM_SUBSCRIBEINFO_SHM_OBJ_INDEX_1, pInfo);
    CHECK_EXPR_ASSERT(ret == 0, -1, "AddNode Failed, desc:{}", desc);
    ret = pEventKeyList->AddNode(NF_SHM_SUBSCRIBEINFO_EVENT_KEY_INDEX_0, pInfo);
    CHECK_EXPR_ASSERT(ret == 0, -1, "AddNode Failed, desc:{}", desc);

    NFLogTrace(NF_LOG_DEFAULT, 0, "ShmObj:{} ShmObjType:{} Subscribe:{}", pSink->GetGlobalId(), pSink->GetClassName(), pInfo->ToString());
    return 0;
}

int NFMemEventMgr::UnSubscribe(const NFObject* pSink, NF_SERVER_TYPE serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId)
{
    CHECK_NULL(0, pSink);

    NFMemEventKey skey;
    skey.m_serverType = serverType;
    skey.m_eventId = eventId;
    skey.m_srcType = srcType;
    skey.m_srcId = srcId;

    /**
    *@brief 判断pSink指针对象有没有存在，不存在直接退出
    *		存在的话，删除对应的key, 如果pSink集合为空的话，
    *       删除pSink
    */
    auto shmObjListIter = m_shmObjAllSubscribe.find(pSink->GetGlobalId());
    if (shmObjListIter == m_shmObjAllSubscribe.end())
    {
        return -1;
    }

    auto pShmObjList = &shmObjListIter->second;

    auto pNode = pShmObjList->GetHeadNodeObj(NF_SHM_SUBSCRIBEINFO_SHM_OBJ_INDEX_1);
    while (pNode)
    {
        CHECK_EXPR_ASSERT(pNode->m_shmObjId == pSink->GetGlobalId(), -1, "");
        if (pNode->m_eventKey == skey)
        {
            auto pLastNode = pNode;
            pNode = pShmObjList->GetNextNodeObj(NF_SHM_SUBSCRIBEINFO_SHM_OBJ_INDEX_1, pNode);
            pShmObjList->RemoveNode(NF_SHM_SUBSCRIBEINFO_SHM_OBJ_INDEX_1, pLastNode);
            NFLogTrace(NF_LOG_DEFAULT, 0, "ShmObj:{} ShmObjType:{} Delete Subscribe:{}", pSink->GetGlobalId(), pSink->GetClassName(), pLastNode->ToString());
            DelEventKeyListSubscribeInfo(pLastNode);
        }
        else
        {
            pNode = pShmObjList->GetNextNodeObj(NF_SHM_SUBSCRIBEINFO_SHM_OBJ_INDEX_1, pNode);
        }
    }

    if (pShmObjList->IsEmpty())
    {
        m_shmObjAllSubscribe.erase(pSink->GetGlobalId());
    }

    return 0;
}

int NFMemEventMgr::UnSubscribeAll(const NFObject* pSink)
{
    CHECK_NULL(0, pSink);
    return UnSubscribeAll(pSink->GetGlobalId());
}

int NFMemEventMgr::UnSubscribeAll(int globalId)
{
    auto shmObjListIter = m_shmObjAllSubscribe.find(globalId);
    if (shmObjListIter == m_shmObjAllSubscribe.end())
    {
        return -1;
    }

    auto pShmObjList = &shmObjListIter->second;

    auto pNode = pShmObjList->GetHeadNodeObj(NF_SHM_SUBSCRIBEINFO_SHM_OBJ_INDEX_1);
    while (pNode)
    {
        CHECK_EXPR_ASSERT(pNode->m_shmObjId == globalId, -1, "");
        auto pLastNode = pNode;
        pNode = pShmObjList->GetNextNodeObj(NF_SHM_SUBSCRIBEINFO_SHM_OBJ_INDEX_1, pNode);
        pShmObjList->RemoveNode(NF_SHM_SUBSCRIBEINFO_SHM_OBJ_INDEX_1, pLastNode);

        DelEventKeyListSubscribeInfo(pLastNode);
    }

    m_shmObjAllSubscribe.erase(globalId);

    return 0;
}

int NFMemEventMgr::Fire(NF_SERVER_TYPE serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const google::protobuf::Message& message)
{
    int ret = 0;
    NFMemEventKey skey;
    skey.m_serverType = serverType;
    skey.m_eventId = eventId;
    skey.m_srcType = srcType;
    skey.m_srcId = srcId;

    /**
    * @brief 先执行完全匹配的
    */
    if (skey.m_srcId != 0)
    {
        ret = Fire(skey, serverType, eventId, srcType, srcId, message);
        if (ret != 0)
        {
            return ret;
        }
    }

    /**
    * @brief 再执行， 针对整个事件nEventID,类型为bySrcType
    * 比如订阅时，订阅了所有玩家类的事件，而不是对一个玩家的事件，
    * 订阅时将nSrcId=0，会受到所有玩家产生的该类事件
    */
    skey.m_srcId = 0;
    ret = Fire(skey, serverType, eventId, srcType, srcId, message);
    if (ret != 0)
    {
        return ret;
    }

    NFLogDebug(NF_LOG_DEFAULT, 0, "Fire Event, serverType:{} m_eventId:{} m_srcType:{} m_srcId:{}, content:{}", serverType, eventId, srcType, srcId, message.Utf8DebugString());
    return 0;
}

int NFMemEventMgr::Fire(const NFMemEventKey& key, uint32_t serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const google::protobuf::Message& message)
{
    m_nFireLayer++;
    if (m_nFireLayer >= EVENT_FIRE_MAX_LAYER)
    {
        NFLogError(NF_LOG_DEFAULT, 0,
                   "m_nFireLayer >= EVENT_FIRE_MAX_LAYER.....{} fireLayer:{}", key.ToString(), m_nFireLayer);
        m_nFireLayer--;
        return -1;
    }

    std::vector<int> errVec;
    auto eventKeyListIter = m_eventKeyAllSubscribe.find(key);
    if (eventKeyListIter != m_eventKeyAllSubscribe.end())
    {
        auto pEventKeyList = &eventKeyListIter->second;
        auto pNode = pEventKeyList->GetHeadNodeObj(NF_SHM_SUBSCRIBEINFO_EVENT_KEY_INDEX_0);
        while (pNode)
        {
            if (pNode->m_refCount >= EVENT_REF_MAX_CNT)
            {
                NFLogError(NF_LOG_DEFAULT, 0, "pSubscribeInfo->m_refCount >= EVENT_REF_MAX_CNT....{}", pNode->ToString());
                m_nFireLayer--;
                return -1;
            }

            if (!pNode->m_removeFlag)
            {
                int bRes = 0;
                try
                {
                    pNode->Add();
                    //智能指针，自动转空
                    if (pNode->m_pSink)
                    {
                        bRes = pNode->m_pSink->OnExecute(serverType, eventId, srcType, srcId, &message);
                    }
                    else
                    {
                        errVec.push_back(pNode->m_shmObjId);

                        NFLogError(NF_LOG_DEFAULT, 0, "[Event] pNode->m_pSink = NULL....{}", pNode->ToString());
                    }
                    pNode->Sub();
                }
                catch (...)
                {
                    NFLogError(NF_LOG_DEFAULT, 0, "[Event] pSubscribeInfo->m_refCount >= EVENT_REF_MAX_CNT....{}", pNode->ToString());
                    m_nFireLayer--;
                    return -1;
                }

                if (pNode->m_removeFlag && 0 == pNode->m_refCount)
                {
                    auto pLastNode = pNode;
                    pNode = pEventKeyList->GetNextNodeObj(NF_SHM_SUBSCRIBEINFO_EVENT_KEY_INDEX_0, pNode);
                    pEventKeyList->RemoveNode(NF_SHM_SUBSCRIBEINFO_EVENT_KEY_INDEX_0, pLastNode);
                    FindModule<NFIMemMngModule>()->DestroyObj(pLastNode);
                }
                else
                {
                    pNode = pEventKeyList->GetNextNodeObj(NF_SHM_SUBSCRIBEINFO_EVENT_KEY_INDEX_0, pNode);
                }
                if (bRes != 0)
                {
                    NFLogError(NF_LOG_DEFAULT, 0, "[Event] ret != 0 ....{}", pNode->ToString());
                }
            }
            else
            {
                if (0 == pNode->m_refCount)
                {
                    auto pLastNode = pNode;
                    pNode = pEventKeyList->GetNextNodeObj(NF_SHM_SUBSCRIBEINFO_EVENT_KEY_INDEX_0, pNode);
                    pEventKeyList->RemoveNode(NF_SHM_SUBSCRIBEINFO_EVENT_KEY_INDEX_0, pLastNode);
                    FindModule<NFIMemMngModule>()->DestroyObj(pLastNode);
                }
                else
                {
                    pNode = pEventKeyList->GetNextNodeObj(NF_SHM_SUBSCRIBEINFO_EVENT_KEY_INDEX_0, pNode);
                }
            }
        }

        if (pEventKeyList->IsEmpty())
        {
            m_eventKeyAllSubscribe.erase(key);
        }
    }

    m_nFireLayer--;

    for (int i = 0; i < static_cast<int>(errVec.size()); i++)
    {
        UnSubscribeAll(errVec[i]);
    }

    return 0;
}

int NFMemEventMgr::DelEventKeyListSubscribeInfo(NFMemSubscribeInfo* pLastNode)
{
    auto eventKeyListIter = m_eventKeyAllSubscribe.find(pLastNode->m_eventKey);
    if (eventKeyListIter != m_eventKeyAllSubscribe.end())
    {
        auto pEventKeyList = &eventKeyListIter->second;
        if (pLastNode->m_refCount == 0)
        {
            pEventKeyList->RemoveNode(NF_SHM_SUBSCRIBEINFO_EVENT_KEY_INDEX_0, pLastNode);
            FindModule<NFIMemMngModule>()->DestroyObj(pLastNode);
        }
        else
        {
            pLastNode->m_removeFlag = true;
        }

        if (pEventKeyList->IsEmpty())
        {
            m_eventKeyAllSubscribe.erase(pLastNode->m_eventKey);
        }
    }

    return 0;
}
