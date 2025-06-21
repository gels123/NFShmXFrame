﻿// -------------------------------------------------------------------------
//    @FileName         :    NFGameServerModule.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFGameServerPlugin
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFPluginModule/NFServerDefine.h"
#include "NFComm/NFCore/NFCommMapEx.hpp"
#include "NFComm/NFCore/NFCommMap.hpp"
#include "NFServerComm/NFServerCommon/NFIRouteAgentServerModule.h"

class NFIMessageModule;
class NFCRouteAgentServerModule : public NFIRouteAgentServerModule
{
public:
	explicit NFCRouteAgentServerModule(NFIPluginManager* p);
	virtual ~NFCRouteAgentServerModule();

	virtual bool Awake() override;

	virtual bool Init() override;

	virtual bool Execute() override;

	virtual bool OnDynamicPlugin() override;

    virtual int OnTimer(uint32_t nTimerID) override;

    virtual int OnExecute(uint32_t serverType, uint32_t nEventID, uint32_t bySrcType, uint64_t nSrcID, const google::protobuf::Message* pMessage) override;

	int OnRouteAgentSocketEvent(eMsgType nEvent, uint64_t unLinkId);
	int OnHandleOtherMessage(uint64_t unLinkId, NFDataPackage& packet);

	int OnServerRegisterProcess(uint64_t unLinkId, NFDataPackage& packet);
	int OnHandleServerReport(uint64_t unLinkId, NFDataPackage& packet);

	int RegisterMasterServer(uint32_t serverState);
	int ServerReport();

	int OnHandleRouteServerReport(const NFrame::ServerInfoReport& xData);
	int OnRouteServerSocketEvent(eMsgType nEvent, uint64_t unLinkId);
	int OnHandleRouteOtherMessage(uint64_t unLinkId, NFDataPackage& packet);
	int RegisterRouteServer(uint64_t unLinkId);

	int RegisterServerInfoToRouteSvr(const NFrame::ServerInfoReportList& xData);
    int RegisterAllServerInfoToRouteSvr();

	int OnHandleServerDisconnect(uint64_t unLinkId);

	/*
		处理Master服务器链接事件和未注册消息
	*/
    int ConnectMasterServer(const NFrame::ServerInfoReport& xData);
	int OnMasterSocketEvent(eMsgType nEvent, uint64_t unLinkId);
	int OnHandleMasterOtherMessage(uint64_t unLinkId, NFDataPackage& packet);
};
