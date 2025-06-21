// -------------------------------------------------------------------------
//    @FileName         :    NFMessageMgr.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFMessageMgr.cpp
//
// -------------------------------------------------------------------------

#include "NFServerMessageModule.h"
#include "NFComm/NFPluginModule/NFCheck.h"
#include "NFComm/NFPluginModule/NFIConfigModule.h"
#include "NFComm/NFPluginModule/NFICoroutineModule.h"
#include "NFComm/NFPluginModule/NFProtobufCommon.h"

NFServerMessageModule::NFServerMessageModule(NFIPluginManager* pPluginManager) : NFIServerMessageModule(pPluginManager)
{
}

NFServerMessageModule::~NFServerMessageModule()
{
}

int NFServerMessageModule::SendMsgToMasterServer(NF_SERVER_TYPE eSendType, uint32_t nMsgId, const google::protobuf::Message& xData, uint64_t nParam1,
                                                 uint64_t nParam2)
{
    auto pServerData = FindModule<NFIMessageModule>()->GetMasterData(eSendType);
    CHECK_EXPR(pServerData, -1, "pServerData == NULL, eType error:{}", (int) eSendType);

    FindModule<NFIMessageModule>()->Send(pServerData->mUnlinkId, nMsgId, xData, nParam1, nParam2);
    return 0;
}

int NFServerMessageModule::SendMsgToMasterServer(NF_SERVER_TYPE eSendType, uint32_t nModuleId, uint32_t nMsgId, const google::protobuf::Message& xData, uint64_t nParam1,
                                                 uint64_t nParam2)
{
    auto pServerData = FindModule<NFIMessageModule>()->GetMasterData(eSendType);
    CHECK_EXPR(pServerData, -1, "pServerData == NULL, eType error:{}", (int) eSendType);

    FindModule<NFIMessageModule>()->Send(pServerData->mUnlinkId, nModuleId, nMsgId, xData, nParam1, nParam2);
    return 0;
}

int NFServerMessageModule::SendProxyMsgByBusId(NF_SERVER_TYPE eType, uint32_t nDstId, uint32_t nMsgId, const google::protobuf::Message& xData,
                                               uint64_t nParam1, uint64_t nParam2)
{
    return SendProxyMsgByBusId(eType, nDstId, NF_MODULE_SERVER, nMsgId, xData, nParam1, nParam2);
}

int NFServerMessageModule::SendProxyMsgByBusId(NF_SERVER_TYPE eType, uint32_t nDstId, uint32_t nModuleId, uint32_t nMsgId,
                                               const google::protobuf::Message& xData, uint64_t nParam1, uint64_t nParam2)
{
    auto pConfig = FindModule<NFIConfigModule>()->GetAppConfig(eType);
    CHECK_EXPR(pConfig, -1, "pConfig == NULL");

    NF_SHARE_PTR<NFServerData> pServerData = FindModule<NFIMessageModule>()->GetServerByServerId(eType, nDstId);
    CHECK_EXPR(pServerData, -1, "pServerData == NULL, busId:{}", nDstId);

    FindModule<NFIMessageModule>()->Send(pServerData->mUnlinkId, nModuleId, nMsgId, xData, nParam1, nParam2, pConfig->BusId, nDstId);
    return 0;
}

int NFServerMessageModule::SendProxyMsgByBusId(NF_SERVER_TYPE eType, uint32_t nDstId, uint32_t nMsgId, const char* msg, uint32_t nLen, uint64_t nParam1,
                                               uint64_t nParam2)
{
    return SendProxyMsgByBusId(eType, nDstId, NF_MODULE_SERVER, nMsgId, msg, nLen, nParam1, nParam2);
}

int NFServerMessageModule::SendProxyMsgByBusId(NF_SERVER_TYPE eType, uint32_t nDstId, uint32_t nModuleId, uint32_t nMsgId, const char* msg,
                                               uint32_t nLen, uint64_t nParam1, uint64_t nParam2)
{
    auto pConfig = FindModule<NFIConfigModule>()->GetAppConfig(eType);
    CHECK_EXPR(pConfig, -1, "pConfig == NULL");

    NF_SHARE_PTR<NFServerData> pServerData = FindModule<NFIMessageModule>()->GetServerByServerId(eType, nDstId);
    CHECK_EXPR(pServerData, -1, "pServerData == NULL, busId:{}", nDstId);

    FindModule<NFIMessageModule>()->Send(pServerData->mUnlinkId, nModuleId, nMsgId, msg, nLen, nParam1, nParam2, pConfig->BusId, nDstId);
    return 0;
}

int NFServerMessageModule::SendRedirectMsgToProxyServer(NF_SERVER_TYPE eType, uint32_t nDstId, const std::unordered_set<uint64_t>& ids, uint32_t nMsgId,
                                                        const google::protobuf::Message& xData)
{
    NFrame::Proto_FramePkg svrPkg;
    svrPkg.set_msg_id(nMsgId);
    svrPkg.set_msg_data(xData.SerializePartialAsString());

    for (auto iter = ids.begin(); iter != ids.end(); iter++)
    {
        svrPkg.mutable_redirect_info()->add_id(*iter);
    }

    SendMsgToProxyServer(eType, nDstId, NF_MODULE_FRAME, NFrame::NF_SERVER_REDIRECT_MSG_TO_PROXY_SERVER_CMD, svrPkg);
    return 0;
}

int NFServerMessageModule::SendRedirectMsgToAllProxyServer(NF_SERVER_TYPE eType, const std::unordered_set<uint64_t>& ids, uint32_t nMsgId,
                                                           const google::protobuf::Message& xData)
{
    CHECK_EXPR(ids.size() > 0, 0, "ids empty");

    NFrame::Proto_FramePkg svrPkg;
    svrPkg.set_msg_id(nMsgId);
    svrPkg.set_msg_data(xData.SerializePartialAsString());

    for (auto iter = ids.begin(); iter != ids.end(); iter++)
    {
        svrPkg.mutable_redirect_info()->add_id(*iter);
    }

    std::vector<NF_SHARE_PTR<NFServerData>> vecServer = FindModule<NFIMessageModule>()->GetAllServer(eType, NF_ST_PROXY_SERVER);
    for (int i = 0; i < (int)vecServer.size(); i++)
    {
        auto pServerData = vecServer[i];
        if (pServerData)
        {
            SendMsgToProxyServer(eType, pServerData->mServerInfo.bus_id(), NF_MODULE_FRAME, NFrame::NF_SERVER_REDIRECT_MSG_TO_PROXY_SERVER_CMD, svrPkg);
        }
    }

    return 0;
}

