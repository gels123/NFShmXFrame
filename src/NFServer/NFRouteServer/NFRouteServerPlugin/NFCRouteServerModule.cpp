// -------------------------------------------------------------------------
//    @FileName         :    NFGameServerModule.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFGameServerPlugin
//
// -------------------------------------------------------------------------

#include <NFServerComm/NFServerCommon/NFServerCommonDefine.h>
#include "NFCRouteServerModule.h"

#include "NFComm/NFPluginModule/NFIPluginManager.h"
#include "NFComm/NFPluginModule/NFIConfigModule.h"
#include "NFComm/NFPluginModule/NFIMessageModule.h"
#include "NFComm/NFPluginModule/NFCheck.h"
#include "NFServerComm/NFServerCommon/NFIServerMessageModule.h"
#include "NFComm/NFPluginModule/NFIMonitorModule.h"
#include "NFComm/NFCore/NFServerIDUtil.h"

#define ROUTE_SERVER_CONNECT_MASTER_SERVER "RouteServer Connect MasterServer"

NFCRouteServerModule::NFCRouteServerModule(NFIPluginManager* p) : NFIRouteServerModule(p)
{
}

NFCRouteServerModule::~NFCRouteServerModule()
{
}

bool NFCRouteServerModule::Awake()
{
    //不需要固定帧，需要尽可能跑得快
    //m_pObjPluginManager->SetFixedFrame(false);
    FindModule<NFIMessageModule>()->AddMessageCallBack(NF_ST_ROUTE_SERVER, NF_MODULE_FRAME, NFrame::NF_SERVER_TO_SERVER_REGISTER, this,
                                                       &NFCRouteServerModule::OnServerRegisterProcess);
    FindModule<NFIMessageModule>()->AddMessageCallBack(NF_ST_ROUTE_SERVER,
                                                       NF_MODULE_FRAME, NFrame::NF_MASTER_SERVER_SEND_OTHERS_TO_SERVER, this,
                                                       &NFCRouteServerModule::OnHandleServerReport);
    FindModule<NFIMessageModule>()->AddMessageCallBack(NF_ST_ROUTE_SERVER,
                                                       NF_MODULE_FRAME, NFrame::NF_ROUTER_CMD_INTERNAL_C2R_REG_RAASSOCAPPSVS, this,
                                                       &NFCRouteServerModule::OnHandleServerRegisterRouteAgent);
    
    //注册要完成的服务器启动任务
    RegisterAppTask(NF_ST_ROUTE_SERVER, APP_INIT_CONNECT_MASTER, ROUTE_SERVER_CONNECT_MASTER_SERVER, APP_INIT_TASK_GROUP_SERVER_CONNECT);
    
    NFServerConfig* pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_ROUTE_SERVER);
    if (pConfig)
    {
        m_pObjPluginManager->SetIdleSleepUs(pConfig->IdleSleepUS);
        uint64_t unlinkId = FindModule<NFIMessageModule>()->BindServer(NF_ST_ROUTE_SERVER, pConfig->Url,
                                                                       pConfig->NetThreadNum, pConfig->MaxConnectNum, PACKET_PARSE_TYPE_INTERNAL);
        if (unlinkId > 0)
        {
            /*
                注册客户端事件
            */
            uint64_t routeServerLinkId = unlinkId;
            FindModule<NFIMessageModule>()->SetServerLinkId(NF_ST_ROUTE_SERVER, routeServerLinkId);
            FindModule<NFIMessageModule>()->AddEventCallBack(NF_ST_ROUTE_SERVER, routeServerLinkId, this,
                                                             &NFCRouteServerModule::OnRouteSocketEvent);
            FindModule<NFIMessageModule>()->AddOtherCallBack(NF_ST_ROUTE_SERVER, routeServerLinkId, this,
                                                             &NFCRouteServerModule::OnHandleOtherMessage);
            NFLogInfo(NF_LOG_DEFAULT, 0, "route server listen success, serverId:{}, ip:{}, port:{}",
                      pConfig->ServerId, pConfig->ServerIp, pConfig->ServerPort);
        }
        else
        {
            NFLogInfo(NF_LOG_DEFAULT, 0, "route server listen failed, serverId:{}, ip:{}, port:{}",
                      pConfig->ServerId, pConfig->ServerIp, pConfig->ServerPort);
            return false;
        }
    }
    else
    {
        NFLogError(NF_LOG_DEFAULT, 0, "I Can't get the Game Server config!");
        return false;
    }
    
    Subscribe(NF_ST_ROUTE_SERVER, NFrame::NF_EVENT_SERVER_DEAD_EVENT, NFrame::NF_EVENT_SERVER_TYPE, 0, __FUNCTION__);
    Subscribe(NF_ST_ROUTE_SERVER, NFrame::NF_EVENT_SERVER_APP_FINISH_INITED, NFrame::NF_EVENT_SERVER_TYPE, 0, __FUNCTION__);
    
    return true;
}

