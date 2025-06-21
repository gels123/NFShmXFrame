// -------------------------------------------------------------------------
//    @FileName         :    NFGameServerModule.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFGameServerPlugin
//
// -------------------------------------------------------------------------

#include <NFServerComm/NFServerCommon/NFServerCommonDefine.h>
#include "NFCStoreServerModule.h"
#include "NFComm/NFPluginModule/NFIPluginManager.h"
#include "NFComm/NFPluginModule/NFIConfigModule.h"
#include "NFComm/NFPluginModule/NFIMessageModule.h"
#include "NFComm/NFPluginModule/NFIAsyDBModule.h"
#include "NFComm/NFPluginModule/NFCheck.h"
#include "NFComm/NFPluginModule/NFProtobufCommon.h"
#include "NFComm/NFPluginModule/NFIMysqlModule.h"
#include "NFCommPlugin/NFKernelPlugin/NFCoroutine.h"
#include "NFComm/NFKernelMessage/FrameMsg.pb.h"
#include "NFComm/NFPluginModule/NFINosqlModule.h"
#include "NFComm/NFPluginModule/NFIAsyMysqlModule.h"

#define STORE_SERVER_TIMER_CLOSE_MYSQL 200

NFCStoreServerModule::NFCStoreServerModule(NFIPluginManager *p) : NFIStoreServerModule(p)
{
    m_useCache = false;
    SetConnectProxyAgentServer(false);
}

NFCStoreServerModule::~NFCStoreServerModule()
{
}

bool NFCStoreServerModule::Awake()
{
    //////rpc service//////////////////////
    FindModule<NFIMessageModule>()->AddRpcService<NF_MODULE_FRAME, NFrame::NF_STORESVR_C2S_SELECTOBJ>(NF_ST_STORE_SERVER, this,
                                                                                       &NFCStoreServerModule::OnHandleSelectObjRpc, true);
    FindModule<NFIMessageModule>()->AddRpcService<NF_MODULE_FRAME, NFrame::NF_STORESVR_C2S_SELECT>(NF_ST_STORE_SERVER, this,
                                                                                    &NFCStoreServerModule::OnHandleSelectRpc, true);
    FindModule<NFIMessageModule>()->AddRpcService<NF_MODULE_FRAME, NFrame::NF_STORESVR_C2S_INSERTOBJ>(NF_ST_STORE_SERVER, this,
                                                                                    &NFCStoreServerModule::OnHandleInsertObjRpc, true);
    FindModule<NFIMessageModule>()->AddRpcService<NF_MODULE_FRAME, NFrame::NF_STORESVR_C2S_MODIFYOBJ>(NF_ST_STORE_SERVER, this,
                                                                                       &NFCStoreServerModule::OnHandleModifyObjRpc, true);
    FindModule<NFIMessageModule>()->AddRpcService<NF_MODULE_FRAME, NFrame::NF_STORESVR_C2S_MODIFY>(NF_ST_STORE_SERVER, this,
                                                                                       &NFCStoreServerModule::OnHandleModifyRpc, true);
    FindModule<NFIMessageModule>()->AddRpcService<NF_MODULE_FRAME, NFrame::NF_STORESVR_C2S_UPDATE>(NF_ST_STORE_SERVER, this,
                                                                                    &NFCStoreServerModule::OnHandleUpdateRpc, true);
    FindModule<NFIMessageModule>()->AddRpcService<NF_MODULE_FRAME, NFrame::NF_STORESVR_C2S_UPDATEOBJ>(NF_ST_STORE_SERVER, this,
                                                                                       &NFCStoreServerModule::OnHandleUpdateObjRpc, true);
    FindModule<NFIMessageModule>()->AddRpcService<NF_MODULE_FRAME, NFrame::NF_STORESVR_C2S_EXECUTE>(NF_ST_STORE_SERVER, this,
                                                                                       &NFCStoreServerModule::OnHandleExecuteRpc, true);
    FindModule<NFIMessageModule>()->AddRpcService<NF_MODULE_FRAME, NFrame::NF_STORESVR_C2S_EXECUTE_MORE>(NF_ST_STORE_SERVER, this,
                                                                                     &NFCStoreServerModule::OnHandleExecuteMoreRpc, true);
    FindModule<NFIMessageModule>()->AddRpcService<NF_MODULE_FRAME, NFrame::NF_STORESVR_C2S_DELETE>(NF_ST_STORE_SERVER, this,
                                                                                     &NFCStoreServerModule::OnHandleDeleteRpc, true);
    FindModule<NFIMessageModule>()->AddRpcService<NF_MODULE_FRAME, NFrame::NF_STORESVR_C2S_DELETEOBJ>(NF_ST_STORE_SERVER, this,
                                                                                    &NFCStoreServerModule::OnHandleDeleteObjRpc, true);
    //////other server///////////////////////////////////
    RegisterServerMessage(NF_ST_STORE_SERVER, NF_MODULE_FRAME, NFrame::NF_SERVER_TO_STORE_SERVER_DB_CMD);

    if (!LoadPbAndCheckDB())
    {
        return false;
    }

    NFServerConfig *pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_STORE_SERVER);
    CHECK_EXPR_ASSERT(pConfig, false, "GetAppConfig Failed, server type:{}", NF_ST_STORE_SERVER);

    int iRet = 0;
    if (pConfig->RedisConfig.RedisIp.empty())
    {
        iRet = FindModule<NFIAsyMysqlModule>()->AddMysqlServer(pConfig->MysqlConfig.MysqlDbName, pConfig->MysqlConfig.MysqlIp,
                                                             pConfig->MysqlConfig.MysqlPort, pConfig->MysqlConfig.MysqlDbName,
                                                             pConfig->MysqlConfig.MysqlUser, pConfig->MysqlConfig.MysqlPassword);
    }
    else {
        iRet = FindModule<NFIAsyDbModule>()->AddDbServer(pConfig->MysqlConfig.MysqlDbName, pConfig->MysqlConfig.MysqlIp,
                                                             pConfig->MysqlConfig.MysqlPort, pConfig->MysqlConfig.MysqlDbName,
                                                             pConfig->MysqlConfig.MysqlUser, pConfig->MysqlConfig.MysqlPassword,
                                                             pConfig->RedisConfig.RedisIp, pConfig->RedisConfig.RedisPort, pConfig->RedisConfig.RedisPass);
        m_useCache = true;
    }

    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "store server connect db failed");
        return false;
    }

    BindServer();

    return true;
}

int NFCStoreServerModule::OnTimer(uint32_t nTimerID)
{
    NFIStoreServerModule::OnTimer(nTimerID);
    if (nTimerID == STORE_SERVER_TIMER_CLOSE_MYSQL)
    {
        FindModule<NFIMysqlModule>()->CloseMysql(INFORMATION_SCHEMA);
        KillTimer(STORE_SERVER_TIMER_CLOSE_MYSQL);
        for (int i = 0; i < (int) NFProtobufCommon::Instance()->m_pOldPoolVec.size(); i++)
        {
            NF_SAFE_DELETE(NFProtobufCommon::Instance()->m_pOldPoolVec[i]);
        }
        NFProtobufCommon::Instance()->m_pOldPoolVec.clear();
    }

    return 0;
}

struct DBTableColCreateInfo
{
    bool bExistTable = false;
    std::map<std::string, DBTableColInfo> primaryKey;
    std::multimap<uint32_t, std::string> needCreateColumn;
};

