// -------------------------------------------------------------------------
//    @FileName         :    NFGameServerModule.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFGameServerPlugin
//
// -------------------------------------------------------------------------

#include <NFServerComm/NFServerCommon/NFServerCommonDefine.h>
#include "NFRouteAgentServerModule.h"

#include "NFComm/NFPluginModule/NFIPluginManager.h"
#include "NFComm/NFPluginModule/NFIConfigModule.h"
#include "NFComm/NFPluginModule/NFIMessageModule.h"
#include "NFServerComm/NFServerCommon/NFIServerMessageModule.h"
#include "NFComm/NFPluginModule/NFIMonitorModule.h"
#include "NFComm/NFPluginModule/NFCheck.h"
#include "NFComm/NFCore/NFServerIDUtil.h"

#define ROUTEAGENT_SERVER_CONNECT_MASTER_SERVER "RouteAgentServer Connect MasterServer"
#define ROUTEAGENT_SERVER_CONNECT_ROUTE_SERVER "RouteAgentServer Connect RouteServer"

NFCRouteAgentServerModule::NFCRouteAgentServerModule(NFIPluginManager *p) : NFIRouteAgentServerModule(p)
{
}

NFCRouteAgentServerModule::~NFCRouteAgentServerModule()
{
}

bool NFCRouteAgentServerModule::Awake()
{
    //不需要固定帧，需要尽可能跑得快
    m_pObjPluginManager->SetFixedFrame(false);

    /////////////master msg/////////////////////////////
    FindModule<NFIMessageModule>()->AddMessageCallBack(NF_ST_ROUTE_AGENT_SERVER, NF_MODULE_FRAME, NFrame::NF_SERVER_TO_SERVER_REGISTER,
                                                       this, &NFCRouteAgentServerModule::OnServerRegisterProcess);
    FindModule<NFIMessageModule>()->AddMessageCallBack(NF_ST_ROUTE_AGENT_SERVER,
                                                       NF_MODULE_FRAME, NFrame::NF_MASTER_SERVER_SEND_OTHERS_TO_SERVER, this,
                                                       &NFCRouteAgentServerModule::OnHandleServerReport);

    //注册要完成的服务器启动任务
    RegisterAppTask(NF_ST_ROUTE_AGENT_SERVER, APP_INIT_CONNECT_MASTER, ROUTEAGENT_SERVER_CONNECT_MASTER_SERVER, APP_INIT_TASK_GROUP_SERVER_CONNECT);
    RegisterAppTask(NF_ST_ROUTE_AGENT_SERVER, APP_INIT_CONNECT_ROUTE_SERVER, ROUTEAGENT_SERVER_CONNECT_ROUTE_SERVER, APP_INIT_TASK_GROUP_SERVER_CONNECT);

    NFServerConfig *pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_ROUTE_AGENT_SERVER);
    if (pConfig)
    {
        m_pObjPluginManager->SetIdleSleepUs(pConfig->IdleSleepUS);
        uint64_t unlinkId = FindModule<NFIMessageModule>()->BindServer(NF_ST_ROUTE_AGENT_SERVER, pConfig->Url, pConfig->NetThreadNum, pConfig->MaxConnectNum, PACKET_PARSE_TYPE_INTERNAL);
        if (unlinkId > 0)
        {
            /*
                注册客户端事件
            */
            uint64_t routeAgentServerLinkId = unlinkId;
            FindModule<NFIMessageModule>()->SetServerLinkId(NF_ST_ROUTE_AGENT_SERVER, routeAgentServerLinkId);
            FindModule<NFIMessageModule>()->AddEventCallBack(NF_ST_ROUTE_AGENT_SERVER, routeAgentServerLinkId, this,
                                                             &NFCRouteAgentServerModule::OnRouteAgentSocketEvent);
            FindModule<NFIMessageModule>()->AddOtherCallBack(NF_ST_ROUTE_AGENT_SERVER, routeAgentServerLinkId, this, &NFCRouteAgentServerModule::OnHandleOtherMessage);
            NFLogInfo(NF_LOG_DEFAULT, 0, "route agent server listen success, serverId:{}, ip:{}, port:{}", pConfig->ServerId, pConfig->ServerIp, pConfig->ServerPort);
        }
        else
        {
            NFLogInfo(NF_LOG_DEFAULT, 0, "route agent listen failed, serverId:{}, ip:{}, port:{}", pConfig->ServerId, pConfig->ServerIp, pConfig->ServerPort);
            return false;
        }
    }
    else
    {
        NFLogError(NF_LOG_DEFAULT, 0, "I Can't get the Game Server config!");
        return false;
    }

    SetTimer(1, 60000);
    Subscribe(NF_ST_ROUTE_AGENT_SERVER, NFrame::NF_EVENT_SERVER_DEAD_EVENT, NFrame::NF_EVENT_SERVER_TYPE, 0, __FUNCTION__);
    Subscribe(NF_ST_ROUTE_AGENT_SERVER, NFrame::NF_EVENT_SERVER_APP_FINISH_INITED, NFrame::NF_EVENT_SERVER_TYPE, 0, __FUNCTION__);
    return true;
}

int NFCRouteAgentServerModule::ConnectMasterServer(const NFrame::ServerInfoReport &xData)
{
    NFServerConfig *pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_ROUTE_AGENT_SERVER);
    if (pConfig)
    {
        auto pMasterServerData = FindModule<NFIMessageModule>()->GetMasterData(NF_ST_ROUTE_AGENT_SERVER);
        if (pMasterServerData->mUnlinkId <= 0)
        {
            pMasterServerData->mUnlinkId = FindModule<NFIMessageModule>()->ConnectServer(NF_ST_ROUTE_AGENT_SERVER, xData.url(), PACKET_PARSE_TYPE_INTERNAL);
            FindModule<NFIMessageModule>()->AddEventCallBack(NF_ST_ROUTE_AGENT_SERVER, pMasterServerData->mUnlinkId, this, &NFCRouteAgentServerModule::OnMasterSocketEvent);
            FindModule<NFIMessageModule>()->AddOtherCallBack(NF_ST_ROUTE_AGENT_SERVER, pMasterServerData->mUnlinkId, this, &NFCRouteAgentServerModule::OnHandleMasterOtherMessage);
        }

        pMasterServerData->mServerInfo = xData;
    }
    else
    {
        NFLogError(NF_LOG_DEFAULT, 0, "I Can't get the Route Agent Server config!");
        return -1;
    }

    return 0;
}