int NFServerMessageModule::SendRedirectMsgToAllProxyServer(NF_SERVER_TYPE eType, uint32_t nMsgId,
                                                           const google::protobuf::Message& xData)
{
    NFrame::Proto_FramePkg svrPkg;
    svrPkg.set_msg_id(nMsgId);
    svrPkg.set_msg_data(xData.SerializePartialAsString());

    svrPkg.mutable_redirect_info()->set_all(true);

    std::vector<NF_SHARE_PTR<NFServerData>> vecServer = FindModule<NFIMessageModule>()->GetAllServer(eType, NF_ST_PROXY_SERVER);
    for (int i = 0; i < (int)vecServer.size(); i++)
    {
        auto pServerData = vecServer[i];
        if (pServerData)
        {
            SendMsgToProxyServer(eType, pServerData->mServerInfo.bus_id(), NF_MODULE_FRAME, NFrame::NF_SERVER_REDIRECT_MSG_TO_PROXY_SERVER_CMD, svrPkg);
        }
    }

    return 0;
}

int NFServerMessageModule::SendMsgToProxyServer(NF_SERVER_TYPE eType, uint32_t nDstId, uint32_t nMsgId, const google::protobuf::Message& xData,
                                                uint64_t nParam1, uint64_t nParam2)
{
    return SendMsgToProxyServer(eType, nDstId, NF_MODULE_SERVER, nMsgId, xData, nParam1, nParam2);
}

int NFServerMessageModule::SendMsgToProxyServer(NF_SERVER_TYPE eType, uint32_t nDstId, uint32_t nModuleId, uint32_t nMsgId,
                                                const google::protobuf::Message& xData, uint64_t nParam1, uint64_t nParam2)
{
    auto pConfig = FindModule<NFIConfigModule>()->GetAppConfig(eType);
    CHECK_EXPR(pConfig, -1, "pConfig == NULL");

    auto pServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(eType, NF_ST_PROXY_AGENT_SERVER);
    if (pServerData)
    {
        FindModule<NFIMessageModule>()->Send(pServerData->mUnlinkId, nModuleId, nMsgId, xData, nParam1, nParam2, pConfig->BusId, nDstId);
        return 0;
    }

    pServerData = FindModule<NFIMessageModule>()->GetServerByServerId(eType, nDstId);
    CHECK_EXPR(pServerData, -1, "pServerData == NULL, busId:{}", nDstId);
    FindModule<NFIMessageModule>()->Send(pServerData->mUnlinkId, nModuleId, nMsgId, xData, nParam1, nParam2, pConfig->BusId, nDstId);
    return 0;
}

int NFServerMessageModule::SendMsgToProxyServer(NF_SERVER_TYPE eType, uint32_t nDstId, uint32_t nMsgId, const std::string& xData,
                                                uint64_t nParam1, uint64_t nParam2)
{
    return SendMsgToProxyServer(eType, nDstId, NF_MODULE_SERVER, nMsgId, xData, nParam1, nParam2);
}

int NFServerMessageModule::SendMsgToProxyServer(NF_SERVER_TYPE eType, uint32_t nDstId, uint32_t nModuleId, uint32_t nMsgId,
                                                const std::string& xData, uint64_t nParam1, uint64_t nParam2)
{
    auto pConfig = FindModule<NFIConfigModule>()->GetAppConfig(eType);
    CHECK_EXPR(pConfig, -1, "pConfig == NULL");

    auto pServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(eType, NF_ST_PROXY_AGENT_SERVER);
    if (pServerData)
    {
        FindModule<NFIMessageModule>()->Send(pServerData->mUnlinkId, nModuleId, nMsgId, xData, nParam1, nParam2, pConfig->BusId, nDstId);
        return 0;
    }

    pServerData = FindModule<NFIMessageModule>()->GetServerByServerId(eType, nDstId);
    CHECK_EXPR(pServerData, -1, "pServerData == NULL, busId:{}", nDstId);
    FindModule<NFIMessageModule>()->Send(pServerData->mUnlinkId, nModuleId, nMsgId, xData, nParam1, nParam2, pConfig->BusId, nDstId);
    return 0;
}

int NFServerMessageModule::SendMsgToProxyServer(NF_SERVER_TYPE eType, uint32_t nDstId, uint32_t nMsgId, const char* pData, int dataLen,
                                                uint64_t nParam1, uint64_t nParam2)
{
    return SendMsgToProxyServer(eType, nDstId, NF_MODULE_SERVER, nMsgId, pData, dataLen, nParam1, nParam2);
}

int NFServerMessageModule::SendMsgToProxyServer(NF_SERVER_TYPE eType, uint32_t nDstId, uint32_t nModuleId, uint32_t nMsgId,
                                                const char* pData, int dataLen, uint64_t nParam1, uint64_t nParam2)
{
    auto pConfig = FindModule<NFIConfigModule>()->GetAppConfig(eType);
    CHECK_EXPR(pConfig, -1, "pConfig == NULL");

    auto pServerData = FindModule<NFIMessageModule>()->GetRandomServerByServerType(eType, NF_ST_PROXY_AGENT_SERVER);
    if (pServerData)
    {
        FindModule<NFIMessageModule>()->Send(pServerData->mUnlinkId, nModuleId, nMsgId, pData, dataLen, nParam1, nParam2, pConfig->BusId, nDstId);
        return 0;
    }

    pServerData = FindModule<NFIMessageModule>()->GetServerByServerId(eType, nDstId);
    CHECK_EXPR(pServerData, -1, "pServerData == NULL, busId:{}", nDstId);
    FindModule<NFIMessageModule>()->Send(pServerData->mUnlinkId, nModuleId, nMsgId, pData, dataLen, nParam1, nParam2, pConfig->BusId, nDstId);
    return 0;
}