int NFCRouteServerModule::OnExecute(uint32_t serverType, uint32_t nEventID, uint32_t bySrcType, uint64_t nSrcID, const google::protobuf::Message* pMessage)
{
    CHECK_EXPR(serverType == NF_ST_ROUTE_SERVER, -1, "");
    if (bySrcType == NFrame::NF_EVENT_SERVER_TYPE)
    {
        if (nEventID == NFrame::NF_EVENT_SERVER_DEAD_EVENT)
        {
            SetTimer(10000, 10000, 0);
        }
        else if (nEventID == NFrame::NF_EVENT_SERVER_APP_FINISH_INITED)
        {
            RegisterMasterServer(NFrame::EST_NARMAL);
        }
    }
    
    return 0;
}

int NFCRouteServerModule::OnTimer(uint32_t nTimerID)
{
    if (nTimerID == 10000)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "kill the exe..................");
        NFSLEEP(1000);
        exit(0);
    }
    return 0;
}

int NFCRouteServerModule::ConnectMasterServer(const NFrame::ServerInfoReport& xData)
{
    NFServerConfig* pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_ROUTE_SERVER);
    if (pConfig)
    {
        auto pMasterServerData = FindModule<NFIMessageModule>()->GetMasterData(NF_ST_ROUTE_SERVER);
        if (pMasterServerData->mUnlinkId <= 0)
        {
            pMasterServerData->mUnlinkId = FindModule<NFIMessageModule>()->ConnectServer(NF_ST_ROUTE_SERVER, xData.url(),
                                                                                         PACKET_PARSE_TYPE_INTERNAL);
            
            FindModule<NFIMessageModule>()->AddEventCallBack(NF_ST_ROUTE_SERVER, pMasterServerData->mUnlinkId, this,
                                                             &NFCRouteServerModule::OnMasterSocketEvent);
            FindModule<NFIMessageModule>()->AddOtherCallBack(NF_ST_ROUTE_SERVER, pMasterServerData->mUnlinkId, this,
                                                             &NFCRouteServerModule::OnHandleMasterOtherMessage);
        }
        
        pMasterServerData->mServerInfo = xData;
    }
    else
    {
        NFLogError(NF_LOG_DEFAULT, 0, "I Can't get the route Server config!");
        return -1;
    }
    
    return 0;
}

bool NFCRouteServerModule::Init()
{
#if NF_PLATFORM == NF_PLATFORM_WIN
    NFrame::ServerInfoReport masterData = FindModule<NFIConfigModule>()->GetDefaultMasterInfo(NF_ST_ROUTE_SERVER);
    int32_t ret = ConnectMasterServer(masterData);
    CHECK_EXPR(ret == 0, false, "ConnectMasterServer Failed, url:{}", masterData.DebugString());
#else
    NFServerConfig* pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_ROUTE_SERVER);
    if (pConfig && pConfig->RouteConfig.NamingHost.empty())
    {
        NFrame::ServerInfoReport masterData = FindModule<NFIConfigModule>()->GetDefaultMasterInfo(NF_ST_ROUTE_SERVER);
        int32_t ret = ConnectMasterServer(masterData);
        CHECK_EXPR(ret == 0, false, "ConnectMasterServer Failed, url:{}", masterData.DebugString());
    }
#endif
    
    return true;
}

bool NFCRouteServerModule::Execute()
{
    ServerReport();
    return true;
}

bool NFCRouteServerModule::OnDynamicPlugin()
{
    FindModule<NFIMessageModule>()->CloseAllLink(NF_ST_ROUTE_SERVER);
    return true;
}