bool NFCStoreServerModule::LoadPbAndCheckDB()
{
    NFServerConfig *pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_STORE_SERVER);
    CHECK_EXPR_ASSERT(pConfig, false, "GetAppConfig Failed, server type:{}", NF_ST_STORE_SERVER);

    int iRet = NFProtobufCommon::Instance()->LoadProtoDsFile(m_pObjPluginManager->GetConfigPath() + "/" + pConfig->LoadProtoDs);
    if (iRet == 0)
    {
        NFLogInfo(NF_LOG_DEFAULT, 0, "Reload proto ds success:{}", pConfig->LoadProtoDs);
    }
    else
    {
        NFLogInfo(NF_LOG_DEFAULT, 0, "Reload proto ds fail:{}", pConfig->LoadProtoDs);
        return false;
    }

    KillTimer(STORE_SERVER_TIMER_CLOSE_MYSQL);

    iRet = FindModule<NFIMysqlModule>()->AddMysqlServer(INFORMATION_SCHEMA, pConfig->MysqlConfig.MysqlIp,
                                                        pConfig->MysqlConfig.MysqlPort, INFORMATION_SCHEMA,
                                                        pConfig->MysqlConfig.MysqlUser, pConfig->MysqlConfig.MysqlPassword);
    if (iRet != 0)
    {
        NFLogInfo(NF_LOG_DEFAULT, -1, "store server connect mysql failed");
        return false;
    }

    bool bExistDB;
    iRet = FindModule<NFIMysqlModule>()->ExistsDB(INFORMATION_SCHEMA, pConfig->MysqlConfig.MysqlDbName, bExistDB);
    if (iRet != 0)
    {
        NFLogInfo(NF_LOG_DEFAULT, -1, "store server ExistsDB failed");
        return false;
    }

    if (!bExistDB)
    {
        iRet = FindModule<NFIMysqlModule>()->CreateDB(INFORMATION_SCHEMA, pConfig->MysqlConfig.MysqlDbName);
        if (iRet != 0)
        {
            NFLogInfo(NF_LOG_DEFAULT, -1, "store server CreateDB failed");
            return false;
        }
    }

    std::map<std::string, DBTableColCreateInfo> tbCreateInfo;
    for (int i = 0; i < (int) pConfig->MysqlConfig.TBConfList.size(); i++)
    {
        struct pbTableConfig &tableConfig = pConfig->MysqlConfig.TBConfList[i];
        if (tableConfig.TableName.empty()) continue;

        DBTableColCreateInfo &createInfo = tbCreateInfo[tableConfig.TableName];

        iRet = FindModule<NFIMysqlModule>()->QueryTableInfo(INFORMATION_SCHEMA, pConfig->MysqlConfig.MysqlDbName, tableConfig.TableName,
                                                            createInfo.bExistTable, createInfo.primaryKey, createInfo.needCreateColumn);
        if (iRet != 0)
        {
            NFLogInfo(NF_LOG_DEFAULT, -1, "store server CreateDB failed");
            return false;
        }
    }

    iRet = FindModule<NFIMysqlModule>()->SelectDB(INFORMATION_SCHEMA, pConfig->MysqlConfig.MysqlDbName);
    if (iRet != 0)
    {
        NFLogInfo(NF_LOG_DEFAULT, -1, "store server SelectDB failed");
        return false;
    }

    for (auto iter = tbCreateInfo.begin(); iter != tbCreateInfo.end(); iter++)
    {
        std::string tableName = iter->first;
        DBTableColCreateInfo &createInfo = iter->second;
        if (!createInfo.bExistTable)
        {
            iRet = FindModule<NFIMysqlModule>()->CreateTable(INFORMATION_SCHEMA, tableName, createInfo.primaryKey, createInfo.needCreateColumn);
            if (iRet != 0)
            {
                NFLogInfo(NF_LOG_DEFAULT, -1, "store server CreateTable failed");
                return false;
            }
        }
        else
        {
            iRet = FindModule<NFIMysqlModule>()->AddTableRow(INFORMATION_SCHEMA, tableName, createInfo.needCreateColumn);
            if (iRet != 0)
            {
                NFLogInfo(NF_LOG_DEFAULT, -1, "store server CreateTable failed");
                return false;
            }
        }
    }

    iRet = FindModule<NFIMysqlModule>()->SelectDB(INFORMATION_SCHEMA, INFORMATION_SCHEMA);
    if (iRet != 0)
    {
        NFLogInfo(NF_LOG_DEFAULT, -1, "store server SelectDB failed");
        return false;
    }

    SetTimer(STORE_SERVER_TIMER_CLOSE_MYSQL, 60000, 0);
    return true;
}

bool NFCStoreServerModule::Init()
{
    ConnectMasterServer();
    return true;
}

bool NFCStoreServerModule::Execute()
{
    return true;
}

bool NFCStoreServerModule::OnDynamicPlugin()
{
    FindModule<NFIMessageModule>()->CloseAllLink(NF_ST_STORE_SERVER);
    return true;
}

bool NFCStoreServerModule::OnReloadConfig()
{
    return LoadPbAndCheckDB();
}

int NFCStoreServerModule::OnHandleServerMessage(uint64_t unLinkId, NFDataPackage &packet)
{
    int retCode = 0;
    switch (packet.nMsgId)
    {
        case NFrame::NF_SERVER_TO_STORE_SERVER_DB_CMD:
            retCode = OnHandleStoreReq(unLinkId, packet);
            break;
        default:
            NFLogError(NF_LOG_DEFAULT, 0, "msg:({}) not handle", packet.ToString());
            break;
    }

    if (retCode != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "msg:({}) handle exist error", packet.ToString());
    }
    return 0;
}