bool NFCRouteAgentServerModule::Init()
{
#if NF_PLATFORM == NF_PLATFORM_WIN
    NFrame::ServerInfoReport masterData = FindModule<NFIConfigModule>()->GetDefaultMasterInfo(NF_ST_ROUTE_AGENT_SERVER);
    int32_t ret = ConnectMasterServer(masterData);
    CHECK_EXPR(ret == 0, false, "ConnectMasterServer Failed, url:{}", masterData.DebugString());
#else
    NFServerConfig *pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_ROUTE_AGENT_SERVER);
    if (pConfig && pConfig->RouteConfig.NamingHost.empty())
    {
        NFrame::ServerInfoReport masterData = FindModule<NFIConfigModule>()->GetDefaultMasterInfo(NF_ST_ROUTE_AGENT_SERVER);
        int32_t ret = ConnectMasterServer(masterData);
        CHECK_EXPR(ret == 0, false, "ConnectMasterServer Failed, url:{}", masterData.DebugString());
    }
#endif

    return true;
}

bool NFCRouteAgentServerModule::Execute()
{
    ServerReport();
    return true;
}

bool NFCRouteAgentServerModule::OnDynamicPlugin()
{
    FindModule<NFIMessageModule>()->CloseAllLink(NF_ST_ROUTE_AGENT_SERVER);
    return true;
}

int NFCRouteAgentServerModule::OnTimer(uint32_t nTimerID)
{
    RegisterAllServerInfoToRouteSvr();

    if (nTimerID == 10000)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "kill the exe..................");
        NFSLEEP(1000);
        exit(0);
    }
    return 0;
}