int NFCRouteServerModule::OnRouteSocketEvent(eMsgType nEvent, uint64_t unLinkId)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    if (nEvent == eMsgType_CONNECTED)
    {
    
    }
    else if (nEvent == eMsgType_DISCONNECTED)
    {
        OnHandleServerDisconnect(unLinkId);
    }
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCRouteServerModule::OnHandleServerDisconnect(uint64_t unLinkId)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    NF_SHARE_PTR<NFServerData> pServerData = FindModule<NFIMessageModule>()->GetServerByUnlinkId(NF_ST_ROUTE_SERVER,
                                                                                                 unLinkId);
    if (pServerData)
    {
        pServerData->mServerInfo.set_server_state(NFrame::EST_CRASH);
        pServerData->mUnlinkId = 0;
        
        NFLogError(NF_LOG_DEFAULT, 0,
                   "the server disconnect from route server, serverName:{}, busid:{}, busname:{}. serverIp:{}, serverPort:{}",
                   pServerData->mServerInfo.server_name(), pServerData->mServerInfo.bus_id(),
                   pServerData->mServerInfo.server_id(), pServerData->mServerInfo.server_ip(),
                   pServerData->mServerInfo.server_port());
    }
    
    FindModule<NFIMessageModule>()->DelServerLink(NF_ST_ROUTE_SERVER, unLinkId);
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCRouteServerModule::OnHandleOtherMessage(uint64_t unLinkId, NFDataPackage& packet)
{
    uint32_t fromBusId = GetBusIdFromUnlinkId(packet.nSrcId);
    uint32_t fromServerType = GetServerTypeFromUnlinkId(packet.nSrcId);
    
    uint32_t serverType = GetServerTypeFromUnlinkId(packet.nDstId);
    uint32_t destBusId = GetBusIdFromUnlinkId(packet.nDstId);
    
    auto pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_ROUTE_SERVER);
    CHECK_EXPR(pConfig != NULL, -1, "pConfig == NULL");
    
    NF_SHARE_PTR<NFServerData> pServerData = FindModule<NFIMessageModule>()->GetServerByUnlinkId(NF_ST_ROUTE_SERVER, unLinkId);
    CHECK_EXPR(pServerData != NULL, -1, "pServer == NULL");

    if (packet.nErrCode == NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST)
    {
        NFLogError(NF_LOG_DEFAULT, 0,
           "the trans msg failed, can't find dest server, --{}:{} cant' trans route agent server({}:{}) msg from {}:{} to {}:{}, packet:{}", pConfig->ServerName, pConfig->ServerId, pServerData->mServerInfo.server_name(), pServerData->mServerInfo.server_id(),
           GetServerName((NF_SERVER_TYPE)fromServerType), NFServerIDUtil::GetBusNameFromBusID(fromBusId), GetServerName((NF_SERVER_TYPE)serverType), NFServerIDUtil::GetBusNameFromBusID(destBusId), packet.ToString());

        NF_SHARE_PTR<NFServerData> pRegServerData = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_SERVER, fromBusId);
        if (pRegServerData)
        {
            NF_SHARE_PTR<NFServerData> pRouteAgent = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_SERVER, pRegServerData->mRouteAgentBusId);
            if (pRouteAgent)
            {
                FindModule<NFIMessageModule>()->TransPackage(pRouteAgent->mUnlinkId, packet);
            }
            else
            {
                NFLogError(NF_LOG_DEFAULT, 0, "the trans msg failed, can't find dest server, packet:{} trans failed, fromServer:{}:{} to destServer:{}:{}",
                           packet.ToString(), GetServerName((NF_SERVER_TYPE)fromServerType),
                           NFServerIDUtil::GetBusNameFromBusID(fromBusId), GetServerName((NF_SERVER_TYPE)serverType), NFServerIDUtil::GetBusNameFromBusID(destBusId));
            }
        }
        else
        {
            NFLogError(NF_LOG_DEFAULT, 0, "the trans msg failed, can't find dest server, packet:{} trans failed, fromServer:{}:{} to destServer:{}:{}",
                       packet.ToString(), GetServerName((NF_SERVER_TYPE)fromServerType),
                       NFServerIDUtil::GetBusNameFromBusID(fromBusId), GetServerName((NF_SERVER_TYPE)serverType), NFServerIDUtil::GetBusNameFromBusID(destBusId));
        }
        return 0;
    }

    if (destBusId <= LOCAL_AND_CROSS_MAX)
    {
        NFLogInfo(NF_LOG_DEFAULT, 0,
            "--{}:{} trans route agent server({}:{}) msg from {}:{} to {}:{}, packet:{} --", pConfig->ServerName, pConfig->ServerId, pServerData->mServerInfo.server_name(), pServerData->mServerInfo.server_id(),
            GetServerName((NF_SERVER_TYPE)fromServerType), NFServerIDUtil::GetBusNameFromBusID(fromBusId), GetServerName((NF_SERVER_TYPE)serverType), destBusId, packet.ToString());
    }
    else
    {
        NFLogInfo(NF_LOG_DEFAULT, 0,
            "--{}:{} trans route agent server({}:{}) msg from {}:{} to {}:{}, packet:{} --", pConfig->ServerName, pConfig->ServerId, pServerData->mServerInfo.server_name(), pServerData->mServerInfo.server_id(),
            GetServerName((NF_SERVER_TYPE)fromServerType), NFServerIDUtil::GetBusNameFromBusID(fromBusId), GetServerName((NF_SERVER_TYPE)serverType), NFServerIDUtil::GetBusNameFromBusID(destBusId), packet.ToString());
    }

    /**
    * @brief 当目标busId==LOCAL_ROUTE, 本服路由机制，除非明确表示要发往跨服服务器，否则就是本服路由(包过跨服服务器的本服路由) (需要保证本跨服服务器只能连接跨服route agent， 不跨服服务器只能连接不跨服的route agent.)
    */
    if (destBusId == LOCAL_ROUTE)
    {
        NF_SHARE_PTR<NFServerData> pRegServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_SERVER, (NF_SERVER_TYPE)serverType, pConfig->IsCrossServer());
        if (pRegServerData)
        {
            NF_SHARE_PTR<NFServerData> pRouteAgent = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_SERVER, pRegServerData->mRouteAgentBusId);
            if (pRouteAgent)
            {
                FindModule<NFIMessageModule>()->TransPackage(pRouteAgent->mUnlinkId, packet);
            }
            else
            {
                packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
                FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
                NFLogError(NF_LOG_DEFAULT, 0, "packet:{} trans failed, fromServer:{}:{} to destServer:{}:{}",
                           packet.ToString(), GetServerName((NF_SERVER_TYPE)fromServerType), NFServerIDUtil::GetBusNameFromBusID(fromBusId), GetServerName((NF_SERVER_TYPE)serverType), NFServerIDUtil::GetBusNameFromBusID(destBusId));
            }
        }
        else
        {
            packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
            FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
            NFLogError(NF_LOG_DEFAULT, 0, "can't find destBusId, packet:{} trans failed, fromServer:{}:{} to destServer:{}:{}",
                       packet.ToString(), GetServerName((NF_SERVER_TYPE)fromServerType),
                       NFServerIDUtil::GetBusNameFromBusID(fromBusId), GetServerName((NF_SERVER_TYPE)serverType), NFServerIDUtil::GetBusNameFromBusID(destBusId));
        }
    }
    /**
     * @brief 本服索引路由  LOCAL_ROUTE+index       本服路由机制，除非明确表示要发往跨服服务器，否则就是本服路由(包过跨服服务器的本服路由) (需要保证本跨服服务器只能连接跨服route agent， 不跨服服务器只能连接不跨服的route agent.)
     */
    else if (destBusId > LOCAL_ROUTE && destBusId < CROSS_ROUTE)
    {
        uint32_t index = destBusId - LOCAL_ROUTE;
        uint32_t realDestBusId = NFServerIDUtil::MakeProcID(pConfig->GetWorldId(), pConfig->GetZoneId(), serverType, index);
        NF_SHARE_PTR<NFServerData> pRegServerData = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_SERVER, realDestBusId);
        if (pRegServerData)
        {
            NF_SHARE_PTR<NFServerData> pRouteAgent = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_SERVER, pRegServerData->mRouteAgentBusId);
            if (pRouteAgent)
            {
                FindModule<NFIMessageModule>()->TransPackage(pRouteAgent->mUnlinkId, packet);
            }
            else
            {
                packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
                FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
                NFLogError(NF_LOG_DEFAULT, 0, "packet:{} trans failed, fromServer:{}:{} to destServer:{}:{}",
                           packet.ToString(), GetServerName((NF_SERVER_TYPE)fromServerType),
                           NFServerIDUtil::GetBusNameFromBusID(fromBusId), GetServerName((NF_SERVER_TYPE)serverType), NFServerIDUtil::GetBusNameFromBusID(realDestBusId));
            }
        }
        else
        {
            packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
            FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
            NFLogError(NF_LOG_DEFAULT, 0, "can't find destBusId, packet:{} trans failed, fromServer:{}:{} to destServer:{}:{}",
                       packet.ToString(), GetServerName((NF_SERVER_TYPE)fromServerType),
                       NFServerIDUtil::GetBusNameFromBusID(fromBusId), GetServerName((NF_SERVER_TYPE)serverType), NFServerIDUtil::GetBusNameFromBusID(realDestBusId));
        }
    }
    /**
     * @brief 跨服路由(明确指定要找跨服服务器， 才走跨服路由)
     */
    else if (destBusId == CROSS_ROUTE)
    {
        NF_SHARE_PTR<NFServerData> pRegServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_SERVER, (NF_SERVER_TYPE)serverType, true);
        if (pRegServerData)
        {
            NF_SHARE_PTR<NFServerData> pRouteAgent = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_SERVER, pRegServerData->mRouteAgentBusId);
            if (pRouteAgent)
            {
                FindModule<NFIMessageModule>()->TransPackage(pRouteAgent->mUnlinkId, packet);
            }
            else
            {
                packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
                FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
                NFLogError(NF_LOG_DEFAULT, 0, "packet:{} trans failed, fromServer:{}:{} to destServer:{}:CROSS_ROUTE",
                           packet.ToString(), GetServerName((NF_SERVER_TYPE)fromServerType), NFServerIDUtil::GetBusNameFromBusID(fromBusId), GetServerName((NF_SERVER_TYPE)serverType));
            }
        }
        else
        {
            packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
            FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
            NFLogError(NF_LOG_DEFAULT, 0, "can't find destBusId, packet:{} trans failed, fromServer:{}:{} to destServer:{}:CROSS_ROUTE",
                       packet.ToString(), GetServerName((NF_SERVER_TYPE)fromServerType),
                       NFServerIDUtil::GetBusNameFromBusID(fromBusId), GetServerName((NF_SERVER_TYPE)serverType));
        }
    }
    //跨服索引路由 CROSS_ROUTE+index          明确指定要找跨服服务器， 才走跨服路由
    else if (destBusId > CROSS_ROUTE && destBusId < LOCAL_ROUTE_ZONE)
    {
        if (!pConfig->IsCrossServer())
        {
            NFLogError(NF_LOG_DEFAULT, 0, "destBusId:{} route error, the pConfig is not cross server", destBusId);
            packet.nErrCode = NFrame::ERR_CODE_ROUTER_NOT_SUPPORTTED;
            FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
            return 0;
        }
        uint32_t index = destBusId - CROSS_ROUTE;
        uint32_t realDestBusId = NFServerIDUtil::MakeProcID(pConfig->GetWorldId(), pConfig->GetZoneId(), serverType, index);
        NF_SHARE_PTR<NFServerData> pRegServerData = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_SERVER, realDestBusId);
        if (pRegServerData)
        {
            NF_SHARE_PTR<NFServerData> pRouteAgent = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_SERVER, pRegServerData->mRouteAgentBusId);
            if (pRouteAgent)
            {
                FindModule<NFIMessageModule>()->TransPackage(pRouteAgent->mUnlinkId, packet);
            }
            else
            {
                packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
                FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
                NFLogError(NF_LOG_DEFAULT, 0, "packet:{} trans failed, fromServer:{}:{} to destServer:{}:{}",
                           packet.ToString(), GetServerName((NF_SERVER_TYPE)fromServerType),
                           NFServerIDUtil::GetBusNameFromBusID(fromBusId), GetServerName((NF_SERVER_TYPE)serverType), NFServerIDUtil::GetBusNameFromBusID(realDestBusId));
            }
        }
        else
        {
            packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
            FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
            NFLogError(NF_LOG_DEFAULT, 0, "can't find destBusId, packet:{} trans failed, fromServer:{}:{} to destServer:{}:{}",
                       packet.ToString(), GetServerName((NF_SERVER_TYPE)fromServerType),
                       NFServerIDUtil::GetBusNameFromBusID(fromBusId), GetServerName((NF_SERVER_TYPE)serverType), NFServerIDUtil::GetBusNameFromBusID(realDestBusId));
        }
    }
    /**
     * @brief 区服路由  LOCAL_ROUTE_ZONE+区服的zid(1-4096) 只有跨服route server服务器，才有区服路由的能力
     */
    else if (destBusId > LOCAL_ROUTE_ZONE && destBusId < CROSS_ROUTE_ZONE)
    {
        uint32_t realDestBusId = destBusId - LOCAL_ROUTE_ZONE;
        if (pConfig->GetZoneId() != realDestBusId)
        {
            if (!pConfig->IsCrossServer())
            {
                NFLogError(NF_LOG_DEFAULT, 0, "destBusId:{} zid route error, the pConfig is not cross server", realDestBusId);
                packet.nErrCode = NFrame::ERR_CODE_ROUTER_NOT_SUPPORTTED;
                FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
                return 0;
            }
        }

        bool find = false;
        std::vector<NF_SHARE_PTR<NFServerData>> allServer = FindModule<NFIMessageModule>()->GetAllServer(NF_ST_ROUTE_SERVER, (NF_SERVER_TYPE)serverType);
        for(int i = 0; i < (int)allServer.size(); i++)
        {
            auto pRegServerData = allServer[i];
            if (pRegServerData && NFServerIDUtil::GetZoneID(pRegServerData->mServerInfo.bus_id()) == realDestBusId)
            {
                NF_SHARE_PTR<NFServerData> pRouteAgent = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_SERVER, pRegServerData->mRouteAgentBusId);
                if (pRouteAgent)
                {
                    FindModule<NFIMessageModule>()->TransPackage(pRouteAgent->mUnlinkId, packet);
                    find = true;
                }
                else
                {
                    NFLogError(NF_LOG_DEFAULT, 0, "packet:{} trans failed, fromServer:{}:{} to destServer:{}:{}",
                               packet.ToString(), GetServerName((NF_SERVER_TYPE)fromServerType), NFServerIDUtil::GetBusNameFromBusID(fromBusId), GetServerName((NF_SERVER_TYPE)serverType), NFServerIDUtil::GetBusNameFromBusID(realDestBusId));
                }
                break;
            }
        }

        if (!find)
        {
            packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
            FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
            NFLogError(NF_LOG_DEFAULT, 0, "can't find destBusId, packet:{} trans failed, fromServer:{}:{} to destServer:{}, Zid:{}",
                       packet.ToString(), GetServerName((NF_SERVER_TYPE)fromServerType),
                       NFServerIDUtil::GetBusNameFromBusID(fromBusId), GetServerName((NF_SERVER_TYPE)serverType), realDestBusId)
        }
    }
    /**
     * @brief 跨服路由同服务器类型群发路由 (最大分区4096， 所以CROSS_ROUTE_ZONE+zoneid
     */
    else if (destBusId > CROSS_ROUTE_ZONE && destBusId < LOCAL_ALL_ROUTE)
    {
        uint32_t realDestBusId = destBusId - CROSS_ROUTE_ZONE;
        if (pConfig->GetZoneId() != realDestBusId)
        {
            if (!pConfig->IsCrossServer())
            {
                NFLogError(NF_LOG_DEFAULT, 0, "destBusId:{} zid route error, the pConfig is not cross server", destBusId);
                packet.nErrCode = NFrame::ERR_CODE_ROUTER_NOT_SUPPORTTED;
                FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
                return 0;
            }
        }

        std::vector<NF_SHARE_PTR<NFServerData>> allServer = FindModule<NFIMessageModule>()->GetAllServer(NF_ST_ROUTE_SERVER, NF_ST_ROUTE_AGENT_SERVER);
        for(int i = 0; i < (int)allServer.size(); i++)
        {
            auto pRouteServerData = allServer[i];
            if (pRouteServerData && NFServerIDUtil::GetZoneID(pRouteServerData->mServerInfo.bus_id()) == realDestBusId)
            {
                FindModule<NFIMessageModule>()->TransPackage(pRouteServerData->mUnlinkId, packet);
            }
        }
    }
    else if (destBusId == LOCAL_ALL_ROUTE)
    {
        std::vector<NF_SHARE_PTR<NFServerData>> vecRegServerData = FindModule<NFIMessageModule>()->GetAllServer(NF_ST_ROUTE_SERVER, NF_ST_ROUTE_AGENT_SERVER, pConfig->IsCrossServer());
        if (vecRegServerData.size() > 0)
        {
            for (int i = 0; i < (int)vecRegServerData.size(); i++)
            {
                auto pRegServerData = vecRegServerData[i];
                if (pRegServerData)
                {
                    FindModule<NFIMessageModule>()->TransPackage(pRegServerData->mUnlinkId, packet);
                }
            }
        }
    }
    else if (destBusId == CROSS_ALL_ROUTE)
    {
        std::vector<NF_SHARE_PTR<NFServerData>> vecRegServerData = FindModule<NFIMessageModule>()->GetAllServer(NF_ST_ROUTE_SERVER, NF_ST_ROUTE_AGENT_SERVER, true);
        if (vecRegServerData.size() > 0)
        {
            for (int i = 0; i < (int)vecRegServerData.size(); i++)
            {
                auto pRegServerData = vecRegServerData[i];
                if (pRegServerData)
                {
                    FindModule<NFIMessageModule>()->TransPackage(pRegServerData->mUnlinkId, packet);
                }
            }
        }
    }
    else if (destBusId == LOCAL_AND_CROSS_ALL_ROUTE)
    {
        std::vector<NF_SHARE_PTR<NFServerData>> vecRegServerData = FindModule<NFIMessageModule>()->GetAllServer(NF_ST_ROUTE_SERVER, NF_ST_ROUTE_AGENT_SERVER, pConfig->IsCrossServer());
        if (vecRegServerData.size() > 0)
        {
            for (int i = 0; i < (int)vecRegServerData.size(); i++)
            {
                auto pRegServerData = vecRegServerData[i];
                if (pRegServerData)
                {
                    FindModule<NFIMessageModule>()->TransPackage(pRegServerData->mUnlinkId, packet);
                }
            }
        }
    }
    else if (destBusId == ALL_LOCAL_AND_ALL_CROSS_ROUTE)
    {
        std::vector<NF_SHARE_PTR<NFServerData>> vecRegServerData = FindModule<NFIMessageModule>()->GetAllServer(NF_ST_ROUTE_SERVER, NF_ST_ROUTE_AGENT_SERVER);
        if (vecRegServerData.size() > 0)
        {
            for (int i = 0; i < (int)vecRegServerData.size(); i++)
            {
                auto pRegServerData = vecRegServerData[i];
                if (pRegServerData)
                {
                    FindModule<NFIMessageModule>()->TransPackage(pRegServerData->mUnlinkId, packet);
                }
            }
        }
    }
    else
    {
        NF_SHARE_PTR<NFServerData> pRegServerData = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_SERVER, destBusId);
        if (pRegServerData)
        {
            NF_SHARE_PTR<NFServerData> pRouteAgent = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_SERVER, pRegServerData->mRouteAgentBusId);
            if (pRouteAgent)
            {
                FindModule<NFIMessageModule>()->TransPackage(pRouteAgent->mUnlinkId, packet);
            }
            else
            {
                packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
                FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
                NFLogError(NF_LOG_DEFAULT, 0, "packet:{} trans failed, fromServer:{}:{} to destServer:{}:{}",
                           packet.ToString(), GetServerName((NF_SERVER_TYPE)fromServerType),
                           NFServerIDUtil::GetBusNameFromBusID(fromBusId), GetServerName((NF_SERVER_TYPE)serverType), NFServerIDUtil::GetBusNameFromBusID(destBusId));
            }
        }
        else
        {
            packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
            FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
            NFLogError(NF_LOG_DEFAULT, 0, "can't find destBusId, packet:{} trans failed, fromServer:{}:{} to destServer:{}:{}",
                       packet.ToString(), GetServerName((NF_SERVER_TYPE)fromServerType),
                       NFServerIDUtil::GetBusNameFromBusID(fromBusId), GetServerName((NF_SERVER_TYPE)serverType), NFServerIDUtil::GetBusNameFromBusID(destBusId));
        }
    }

    return 0;
}