int NFServerMessageModule::SendMsgToWorldServer(NF_SERVER_TYPE eType, uint32_t nMsgId, const google::protobuf::Message& xData, uint64_t nParam1,
                                                uint64_t nParam2)
{
    return SendMsgToWorldServer(eType, NF_MODULE_SERVER, nMsgId, xData, nParam1, nParam2);
}

int NFServerMessageModule::SendMsgToWorldServer(NF_SERVER_TYPE eType, uint32_t nModuleId, uint32_t nMsgId, const google::protobuf::Message& xData,
                                                uint64_t nParam1, uint64_t nParam2)
{
    return FindModule<NFIMessageModule>()->SendMsgToServer(eType, NF_ST_WORLD_SERVER, 0, 0, nModuleId, nMsgId, xData, nParam1, nParam2);
}

int NFServerMessageModule::SendTransToWorldServer(NF_SERVER_TYPE eType, uint32_t nMsgId, const google::protobuf::Message& xData, uint32_t req_trans_id,
                                                  uint32_t rsp_trans_id)
{
    return FindModule<NFIMessageModule>()->SendTrans(eType, NF_ST_WORLD_SERVER, 0, 0, nMsgId, xData, req_trans_id, rsp_trans_id);
}

int NFServerMessageModule::SendMsgToCenterServer(NF_SERVER_TYPE eType, uint32_t nMsgId, const google::protobuf::Message& xData, uint64_t nParam1,
                                                 uint64_t nParam2)
{
    return SendMsgToCenterServer(eType, NF_MODULE_SERVER, nMsgId, xData, nParam1, nParam2);
}

int NFServerMessageModule::SendMsgToCenterServer(NF_SERVER_TYPE eType, uint32_t nModuleId, uint32_t nMsgId, const google::protobuf::Message& xData,
                                                 uint64_t nParam1, uint64_t nParam2)
{
    auto pServerData = FindModule<NFIMessageModule>()->GetFirstServerByServerType(eType, NF_ST_CENTER_SERVER, false);
    CHECK_NULL(0, pServerData);
    return FindModule<NFIMessageModule>()->SendMsgToServer(eType, NF_ST_CENTER_SERVER, 0, pServerData->mServerInfo.bus_id(), nModuleId, nMsgId, xData, nParam1, nParam2);
}

int NFServerMessageModule::SendTransToCenterServer(NF_SERVER_TYPE eType, uint32_t nMsgId, const google::protobuf::Message& xData, uint32_t req_trans_id,
                                                   uint32_t rsp_trans_id)
{
    auto pServerData = FindModule<NFIMessageModule>()->GetFirstServerByServerType(eType, NF_ST_CENTER_SERVER, false);
    CHECK_NULL(0, pServerData);
    return FindModule<NFIMessageModule>()->SendTrans(eType, NF_ST_CENTER_SERVER, 0, pServerData->mServerInfo.bus_id(), nMsgId, xData, req_trans_id, rsp_trans_id);
}

int NFServerMessageModule::SendMsgToCrossCenterServer(NF_SERVER_TYPE eType, uint32_t nMsgId, const google::protobuf::Message& xData, uint64_t nParam1,
                                                      uint64_t nParam2)
{
    return SendMsgToCrossCenterServer(eType, NF_MODULE_SERVER, nMsgId, xData, nParam1, nParam2);
}

int NFServerMessageModule::SendMsgToCrossCenterServer(NF_SERVER_TYPE eType, uint32_t nModuleId, uint32_t nMsgId, const google::protobuf::Message& xData,
                                                      uint64_t nParam1, uint64_t nParam2)
{
    auto pServerData = FindModule<NFIMessageModule>()->GetFirstServerByServerType(eType, NF_ST_CENTER_SERVER, true);
    CHECK_NULL(0, pServerData);
    return FindModule<NFIMessageModule>()->SendMsgToServer(eType, NF_ST_CENTER_SERVER, 0, pServerData->mServerInfo.bus_id(), nModuleId, nMsgId, xData, nParam1, nParam2);
}

int NFServerMessageModule::SendTransToCrossCenterServer(NF_SERVER_TYPE eType, uint32_t nMsgId, const google::protobuf::Message& xData, uint32_t req_trans_id,
                                                        uint32_t rsp_trans_id)
{
    auto pServerData = FindModule<NFIMessageModule>()->GetFirstServerByServerType(eType, NF_ST_CENTER_SERVER, true);
    CHECK_NULL(0, pServerData);
    return FindModule<NFIMessageModule>()->SendTrans(eType, NF_ST_CENTER_SERVER, 0, pServerData->mServerInfo.bus_id(), nMsgId, xData, req_trans_id, rsp_trans_id);
}

int NFServerMessageModule::SendMsgToSnsServerBySnsType(NF_SERVER_TYPE eType, NF_SNS_SERVER_TYPE snsType, uint32_t nMsgId, const google::protobuf::Message& xData, uint64_t nParam1, uint64_t nParam2)
{
    return FindModule<NFIMessageModule>()->SendMsgToServer(eType, NF_ST_SNS_SERVER, 0, snsType, NF_MODULE_SERVER, nMsgId, xData, nParam1, nParam2);
}

int NFServerMessageModule::SendMsgToSnsServer(NF_SERVER_TYPE eType, uint32_t nMsgId, const google::protobuf::Message& xData, uint64_t nParam1,
                                              uint64_t nParam2)
{
    return SendMsgToSnsServer(eType, NF_MODULE_SERVER, nMsgId, xData, nParam1, nParam2);
}

int NFServerMessageModule::SendMsgToSnsServer(NF_SERVER_TYPE eType, uint32_t nModuleId, uint32_t nMsgId, const google::protobuf::Message& xData,
                                              uint64_t nParam1, uint64_t nParam2)
{
    return FindModule<NFIMessageModule>()->SendMsgToServer(eType, NF_ST_SNS_SERVER, 0, 0, nModuleId, nMsgId, xData, nParam1, nParam2);
}