int NFCRouteAgentServerModule::OnExecute(uint32_t serverType, uint32_t nEventID, uint32_t bySrcType, uint64_t nSrcID, const google::protobuf::Message *pMessage)
{
    CHECK_EXPR(serverType == NF_ST_ROUTE_AGENT_SERVER, -1, "");
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

int NFCRouteAgentServerModule::OnRouteAgentSocketEvent(eMsgType nEvent, uint64_t unLinkId)
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

int NFCRouteAgentServerModule::OnHandleOtherMessage(uint64_t unLinkId, NFDataPackage &packet)
{
    uint32_t fromBusId = GetBusIdFromUnlinkId(packet.nSrcId);
    uint32_t fromServerType = GetServerTypeFromUnlinkId(packet.nSrcId);

    uint32_t serverType = GetServerTypeFromUnlinkId(packet.nDstId);
    uint32_t destBusId = GetBusIdFromUnlinkId(packet.nDstId);

    auto pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_ROUTE_AGENT_SERVER);
    CHECK_EXPR(pConfig != NULL, -1, "pConfig == NULL");

    if (packet.nErrCode == NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST)
    {
        return 0;
    }

    if (destBusId <= LOCAL_AND_CROSS_MAX)
    {
        NFLogInfo(NF_LOG_DEFAULT, 0,
                  "--{}:{} trans msg from {}:{} to {}:{}, packet:{} --", pConfig->ServerName, pConfig->ServerId,
                  GetServerName((NF_SERVER_TYPE)fromServerType), NFServerIDUtil::GetBusNameFromBusID(fromBusId), GetServerName((NF_SERVER_TYPE)serverType), destBusId, packet.ToString());
    }
    else
    {
        NFLogInfo(NF_LOG_DEFAULT, 0,
                  "--{}:{} trans msg from {}:{} to {}:{}, packet:{} --", pConfig->ServerName, pConfig->ServerId,
                  GetServerName((NF_SERVER_TYPE)fromServerType), NFServerIDUtil::GetBusNameFromBusID(fromBusId), GetServerName((NF_SERVER_TYPE)serverType), NFServerIDUtil::GetBusNameFromBusID(destBusId), packet.ToString());
    }

    /**
    * @brief 当目标busId==LOCAL_ROUTE, 本服路由机制，除非明确表示要发往跨服服务器，否则就是本服路由(包过跨服服务器的本服路由) (需要保证本跨服服务器只能连接跨服route agent， 不跨服服务器只能连接不跨服的route agent.)
    * LOCAL_ROUTE 只发一个
    */
    if (destBusId == LOCAL_ROUTE)
    {
        /**
         * @brief 本服路由(包过跨服服务器的本服路由)
         */
        NF_SHARE_PTR<NFServerData> pServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_AGENT_SERVER, (NF_SERVER_TYPE) serverType, pConfig->IsCrossServer());
        if (pServerData)
        {
            packet.nSrcId = fromBusId;
            packet.nDstId = destBusId;
            FindModule<NFIMessageModule>()->TransPackage(pServerData->mUnlinkId, packet);
        }
        else
        {
            /**
             * @brief 如果自己是跨服routeagent，就发往跨服route server, 不是跨服，就发往非跨服route server
             */
            auto pRouteServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_AGENT_SERVER, NF_ST_ROUTE_SERVER, pConfig->IsCrossServer());
            if (pRouteServerData == NULL)
            {
                packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
                FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
            }
            CHECK_NULL(0, pRouteServerData);
            FindModule<NFIMessageModule>()->TransPackage(pRouteServerData->mUnlinkId, packet);
        }
    }
    /**
     * @brief 本服索引路由  LOCAL_ROUTE+index       本服路由机制，除非明确表示要发往跨服服务器，否则就是本服路由(包过跨服服务器的本服路由) (需要保证本跨服服务器只能连接跨服route agent， 不跨服服务器只能连接不跨服的route agent.)
     */
    else if (destBusId > LOCAL_ROUTE && destBusId < CROSS_ROUTE)
    {
        uint32_t index = destBusId - LOCAL_ROUTE;
        uint32_t realDestBusId = NFServerIDUtil::MakeProcID(pConfig->GetWorldId(), pConfig->GetZoneId(), serverType, index);
        NF_SHARE_PTR<NFServerData> pServerData = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_AGENT_SERVER, realDestBusId);
        if (pServerData)
        {
            packet.nSrcId = fromBusId;
            packet.nDstId = realDestBusId;
            FindModule<NFIMessageModule>()->TransPackage(pServerData->mUnlinkId, packet);
        }
        else
        {
            /**
             * @brief 如果自己是跨服routeagent，就发往跨服route server, 不是跨服，就发往非跨服route server
             */
            auto pRouteServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_AGENT_SERVER, NF_ST_ROUTE_SERVER, pConfig->IsCrossServer());
            if (pRouteServerData == NULL)
            {
                packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
                FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
            }
            CHECK_NULL(0, pRouteServerData);
            FindModule<NFIMessageModule>()->TransPackage(pRouteServerData->mUnlinkId, packet);
        }
    }
    /**
     * @brief 跨服路由(明确指定要找跨服服务器， 才走跨服路由)
     */
    else if (destBusId == CROSS_ROUTE)
    {
        NF_SHARE_PTR<NFServerData> pServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_AGENT_SERVER, (NF_SERVER_TYPE) serverType, true);
        if (pServerData)
        {
            packet.nSrcId = fromBusId;
            packet.nDstId = destBusId;
            FindModule<NFIMessageModule>()->TransPackage(pServerData->mUnlinkId, packet);
        }
        else
        {
            /**
             * @brief 发往跨服route server
             */
            auto pRouteServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_AGENT_SERVER, NF_ST_ROUTE_SERVER, true);
            if (pRouteServerData == NULL)
            {
                packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
                FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
            }
            CHECK_NULL(0, pRouteServerData);
            FindModule<NFIMessageModule>()->TransPackage(pRouteServerData->mUnlinkId, packet);
        }
    }
    //跨服索引路由 CROSS_ROUTE+index          明确指定要找跨服服务器， 才走跨服路由
    else if (destBusId > CROSS_ROUTE && destBusId < LOCAL_ROUTE_ZONE)
    {
        uint32_t index = destBusId - CROSS_ROUTE;
        if (pConfig->IsCrossServer())
        {
            uint32_t realDestBusId = NFServerIDUtil::MakeProcID(pConfig->GetWorldId(), pConfig->GetZoneId(), serverType, index);
            NF_SHARE_PTR<NFServerData> pServerData = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_AGENT_SERVER, realDestBusId);
            if (pServerData)
            {
                packet.nSrcId = fromBusId;
                packet.nDstId = realDestBusId;
                FindModule<NFIMessageModule>()->TransPackage(pServerData->mUnlinkId, packet);
            }
            else
            {
                auto pRouteServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_AGENT_SERVER, NF_ST_ROUTE_SERVER, true);
                if (pRouteServerData == NULL)
                {
                    packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
                    FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
                }
                CHECK_NULL(0, pRouteServerData);
                FindModule<NFIMessageModule>()->TransPackage(pRouteServerData->mUnlinkId, packet);
            }
        }
        else
        {
            auto pRouteServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_AGENT_SERVER, NF_ST_ROUTE_SERVER, true);
            if (pRouteServerData == NULL)
            {
                packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
                FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
            }
            CHECK_NULL(0, pRouteServerData);
            FindModule<NFIMessageModule>()->TransPackage(pRouteServerData->mUnlinkId, packet);
        }
    }
    /**
     * @brief 区服路由  区服的zid(1-4096) 只有跨服route server服务器，才有区服路由的能力
     */
    else if (destBusId > LOCAL_ROUTE_ZONE && destBusId < CROSS_ROUTE_ZONE)
    {
        uint32_t realDestBusId = destBusId - LOCAL_ROUTE_ZONE;
        if (pConfig->GetZoneId() == realDestBusId)
        {
            /**
             * @brief 本服路由(包过跨服服务器的本服路由)
             */
            NF_SHARE_PTR<NFServerData> pServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_AGENT_SERVER, (NF_SERVER_TYPE) serverType, pConfig->IsCrossServer());
            if (pServerData)
            {
                packet.nSrcId = fromBusId;
                packet.nDstId = realDestBusId;
                FindModule<NFIMessageModule>()->TransPackage(pServerData->mUnlinkId, packet);
            }
            else
            {
                /**
                 * @brief 如果自己是跨服routeagent，就发往跨服route server, 不是跨服，就发往非跨服route server
                 */
                auto pRouteServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_AGENT_SERVER, NF_ST_ROUTE_SERVER, pConfig->IsCrossServer());
                if (pRouteServerData == NULL)
                {
                    packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
                    FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
                }
                CHECK_NULL(0, pRouteServerData);
                FindModule<NFIMessageModule>()->TransPackage(pRouteServerData->mUnlinkId, packet);
            }
        }
        else
        {
            if (!pConfig->IsCrossServer())
            {
                NFLogError(NF_LOG_DEFAULT, 0, "destBusId:{} zid route error, the pConfig is not cross server", destBusId);
                packet.nErrCode = NFrame::ERR_CODE_ROUTER_NOT_SUPPORTTED;
                FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
                return 0;
            }

            auto pRouteServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_AGENT_SERVER, NF_ST_ROUTE_SERVER, true);
            if (pRouteServerData == NULL)
            {
                packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
                FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
            }
            CHECK_NULL(0, pRouteServerData);
            FindModule<NFIMessageModule>()->TransPackage(pRouteServerData->mUnlinkId, packet);
        }
    }
    /**
     * @brief 跨服路由同服务器类型群发路由 (最大分区4096， 所以10000+zoneid, 10001-14096之间)
     */
    else if (destBusId > CROSS_ROUTE_ZONE && destBusId < LOCAL_ALL_ROUTE)
    {
        uint32_t realDestBusId = destBusId - CROSS_ROUTE_ZONE;
        if (pConfig->GetZoneId() == realDestBusId)
        {
            auto pRouteServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_AGENT_SERVER, NF_ST_ROUTE_SERVER, pConfig->IsCrossServer());
            if (pRouteServerData == NULL)
            {
                packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
                FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
            }
            CHECK_NULL(0, pRouteServerData);
            FindModule<NFIMessageModule>()->TransPackage(pRouteServerData->mUnlinkId, packet);
        }
        else
        {
            if (!pConfig->IsCrossServer())
            {
                NFLogError(NF_LOG_DEFAULT, 0, "destBusId:{} zid route error, the pConfig is not cross server", destBusId);
                packet.nErrCode = NFrame::ERR_CODE_ROUTER_NOT_SUPPORTTED;
                FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
                return 0;
            }

            auto pRouteServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_AGENT_SERVER, NF_ST_ROUTE_SERVER, true);
            if (pRouteServerData == NULL)
            {
                packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
                FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
            }
            CHECK_NULL(0, pRouteServerData);
            FindModule<NFIMessageModule>()->TransPackage(pRouteServerData->mUnlinkId, packet);
        }
    }
    /**
    * @brief 当目标busId==LOCAL_ALL_ROUTE, 本服路由机制，除非明确表示要发往跨服服务器，否则就是本服路由(包过跨服服务器的本服路由) (需要保证本跨服服务器只能连接跨服route agent， 不跨服服务器只能连接不跨服的route agent.)
    * LOCAL_ALL_ROUTE 所有同类型的服务器都发一个
    */
    else if (destBusId == LOCAL_ALL_ROUTE)
    {
        /**
         * @brief 如果自己是跨服routeagent，就发往跨服route server, 不是跨服，就发往非跨服route server
         */
        auto pRouteServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_AGENT_SERVER, NF_ST_ROUTE_SERVER, pConfig->IsCrossServer());
        if (pRouteServerData == NULL)
        {
            packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
            FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
        }
        CHECK_NULL(0, pRouteServerData);
        FindModule<NFIMessageModule>()->TransPackage(pRouteServerData->mUnlinkId, packet);
    }
    /**
     * @brief 跨服路由(明确指定要找跨服服务器， 才走跨服路由)
     * CROSS_ALL_ROUTE 所有同类型的服务器都发一个
     */
    else if (destBusId == CROSS_ALL_ROUTE)
    {
        /**
         * @brief 发往跨服route server
         */
        auto pRouteServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_AGENT_SERVER, NF_ST_ROUTE_SERVER, true);
        if (pRouteServerData == NULL)
        {
            packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
            FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
        }
        CHECK_NULL(0, pRouteServerData);
        FindModule<NFIMessageModule>()->TransPackage(pRouteServerData->mUnlinkId, packet);
    }
    else if (destBusId == LOCAL_AND_CROSS_ALL_ROUTE)
    {
        if (!pConfig->IsCrossServer())
        {
            auto pRouteServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_AGENT_SERVER, NF_ST_ROUTE_SERVER, false);
            if (pRouteServerData == NULL)
            {
                packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
                FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
            }
            CHECK_NULL(0, pRouteServerData);
            FindModule<NFIMessageModule>()->TransPackage(pRouteServerData->mUnlinkId, packet);
        }

        /**
         * @brief 发往跨服route server
         */
        auto pRouteServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_AGENT_SERVER, NF_ST_ROUTE_SERVER, true);
        if (pRouteServerData == NULL)
        {
            packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
            FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
        }
        CHECK_NULL(0, pRouteServerData);
        FindModule<NFIMessageModule>()->TransPackage(pRouteServerData->mUnlinkId, packet);
    }
    else if (destBusId == ALL_LOCAL_AND_ALL_CROSS_ROUTE)
    {
        /**
         * @brief 发往跨服route server
         */
        auto pRouteServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_AGENT_SERVER, NF_ST_ROUTE_SERVER, true);
        if (pRouteServerData == NULL)
        {
            pRouteServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_AGENT_SERVER, NF_ST_ROUTE_SERVER, pConfig->IsCrossServer());
        }
        if (pRouteServerData == NULL)
        {
            packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
            FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
        }
        CHECK_NULL(0, pRouteServerData);
        FindModule<NFIMessageModule>()->TransPackage(pRouteServerData->mUnlinkId, packet);
    }
    else
    {
        NF_SHARE_PTR<NFServerData> pServerData = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_AGENT_SERVER, destBusId);
        if (pServerData)
        {
            packet.nSrcId = fromBusId;
            packet.nDstId = destBusId;
            FindModule<NFIMessageModule>()->TransPackage(pServerData->mUnlinkId, packet);
        }
        else
        {
            /**
             * @brief 如果自己是跨服routeagent，就发往跨服route server, 不是跨服，就发往非跨服route server
             */
            auto pRouteServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_AGENT_SERVER, NF_ST_ROUTE_SERVER, pConfig->IsCrossServer());
            if (pRouteServerData == NULL)
            {
                packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
                FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
            }
            CHECK_NULL(0, pRouteServerData);
            FindModule<NFIMessageModule>()->TransPackage(pRouteServerData->mUnlinkId, packet);
        }
    }

    return 0;
}