/*
	处理Master服务器链接事件
*/
int NFCRouteServerModule::OnMasterSocketEvent(eMsgType nEvent, uint64_t unLinkId)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    
    if (nEvent == eMsgType_CONNECTED)
    {
        std::string ip = FindModule<NFIMessageModule>()->GetLinkIp(unLinkId);
        NFLogDebug(NF_LOG_DEFAULT, 0, "route server connect master success!");
        
        if (!m_pObjPluginManager->IsInited(NF_ST_ROUTE_SERVER))
        {
            RegisterMasterServer(NFrame::EST_INIT);
        }
        else
        {
            RegisterMasterServer(NFrame::EST_NARMAL);
        }
        
        //完成服务器启动任务
        if (!m_pObjPluginManager->IsInited(NF_ST_ROUTE_SERVER))
        {
            FinishAppTask(NF_ST_ROUTE_SERVER, APP_INIT_CONNECT_MASTER, APP_INIT_TASK_GROUP_SERVER_CONNECT);
        }
        
    }
    else if (nEvent == eMsgType_DISCONNECTED)
    {
        std::string ip = FindModule<NFIMessageModule>()->GetLinkIp(unLinkId);
        NFLogError(NF_LOG_DEFAULT, 0, "route server disconnect master success");
    }
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