int
NFCStoreServerModule::OnHandleStoreReq(uint64_t unLinkId, NFDataPackage &packet)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");

    NFServerConfig *pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_STORE_SERVER);
    NF_ASSERT(pConfig);

    NFIAsySqlModule* pNFIAsySqlModule = NULL;
    if (m_useCache)
    {
        pNFIAsySqlModule = FindModule<NFIAsyDbModule>();
    }
    else {
        pNFIAsySqlModule = FindModule<NFIAsyMysqlModule>();
    }
    NF_ASSERT(pNFIAsySqlModule);

    NFrame::Proto_FramePkg xMsg;
    CLIENT_MSG_PROCESS_WITH_PRINTF(packet, xMsg);
    uint64_t sendLinkId = GetUnLinkId(NF_IS_NONE, NF_ST_STORE_SERVER, pConfig->BusId, 0);
    uint64_t destLinkId = GetUnLinkId(NF_IS_NONE, 0, packet.nSrcId, 0);

    NFrame::Proto_FramePkg retMsg;
    retMsg.set_msg_id(xMsg.msg_id());
    *retMsg.mutable_disp_info() = xMsg.disp_info();
    *retMsg.mutable_store_info() = xMsg.store_info();

    switch (xMsg.msg_id())
    {
        case NFrame::NF_STORESVR_C2S_SELECT:
        {
            retMsg.set_msg_id(NFrame::NF_STORESVR_S2C_SELECT);

            NFrame::storesvr_sel select;
            select.ParsePartialFromString(xMsg.msg_data());

            bool cache = false;
            auto iter = pConfig->mTBConfMap.find(select.baseinfo().tbname());
            if (iter != pConfig->mTBConfMap.end())
            {
                uint32_t count = iter->second.TableCount;
                if (count > 1)
                {
                    uint32_t index = select.cond().mod_key() % count;
                    std::string newTableName = select.baseinfo().tbname() + "_" + NFCommon::tostr(index);
                    select.mutable_baseinfo()->set_tbname(newTableName);
                }
                cache = iter->second.Cache;
            }

            pNFIAsySqlModule->SelectByCond
                    (select.baseinfo().dbname(), select, cache,
                     [=](int iRet, NFrame::storesvr_sel_res &select_res) mutable
                     {
                         select_res.mutable_opres()->set_err_code(iRet);
                         if (iRet != 0)
                         {
                             retMsg.mutable_disp_info()->set_err_code(NFrame::ERR_CODE_STORESVR_ERRCODE_BUSY);
                         }
                         else
                         {
                             retMsg.set_msg_data(select_res.SerializePartialAsString());
                         }

                         NFLogTrace(NF_LOG_DEFAULT, 0, "ret msg:{}", retMsg.Utf8DebugString());
                         if (retMsg.store_info().id() > 0)
                             FindModule<NFIMessageModule>()->Send(unLinkId, NF_MODULE_FRAME,
                                                                  NFrame::NF_STORE_SERVER_TO_SERVER_DB_CMD,
                                                                  retMsg, 0, 0, sendLinkId, destLinkId);
                     });
        }
            break;
        case NFrame::NF_STORESVR_C2S_SELECTOBJ:
        {
            retMsg.set_msg_id(NFrame::NF_STORESVR_S2C_SELECTOBJ);

            NFrame::storesvr_selobj select;
            select.ParsePartialFromString(xMsg.msg_data());

            bool cache = false;
            auto iter = pConfig->mTBConfMap.find(select.baseinfo().tbname());
            if (iter != pConfig->mTBConfMap.end())
            {
                uint32_t count = iter->second.TableCount;
                if (count > 1)
                {
                    uint32_t index = select.mod_key() % count;
                    std::string newTableName = select.baseinfo().tbname() + "_" + NFCommon::tostr(index);
                    select.mutable_baseinfo()->set_tbname(newTableName);
                }
                cache = iter->second.Cache;
            }

            pNFIAsySqlModule->SelectObj
                    (select.baseinfo().dbname(), select, cache,
                     [=](int iRet, NFrame::storesvr_selobj_res &select_res) mutable
                     {
                         select_res.mutable_opres()->set_err_code(iRet);
                         if (iRet != 0)
                         {
                             if (iRet == NFrame::ERR_CODE_STORESVR_ERRCODE_SELECT_EMPTY &&
                                 select_res.opres().errmsg().empty())
                             {
                                 retMsg.mutable_disp_info()->set_err_code(
                                         NFrame::ERR_CODE_STORESVR_ERRCODE_SELECT_EMPTY);
                             }
                             else
                             {
                                 retMsg.mutable_disp_info()->set_err_code(
                                         NFrame::ERR_CODE_STORESVR_ERRCODE_UNKNOWN);
                             }
                         }
                         else
                         {
                             retMsg.set_msg_data(select_res.SerializePartialAsString());
                         }

                         NFLogTrace(NF_LOG_DEFAULT, 0, "ret msg:{}", retMsg.Utf8DebugString());
                         if (retMsg.store_info().id() > 0)
                             FindModule<NFIMessageModule>()->Send(unLinkId, NF_MODULE_FRAME,
                                                                  NFrame::NF_STORE_SERVER_TO_SERVER_DB_CMD,
                                                                  retMsg, 0, 0, sendLinkId, destLinkId);
                     });
        }
            break;
        case NFrame::NF_STORESVR_C2S_INSERTOBJ:
        {
            retMsg.set_msg_id(NFrame::NF_STORESVR_S2C_INSERTOBJ);

            NFrame::storesvr_insertobj select;
            select.ParsePartialFromString(xMsg.msg_data());

            bool cache = false;
            auto iter = pConfig->mTBConfMap.find(select.baseinfo().tbname());
            if (iter != pConfig->mTBConfMap.end())
            {
                uint32_t count = iter->second.TableCount;
                if (count > 1)
                {
                    uint32_t index = select.mod_key() % count;
                    std::string newTableName = select.baseinfo().tbname() + "_" + NFCommon::tostr(index);
                    select.mutable_baseinfo()->set_tbname(newTableName);
                }
                cache = iter->second.Cache;
            }

            pNFIAsySqlModule->InsertObj
                    (select.baseinfo().dbname(), select, cache,
                     [=](int iRet, NFrame::storesvr_insertobj_res &select_res) mutable
                     {
                         select_res.mutable_opres()->set_err_code(iRet);
                         if (iRet != 0)
                         {
                             retMsg.mutable_disp_info()->set_err_code(
                                     NFrame::ERR_CODE_STORESVR_ERRCODE_INSERTFAILED);
                         }
                         else
                         {
                             retMsg.set_msg_data(select_res.SerializePartialAsString());
                         }

                         NFLogTrace(NF_LOG_DEFAULT, 0, "ret msg:{}", retMsg.Utf8DebugString());
                         if (retMsg.store_info().id() > 0)
                             FindModule<NFIMessageModule>()->Send(unLinkId, NF_MODULE_FRAME,
                                                                  NFrame::NF_STORE_SERVER_TO_SERVER_DB_CMD,
                                                                  retMsg, 0, 0, sendLinkId, destLinkId);
                     });
        }
            break;
        case NFrame::NF_STORESVR_C2S_DELETE:
        {
            retMsg.set_msg_id(NFrame::NF_STORESVR_S2C_DELETE);

            NFrame::storesvr_del select;
            select.ParsePartialFromString(xMsg.msg_data());

            bool cache = false;
            auto iter = pConfig->mTBConfMap.find(select.baseinfo().tbname());
            if (iter != pConfig->mTBConfMap.end())
            {
                uint32_t count = iter->second.TableCount;
                if (count > 1)
                {
                    uint32_t index = select.cond().mod_key() % count;
                    std::string newTableName = select.baseinfo().tbname() + "_" + NFCommon::tostr(index);
                    select.mutable_baseinfo()->set_tbname(newTableName);
                }
                cache = iter->second.Cache;
            }

            pNFIAsySqlModule->DeleteByCond
                    (select.baseinfo().dbname(), select, cache,
                     [=](int iRet, NFrame::storesvr_del_res &select_res) mutable
                     {
                         select_res.mutable_opres()->set_err_code(iRet);
                         if (iRet != 0)
                         {
                             retMsg.mutable_disp_info()->set_err_code(
                                     NFrame::ERR_CODE_STORESVR_ERRCODE_UNKNOWN);
                         }
                         else
                         {
                             retMsg.set_msg_data(select_res.SerializePartialAsString());
                         }

                         NFLogTrace(NF_LOG_DEFAULT, 0, "ret msg:{}", retMsg.Utf8DebugString());
                         if (retMsg.store_info().id() > 0)
                             FindModule<NFIMessageModule>()->Send(unLinkId, NF_MODULE_FRAME,
                                                                  NFrame::NF_STORE_SERVER_TO_SERVER_DB_CMD,
                                                                  retMsg, 0, 0, sendLinkId, destLinkId);
                     });
        }
            break;
        case NFrame::NF_STORESVR_C2S_DELETEOBJ:
        {
            retMsg.set_msg_id(NFrame::NF_STORESVR_S2C_DELETEOBJ);

            NFrame::storesvr_delobj select;
            select.ParsePartialFromString(xMsg.msg_data());

            bool cache = false;
            auto iter = pConfig->mTBConfMap.find(select.baseinfo().tbname());
            if (iter != pConfig->mTBConfMap.end())
            {
                uint32_t count = iter->second.TableCount;
                if (count > 1)
                {
                    uint32_t index = select.mod_key() % count;
                    std::string newTableName = select.baseinfo().tbname() + "_" + NFCommon::tostr(index);
                    select.mutable_baseinfo()->set_tbname(newTableName);
                }
                cache = iter->second.Cache;
            }

            pNFIAsySqlModule->DeleteObj
                    (select.baseinfo().dbname(), select, cache,
                     [=](int iRet, NFrame::storesvr_delobj_res &select_res) mutable
                     {
                         select_res.mutable_opres()->set_err_code(iRet);
                         if (iRet != 0)
                         {
                             retMsg.mutable_disp_info()->set_err_code(
                                     NFrame::ERR_CODE_STORESVR_ERRCODE_DELETEFAILED);
                         }
                         else
                         {
                             retMsg.set_msg_data(select_res.SerializePartialAsString());
                         }

                         NFLogTrace(NF_LOG_DEFAULT, 0, "ret msg:{}", retMsg.Utf8DebugString());
                         if (retMsg.store_info().id() > 0)
                             FindModule<NFIMessageModule>()->Send(unLinkId, NF_MODULE_FRAME,
                                                                  NFrame::NF_STORE_SERVER_TO_SERVER_DB_CMD,
                                                                  retMsg, 0, 0, sendLinkId, destLinkId);
                     });
        }
            break;
        case NFrame::NF_STORESVR_C2S_MODIFY:
        {
            retMsg.set_msg_id(NFrame::NF_STORESVR_S2C_MODIFY);

            NFrame::storesvr_mod select;
            select.ParsePartialFromString(xMsg.msg_data());

            bool cache = false;
            auto iter = pConfig->mTBConfMap.find(select.baseinfo().tbname());
            if (iter != pConfig->mTBConfMap.end())
            {
                uint32_t count = iter->second.TableCount;
                if (count > 1)
                {
                    uint32_t index = select.cond().mod_key() % count;
                    std::string newTableName = select.baseinfo().tbname() + "_" + NFCommon::tostr(index);
                    select.mutable_baseinfo()->set_tbname(newTableName);
                }
                cache = iter->second.Cache;
            }

            pNFIAsySqlModule->ModifyByCond
                    (select.baseinfo().dbname(), select, cache,
                     [=](int iRet, NFrame::storesvr_mod_res &select_res) mutable
                     {
                         select_res.mutable_opres()->set_err_code(iRet);
                         if (iRet != 0)
                         {
                             retMsg.mutable_disp_info()->set_err_code(
                                     NFrame::ERR_CODE_STORESVR_ERRCODE_UPDATEFAILED);
                         }
                         else
                         {
                             retMsg.set_msg_data(select_res.SerializePartialAsString());
                         }

                         NFLogTrace(NF_LOG_DEFAULT, 0, "ret msg:{}", retMsg.Utf8DebugString());
                         if (retMsg.store_info().id() > 0)
                             FindModule<NFIMessageModule>()->Send(unLinkId, NF_MODULE_FRAME,
                                                                  NFrame::NF_STORE_SERVER_TO_SERVER_DB_CMD,
                                                                  retMsg, 0, 0, sendLinkId, destLinkId);
                     });
        }
            break;
        case NFrame::NF_STORESVR_C2S_MODIFYOBJ:
        {
            retMsg.set_msg_id(NFrame::NF_STORESVR_S2C_MODIFYOBJ);

            NFrame::storesvr_modobj select;
            select.ParsePartialFromString(xMsg.msg_data());

            bool cache = false;
            auto iter = pConfig->mTBConfMap.find(select.baseinfo().tbname());
            if (iter != pConfig->mTBConfMap.end())
            {
                uint32_t count = iter->second.TableCount;
                if (count > 1)
                {
                    uint32_t index = select.mod_key() % count;
                    std::string newTableName = select.baseinfo().tbname() + "_" + NFCommon::tostr(index);
                    select.mutable_baseinfo()->set_tbname(newTableName);
                }
                cache = iter->second.Cache;
            }

            pNFIAsySqlModule->ModifyObj
                    (select.baseinfo().dbname(), select, cache,
                     [=](int iRet, NFrame::storesvr_modobj_res &select_res) mutable
                     {
                         select_res.mutable_opres()->set_err_code(iRet);
                         if (iRet != 0)
                         {
                             retMsg.mutable_disp_info()->set_err_code(
                                     NFrame::ERR_CODE_STORESVR_ERRCODE_UPDATEFAILED);
                         }
                         else
                         {
                             retMsg.set_msg_data(select_res.SerializePartialAsString());
                         }

                         NFLogTrace(NF_LOG_DEFAULT, 0, "ret msg:{}", retMsg.Utf8DebugString());
                         if (retMsg.store_info().id() > 0)
                             FindModule<NFIMessageModule>()->Send(unLinkId, NF_MODULE_FRAME,
                                                                  NFrame::NF_STORE_SERVER_TO_SERVER_DB_CMD,
                                                                  retMsg, 0, 0, sendLinkId, destLinkId);
                     });
        }
            break;
        case NFrame::NF_STORESVR_C2S_UPDATE:
        {
            retMsg.set_msg_id(NFrame::NF_STORESVR_S2C_UPDATE);

            NFrame::storesvr_update select;
            select.ParsePartialFromString(xMsg.msg_data());

            bool cache = false;
            auto iter = pConfig->mTBConfMap.find(select.baseinfo().tbname());
            if (iter != pConfig->mTBConfMap.end())
            {
                uint32_t count = iter->second.TableCount;
                if (count > 1)
                {
                    uint32_t index = select.cond().mod_key() % count;
                    std::string newTableName = select.baseinfo().tbname() + "_" + NFCommon::tostr(index);
                    select.mutable_baseinfo()->set_tbname(newTableName);
                }
                cache = iter->second.Cache;
            }

            pNFIAsySqlModule->UpdateByCond
                    (select.baseinfo().dbname(), select, cache,
                     [=](int iRet, NFrame::storesvr_update_res &select_res) mutable
                     {
                         select_res.mutable_opres()->set_err_code(iRet);
                         if (iRet != 0)
                         {
                             retMsg.mutable_disp_info()->set_err_code(
                                     NFrame::ERR_CODE_STORESVR_ERRCODE_UPDATEINSERTFAILED);
                         }
                         else
                         {
                             retMsg.set_msg_data(select_res.SerializePartialAsString());
                         }

                         NFLogTrace(NF_LOG_DEFAULT, 0, "ret msg:{}", retMsg.Utf8DebugString());
                         if (retMsg.store_info().id() > 0)
                             FindModule<NFIMessageModule>()->Send(unLinkId, NF_MODULE_FRAME,
                                                                  NFrame::NF_STORE_SERVER_TO_SERVER_DB_CMD,
                                                                  retMsg, 0, 0, sendLinkId, destLinkId);
                     });
        }
            break;
        case NFrame::NF_STORESVR_C2S_UPDATEOBJ:
        {
            retMsg.set_msg_id(NFrame::NF_STORESVR_S2C_UPDATEOBJ);

            NFrame::storesvr_updateobj select;
            select.ParsePartialFromString(xMsg.msg_data());

            bool cache = false;
            auto iter = pConfig->mTBConfMap.find(select.baseinfo().tbname());
            if (iter != pConfig->mTBConfMap.end())
            {
                uint32_t count = iter->second.TableCount;
                if (count > 1)
                {
                    uint32_t index = select.mod_key() % count;
                    std::string newTableName = select.baseinfo().tbname() + "_" + NFCommon::tostr(index);
                    select.mutable_baseinfo()->set_tbname(newTableName);
                }
                cache = iter->second.Cache;
            }

            pNFIAsySqlModule->UpdateObj
                    (select.baseinfo().dbname(), select, cache,
                     [=](int iRet, NFrame::storesvr_updateobj_res &select_res) mutable
                     {
                         select_res.mutable_opres()->set_err_code(iRet);
                         if (iRet != 0)
                         {
                             retMsg.mutable_disp_info()->set_err_code(
                                     NFrame::ERR_CODE_STORESVR_ERRCODE_UPDATEINSERTFAILED);
                         }
                         else
                         {
                             retMsg.set_msg_data(select_res.SerializePartialAsString());
                         }

                         NFLogTrace(NF_LOG_DEFAULT, 0, "ret msg:{}", retMsg.Utf8DebugString());
                         if (retMsg.store_info().id() > 0)
                             FindModule<NFIMessageModule>()->Send(unLinkId, NF_MODULE_FRAME,
                                                                  NFrame::NF_STORE_SERVER_TO_SERVER_DB_CMD,
                                                                  retMsg, 0, 0, sendLinkId, destLinkId);
                     });
        }
            break;
        case NFrame::NF_STORESVR_C2S_EXECUTE:
        {
            retMsg.set_msg_id(NFrame::NF_STORESVR_S2C_EXECUTE);

            NFrame::storesvr_execute select;
            select.ParsePartialFromString(xMsg.msg_data());

            pNFIAsySqlModule->Execute
                    (select.baseinfo().dbname(), select,
                     [=](int iRet, NFrame::storesvr_execute_res &select_res) mutable
                     {
                         select_res.mutable_opres()->set_err_code(iRet);
                         if (iRet != 0)
                         {
                             retMsg.mutable_disp_info()->set_err_code(
                                     NFrame::ERR_CODE_STORESVR_ERRCODE_UPDATEFAILED);
                         }
                         else
                         {
                             retMsg.set_msg_data(select_res.SerializePartialAsString());
                         }

                         NFLogTrace(NF_LOG_DEFAULT, 0, "ret msg:{}", retMsg.Utf8DebugString());
                         if (retMsg.store_info().id() > 0)
                             FindModule<NFIMessageModule>()->Send(unLinkId, NF_MODULE_FRAME,
                                                                  NFrame::NF_STORE_SERVER_TO_SERVER_DB_CMD, retMsg,
                                                                  0, 0, sendLinkId, destLinkId);
                     });
        }
            break;
        default:
        {

        }
            break;
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCStoreServerModule::OnHandleSelectObjRpc(NFrame::storesvr_selobj &request, NFrame::storesvr_selobj_res &respone)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    NF_ASSERT(FindModule<NFICoroutineModule>()->IsInCoroutine());

    NFServerConfig *pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_STORE_SERVER);
    NF_ASSERT(pConfig);

    NFIAsySqlModule* pNFIAsySqlModule = NULL;
    if (m_useCache)
    {
        pNFIAsySqlModule = FindModule<NFIAsyDbModule>();
    }
    else {
        pNFIAsySqlModule = FindModule<NFIAsyMysqlModule>();
    }
    NF_ASSERT(pNFIAsySqlModule);

    bool cache = false;
    auto iter = pConfig->mTBConfMap.find(request.baseinfo().tbname());
    if (iter != pConfig->mTBConfMap.end())
    {
        uint32_t count = iter->second.TableCount;
        if (count > 1)
        {
            uint32_t index = request.mod_key() % count;
            std::string newTableName = request.baseinfo().tbname() + "_" + NFCommon::tostr(index);
            request.mutable_baseinfo()->set_tbname(newTableName);
        }

        cache = iter->second.Cache;
    }

    int64_t coId = FindModule<NFICoroutineModule>()->CurrentTaskId();
    int iRet = pNFIAsySqlModule->SelectObj
            (request.baseinfo().dbname(), request, cache,
             [this, coId, &respone](int iRet, NFrame::storesvr_selobj_res &select_res) mutable
             {
                 if (!FindModule<NFICoroutineModule>()->IsYielding(coId))
                 {
                     NFLogError(NF_LOG_DEFAULT, 0,
                                "SelectObj, But Coroutine Status Error..........Not Yielding");
                     return;
                 }

                 if (iRet != 0)
                 {
                     if (iRet == NFrame::ERR_CODE_STORESVR_ERRCODE_SELECT_EMPTY &&
                         select_res.opres().errmsg().empty())
                     {
                         iRet = NFrame::ERR_CODE_STORESVR_ERRCODE_SELECT_EMPTY;
                     }
                     else
                     {
                         iRet = NFrame::ERR_CODE_STORESVR_ERRCODE_SELECTFAILED;
                         NFLogError(NF_LOG_DEFAULT, 0, "SelectObj Failed, iRet:{}", iRet);
                     }
                     select_res.mutable_opres()->set_err_code(iRet);
                 }
                 else
                 {
                     NFLogTrace(NF_LOG_DEFAULT, 0, "SelectObj Success select_res:{}",
                                select_res.Utf8DebugString());
                 }

                 respone.CopyFrom(select_res);

                 FindModule<NFICoroutineModule>()->Resume(coId, 0);
             });

    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "FindModule<NFIAsyMysqlModule>()->SelectObj Failed, iRet:{}", iRet);
        return iRet;
    }

    iRet = FindModule<NFICoroutineModule>()->Yield(DEFINE_RPC_SERVICE_TIME_OUT_MS / 2);

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCStoreServerModule::OnHandleSelectRpc(NFrame::storesvr_sel &request, NFrame::storesvr_sel_res &respone,
                                            const std::function<void()> &cb)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    NF_ASSERT(FindModule<NFICoroutineModule>()->IsInCoroutine());

    NFServerConfig *pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_STORE_SERVER);
    NF_ASSERT(pConfig);

    NFIAsySqlModule* pNFIAsySqlModule = NULL;
    if (m_useCache)
    {
        pNFIAsySqlModule = FindModule<NFIAsyDbModule>();
    }
    else {
        pNFIAsySqlModule = FindModule<NFIAsyMysqlModule>();
    }
    NF_ASSERT(pNFIAsySqlModule);

    bool cache = false;
    auto iter = pConfig->mTBConfMap.find(request.baseinfo().tbname());
    if (iter != pConfig->mTBConfMap.end())
    {
        uint32_t count = iter->second.TableCount;
        if (count > 1)
        {
            uint32_t index = request.cond().mod_key() % count;
            std::string newTableName = request.baseinfo().tbname() + "_" + NFCommon::tostr(index);
            request.mutable_baseinfo()->set_tbname(newTableName);
        }
        cache = iter->second.Cache;
    }

    int64_t coId = FindModule<NFICoroutineModule>()->CurrentTaskId();

    int iRet = pNFIAsySqlModule->SelectByCond
            (request.baseinfo().dbname(), request, cache,
             [this, coId, &respone](int iRet, NFrame::storesvr_sel_res &select_res) mutable
             {
                 if (!FindModule<NFICoroutineModule>()->IsYielding(coId))
                 {
                     NFLogError(NF_LOG_DEFAULT, 0,
                                "SelectByCond, But Coroutine Status Error..........Not Yielding");
                     return;
                 }

                 if (iRet != 0)
                 {
                     iRet = NFrame::ERR_CODE_STORESVR_ERRCODE_BUSY;
                     NFLogError(NF_LOG_DEFAULT, 0, "SelectByCond Failed, iRet:{}", iRet);
                     select_res.mutable_opres()->set_err_code(iRet);
                 }
                 else
                 {
                     NFLogTrace(NF_LOG_DEFAULT, 0, "SelectByCond Success select_res:{}",
                                select_res.Utf8DebugString());
                 }

                 respone.CopyFrom(select_res);

                 FindModule<NFICoroutineModule>()->Resume(coId, 0);
             });

    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "FindModule<NFIAsyMysqlModule>()->SelectByCond Failed, iRet:{}", iRet);
        return iRet;
    }

    do
    {
        iRet = FindModule<NFICoroutineModule>()->Yield(DEFINE_RPC_SERVICE_TIME_OUT_MS / 2);
        if (iRet != 0)
        {
            break;
        }

        if (respone.is_lastbatch())
        {
            break;
        }
        else
        {
            if (cb)
            {
                cb();
            }
        }
    } while (true);
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCStoreServerModule::OnHandleInsertObjRpc(NFrame::storesvr_insertobj &request, NFrame::storesvr_insertobj_res &respone)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    NF_ASSERT(FindModule<NFICoroutineModule>()->IsInCoroutine());

    NFServerConfig *pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_STORE_SERVER);
    NF_ASSERT(pConfig);

    NFIAsySqlModule* pNFIAsySqlModule = NULL;
    if (m_useCache)
    {
        pNFIAsySqlModule = FindModule<NFIAsyDbModule>();
    }
    else {
        pNFIAsySqlModule = FindModule<NFIAsyMysqlModule>();
    }
    NF_ASSERT(pNFIAsySqlModule);

    bool cache = false;
    auto iter = pConfig->mTBConfMap.find(request.baseinfo().tbname());
    if (iter != pConfig->mTBConfMap.end())
    {
        uint32_t count = iter->second.TableCount;
        if (count > 1)
        {
            uint32_t index = request.mod_key() % count;
            std::string newTableName = request.baseinfo().tbname() + "_" + NFCommon::tostr(index);
            request.mutable_baseinfo()->set_tbname(newTableName);
        }

        cache = iter->second.Cache;
    }

    int64_t coId = FindModule<NFICoroutineModule>()->CurrentTaskId();
    int iRet = pNFIAsySqlModule->InsertObj
            (request.baseinfo().dbname(), request, cache,
             [this, coId, &respone](int iRet, NFrame::storesvr_insertobj_res &select_res) mutable
             {
                 if (!FindModule<NFICoroutineModule>()->IsYielding(coId))
                 {
                     NFLogError(NF_LOG_DEFAULT, 0, "SaveObj, But Coroutine Status Error..........Not Yielding");
                     return;
                 }

                 if (iRet != 0)
                 {
                     NFLogError(NF_LOG_DEFAULT, 0, "SelectObj Failed, iRet:{}", GetErrorStr(iRet));
                     iRet = NFrame::ERR_CODE_STORESVR_ERRCODE_INSERTFAILED;
                     select_res.mutable_opres()->set_err_code(iRet);
                 }
                 else
                 {
                     NFLogTrace(NF_LOG_DEFAULT, 0, "SelectObj Success select_res:{}", select_res.Utf8DebugString());
                 }

                 respone.CopyFrom(select_res);

                 FindModule<NFICoroutineModule>()->Resume(coId, 0);
             });

    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "FindModule<NFIAsyMysqlModule>()->SelectObj Failed, iRet:{}", GetErrorStr(iRet));
        return iRet;
    }

    iRet = FindModule<NFICoroutineModule>()->Yield(DEFINE_RPC_SERVICE_TIME_OUT_MS / 2);
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCStoreServerModule::OnHandleModifyObjRpc(NFrame::storesvr_modobj &request, NFrame::storesvr_modobj_res &respone)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    NF_ASSERT(FindModule<NFICoroutineModule>()->IsInCoroutine());

    NFServerConfig *pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_STORE_SERVER);
    NF_ASSERT(pConfig);

    NFIAsySqlModule* pNFIAsySqlModule = NULL;
    if (m_useCache)
    {
        pNFIAsySqlModule = FindModule<NFIAsyDbModule>();
    }
    else {
        pNFIAsySqlModule = FindModule<NFIAsyMysqlModule>();
    }
    NF_ASSERT(pNFIAsySqlModule);

    bool cache = false;
    auto iter = pConfig->mTBConfMap.find(request.baseinfo().tbname());
    if (iter != pConfig->mTBConfMap.end())
    {
        uint32_t count = iter->second.TableCount;
        if (count > 1)
        {
            uint32_t index = request.mod_key() % count;
            std::string newTableName = request.baseinfo().tbname() + "_" + NFCommon::tostr(index);
            request.mutable_baseinfo()->set_tbname(newTableName);
        }

        cache = iter->second.Cache;
    }

    int64_t coId = FindModule<NFICoroutineModule>()->CurrentTaskId();
    int iRet = pNFIAsySqlModule->ModifyObj
            (request.baseinfo().dbname(), request, cache,
             [this, coId, &respone](int iRet, NFrame::storesvr_modobj_res &select_res) mutable
             {
                 if (!FindModule<NFICoroutineModule>()->IsYielding(coId))
                 {
                     NFLogError(NF_LOG_DEFAULT, 0, "ModifyObj, But Coroutine Status Error..........Not Yielding");
                     return;
                 }

                 if (iRet != 0)
                 {
                     NFLogError(NF_LOG_DEFAULT, 0, "ModifyObj Failed, iRet:{}", GetErrorStr(iRet));
                     iRet = NFrame::ERR_CODE_STORESVR_ERRCODE_UPDATEFAILED;
                     select_res.mutable_opres()->set_err_code(iRet);
                 }
                 else
                 {
                     NFLogTrace(NF_LOG_DEFAULT, 0, "ModifyObj Success select_res:{}", select_res.Utf8DebugString());
                 }

                 respone.CopyFrom(select_res);

                 FindModule<NFICoroutineModule>()->Resume(coId, 0);
             });

    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "FindModule<NFIAsyMysqlModule>()->ModifyObj Failed, iRet:{}", GetErrorStr(iRet));
        return iRet;
    }

    iRet = FindModule<NFICoroutineModule>()->Yield(DEFINE_RPC_SERVICE_TIME_OUT_MS / 2);
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCStoreServerModule::OnHandleModifyRpc(NFrame::storesvr_mod &request, NFrame::storesvr_mod_res &respone)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    NF_ASSERT(FindModule<NFICoroutineModule>()->IsInCoroutine());

    NFServerConfig *pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_STORE_SERVER);
    NF_ASSERT(pConfig);

    NFIAsySqlModule* pNFIAsySqlModule = NULL;
    if (m_useCache)
    {
        pNFIAsySqlModule = FindModule<NFIAsyDbModule>();
    }
    else {
        pNFIAsySqlModule = FindModule<NFIAsyMysqlModule>();
    }
    NF_ASSERT(pNFIAsySqlModule);

    bool cache = false;
    auto iter = pConfig->mTBConfMap.find(request.baseinfo().tbname());
    if (iter != pConfig->mTBConfMap.end())
    {
        uint32_t count = iter->second.TableCount;
        if (count > 1)
        {
            uint32_t index = request.cond().mod_key() % count;
            std::string newTableName = request.baseinfo().tbname() + "_" + NFCommon::tostr(index);
            request.mutable_baseinfo()->set_tbname(newTableName);
        }
        cache = iter->second.Cache;
    }

    int64_t coId = FindModule<NFICoroutineModule>()->CurrentTaskId();
    int iRet = pNFIAsySqlModule->ModifyByCond
            (request.baseinfo().dbname(), request, cache,
             [this, coId, &respone](int iRet, NFrame::storesvr_mod_res &select_res) mutable
             {
                 if (!FindModule<NFICoroutineModule>()->IsYielding(coId))
                 {
                     NFLogError(NF_LOG_DEFAULT, 0, "ModifyByCond, But Coroutine Status Error..........Not Yielding");
                     return;
                 }

                 if (iRet != 0)
                 {
                     NFLogError(NF_LOG_DEFAULT, 0, "ModifyByCond Failed, iRet:{}", GetErrorStr(iRet));
                     iRet = NFrame::ERR_CODE_STORESVR_ERRCODE_UPDATEFAILED;
                     select_res.mutable_opres()->set_err_code(iRet);
                 }
                 else
                 {
                     NFLogTrace(NF_LOG_DEFAULT, 0, "ModifyByCond Success select_res:{}", select_res.Utf8DebugString());
                 }

                 respone.CopyFrom(select_res);

                 FindModule<NFICoroutineModule>()->Resume(coId, 0);
             });

    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "FindModule<NFIAsyMysqlModule>()->ModifyByCond Failed, iRet:{}", GetErrorStr(iRet));
        return iRet;
    }

    iRet = FindModule<NFICoroutineModule>()->Yield(DEFINE_RPC_SERVICE_TIME_OUT_MS / 2);
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCStoreServerModule::OnHandleUpdateRpc(NFrame::storesvr_update &request, NFrame::storesvr_update_res &respone)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    NF_ASSERT(FindModule<NFICoroutineModule>()->IsInCoroutine());

    NFServerConfig *pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_STORE_SERVER);
    NF_ASSERT(pConfig);

    NFIAsySqlModule* pNFIAsySqlModule = NULL;
    if (m_useCache)
    {
        pNFIAsySqlModule = FindModule<NFIAsyDbModule>();
    }
    else {
        pNFIAsySqlModule = FindModule<NFIAsyMysqlModule>();
    }
    NF_ASSERT(pNFIAsySqlModule);

    bool cache = false;
    auto iter = pConfig->mTBConfMap.find(request.baseinfo().tbname());
    if (iter != pConfig->mTBConfMap.end())
    {
        uint32_t count = iter->second.TableCount;
        if (count > 1)
        {
            uint32_t index = request.cond().mod_key() % count;
            std::string newTableName = request.baseinfo().tbname() + "_" + NFCommon::tostr(index);
            request.mutable_baseinfo()->set_tbname(newTableName);
        }
        cache = iter->second.Cache;
    }

    int64_t coId = FindModule<NFICoroutineModule>()->CurrentTaskId();
    int iRet = pNFIAsySqlModule->UpdateByCond
            (request.baseinfo().dbname(), request, cache,
             [this, coId, &respone](int iRet, NFrame::storesvr_update_res &select_res) mutable
             {
                 if (!FindModule<NFICoroutineModule>()->IsYielding(coId))
                 {
                     NFLogError(NF_LOG_DEFAULT, 0, "UpdateByCond, But Coroutine Status Error..........Not Yielding");
                     return;
                 }

                 if (iRet != 0)
                 {
                     NFLogError(NF_LOG_DEFAULT, 0, "UpdateByCond Failed, iRet:{}", GetErrorStr(iRet));
                     iRet = NFrame::ERR_CODE_STORESVR_ERRCODE_UPDATEINSERTFAILED;
                     select_res.mutable_opres()->set_err_code(iRet);
                 }
                 else
                 {
                     NFLogTrace(NF_LOG_DEFAULT, 0, "UpdateByCond Success select_res:{}", select_res.Utf8DebugString());
                 }

                 respone.CopyFrom(select_res);

                 FindModule<NFICoroutineModule>()->Resume(coId, 0);
             });

    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "FindModule<NFIAsyMysqlModule>()->UpdateByCond Failed, iRet:{}", GetErrorStr(iRet));
        return iRet;
    }

    iRet = FindModule<NFICoroutineModule>()->Yield(DEFINE_RPC_SERVICE_TIME_OUT_MS / 2);
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCStoreServerModule::OnHandleUpdateObjRpc(NFrame::storesvr_updateobj &request, NFrame::storesvr_updateobj_res &respone)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    NF_ASSERT(FindModule<NFICoroutineModule>()->IsInCoroutine());

    NFServerConfig *pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_STORE_SERVER);
    NF_ASSERT(pConfig);

    NFIAsySqlModule* pNFIAsySqlModule = NULL;
    if (m_useCache)
    {
        pNFIAsySqlModule = FindModule<NFIAsyDbModule>();
    }
    else {
        pNFIAsySqlModule = FindModule<NFIAsyMysqlModule>();
    }
    NF_ASSERT(pNFIAsySqlModule);

    bool cache = false;
    auto iter = pConfig->mTBConfMap.find(request.baseinfo().tbname());
    if (iter != pConfig->mTBConfMap.end())
    {
        uint32_t count = iter->second.TableCount;
        if (count > 1)
        {
            uint32_t index = request.mod_key() % count;
            std::string newTableName = request.baseinfo().tbname() + "_" + NFCommon::tostr(index);
            request.mutable_baseinfo()->set_tbname(newTableName);
        }

        cache = iter->second.Cache;
    }

    int64_t coId = FindModule<NFICoroutineModule>()->CurrentTaskId();
    int iRet = pNFIAsySqlModule->UpdateObj
            (request.baseinfo().dbname(), request, cache,
             [this, coId, &respone](int iRet, NFrame::storesvr_updateobj_res &select_res) mutable
             {
                 if (!FindModule<NFICoroutineModule>()->IsYielding(coId))
                 {
                     NFLogError(NF_LOG_DEFAULT, 0, "UpdateObj, But Coroutine Status Error..........Not Yielding");
                     return;
                 }

                 if (iRet != 0)
                 {
                     NFLogError(NF_LOG_DEFAULT, 0, "UpdateObj Failed, iRet:{}", GetErrorStr(iRet));
                     iRet = NFrame::ERR_CODE_STORESVR_ERRCODE_UPDATEINSERTFAILED;
                     select_res.mutable_opres()->set_err_code(iRet);
                 }
                 else
                 {
                     NFLogTrace(NF_LOG_DEFAULT, 0, "UpdateObj Success select_res:{}", select_res.Utf8DebugString());
                 }

                 respone.CopyFrom(select_res);

                 FindModule<NFICoroutineModule>()->Resume(coId, 0);
             });

    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "FindModule<NFIAsyMysqlModule>()->UpdateObj Failed, iRet:{}", GetErrorStr(iRet));
        return iRet;
    }

    iRet = FindModule<NFICoroutineModule>()->Yield(DEFINE_RPC_SERVICE_TIME_OUT_MS / 2);
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCStoreServerModule::OnHandleExecuteRpc(NFrame::storesvr_execute &request, NFrame::storesvr_execute_res &respone)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    NF_ASSERT(FindModule<NFICoroutineModule>()->IsInCoroutine());

    NFServerConfig *pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_STORE_SERVER);
    NF_ASSERT(pConfig);

    NFIAsySqlModule* pNFIAsySqlModule = NULL;
    if (m_useCache)
    {
        pNFIAsySqlModule = FindModule<NFIAsyDbModule>();
    }
    else {
        pNFIAsySqlModule = FindModule<NFIAsyMysqlModule>();
    }
    NF_ASSERT(pNFIAsySqlModule);

    int64_t coId = FindModule<NFICoroutineModule>()->CurrentTaskId();
    int iRet = pNFIAsySqlModule->Execute
            (request.baseinfo().dbname(), request,
             [this, coId, &respone](int iRet, NFrame::storesvr_execute_res &select_res) mutable
             {
                 if (!FindModule<NFICoroutineModule>()->IsYielding(coId))
                 {
                     NFLogError(NF_LOG_DEFAULT, 0, "Execute, But Coroutine Status Error..........Not Yielding");
                     return;
                 }

                 if (iRet != 0)
                 {
                     NFLogError(NF_LOG_DEFAULT, 0, "Execute Failed, iRet:{}", GetErrorStr(iRet));
                     iRet = NFrame::ERR_CODE_STORESVR_ERRCODE_UNKNOWN;
                     select_res.mutable_opres()->set_err_code(iRet);
                 }
                 else
                 {
                     NFLogTrace(NF_LOG_DEFAULT, 0, "Execute Success select_res:{}", select_res.Utf8DebugString());
                 }

                 respone.CopyFrom(select_res);

                 FindModule<NFICoroutineModule>()->Resume(coId, 0);
             });

    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "FindModule<NFIAsyMysqlModule>()->DeleteByCond Failed, iRet:{}", GetErrorStr(iRet));
        return iRet;
    }

    iRet = FindModule<NFICoroutineModule>()->Yield(DEFINE_RPC_SERVICE_TIME_OUT_MS / 2);
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCStoreServerModule::OnHandleExecuteMoreRpc(NFrame::storesvr_execute_more& request, NFrame::storesvr_execute_more_res& respone, const std::function<void()>& cb)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    NF_ASSERT(FindModule<NFICoroutineModule>()->IsInCoroutine());

    NFServerConfig *pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_STORE_SERVER);
    NF_ASSERT(pConfig);

    NFIAsySqlModule* pNFIAsySqlModule = NULL;
    if (m_useCache)
    {
        pNFIAsySqlModule = FindModule<NFIAsyDbModule>();
    }
    else {
        pNFIAsySqlModule = FindModule<NFIAsyMysqlModule>();
    }
    NF_ASSERT(pNFIAsySqlModule);

    int64_t coId = FindModule<NFICoroutineModule>()->CurrentTaskId();

    int iRet = pNFIAsySqlModule->ExecuteMore
            (request.baseinfo().dbname(), request,
             [this, coId, &respone](int iRet, NFrame::storesvr_execute_more_res &select_res) mutable
             {
                 if (!FindModule<NFICoroutineModule>()->IsYielding(coId))
                 {
                     NFLogError(NF_LOG_DEFAULT, 0,
                                "ExecuteMore, But Coroutine Status Error..........Not Yielding");
                     return;
                 }

                 if (iRet != 0)
                 {
                     iRet = NFrame::ERR_CODE_STORESVR_ERRCODE_BUSY;
                     NFLogError(NF_LOG_DEFAULT, 0, "ExecuteMore Failed, iRet:{}", iRet);
                     select_res.mutable_opres()->set_err_code(iRet);
                 }
                 else
                 {
                     NFLogTrace(NF_LOG_DEFAULT, 0, "ExecuteMore Success select_res:{}",
                                select_res.Utf8DebugString());
                 }
                 respone.CopyFrom(select_res);

                 FindModule<NFICoroutineModule>()->Resume(coId, 0);
             });

    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "FindModule<NFIAsyMysqlModule>()->ExecuteMore Failed, iRet:{}", iRet);
        return iRet;
    }

    do
    {
        iRet = FindModule<NFICoroutineModule>()->Yield(DEFINE_RPC_SERVICE_TIME_OUT_MS / 2);
        if (iRet != 0)
        {
            break;
        }

        if (respone.is_lastbatch())
        {
            break;
        }
        else
        {
            if (cb)
            {
                cb();
            }
        }
    } while (true);
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCStoreServerModule::OnHandleDeleteRpc(NFrame::storesvr_del &request, NFrame::storesvr_del_res &respone)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    NF_ASSERT(FindModule<NFICoroutineModule>()->IsInCoroutine());

    NFServerConfig *pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_STORE_SERVER);
    NF_ASSERT(pConfig);

    NFIAsySqlModule* pNFIAsySqlModule = NULL;
    if (m_useCache)
    {
        pNFIAsySqlModule = FindModule<NFIAsyDbModule>();
    }
    else {
        pNFIAsySqlModule = FindModule<NFIAsyMysqlModule>();
    }
    NF_ASSERT(pNFIAsySqlModule);

    bool cache = false;
    auto iter = pConfig->mTBConfMap.find(request.baseinfo().tbname());
    if (iter != pConfig->mTBConfMap.end())
    {
        uint32_t count = iter->second.TableCount;
        if (count > 1)
        {
            uint32_t index = request.cond().mod_key() % count;
            std::string newTableName = request.baseinfo().tbname() + "_" + NFCommon::tostr(index);
            request.mutable_baseinfo()->set_tbname(newTableName);
        }
        cache = iter->second.Cache;
    }

    int64_t coId = FindModule<NFICoroutineModule>()->CurrentTaskId();
    int iRet = pNFIAsySqlModule->DeleteByCond
            (request.baseinfo().dbname(), request, cache,
             [this, coId, &respone](int iRet, NFrame::storesvr_del_res &select_res) mutable
             {
                 if (!FindModule<NFICoroutineModule>()->IsYielding(coId))
                 {
                     NFLogError(NF_LOG_DEFAULT, 0, "DeleteByCond, But Coroutine Status Error..........Not Yielding");
                     return;
                 }

                 if (iRet != 0)
                 {
                     NFLogError(NF_LOG_DEFAULT, 0, "DeleteByCond Failed, iRet:{}", GetErrorStr(iRet));
                     iRet = NFrame::ERR_CODE_STORESVR_ERRCODE_UPDATEINSERTFAILED;
                     select_res.mutable_opres()->set_err_code(iRet);
                 }
                 else
                 {
                     NFLogTrace(NF_LOG_DEFAULT, 0, "DeleteByCond Success select_res:{}", select_res.Utf8DebugString());
                 }
                 respone.CopyFrom(select_res);

                 FindModule<NFICoroutineModule>()->Resume(coId, 0);
             });

    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "FindModule<NFIAsyMysqlModule>()->DeleteByCond Failed, iRet:{}", GetErrorStr(iRet));
        return iRet;
    }

    iRet = FindModule<NFICoroutineModule>()->Yield(DEFINE_RPC_SERVICE_TIME_OUT_MS / 2);
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}