/*
	处理Master服务器链接事件
*/
int NFCRouteAgentServerModule::OnMasterSocketEvent(eMsgType nEvent, uint64_t unLinkId)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");

    if (nEvent == eMsgType_CONNECTED)
    {
        std::string ip = FindModule<NFIMessageModule>()->GetLinkIp(unLinkId);
        NFLogDebug(NF_LOG_DEFAULT, 0, "route agent server connect master success!");
        if (!m_pObjPluginManager->IsInited(NF_ST_ROUTE_AGENT_SERVER))
        {
            RegisterMasterServer(NFrame::EST_INIT);
        }
        else
        {
            RegisterMasterServer(NFrame::EST_NARMAL);
        }

        //完成服务器启动任务
        if (!m_pObjPluginManager->IsInited(NF_ST_ROUTE_AGENT_SERVER))
        {
            FinishAppTask(NF_ST_ROUTE_AGENT_SERVER, APP_INIT_CONNECT_MASTER, APP_INIT_TASK_GROUP_SERVER_CONNECT);
        }
    }
    else if (nEvent == eMsgType_DISCONNECTED)
    {
        std::string ip = FindModule<NFIMessageModule>()->GetLinkIp(unLinkId);
        NFLogError(NF_LOG_DEFAULT, 0, "route agent server disconnect master success");
    }
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

/*
	处理Master服务器未注册协议
*/
int NFCRouteAgentServerModule::OnHandleMasterOtherMessage(uint64_t unLinkId, NFDataPackage &packet)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::string ip = FindModule<NFIMessageModule>()->GetLinkIp(unLinkId);
    NFLogWarning(NF_LOG_DEFAULT, 0, "master server other message not handled:packet:{},ip:{}", packet.ToString(), ip);
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCRouteAgentServerModule::RegisterMasterServer(uint32_t serverState)
{
    NFServerConfig *pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_ROUTE_AGENT_SERVER);
    if (pConfig)
    {
        NFrame::ServerInfoReportList xMsg;
        NFrame::ServerInfoReport *pData = xMsg.add_server_list();
        NFServerCommon::WriteServerInfo(pData, pConfig);
        pData->set_server_state(serverState);

        FindModule<NFIServerMessageModule>()->SendMsgToMasterServer(NF_ST_ROUTE_AGENT_SERVER, NF_MODULE_FRAME, NFrame::NF_SERVER_TO_SERVER_REGISTER, xMsg);
    }
    return 0;
}