int NFServerMessageModule::SendTransToSnsServer(NF_SERVER_TYPE eType, uint32_t nMsgId, const google::protobuf::Message& xData, uint32_t req_trans_id,
                                                uint32_t rsp_trans_id)
{
    return FindModule<NFIMessageModule>()->SendTrans(eType, NF_ST_SNS_SERVER, 0, 0, nMsgId, xData, req_trans_id, rsp_trans_id);
}

int NFServerMessageModule::SendMsgToOnlineServer(NF_SERVER_TYPE eType, uint32_t nMsgId, const google::protobuf::Message& xData, uint64_t nParam1, uint64_t nParam2)
{
    return SendMsgToOnlineServer(eType, NF_MODULE_SERVER, nMsgId, xData, nParam1, nParam2);
}

int NFServerMessageModule::SendMsgToOnlineServer(NF_SERVER_TYPE eType, uint32_t nModuleId, uint32_t nMsgId, const google::protobuf::Message& xData, uint64_t nParam1, uint64_t nParam2)
{
    return FindModule<NFIMessageModule>()->SendMsgToServer(eType, NF_ST_ONLINE_SERVER, 0, 0, nModuleId, nMsgId, xData, nParam1, nParam2);
}

int NFServerMessageModule::SendTransToOnlineServer(NF_SERVER_TYPE eType, uint32_t nMsgId, const google::protobuf::Message& xData, uint32_t req_trans_id, uint32_t rsp_trans_id)
{
    return FindModule<NFIMessageModule>()->SendTrans(eType, NF_ST_ONLINE_SERVER, 0, 0, nMsgId, xData, req_trans_id, rsp_trans_id);
}

int NFServerMessageModule::SendMsgToGameServer(NF_SERVER_TYPE eType, uint32_t nDstId, uint32_t nMsgId, const google::protobuf::Message& xData,
                                               uint64_t nParam1, uint64_t nParam2)
{
    return SendMsgToGameServer(eType, nDstId, NF_MODULE_SERVER, nMsgId, xData, nParam1, nParam2);
}

int NFServerMessageModule::SendMsgToGameServer(NF_SERVER_TYPE eType, uint32_t nDstId, uint32_t nModuleId, uint32_t nMsgId,
                                               const google::protobuf::Message& xData, uint64_t nParam1, uint64_t nParam2)
{
    return FindModule<NFIMessageModule>()->SendMsgToServer(eType, NF_ST_GAME_SERVER, 0, nDstId, nModuleId, nMsgId, xData, nParam1, nParam2);
}

int NFServerMessageModule::SendTransToGameServer(NF_SERVER_TYPE eType, uint32_t nDstId, uint32_t nMsgId, const google::protobuf::Message& xData,
                                                 uint32_t req_trans_id, uint32_t rsp_trans_id)
{
    return FindModule<NFIMessageModule>()->SendTrans(eType, NF_ST_GAME_SERVER, 0, nDstId, nMsgId, xData, req_trans_id, rsp_trans_id);
}

int NFServerMessageModule::SendMsgToLogicServer(NF_SERVER_TYPE eType, uint32_t nDstId, uint32_t nMsgId, const google::protobuf::Message& xData,
                                                uint64_t nParam1, uint64_t nParam2)
{
    return SendMsgToLogicServer(eType, nDstId, NF_MODULE_SERVER, nMsgId, xData, nParam1, nParam2);
}

int NFServerMessageModule::SendMsgToLogicServer(NF_SERVER_TYPE eType, uint32_t nDstId, uint32_t nModuleId, uint32_t nMsgId,
                                                const google::protobuf::Message& xData, uint64_t nParam1, uint64_t nParam2)
{
    return FindModule<NFIMessageModule>()->SendMsgToServer(eType, NF_ST_LOGIC_SERVER, 0, nDstId, nModuleId, nMsgId, xData, nParam1, nParam2);
}

int NFServerMessageModule::SendTransToLogicServer(NF_SERVER_TYPE eType, uint32_t nDstId, uint32_t nMsgId, const google::protobuf::Message& xData,
                                                  uint32_t req_trans_id, uint32_t rsp_trans_id)
{
    return FindModule<NFIMessageModule>()->SendTrans(eType, NF_ST_LOGIC_SERVER, 0, nDstId, nMsgId, xData, req_trans_id, rsp_trans_id);
}

int NFServerMessageModule::SendTransToStoreServer(NF_SERVER_TYPE eType, uint32_t dstBusId, uint32_t cmd, uint32_t table_id, const std::string& dbname,
                                                  const std::string& table_name, const google::protobuf::Message& xData,
                                                  const std::vector<NFrame::storesvr_vk>& vk_list,
                                                  const std::string& where_addtional_conds, int trans_id, uint32_t seq,
                                                  uint64_t mod_key, const std::string& cls_name, uint8_t packet_type)
{
    CHECK_EXPR(cmd == NFrame::NF_STORESVR_C2S_MODIFY || cmd == NFrame::NF_STORESVR_C2S_UPDATE, -1, "error cmd:{}", cmd);
    CHECK_EXPR(NFrame::PacketDispType_IsValid(packet_type), -1, "error msg_type:{}", packet_type);


    NFrame::Proto_FramePkg svrPkg;
    svrPkg.set_msg_id(cmd);
    svrPkg.mutable_disp_info()->set_type((NFrame::PacketDispType)packet_type);
    svrPkg.mutable_disp_info()->set_seq(seq);
    svrPkg.mutable_store_info()->set_table_id(table_id);
    svrPkg.mutable_store_info()->set_id(trans_id);

    NFLogTrace(NF_LOG_DEFAULT, mod_key, "cmd:{} table_id:{} table_name:{} trans_id:{} seq:{} mod_key:{}",
               cmd, table_id, table_name, trans_id, seq, mod_key);

    switch (cmd)
    {
        case NFrame::NF_STORESVR_C2S_MODIFY:
        {
            svrPkg.set_msg_data(NFStoreProtoCommon::storesvr_modifybycond(dbname, table_name, mod_key, xData, vk_list, where_addtional_conds, cls_name));
            break;
        }
        case NFrame::NF_STORESVR_C2S_UPDATE:
        {
            svrPkg.set_msg_data(NFStoreProtoCommon::storesvr_updatebycond(dbname, table_name, mod_key, xData, vk_list, where_addtional_conds, cls_name));
            break;
        }
        default:
        {
            break;
        }
    }

    return FindModule<NFIMessageModule>()->SendMsgToServer(eType, NF_ST_STORE_SERVER, 0, dstBusId,
                                                           NF_MODULE_FRAME, NFrame::NF_SERVER_TO_STORE_SERVER_DB_CMD, svrPkg);
}

