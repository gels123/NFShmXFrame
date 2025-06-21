// -------------------------------------------------------------------------
//    @FileName         :    NFITimerEventModule.cpp
//    @Author           :    gaoyi
//    @Date             :    22-11-15
//    @Email			:    445267987@qq.com
//    @Module           :    NFITimerEventModule
//
// -------------------------------------------------------------------------

#include "NFITimerEventModule.h"
#include "NFIEventModule.h"
#include "NFIMessageModule.h"
#include "NFITimerModule.h"
#include "NFComm/NFPluginModule/NFIMemMngModule.h"

NFITimerEventModule::NFITimerEventModule(NFIPluginManager *pPluginManager): NFIModule(pPluginManager)
{
}

NFITimerEventModule::~NFITimerEventModule()
{
    UnSubscribeAll();
    m_pObjPluginManager->FindModule<NFITimerModule>()->KillAllTimer(this);
}

//发送执行事件
int NFITimerEventModule::FireExecute(NF_SERVER_TYPE serverType, uint32_t nEventID, uint32_t bySrcType, uint64_t nSrcID, const google::protobuf::Message &message)
{
    m_pObjPluginManager->FindModule<NFIEventModule>()->FireExecute(serverType, nEventID, bySrcType, nSrcID, message);

    auto pModule = m_pObjPluginManager->FindModule<NFIMemMngModule>();
    if (pModule)
    {
        pModule->FireExecute(serverType, nEventID, nSrcID, bySrcType, message);
    }
    return 0;
}


int NFITimerEventModule::FireBroadcast(NF_SERVER_TYPE nServerType, NF_SERVER_TYPE nRecvServerType, uint32_t nEventID, uint32_t bySrcType, uint64_t nSrcID, const google::protobuf::Message &message, bool self)
{
    FindModule<NFIMessageModule>()->BroadcastEventToServer(nServerType, nRecvServerType, nEventID, bySrcType, nSrcID, message);
    if (self)
    {
        FireExecute(nServerType, nEventID, bySrcType, nSrcID, message);
    }
    return 0;
}

int NFITimerEventModule::FireBroadcast(NF_SERVER_TYPE nServerType, NF_SERVER_TYPE nRecvServerType, uint32_t busId, uint32_t nEventID, uint32_t bySrcType, uint64_t nSrcID, const google::protobuf::Message &message, bool self)
{
    FindModule<NFIMessageModule>()->BroadcastEventToServer(nServerType, nRecvServerType, busId, nEventID, bySrcType, nSrcID, message);
    if (self)
    {
        FireExecute(nServerType, nEventID, bySrcType, nSrcID, message);
    }
    return 0;
}

int NFITimerEventModule::FireAllBroadcast(NF_SERVER_TYPE nServerType, uint32_t nEventID, uint32_t bySrcType, uint64_t nSrcID, const google::protobuf::Message &message, bool self)
{
    FindModule<NFIMessageModule>()->BroadcastEventToAllServer(nServerType, nEventID, bySrcType, nSrcID, message);
    if (self)
    {
        FireExecute(nServerType, nEventID, bySrcType, nSrcID, message);
    }
    return 0;
}

int NFITimerEventModule::FireAllBroadcast(NF_SERVER_TYPE nServerType, uint32_t busId, uint32_t nEventID, uint32_t bySrcType, uint64_t nSrcID, const google::protobuf::Message &message, bool self)
{
    FindModule<NFIMessageModule>()->BroadcastEventToAllServer(nServerType, busId, nEventID, bySrcType, nSrcID, message);
    if (self)
    {
        FireExecute(nServerType, nEventID, bySrcType, nSrcID, message);
    }
    return 0;
}

//订阅执行事件
bool NFITimerEventModule::Subscribe(NF_SERVER_TYPE serverType, uint32_t nEventID, uint32_t bySrcType, uint64_t nSrcID, const std::string &desc)
{
    return m_pObjPluginManager->FindModule<NFIEventModule>()->Subscribe(this, serverType, nEventID, bySrcType, nSrcID, desc);
}

//取消订阅执行事件
bool NFITimerEventModule::UnSubscribe(NF_SERVER_TYPE serverType, uint32_t nEventID, uint32_t bySrcType, uint64_t nSrcID)
{
    return m_pObjPluginManager->FindModule<NFIEventModule>()->UnSubscribe(this, serverType, nEventID, bySrcType, nSrcID);
}

//取消所有执行事件的订阅
bool NFITimerEventModule::UnSubscribeAll()
{
    return m_pObjPluginManager->FindModule<NFIEventModule>()->UnSubscribeAll(this);
}

//设置定时器
bool NFITimerEventModule::SetTimer(uint32_t nTimerID, uint64_t nInterVal, uint32_t nCallCount)
{
    return m_pObjPluginManager->FindModule<NFITimerModule>()->SetTimer(nTimerID, nInterVal, this, nCallCount);
}

//关闭定时器
bool NFITimerEventModule::KillTimer(uint32_t nTimerID)
{
    return m_pObjPluginManager->FindModule<NFITimerModule>()->KillTimer(nTimerID, this);
}

//关闭所有定时器
bool NFITimerEventModule::KillAllTimer()
{
    return m_pObjPluginManager->FindModule<NFITimerModule>()->KillAllTimer(this);
}

//设置固定时间的定时器
bool NFITimerEventModule::SetFixTimer(uint32_t nTimerID, uint64_t nStartTime, uint32_t nInterSec, uint32_t nCallCount)
{
    return m_pObjPluginManager->FindModule<NFITimerModule>()->SetClocker(nTimerID, nStartTime, nInterSec, this, nCallCount);
}
