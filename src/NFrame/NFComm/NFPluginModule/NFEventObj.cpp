// -------------------------------------------------------------------------
//    @FileName         :    NFEventObj.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------
#include <stdint.h>

#include "NFEventObj.h"
#include "NFIPluginManager.h"
#include "NFIEventModule.h"
#include "NFIMessageModule.h"
#include "NFComm/NFPluginModule/NFIMemMngModule.h"


NFEventObj::NFEventObj(NFIPluginManager* pPluginManager):NFBaseObj(pPluginManager)
{
}

NFEventObj::~NFEventObj()
{
	UnSubscribeAll();
}

//发送执行事件
int NFEventObj::FireExecute(NF_SERVER_TYPE serverType, uint32_t nEventID, uint32_t bySrcType, uint64_t nSrcID, const google::protobuf::Message& message)
{
    m_pObjPluginManager->FindModule<NFIEventModule>()->FireExecute(serverType, nEventID, bySrcType, nSrcID, message);

    auto pModule = m_pObjPluginManager->FindModule<NFIMemMngModule>();
    if (pModule)
    {
        pModule->FireExecute(serverType, nEventID, bySrcType, nSrcID, message);
    }
	return 0;
}

int NFEventObj::FireBroadcast(NF_SERVER_TYPE nServerType, NF_SERVER_TYPE nRecvServerType, uint32_t nEventID, uint32_t bySrcType, uint64_t nSrcID, const google::protobuf::Message &message, bool self)
{
	FindModule<NFIMessageModule>()->BroadcastEventToServer(nServerType, nRecvServerType, nEventID, bySrcType, nSrcID, message);
	if (self)
	{
		FireExecute(nServerType, nEventID, bySrcType, nSrcID, message);
	}
	return 0;
}

int NFEventObj::FireBroadcast(NF_SERVER_TYPE nServerType, NF_SERVER_TYPE nRecvServerType, uint32_t busId, uint32_t nEventID, uint32_t bySrcType, uint64_t nSrcID, const google::protobuf::Message &message, bool self)
{
	FindModule<NFIMessageModule>()->BroadcastEventToServer(nServerType, nRecvServerType, busId, nEventID, bySrcType, nSrcID, message);
	if (self)
	{
		FireExecute(nServerType, nEventID, bySrcType, nSrcID, message);
	}
	return 0;
}

int NFEventObj::FireAllBroadcast(NF_SERVER_TYPE nServerType, uint32_t nEventID, uint32_t bySrcType, uint64_t nSrcID, const google::protobuf::Message &message, bool self)
{
	FindModule<NFIMessageModule>()->BroadcastEventToAllServer(nServerType, nEventID, bySrcType, nSrcID, message);
	if (self)
	{
		FireExecute(nServerType, nEventID, bySrcType, nSrcID, message);
	}
	return 0;
}

int NFEventObj::FireAllBroadcast(NF_SERVER_TYPE nServerType, uint32_t busId, uint32_t nEventID, uint32_t bySrcType, uint64_t nSrcID, const google::protobuf::Message &message, bool self)
{
	FindModule<NFIMessageModule>()->BroadcastEventToAllServer(nServerType, busId, nEventID, bySrcType, nSrcID, message);
	if (self)
	{
		FireExecute(nServerType, nEventID, bySrcType, nSrcID, message);
	}
	return 0;
}

//订阅执行事件
bool NFEventObj::Subscribe(NF_SERVER_TYPE serverType, uint32_t nEventID, uint32_t bySrcType, uint64_t nSrcID, const std::string& desc)
{
	return m_pObjPluginManager->FindModule<NFIEventModule>()->Subscribe(this, serverType, nEventID, bySrcType, nSrcID, desc);
}

//取消订阅执行事件
bool NFEventObj::UnSubscribe(NF_SERVER_TYPE serverType, uint32_t nEventID, uint32_t bySrcType, uint64_t nSrcID)
{
	return m_pObjPluginManager->FindModule<NFIEventModule>()->UnSubscribe(this, serverType, nEventID, bySrcType, nSrcID);
}

//取消所有执行事件的订阅
bool NFEventObj::UnSubscribeAll()
{
	return m_pObjPluginManager->FindModule<NFIEventModule>()->UnSubscribeAll(this);
}