int NFServerMessageModule::SendTransToStoreServer(NF_SERVER_TYPE eType, uint32_t dstBusId, uint32_t cmd, uint32_t table_id, const std::string& dbname,
                                                  const std::string& table_name, const std::vector<std::string>& vecFields,
                                                  const std::vector<NFrame::storesvr_vk>& vk_list,
                                                  const std::string& where_addtional_conds, int max_records, int trans_id, uint32_t seq,
                                                  uint64_t mod_key, const std::string& cls_name, uint8_t packet_type)
{
    CHECK_EXPR(cmd == NFrame::NF_STORESVR_C2S_SELECT || cmd == NFrame::NF_STORESVR_C2S_DELETE, -1, "error cmd:{}", cmd);
    CHECK_EXPR(NFrame::PacketDispType_IsValid(packet_type), -1, "error msg_type:{}", packet_type);

    NFrame::Proto_FramePkg svrPkg;
    svrPkg.set_msg_id(cmd);
    svrPkg.mutable_disp_info()->set_type((NFrame::PacketDispType)packet_type);
    svrPkg.mutable_disp_info()->set_seq(seq);
    svrPkg.mutable_store_info()->set_table_id(table_id);
    svrPkg.mutable_store_info()->set_id(trans_id);

    NFLogTrace(NF_LOG_DEFAULT, mod_key, "cmd:{} table_id:{} table_name:{} trans_id:{} seq:{} mod_key:{}",
               cmd, table_id, table_name, trans_id, seq, mod_key);;

    switch (cmd)
    {
        case NFrame::NF_STORESVR_C2S_SELECT:
        {
            svrPkg.set_msg_data(NFStoreProtoCommon::storesvr_selectbycond(dbname, table_name, mod_key, vecFields, vk_list, where_addtional_conds, max_records, cls_name));
            break;
        }
        case NFrame::NF_STORESVR_C2S_DELETE:
        {
            svrPkg.set_msg_data(NFStoreProtoCommon::storesvr_deletebycond(dbname, table_name, mod_key, vk_list, cls_name));
            break;
        }
        default:
        {
            break;
        }
    }

    return FindModule<NFIMessageModule>()->SendMsgToServer(eType, NF_ST_STORE_SERVER, 0, dstBusId,
                                                           NF_MODULE_FRAME, NFrame::NF_SERVER_TO_STORE_SERVER_DB_CMD, svrPkg);
}

int NFServerMessageModule::SendTransToStoreServer(NF_SERVER_TYPE eType, uint32_t dstBusId, uint32_t cmd, uint32_t table_id,
                                                  const std::string& dbname, const std::string& table_name, const std::string& xData, int max_records,
                                                  int trans_id, uint32_t seq,
                                                  uint64_t mod_key, const std::string& cls_name, uint8_t packet_type)
{
    CHECK_EXPR(cmd == NFrame::NF_STORESVR_C2S_EXECUTE_MORE, -1, "error cmd:{}", cmd);
    CHECK_EXPR(NFrame::PacketDispType_IsValid(packet_type), -1, "error msg_type:{}", packet_type);

    NFrame::Proto_FramePkg svrPkg;
    svrPkg.set_msg_id(cmd);
    svrPkg.mutable_disp_info()->set_type((NFrame::PacketDispType)packet_type);
    svrPkg.mutable_disp_info()->set_seq(seq);
    svrPkg.mutable_store_info()->set_table_id(table_id);
    svrPkg.mutable_store_info()->set_id(trans_id);

    NFLogTrace(NF_LOG_DEFAULT, mod_key, "cmd:{} table_id:{} table_name:{} trans_id:{} seq:{} mod_key:{}",
               cmd, table_id, table_name, trans_id, seq, mod_key);

    //NFLogTrace(NF_LOG_DEFAULT, 0, "message:{}", xData.DebugString());
    switch (cmd)
    {
        case NFrame::NF_STORESVR_C2S_EXECUTE_MORE:
        {
            svrPkg.set_msg_data(NFStoreProtoCommon::storesvr_execute_more(dbname, table_name, mod_key, xData, max_records, cls_name));
        }
        break;
        default:
        {
        }
        break;
    }
    return FindModule<NFIMessageModule>()->SendMsgToServer(eType,
                                                           NF_ST_STORE_SERVER,
                                                           0,
                                                           dstBusId,
                                                           NF_MODULE_FRAME,
                                                           NFrame::NF_SERVER_TO_STORE_SERVER_DB_CMD,
                                                           svrPkg);
}

