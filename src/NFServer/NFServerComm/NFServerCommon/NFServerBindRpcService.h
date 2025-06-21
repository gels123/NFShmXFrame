// -------------------------------------------------------------------------
//    @FileName         :    NFServerBindRpcService.h
//    @Author           :    gaoyi
//    @Date             :    23-3-25
//    @Email			:    445267987@qq.com
//    @Module           :    NFServerBindRpcService
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFPluginModule/NFIRpcService.h"
#include "NFServerComm/NFServerMessage/ServerCommon.pb.h"
#include "ServerMsg.pb.h"

DEFINE_BIND_RPC_SERVICE(NFServer::NF_RPC_SERVICE_GET_SERVER_INFO_REQ, NFServer::RpcRequestGetServerInfo, NFrame::ServerInfoReport)

/**
 * @brief 服务器与服务器直接的注册返回RPC
 */
DEFINE_BIND_RPC_SERVICE(NFrame::NF_SERVER_TO_SERVER_REGISTER, NFrame::ServerInfoReportList, NFrame::ServerInfoReportListRespne)

/**
 * @brief StoreServer Rpc Service
 */
DEFINE_BIND_RPC_SERVICE(NFrame::NF_STORESVR_C2S_SELECTOBJ, NFrame::storesvr_selobj, NFrame::storesvr_selobj_res)
DEFINE_BIND_RPC_SERVICE(NFrame::NF_STORESVR_C2S_SELECT, NFrame::storesvr_sel, NFrame::storesvr_sel_res)
DEFINE_BIND_RPC_SERVICE(NFrame::NF_STORESVR_C2S_INSERTOBJ, NFrame::storesvr_insertobj, NFrame::storesvr_insertobj_res)
DEFINE_BIND_RPC_SERVICE(NFrame::NF_STORESVR_C2S_MODIFYOBJ, NFrame::storesvr_modobj, NFrame::storesvr_modobj_res)
DEFINE_BIND_RPC_SERVICE(NFrame::NF_STORESVR_C2S_MODIFY, NFrame::storesvr_mod, NFrame::storesvr_mod_res)
DEFINE_BIND_RPC_SERVICE(NFrame::NF_STORESVR_C2S_UPDATE, NFrame::storesvr_update, NFrame::storesvr_update_res)
DEFINE_BIND_RPC_SERVICE(NFrame::NF_STORESVR_C2S_UPDATEOBJ, NFrame::storesvr_updateobj, NFrame::storesvr_updateobj_res)
DEFINE_BIND_RPC_SERVICE(NFrame::NF_STORESVR_C2S_EXECUTE, NFrame::storesvr_execute, NFrame::storesvr_execute_res)
DEFINE_BIND_RPC_SERVICE(NFrame::NF_STORESVR_C2S_EXECUTE_MORE, NFrame::storesvr_execute_more, NFrame::storesvr_execute_more_res);
DEFINE_BIND_RPC_SERVICE(NFrame::NF_STORESVR_C2S_DELETE, NFrame::storesvr_del, NFrame::storesvr_del_res)
DEFINE_BIND_RPC_SERVICE(NFrame::NF_STORESVR_C2S_DELETEOBJ, NFrame::storesvr_delobj, NFrame::storesvr_delobj_res)