int NFCStoreServerModule::OnHandleDeleteObjRpc(NFrame::storesvr_delobj &request, NFrame::storesvr_delobj_res &respone)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    NF_ASSERT(FindModule<NFICoroutineModule>()->IsInCoroutine());

    NFServerConfig *pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_STORE_SERVER);
    NF_ASSERT(pConfig);

    NFIAsySqlModule* pNFIAsySqlModule = NULL;
    if (m_useCache)
    {
        pNFIAsySqlModule = FindModule<NFIAsyDbModule>();
    }
    else {
        pNFIAsySqlModule = FindModule<NFIAsyMysqlModule>();
    }
    NF_ASSERT(pNFIAsySqlModule);

    bool cache = false;
    auto iter = pConfig->mTBConfMap.find(request.baseinfo().tbname());
    if (iter != pConfig->mTBConfMap.end())
    {
        uint32_t count = iter->second.TableCount;
        if (count > 1)
        {
            uint32_t index = request.mod_key() % count;
            std::string newTableName = request.baseinfo().tbname() + "_" + NFCommon::tostr(index);
            request.mutable_baseinfo()->set_tbname(newTableName);
        }

        cache = iter->second.Cache;
    }

    int64_t coId = FindModule<NFICoroutineModule>()->CurrentTaskId();
    int iRet = pNFIAsySqlModule->DeleteObj
            (request.baseinfo().dbname(), request, cache,
             [this, coId, &respone](int iRet, NFrame::storesvr_delobj_res &select_res) mutable
             {
                 if (!FindModule<NFICoroutineModule>()->IsYielding(coId))
                 {
                     NFLogError(NF_LOG_DEFAULT, 0, "DeleteObj, But Coroutine Status Error..........Not Yielding");
                     return;
                 }

                 if (iRet != 0)
                 {
                     NFLogError(NF_LOG_DEFAULT, 0, "DeleteObj Failed, iRet:{}", GetErrorStr(iRet));
                     iRet = NFrame::ERR_CODE_STORESVR_ERRCODE_DELETEFAILED;
                     select_res.mutable_opres()->set_err_code(iRet);
                 }
                 else
                 {
                     NFLogTrace(NF_LOG_DEFAULT, 0, "DeleteObj Success select_res:{}", select_res.Utf8DebugString());
                 }
                 respone.CopyFrom(select_res);

                 FindModule<NFICoroutineModule>()->Resume(coId, 0);
             });

    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "FindModule<NFIAsyMysqlModule>()->DeleteObj Failed, iRet:{}", GetErrorStr(iRet));
        return iRet;
    }

    iRet = FindModule<NFICoroutineModule>()->Yield(DEFINE_RPC_SERVICE_TIME_OUT_MS / 2);
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return iRet;
}