int NFCRouteAgentServerModule::ServerReport()
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

    NFServerConfig *pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_ROUTE_AGENT_SERVER);
    if (pConfig)
    {
        NFrame::ServerInfoReportList xMsg;
        NFrame::ServerInfoReport *pData = xMsg.add_server_list();
        NFServerCommon::WriteServerInfo(pData, pConfig);
        pData->set_server_state(NFrame::EST_NARMAL);

        NFIMonitorModule *pMonitorModule = m_pObjPluginManager->FindModule<NFIMonitorModule>();
        if (pMonitorModule)
        {
            const NFSystemInfo &systemInfo = pMonitorModule->GetSystemInfo();
            NFServerCommon::WriteServerInfo(pData, systemInfo);
        }

        if (pData->proc_cpu() > 0 && pData->proc_mem() > 0)
        {
            FindModule<NFIServerMessageModule>()->SendMsgToMasterServer(NF_ST_ROUTE_AGENT_SERVER, NF_MODULE_FRAME, NFrame::NF_SERVER_TO_MASTER_SERVER_REPORT, xMsg);
        }
    }
    return 0;
}

int NFCRouteAgentServerModule::OnServerRegisterProcess(uint64_t unLinkId, NFDataPackage &packet)
{
    NFrame::ServerInfoReportList xMsg;
    CLIENT_MSG_PROCESS_WITH_PRINTF(packet, xMsg);

    auto pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_ROUTE_AGENT_SERVER);
    CHECK_EXPR(pConfig != NULL, -1, "pConfig == NULL");

    for (int i = 0; i < xMsg.server_list_size(); ++i)
    {
        const NFrame::ServerInfoReport &xData = xMsg.server_list(i);
        if (xData.is_cross_server() != pConfig->IsCrossServer())
        {
            NFLogError(NF_LOG_DEFAULT, 0, "Server:{} Register Route Agent Server:{}() Failed, cross not match", xData.server_name(), pConfig->ServerName, pConfig->ServerId);
            continue;
        }

        NF_SHARE_PTR<NFServerData> pServerData = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_AGENT_SERVER, xData.bus_id());
        if (pServerData == nullptr)
        {
            pServerData = FindModule<NFIMessageModule>()->CreateServerByServerId(NF_ST_ROUTE_AGENT_SERVER,
                                                                                 xData.bus_id(), (NF_SERVER_TYPE) xData.server_type(), xData);
        }

        pServerData->mUnlinkId = unLinkId;
        pServerData->mServerInfo = xData;
        FindModule<NFIMessageModule>()->CreateLinkToServer(NF_ST_ROUTE_AGENT_SERVER, xData.bus_id(),
                                                           pServerData->mUnlinkId);
        NFLogInfo(NF_LOG_DEFAULT, 0, "Server:{} Register Route Agent Server:{}({}) Success", xData.server_name(), pConfig->ServerName, pConfig->ServerId);

        NFrame::ServerInfoReportList rsp;
        FindModule<NFIMessageModule>()->Send(pServerData->mUnlinkId, NF_MODULE_FRAME, NFrame::NF_SERVER_TO_SERVER_REGISTER_RSP, rsp,
                                             0);

        ::NFrame::ServerInfoReport *pReport = rsp.add_server_list();
        *pReport = xData;
        RegisterServerInfoToRouteSvr(rsp);
    }
    return 0;
}

int NFCRouteAgentServerModule::OnHandleServerReport(uint64_t unLinkId, NFDataPackage &packet)
{
    NFrame::ServerInfoReportList xMsg;
    CLIENT_MSG_PROCESS_NO_PRINTF(packet, xMsg);

    for (int i = 0; i < xMsg.server_list_size(); ++i)
    {
        const NFrame::ServerInfoReport &xData = xMsg.server_list(i);
        switch (xData.server_type())
        {
            case NF_SERVER_TYPE::NF_ST_ROUTE_SERVER:
            {
                OnHandleRouteServerReport(xData);
            }
            break;
            default: break;
        }
    }
    return 0;
}

int NFCRouteAgentServerModule::OnHandleRouteServerReport(const NFrame::ServerInfoReport &xData)
{
    CHECK_EXPR(xData.server_type() == NF_ST_ROUTE_SERVER, -1, "xData.server_type() == NF_ST_ROUTE_SERVER");

    auto pRouteServerData = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_AGENT_SERVER, xData.bus_id());

    if (pRouteServerData == NULL)
    {
        pRouteServerData = FindModule<NFIMessageModule>()->CreateServerByServerId(NF_ST_ROUTE_AGENT_SERVER, xData.bus_id(), NF_ST_ROUTE_SERVER, xData);

        pRouteServerData->mUnlinkId = FindModule<NFIMessageModule>()->ConnectServer(NF_ST_ROUTE_AGENT_SERVER, xData.url(), PACKET_PARSE_TYPE_INTERNAL);
        FindModule<NFIMessageModule>()->CreateLinkToServer(NF_ST_ROUTE_AGENT_SERVER, xData.bus_id(), pRouteServerData->mUnlinkId);

        FindModule<NFIMessageModule>()->AddEventCallBack(NF_ST_ROUTE_AGENT_SERVER, pRouteServerData->mUnlinkId, this, &NFCRouteAgentServerModule::OnRouteServerSocketEvent);
        FindModule<NFIMessageModule>()->AddOtherCallBack(NF_ST_ROUTE_AGENT_SERVER, pRouteServerData->mUnlinkId, this, &NFCRouteAgentServerModule::OnHandleRouteOtherMessage);
    }
    else
    {
        if (pRouteServerData->mUnlinkId > 0 && pRouteServerData->mServerInfo.url() != xData.url())
        {
            NFLogWarning(NF_LOG_DEFAULT, 0, "the server:{} old url:{} changed, new url:{}", pRouteServerData->mServerInfo.server_name(), pRouteServerData->mServerInfo.url(), xData.url());
            FindModule<NFIMessageModule>()->CloseLinkId(pRouteServerData->mUnlinkId);

            pRouteServerData->mUnlinkId = FindModule<NFIMessageModule>()->ConnectServer(NF_ST_ROUTE_AGENT_SERVER, xData.url(), PACKET_PARSE_TYPE_INTERNAL);
            FindModule<NFIMessageModule>()->CreateLinkToServer(NF_ST_ROUTE_AGENT_SERVER, xData.bus_id(), pRouteServerData->mUnlinkId);

            FindModule<NFIMessageModule>()->AddEventCallBack(NF_ST_ROUTE_AGENT_SERVER, pRouteServerData->mUnlinkId, this, &NFCRouteAgentServerModule::OnRouteServerSocketEvent);
            FindModule<NFIMessageModule>()->AddOtherCallBack(NF_ST_ROUTE_AGENT_SERVER, pRouteServerData->mUnlinkId, this, &NFCRouteAgentServerModule::OnHandleRouteOtherMessage);
        }
    }

    pRouteServerData->mServerInfo = xData;
    return 0;
}