int NFServerMessageModule::SendTransToStoreServer(NF_SERVER_TYPE eType, uint32_t dstBusId, uint32_t cmd, uint32_t table_id,
                                                  const std::string& dbname, const std::string& table_name, const std::string& xData,
                                                  int trans_id, uint32_t seq,
                                                  uint64_t mod_key, const std::string& cls_name, uint8_t packet_type)
{
    CHECK_EXPR(cmd == NFrame::NF_STORESVR_C2S_EXECUTE, -1, "error cmd:{}", cmd);
    CHECK_EXPR(NFrame::PacketDispType_IsValid(packet_type), -1, "error msg_type:{}", packet_type);

    NFrame::Proto_FramePkg svrPkg;
    svrPkg.set_msg_id(cmd);
    svrPkg.mutable_disp_info()->set_type((NFrame::PacketDispType)packet_type);
    svrPkg.mutable_disp_info()->set_seq(seq);
    svrPkg.mutable_store_info()->set_table_id(table_id);
    svrPkg.mutable_store_info()->set_id(trans_id);

    NFLogTrace(NF_LOG_DEFAULT, mod_key, "cmd:{} table_id:{} table_name:{} trans_id:{} seq:{} mod_key:{}",
               cmd, table_id, table_name, trans_id, seq, mod_key);

    //NFLogTrace(NF_LOG_DEFAULT, 0, "message:{}", xData.DebugString());
    switch (cmd)
    {
        case NFrame::NF_STORESVR_C2S_EXECUTE:
        {
            svrPkg.set_msg_data(NFStoreProtoCommon::storesvr_execute(dbname, table_name, mod_key, xData));
        }
        break;
        default:
        {
        }
        break;
    }
    return FindModule<NFIMessageModule>()->SendMsgToServer(eType,
                                                           NF_ST_STORE_SERVER,
                                                           0,
                                                           dstBusId,
                                                           NF_MODULE_FRAME,
                                                           NFrame::NF_SERVER_TO_STORE_SERVER_DB_CMD,
                                                           svrPkg);
}

int NFServerMessageModule::SendTransToStoreServer(NF_SERVER_TYPE eType, uint32_t dstBusId, uint32_t cmd, uint32_t table_id, const std::string& dbname,
                                                  const std::string& table_name, const google::protobuf::Message& xData, int trans_id,
                                                  uint32_t seq,
                                                  uint64_t mod_key, const std::string& cls_name, uint8_t packet_type)
{
    CHECK_EXPR(!(cmd == NFrame::NF_STORESVR_C2S_SELECT || cmd == NFrame::NF_STORESVR_C2S_DELETE || cmd == NFrame::NF_STORESVR_C2S_MODIFY ||
                   cmd == NFrame::NF_STORESVR_C2S_UPDATE),
               -1,
               "error cmd:{}",
               cmd);
    CHECK_EXPR(NFrame::PacketDispType_IsValid(packet_type), -1, "error msg_type:{}", packet_type);

    NFrame::Proto_FramePkg svrPkg;
    svrPkg.set_msg_id(cmd);
    svrPkg.mutable_disp_info()->set_type((NFrame::PacketDispType)packet_type);
    svrPkg.mutable_disp_info()->set_seq(seq);
    svrPkg.mutable_store_info()->set_table_id(table_id);
    svrPkg.mutable_store_info()->set_id(trans_id);

    NFLogTrace(NF_LOG_DEFAULT, mod_key, "cmd:{} table_id:{} table_name:{} trans_id:{} seq:{} mod_key:{}",
               cmd, table_id, table_name, trans_id, seq, mod_key);

    //NFLogTrace(NF_LOG_DEFAULT, 0, "message:{}", xData.DebugString());
    switch (cmd)
    {
        case NFrame::NF_STORESVR_C2S_SELECT:
        {
            std::vector<NFrame::storesvr_vk> vk_list;
            svrPkg.set_msg_data(NFStoreProtoCommon::storesvr_selectbycond(dbname, table_name, mod_key, std::vector<std::string>(), vk_list, "", 100, cls_name));
        }
        break;
        case NFrame::NF_STORESVR_C2S_SELECTOBJ:
        {
            svrPkg.set_msg_data(NFStoreProtoCommon::storesvr_selectobj(dbname, table_name, mod_key, xData, cls_name));
        }
        break;
        case NFrame::NF_STORESVR_C2S_INSERTOBJ:
        {
            svrPkg.set_msg_data(NFStoreProtoCommon::storesvr_insertobj(dbname, table_name, mod_key, xData, cls_name));
        }
        break;
        case NFrame::NF_STORESVR_C2S_DELETE:
        {
            std::vector<NFrame::storesvr_vk> vk_list;
            svrPkg.set_msg_data(NFStoreProtoCommon::storesvr_deletebycond(dbname, table_name, mod_key, vk_list, cls_name));
        }
        break;
        case NFrame::NF_STORESVR_C2S_DELETEOBJ:
        {
            svrPkg.set_msg_data(NFStoreProtoCommon::storesvr_deleteobj(dbname, table_name, mod_key, xData, cls_name));
        }
        break;
        case NFrame::NF_STORESVR_C2S_MODIFYOBJ:
        {
            svrPkg.set_msg_data(NFStoreProtoCommon::storesvr_modifyobj(dbname, table_name, mod_key, xData, cls_name));
        }
        break;
        case NFrame::NF_STORESVR_C2S_UPDATEOBJ:
        {
            svrPkg.set_msg_data(NFStoreProtoCommon::storesvr_updateobj(dbname, table_name, mod_key, xData, cls_name));
        }
        break;
        default:
        {
        }
        break;
    }

    return FindModule<NFIMessageModule>()->SendMsgToServer(eType, NF_ST_STORE_SERVER, 0, dstBusId, NF_MODULE_FRAME, NFrame::NF_SERVER_TO_STORE_SERVER_DB_CMD, svrPkg);
}