/*
	处理Master服务器未注册协议
*/
int NFCRouteServerModule::OnHandleMasterOtherMessage(uint64_t unLinkId, NFDataPackage& packet)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string ip = FindModule<NFIMessageModule>()->GetLinkIp(unLinkId);
    NFLogWarning(NF_LOG_DEFAULT, 0, "master server other message not handled:packet:{},ip:{}", packet.ToString(), ip);
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCRouteServerModule::RegisterMasterServer(uint32_t serverState)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    NFServerConfig* pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_ROUTE_SERVER);
    if (pConfig)
    {
        NFrame::ServerInfoReportList xMsg;
        NFrame::ServerInfoReport* pData = xMsg.add_server_list();
        NFServerCommon::WriteServerInfo(pData, pConfig);
        pData->set_server_state(serverState);
        
        FindModule<NFIServerMessageModule>()->SendMsgToMasterServer(NF_ST_ROUTE_SERVER, NF_MODULE_FRAME, NFrame::NF_SERVER_TO_SERVER_REGISTER, xMsg);
    }
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCRouteServerModule::ServerReport()
{
    if (m_pObjPluginManager->IsLoadAllServer())
    {
        return 0;
    }
    
    static uint64_t mLastReportTime = m_pObjPluginManager->GetNowTime();
    if (mLastReportTime + 100000 > m_pObjPluginManager->GetNowTime())
    {
        return 0;
    }
    
    mLastReportTime = m_pObjPluginManager->GetNowTime();
    
    NFServerConfig* pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_ROUTE_SERVER);
    if (pConfig)
    {
        NFrame::ServerInfoReportList xMsg;
        NFrame::ServerInfoReport* pData = xMsg.add_server_list();
        NFServerCommon::WriteServerInfo(pData, pConfig);
        pData->set_server_state(NFrame::EST_NARMAL);
        
        NFIMonitorModule* pMonitorModule = m_pObjPluginManager->FindModule<NFIMonitorModule>();
        if (pMonitorModule)
        {
            const NFSystemInfo& systemInfo = pMonitorModule->GetSystemInfo();
            NFServerCommon::WriteServerInfo(pData, systemInfo);
        }
        
        if (pData->proc_cpu() > 0 && pData->proc_mem() > 0)
        {
            FindModule<NFIServerMessageModule>()->SendMsgToMasterServer(NF_ST_ROUTE_SERVER, NF_MODULE_FRAME, NFrame::NF_SERVER_TO_MASTER_SERVER_REPORT, xMsg);
        }
    }
    return 0;
}