int NFCRouteAgentServerModule::OnRouteServerSocketEvent(eMsgType nEvent, uint64_t unLinkId)
{
    if (nEvent == eMsgType_CONNECTED)
    {
        NFLogDebug(NF_LOG_DEFAULT, 0, "route agent server connect route server success!");

        RegisterRouteServer(unLinkId);

        //完成服务器启动任务
        if (!m_pObjPluginManager->IsInited(NF_ST_ROUTE_AGENT_SERVER))
        {
            FinishAppTask(NF_ST_ROUTE_AGENT_SERVER, APP_INIT_CONNECT_ROUTE_SERVER, APP_INIT_TASK_GROUP_SERVER_CONNECT);
        }
    }
    else if (nEvent == eMsgType_DISCONNECTED)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "route agent server disconnect route server success");
    }
    return 0;
}

int NFCRouteAgentServerModule::OnHandleRouteOtherMessage(uint64_t unLinkId, NFDataPackage &packet)
{
    uint32_t fromBusId = GetBusIdFromUnlinkId(packet.nSrcId);
    uint32_t fromServerType = GetServerTypeFromUnlinkId(packet.nSrcId);

    uint32_t serverType = GetServerTypeFromUnlinkId(packet.nDstId);
    uint32_t destBusId = GetBusIdFromUnlinkId(packet.nDstId);

    auto pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_ROUTE_AGENT_SERVER);
    CHECK_EXPR(pConfig != NULL, -1, "pConfig == NULL");

    auto pRouteSvrServerData = FindModule<NFIMessageModule>()->GetServerByUnlinkId(NF_ST_ROUTE_AGENT_SERVER, unLinkId);
    CHECK_EXPR(pRouteSvrServerData != NULL, -1, "pRouteSvrServerData == NULL");

    if (packet.nErrCode == NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST)
    {
        NFLogInfo(NF_LOG_DEFAULT, 0,
                  "the trans msg failed, can't find dest server, --{}:{} trans routesvr({}:{}) msg from {}:{} to {}:{}, packet:{} --", pConfig->ServerName, pConfig->ServerId, pRouteSvrServerData->mServerInfo.server_name(), pRouteSvrServerData->mServerInfo.server_id(),
                  GetServerName((NF_SERVER_TYPE)fromServerType), NFServerIDUtil::GetBusNameFromBusID(fromBusId), GetServerName((NF_SERVER_TYPE)serverType), NFServerIDUtil::GetBusNameFromBusID(destBusId), packet.ToString());

        NF_SHARE_PTR<NFServerData> pServerData = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_AGENT_SERVER, fromBusId);
        if (pServerData)
        {
            packet.nSrcId = fromBusId;
            packet.nDstId = destBusId;
            FindModule<NFIMessageModule>()->TransPackage(pServerData->mUnlinkId, packet);
        }
        else
        {
            NFLogError(NF_LOG_DEFAULT, 0,
                       "the trans msg failed, can't find dest server, the route agent can't find the server, busid:{}, server:{} trans packet:{} failed", destBusId,
                       GetServerName((NF_SERVER_TYPE)serverType), packet.ToString());
        }
        return 0;
    }


    if (destBusId <= LOCAL_AND_CROSS_MAX)
    {
        NFLogInfo(NF_LOG_DEFAULT, 0,
                  "--{}:{} trans routesvr({}:{}) msg from {}:{} to {}:{}, packet:{} --", pConfig->ServerName, pConfig->ServerId, pRouteSvrServerData->mServerInfo.server_name(), pRouteSvrServerData->mServerInfo.server_id(),
                  GetServerName((NF_SERVER_TYPE)fromServerType), NFServerIDUtil::GetBusNameFromBusID(fromBusId), GetServerName((NF_SERVER_TYPE)serverType), destBusId, packet.ToString());
    }
    else
    {
        NFLogInfo(NF_LOG_DEFAULT, 0,
                  "--{}:{} trans routesvr({}:{}) msg from {}:{} to {}:{}, packet:{} --", pConfig->ServerName, pConfig->ServerId, pRouteSvrServerData->mServerInfo.server_name(), pRouteSvrServerData->mServerInfo.server_id(),
                  GetServerName((NF_SERVER_TYPE)fromServerType), NFServerIDUtil::GetBusNameFromBusID(fromBusId), GetServerName((NF_SERVER_TYPE)serverType), NFServerIDUtil::GetBusNameFromBusID(destBusId), packet.ToString());
    }

    /**
    * @brief 当目标busId==LOCAL_ROUTE, 本服路由机制，除非明确表示要发往跨服服务器，否则就是本服路由(包过跨服服务器的本服路由) (需要保证本跨服服务器只能连接跨服route agent， 不跨服服务器只能连接不跨服的route agent.)
    */
    if (destBusId == LOCAL_ROUTE)
    {
        NF_SHARE_PTR<NFServerData> pServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_AGENT_SERVER, (NF_SERVER_TYPE) serverType, pConfig->IsCrossServer());
        if (pServerData)
        {
            packet.nSrcId = fromBusId;
            packet.nDstId = destBusId;
            FindModule<NFIMessageModule>()->TransPackage(pServerData->mUnlinkId, packet);
        }
        else
        {
            packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
            FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
            NFLogError(NF_LOG_DEFAULT, 0,
                       "the route agent can't find the server, busid:{}, server:{} trans packet:{} failed", destBusId,
                       GetServerName((NF_SERVER_TYPE)serverType), packet.ToString());
        }
    }
    /**
     * @brief 本服索引路由  LOCAL_ROUTE+index       本服路由机制，除非明确表示要发往跨服服务器，否则就是本服路由(包过跨服服务器的本服路由) (需要保证本跨服服务器只能连接跨服route agent， 不跨服服务器只能连接不跨服的route agent.)
     */
    else if (destBusId > LOCAL_ROUTE && destBusId < CROSS_ROUTE)
    {
        uint32_t index = destBusId - LOCAL_ROUTE;
        uint32_t realDestBusId = NFServerIDUtil::MakeProcID(pConfig->GetWorldId(), pConfig->GetZoneId(), serverType, index);
        NF_SHARE_PTR<NFServerData> pServerData = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_AGENT_SERVER, realDestBusId);
        if (pServerData)
        {
            packet.nSrcId = fromBusId;
            packet.nDstId = realDestBusId;
            FindModule<NFIMessageModule>()->TransPackage(pServerData->mUnlinkId, packet);
        }
        else
        {
            packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
            FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
            NFLogError(NF_LOG_DEFAULT, 0,
                       "the route agent can't find the server, busid:{}, server:{} trans packet:{} failed", realDestBusId,
                       GetServerName((NF_SERVER_TYPE)serverType), packet.ToString());
        }
    }
    /**
     * @brief 跨服路由(明确指定要找跨服服务器， 才走跨服路由)
     */
    else if (destBusId == CROSS_ROUTE)
    {
        NF_SHARE_PTR<NFServerData> pServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_AGENT_SERVER, (NF_SERVER_TYPE) serverType, true);
        if (pServerData)
        {
            packet.nSrcId = fromBusId;
            packet.nDstId = destBusId;
            FindModule<NFIMessageModule>()->TransPackage(pServerData->mUnlinkId, packet);
        }
        else
        {
            packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
            FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
            NFLogError(NF_LOG_DEFAULT, 0,
                       "the route agent can't find the server, busid:{}, server:{} trans packet:{} failed", destBusId,
                       GetServerName((NF_SERVER_TYPE)serverType), packet.ToString());
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
        NF_SHARE_PTR<NFServerData> pServerData = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_AGENT_SERVER, realDestBusId);
        if (pServerData)
        {
            packet.nSrcId = fromBusId;
            packet.nDstId = realDestBusId;
            FindModule<NFIMessageModule>()->TransPackage(pServerData->mUnlinkId, packet);
        }
        else
        {
            packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
            FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
            NFLogError(NF_LOG_DEFAULT, 0,
                       "the route agent can't find the server, busid:{}, server:{} trans packet:{} failed", realDestBusId,
                       GetServerName((NF_SERVER_TYPE)serverType), packet.ToString());
        }
    }
    /**
     * @brief 区服路由  区服的zid(1-4096) 只有跨服route server服务器，才有区服路由的能力
     */
    else if (destBusId > LOCAL_ROUTE_ZONE && destBusId < CROSS_ROUTE_ZONE)
    {
        uint32_t realDestBusId = destBusId - CROSS_ROUTE;
        CHECK_EXPR(pConfig->GetZoneId() == realDestBusId, -1, "the config zoneId:{} != destBusId:{}", pConfig->GetZoneId(), realDestBusId);
        NF_SHARE_PTR<NFServerData> pServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(NF_ST_ROUTE_AGENT_SERVER, (NF_SERVER_TYPE) serverType, pConfig->IsCrossServer());
        if (pServerData)
        {
            packet.nSrcId = fromBusId;
            packet.nDstId = realDestBusId;
            FindModule<NFIMessageModule>()->TransPackage(pServerData->mUnlinkId, packet);
        }
        else
        {
            packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
            FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
            NFLogError(NF_LOG_DEFAULT, 0,
                       "the route agent can't find the server, busid:{}, server:{} trans packet:{} failed", realDestBusId,
                       GetServerName((NF_SERVER_TYPE)serverType), packet.ToString());
        }
    }
    /**
     * @brief 区服路由 跨服路由同服务器类型群发路由 (最大分区4096， 所以10000+zoneid, 10001-14096之间)
     */
    else if (destBusId > CROSS_ROUTE_ZONE && destBusId < LOCAL_ALL_ROUTE)
    {
        uint32_t realDestBusId = destBusId - CROSS_ROUTE_ZONE;
        CHECK_EXPR(pConfig->GetZoneId() == realDestBusId, -1, "the config zoneId:{} != destBusId:{}", pConfig->GetZoneId(), destBusId);
        auto vecServerData = FindModule<NFIMessageModule>()->GetAllServer(NF_ST_ROUTE_AGENT_SERVER, (NF_SERVER_TYPE) serverType, pConfig->IsCrossServer());
        if (vecServerData.size() > 0)
        {
            for(int i = 0; i < (int)vecServerData.size(); i++)
            {
                auto pServerData = vecServerData[i];
                if (pServerData)
                {
                    packet.nSrcId = fromBusId;
                    packet.nDstId = realDestBusId;
                    FindModule<NFIMessageModule>()->TransPackage(pServerData->mUnlinkId, packet);
                }
            }
        }
    }
    else if (destBusId == LOCAL_ALL_ROUTE)
    {
        std::vector<NF_SHARE_PTR<NFServerData>> vecServerData = FindModule<NFIMessageModule>()->GetAllServer(NF_ST_ROUTE_AGENT_SERVER, (NF_SERVER_TYPE)serverType, pConfig->IsCrossServer());
        if (vecServerData.size() > 0)
        {
            for (int i = 0; i < (int)vecServerData.size(); i++)
            {
                auto pServerData = vecServerData[i];
                packet.nSrcId = fromBusId;
                packet.nDstId = destBusId;
                FindModule<NFIMessageModule>()->TransPackage(pServerData->mUnlinkId, packet);
            }
        }
    }
    else if (destBusId == CROSS_ALL_ROUTE)
    {
        std::vector<NF_SHARE_PTR<NFServerData>> vecServerData = FindModule<NFIMessageModule>()->GetAllServer(NF_ST_ROUTE_AGENT_SERVER, (NF_SERVER_TYPE)serverType, true);
        if (vecServerData.size() > 0)
        {
            for (int i = 0; i < (int)vecServerData.size(); i++)
            {
                auto pServerData = vecServerData[i];
                packet.nSrcId = fromBusId;
                packet.nDstId = destBusId;
                FindModule<NFIMessageModule>()->TransPackage(pServerData->mUnlinkId, packet);
            }
        }
    }
    else if (destBusId == LOCAL_AND_CROSS_ALL_ROUTE)
    {
        std::vector<NF_SHARE_PTR<NFServerData>> vecServerData = FindModule<NFIMessageModule>()->GetAllServer(NF_ST_ROUTE_AGENT_SERVER, (NF_SERVER_TYPE)serverType, pConfig->IsCrossServer());
        if (vecServerData.size() > 0)
        {
            for (int i = 0; i < (int)vecServerData.size(); i++)
            {
                auto pServerData = vecServerData[i];
                packet.nSrcId = fromBusId;
                packet.nDstId = destBusId;
                FindModule<NFIMessageModule>()->TransPackage(pServerData->mUnlinkId, packet);
            }
        }
    }
    else if (destBusId == ALL_LOCAL_AND_ALL_CROSS_ROUTE)
    {
        std::vector<NF_SHARE_PTR<NFServerData>> vecServerData = FindModule<NFIMessageModule>()->GetAllServer(NF_ST_ROUTE_AGENT_SERVER, (NF_SERVER_TYPE)serverType);
        if (vecServerData.size() > 0)
        {
            for (int i = 0; i < (int)vecServerData.size(); i++)
            {
                auto pServerData = vecServerData[i];
                packet.nSrcId = fromBusId;
                packet.nDstId = destBusId;
                FindModule<NFIMessageModule>()->TransPackage(pServerData->mUnlinkId, packet);
            }
        }
    }
    else
    {
        NF_SHARE_PTR<NFServerData> pServerData = FindModule<NFIMessageModule>()->GetServerByServerId(NF_ST_ROUTE_AGENT_SERVER, destBusId);
        if (pServerData)
        {
            packet.nSrcId = fromBusId;
            packet.nDstId = destBusId;
            FindModule<NFIMessageModule>()->TransPackage(pServerData->mUnlinkId, packet);
        }
        else
        {
            packet.nErrCode = NFrame::ERR_CODE_ROUTER_DISPATCHFAILD_DESTSVR_NOTEXIST;
            FindModule<NFIMessageModule>()->TransPackage(unLinkId, packet);
            NFLogError(NF_LOG_DEFAULT, 0,
                       "the route agent can't find the server, busid:{}, server:{} trans packet:{} failed", destBusId,
                       GetServerName((NF_SERVER_TYPE)serverType), packet.ToString());
        }
    }
    return 0;
}