int NFServerMessageModule::SendSelectTrans(NF_SERVER_TYPE eType, uint64_t mod_key, const google::protobuf::Message& data, uint32_t table_id, int trans_id,
                                           const std::vector<std::string>& vecFields, const std::vector<NFrame::storesvr_vk>& vk_list, const std::string& where_addtional_conds,
                                           int max_records, uint32_t dstBusId,
                                           const std::string& dbname)
{
    std::string tempDBName = dbname;
    if (dbname.empty())
    {
        NFServerConfig* pConfig = FindModule<NFIConfigModule>()->GetAppConfig(eType);
        if (pConfig)
        {
            tempDBName = pConfig->DefaultDBName;
        }
    }
    CHECK_EXPR(!tempDBName.empty(), -1, "no dbname ........");

    if (dstBusId == 0)
    {
        auto pDbServer = FindModule<NFIMessageModule>()->GetSuitDbServer(eType, tempDBName, mod_key);
        if (pDbServer)
        {
            dstBusId = pDbServer->mServerInfo.bus_id();
        }
    }

    std::string tbname = NFProtobufCommon::GetProtoBaseName(data);
    std::string packageName = NFProtobufCommon::GetProtoPackageName(data);
    CHECK_EXPR(!tbname.empty(), -1, "no tbname ........");

    NFrame::Proto_FramePkg svrPkg;
    svrPkg.set_msg_id(NFrame::NF_STORESVR_C2S_SELECT);
    svrPkg.mutable_disp_info()->set_type(NFrame::E_DISP_TYPE_BY_TRANSACTION);
    svrPkg.mutable_store_info()->set_table_id(table_id);
    svrPkg.mutable_store_info()->set_id(trans_id);

    svrPkg.set_msg_data(NFStoreProtoCommon::storesvr_selectbycond(tempDBName, tbname, mod_key, vecFields, vk_list, where_addtional_conds, max_records,
                                              tbname, packageName));

    NFLogTrace(NF_LOG_DEFAULT, 0,
               "cmd:NFrame::E_STORESVR_C2S_SELECT table_id:{} table_name:{} trans_id:{} mod_key:{}", table_id, tempDBName, trans_id, mod_key);

    return FindModule<NFIMessageModule>()->SendMsgToServer(eType, NF_ST_STORE_SERVER, 0, dstBusId,
                                                           NF_MODULE_FRAME, NFrame::NF_SERVER_TO_STORE_SERVER_DB_CMD, svrPkg);
}

int NFServerMessageModule::SendSelectTrans(NF_SERVER_TYPE eType, uint64_t mod_key, const google::protobuf::Message& data, uint32_t table_id, int trans_id,
                                           const std::vector<std::string>& vecFields, const std::vector<std::string>& privateKeys, int max_records, uint32_t dstBusId,
                                           const std::string& dbname)
{
    std::string tempDBName = dbname;
    if (dbname.empty())
    {
        NFServerConfig* pConfig = FindModule<NFIConfigModule>()->GetAppConfig(eType);
        if (pConfig)
        {
            tempDBName = pConfig->DefaultDBName;
        }
    }
    CHECK_EXPR(!tempDBName.empty(), -1, "no dbname ........");

    if (dstBusId == 0)
    {
        auto pDbServer = FindModule<NFIMessageModule>()->GetSuitDbServer(eType, tempDBName, mod_key);
        if (pDbServer)
        {
            dstBusId = pDbServer->mServerInfo.bus_id();
        }
    }

    std::string tbname = NFProtobufCommon::GetProtoBaseName(data);
    std::string packageName = NFProtobufCommon::GetProtoPackageName(data);
    CHECK_EXPR(!tbname.empty(), -1, "no tbname ........");

    NFrame::Proto_FramePkg svrPkg;
    svrPkg.set_msg_id(NFrame::NF_STORESVR_C2S_SELECT);
    svrPkg.mutable_disp_info()->set_type(NFrame::E_DISP_TYPE_BY_TRANSACTION);
    svrPkg.mutable_store_info()->set_table_id(table_id);
    svrPkg.mutable_store_info()->set_id(trans_id);

    svrPkg.set_msg_data(NFStoreProtoCommon::storesvr_selectbycond(tempDBName, tbname, mod_key, vecFields, privateKeys, max_records,
                                              tbname, packageName));

    NFLogTrace(NF_LOG_DEFAULT, 0,
               "cmd:NFrame::E_STORESVR_C2S_SELECT table_id:{} table_name:{} trans_id:{} mod_key:{}", table_id, tempDBName, trans_id, mod_key);

    return FindModule<NFIMessageModule>()->SendMsgToServer(eType, NF_ST_STORE_SERVER, 0, dstBusId,
                                                           NF_MODULE_FRAME, NFrame::NF_SERVER_TO_STORE_SERVER_DB_CMD, svrPkg);
}


int NFServerMessageModule::SendSelectObjTrans(NF_SERVER_TYPE eType, uint64_t mod_key, google::protobuf::Message& data, uint32_t table_id/* = 0*/, int trans_id/* = 0*/, uint32_t seq/* = 0*/,
                                              const std::vector<std::string>& vecFields/* = std::vector<std::string>()*/, uint32_t dstBusId/* = 0*/,
                                              const std::string& dbname/* = ""*/)
{
    std::string tempDBName = dbname;
    if (dbname.empty())
    {
        NFServerConfig* pConfig = FindModule<NFIConfigModule>()->GetAppConfig(eType);
        if (pConfig)
        {
            tempDBName = pConfig->DefaultDBName;
        }
    }
    CHECK_EXPR(!tempDBName.empty(), -1, "no dbname ........");

    std::string tbname = NFProtobufCommon::GetProtoBaseName(data);
    std::string packageName = NFProtobufCommon::GetProtoPackageName(data);
    CHECK_EXPR(!tbname.empty(), -1, "no tbname ........");

    NFrame::Proto_FramePkg svrPkg;
    svrPkg.set_msg_id(NFrame::NF_STORESVR_C2S_SELECTOBJ);
    svrPkg.mutable_disp_info()->set_type(NFrame::E_DISP_TYPE_BY_TRANSACTION);
    svrPkg.mutable_disp_info()->set_seq(seq);
    svrPkg.mutable_store_info()->set_table_id(table_id);
    svrPkg.mutable_store_info()->set_id(trans_id);

    svrPkg.set_msg_data(NFStoreProtoCommon::storesvr_selectobj(tempDBName, tbname, mod_key, data, tbname, packageName, vecFields));

    NFLogTrace(NF_LOG_DEFAULT, 0,
               "cmd:NFrame::E_STORESVR_C2S_SELECTOBJ table_id:{} table_name:{} trans_id:{} seq:{} mod_key:{}",
               table_id, tempDBName, trans_id, seq, mod_key);

    return FindModule<NFIMessageModule>()->SendMsgToServer(eType, NF_ST_STORE_SERVER, 0, dstBusId,
                                                           NF_MODULE_FRAME, NFrame::NF_SERVER_TO_STORE_SERVER_DB_CMD, svrPkg);
}