int
NFCRouteServerModule::OnServerRegisterProcess(uint64_t unLinkId, NFDataPackage& packet)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    NFrame::ServerInfoReportList xMsg;
    CLIENT_MSG_PROCESS_WITH_PRINTF(packet, xMsg);
    
    for (int i = 0; i < xMsg.server_list_size(); ++i)
    {
        const NFrame::ServerInfoReport& xData = xMsg.server_list(i);
        switch (xData.server_type())
        {
            case NF_SERVER_TYPE::NF_ST_ROUTE_AGENT_SERVER:
            {
                OnHandleRouteAgentRegister(xData, unLinkId);
            }
                break;
            default:break;
        }
    }
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCRouteServerModule::OnHandleServerReport(uint64_t unLinkId, NFDataPackage& packet)
{
    return 0;
}

int NFCRouteServerModule::OnHandleServerRegisterRouteAgent(uint64_t unLinkId, NFDataPackage& packet)
{
//    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    NFrame::ServerInfoReportList xMsg;
    CLIENT_MSG_PROCESS_NO_PRINTF(packet, xMsg);
    
    NFServerConfig* pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_ROUTE_SERVER);
    CHECK_NULL(0, pConfig);
    
    NF_SHARE_PTR<NFServerData> pServerData = FindModule<NFIMessageModule>()->GetServerByUnlinkId(NF_ST_ROUTE_SERVER, unLinkId);
    CHECK_EXPR(pServerData != NULL, -1, "pServerData == NULL");
    CHECK_EXPR(pServerData->mServerInfo.server_type() == NF_ST_ROUTE_AGENT_SERVER, -1, "pServerData server type error");
    
    for (int i = 0; i < xMsg.server_list_size(); ++i)
    {
        const NFrame::ServerInfoReport& xData = xMsg.server_list(i);
        NF_SHARE_PTR<NFServerData> pRegServerData = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_SERVER, xData.bus_id());
        if (pRegServerData)
        {
            if (pRegServerData->mRouteAgentBusId == pServerData->mServerInfo.bus_id())
            {
                pRegServerData->mServerInfo = xData;
            }
            else
            {
                NF_SHARE_PTR<NFServerData> pOldServerData = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_SERVER,
                                                                                                                pRegServerData->mRouteAgentBusId);
                if (pOldServerData)
                {
                    NFLogError(NF_LOG_DEFAULT, 0, "{}({}) has register {}({}), now register {}({})",
                               xData.server_name(), xData.bus_id(),
                               pOldServerData->mServerInfo.server_name(), pOldServerData->mServerInfo.bus_id(),
                               pServerData->mServerInfo.server_name(), pServerData->mServerInfo.bus_id());
                }
                else
                {
                    NFLogError(NF_LOG_DEFAULT, 0, "{}({}) has register {}(can't find), now register {}({})",
                               xData.server_name(), xData.bus_id(),
                               pRegServerData->mRouteAgentBusId,
                               pServerData->mServerInfo.server_name(), pServerData->mServerInfo.bus_id());
                }
                
                pRegServerData->mRouteAgentBusId = pServerData->mServerInfo.bus_id();
                pRegServerData->mServerInfo = xData;
            }
        }
        else
        {
            pRegServerData = FindModule<NFIMessageModule>()->CreateServerByServerId(NF_ST_ROUTE_SERVER, xData.bus_id(), (NF_SERVER_TYPE)xData.server_type(), xData);
            pRegServerData->mRouteAgentBusId = pServerData->mServerInfo.bus_id();
            pRegServerData->mServerInfo = xData;
            
            NFLogInfo(NF_LOG_DEFAULT, 0, "{}({}) register route agent:{}({}) trans to route svr:{}({}) success",
                      pRegServerData->mServerInfo.server_name(), pRegServerData->mServerInfo.server_id(),
                      pServerData->mServerInfo.server_name(), pServerData->mServerInfo.server_id(),
                      pConfig->ServerName, pConfig->ServerId);
        }
    }