int NFCRouteAgentServerModule::RegisterRouteServer(uint64_t unLinkId)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    NFServerConfig *pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_ROUTE_AGENT_SERVER);
    if (pConfig)
    {
        NFrame::ServerInfoReportList xMsg;
        NFrame::ServerInfoReport *pData = xMsg.add_server_list();
        NFServerCommon::WriteServerInfo(pData, pConfig);
        pData->set_server_state(NFrame::EST_NARMAL);

        FindModule<NFIMessageModule>()->Send(unLinkId, NF_MODULE_FRAME, NFrame::NF_SERVER_TO_SERVER_REGISTER, xMsg, 0);

        RegisterAllServerInfoToRouteSvr();
    }
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCRouteAgentServerModule::RegisterServerInfoToRouteSvr(const NFrame::ServerInfoReportList &xData)
{
    //NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    std::vector<NF_SHARE_PTR<NFServerData> > vec = FindModule<NFIMessageModule>()->GetAllServer(NF_ST_ROUTE_AGENT_SERVER, NF_ST_ROUTE_SERVER); //GetRouteData(NF_ST_ROUTE_AGENT_SERVER);
    for (int i = 0; i < (int) vec.size(); i++)
    {
        NF_SHARE_PTR<NFServerData> pRouteServerData = vec[i];
        if (pRouteServerData && pRouteServerData->mUnlinkId > 0)
        {
            FindModule<NFIMessageModule>()->Send(pRouteServerData->mUnlinkId, NF_MODULE_FRAME, NFrame::NF_ROUTER_CMD_INTERNAL_C2R_REG_RAASSOCAPPSVS, xData, 0);
        }
    }

    //NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCRouteAgentServerModule::RegisterAllServerInfoToRouteSvr()
{
    NFrame::ServerInfoReportList xData;
    std::vector<NF_SHARE_PTR<NFServerData> > vec = FindModule<NFIMessageModule>()->GetAllServer(NF_ST_ROUTE_AGENT_SERVER);
    for (size_t i = 0; i < vec.size(); i++)
    {
        NF_SHARE_PTR<NFServerData> pRouteServerData = vec[i];
        if (pRouteServerData)
        {
            if (pRouteServerData->mServerInfo.server_type() != NF_ST_MASTER_SERVER &&
                pRouteServerData->mServerInfo.server_type() != NF_ST_ROUTE_SERVER &&
                pRouteServerData->mServerInfo.server_type() != NF_ST_ROUTE_AGENT_SERVER)
            {
                auto pData = xData.add_server_list();
                *pData = pRouteServerData->mServerInfo;
            }
        }
    }
    RegisterServerInfoToRouteSvr(xData);
    return 0;
}

int NFCRouteAgentServerModule::OnHandleServerDisconnect(uint64_t unLinkId)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    NF_SHARE_PTR<NFServerData> pServerData = FindModule<NFIMessageModule>()->GetServerByUnlinkId(
            NF_ST_ROUTE_AGENT_SERVER, unLinkId);
    if (pServerData)
    {
        pServerData->mServerInfo.set_server_state(NFrame::EST_CRASH);
        pServerData->mUnlinkId = 0;

        NFLogError(NF_LOG_DEFAULT, 0,
                   "the {0} disconnect from route agent server, serverName:{0}, busid:{1}, serverIp:{2}, serverPort:{3}",
                   pServerData->mServerInfo.server_name(), pServerData->mServerInfo.bus_id(),
                   pServerData->mServerInfo.server_ip(), pServerData->mServerInfo.server_port());
    }

    FindModule<NFIMessageModule>()->DelServerLink(NF_ST_ROUTE_AGENT_SERVER, unLinkId);
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}