int NFServerMessageModule::SendInsertObjTrans(NF_SERVER_TYPE eType, uint64_t mod_key, google::protobuf::Message& data, uint32_t table_id/* = 0*/, int trans_id/* = 0*/, uint32_t seq/* = 0*/, uint32_t dstBusId/* = 0*/, const std::string& dbname/* = ""*/)
{
    std::string tempDBName = dbname;
    if (dbname.empty())
    {
        NFServerConfig* pConfig = FindModule<NFIConfigModule>()->GetAppConfig(eType);
        if (pConfig)
        {
            tempDBName = pConfig->DefaultDBName;
        }
    }
    CHECK_EXPR(!tempDBName.empty(), -1, "no dbname ........");

    std::string tbname = NFProtobufCommon::GetProtoBaseName(data);
    std::string packageName = NFProtobufCommon::GetProtoPackageName(data);
    CHECK_EXPR(!tbname.empty(), -1, "no tbname ........");

    NFrame::Proto_FramePkg svrPkg;
    svrPkg.set_msg_id(NFrame::NF_STORESVR_C2S_INSERTOBJ);
    svrPkg.mutable_disp_info()->set_type(NFrame::E_DISP_TYPE_BY_TRANSACTION);
    svrPkg.mutable_disp_info()->set_seq(seq);
    svrPkg.mutable_store_info()->set_table_id(table_id);
    svrPkg.mutable_store_info()->set_id(trans_id);

    svrPkg.set_msg_data(NFStoreProtoCommon::storesvr_insertobj(tempDBName, tbname, mod_key, data, tbname, packageName));

    NFLogTrace(NF_LOG_DEFAULT, 0,
               "cmd:NFrame::NF_STORESVR_C2S_INSERTOBJ table_id:{} table_name:{} trans_id:{} seq:{} mod_key:{}",
               table_id, tempDBName, trans_id, seq, mod_key);

    return FindModule<NFIMessageModule>()->SendMsgToServer(eType, NF_ST_STORE_SERVER, 0, dstBusId, NF_MODULE_FRAME, NFrame::NF_SERVER_TO_STORE_SERVER_DB_CMD, svrPkg);
}

int NFServerMessageModule::SendModifyObjTrans(NF_SERVER_TYPE eType, uint64_t mod_key, google::protobuf::Message& data, uint32_t table_id, int trans_id, uint32_t seq, uint32_t dstBusId, const std::string& dbname)
{
    std::string tempDBName = dbname;
    if (dbname.empty())
    {
        NFServerConfig* pConfig = FindModule<NFIConfigModule>()->GetAppConfig(eType);
        if (pConfig)
        {
            tempDBName = pConfig->DefaultDBName;
        }
    }
    CHECK_EXPR(!tempDBName.empty(), -1, "no dbname ........");

    std::string tbname = NFProtobufCommon::GetProtoBaseName(data);
    std::string packageName = NFProtobufCommon::GetProtoPackageName(data);
    CHECK_EXPR(!tbname.empty(), -1, "no tbname ........");

    NFrame::Proto_FramePkg svrPkg;
    svrPkg.set_msg_id(NFrame::NF_STORESVR_C2S_MODIFYOBJ);
    svrPkg.mutable_disp_info()->set_type(NFrame::E_DISP_TYPE_BY_TRANSACTION);
    svrPkg.mutable_disp_info()->set_seq(seq);
    svrPkg.mutable_store_info()->set_table_id(table_id);
    svrPkg.mutable_store_info()->set_id(trans_id);

    svrPkg.set_msg_data(NFStoreProtoCommon::storesvr_modifyobj(tempDBName, tbname, mod_key, data, tbname, packageName));

    NFLogTrace(NF_LOG_DEFAULT, 0,
               "cmd:NFrame::NF_STORESVR_C2S_MODIFYOBJ table_id:{} table_name:{} trans_id:{} seq:{} mod_key:{}",
               table_id, tempDBName, trans_id, seq, mod_key);

    return FindModule<NFIMessageModule>()->SendMsgToServer(eType, NF_ST_STORE_SERVER, 0, dstBusId, NF_MODULE_FRAME, NFrame::NF_SERVER_TO_STORE_SERVER_DB_CMD, svrPkg);
}

int NFServerMessageModule::SendDeleteObjTrans(NF_SERVER_TYPE eType, uint64_t mod_key, google::protobuf::Message& data, uint32_t table_id, int trans_id, uint32_t seq, uint32_t dstBusId, const std::string& dbname)
{
    std::string tempDBName = dbname;
    if (dbname.empty())
    {
        NFServerConfig* pConfig = FindModule<NFIConfigModule>()->GetAppConfig(eType);
        if (pConfig)
        {
            tempDBName = pConfig->DefaultDBName;
        }
    }
    CHECK_EXPR(!tempDBName.empty(), -1, "no dbname ........");

    std::string tbname = NFProtobufCommon::GetProtoBaseName(data);
    std::string packageName = NFProtobufCommon::GetProtoPackageName(data);
    CHECK_EXPR(!tbname.empty(), -1, "no tbname ........");

    NFrame::Proto_FramePkg svrPkg;
    svrPkg.set_msg_id(NFrame::NF_STORESVR_C2S_DELETEOBJ);
    svrPkg.mutable_disp_info()->set_type(NFrame::E_DISP_TYPE_BY_TRANSACTION);
    svrPkg.mutable_disp_info()->set_seq(seq);
    svrPkg.mutable_store_info()->set_table_id(table_id);
    svrPkg.mutable_store_info()->set_id(trans_id);

    svrPkg.set_msg_data(NFStoreProtoCommon::storesvr_deleteobj(tempDBName, tbname, mod_key, data, tbname, packageName));

    NFLogTrace(NF_LOG_DEFAULT, 0,
               "cmd:NFrame::NF_STORESVR_C2S_DELETEOBJ table_id:{} table_name:{} trans_id:{} seq:{} mod_key:{}",
               table_id, tempDBName, trans_id, seq, mod_key);

    return FindModule<NFIMessageModule>()->SendMsgToServer(eType, NF_ST_STORE_SERVER, 0, dstBusId, NF_MODULE_FRAME, NFrame::NF_SERVER_TO_STORE_SERVER_DB_CMD, svrPkg);
}