//    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

//游戏服务器注册协议回调
int NFCRouteServerModule::OnHandleRouteAgentRegister(const NFrame::ServerInfoReport& xData, uint64_t unlinkId)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    CHECK_EXPR(xData.server_type() == NF_ST_ROUTE_AGENT_SERVER, -1, "xData.server_type() == NF_ST_ROUTE_AGENT_SERVER");
    NFServerConfig* pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_ROUTE_SERVER);
    CHECK_NULL(0, pConfig);
    
    NF_SHARE_PTR<NFServerData> pServerData = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_SERVER, xData.bus_id());
    if (!pServerData)
    {
        pServerData = FindModule<NFIMessageModule>()->CreateServerByServerId(NF_ST_ROUTE_SERVER, xData.bus_id(), NF_ST_ROUTE_AGENT_SERVER, xData);
    }
    
    pServerData->mUnlinkId = unlinkId;
    pServerData->mServerInfo = xData;
    
    FindModule<NFIMessageModule>()->CreateLinkToServer(NF_ST_ROUTE_SERVER, xData.bus_id(), pServerData->mUnlinkId);
    
    NFLogInfo(NF_LOG_DEFAULT, 0,
              "Route Agent Server:{}({}) Register Route Server:{}({}) Success",
              pServerData->mServerInfo.server_name(), pServerData->mServerInfo.server_id(),
              pConfig->ServerName, pConfig->ServerId);
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}



