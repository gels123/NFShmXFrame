// -------------------------------------------------------------------------
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
#include "NFServerComm/NFServerCommon/NFIStoreServerModule.h"

class NFCStoreServerModule : public NFIStoreServerModule
{
public:
	explicit NFCStoreServerModule(NFIPluginManager* p);
	virtual ~NFCStoreServerModule();

	virtual bool Awake() override;

	virtual bool Init() override;

	virtual bool Execute() override;

	virtual bool OnDynamicPlugin() override;

    virtual bool OnReloadConfig() override;

    virtual int OnTimer(uint32_t nTimerID) override;

    /**
     * @brief 处理来自服务器的信息
     * @param unLinkId
     * @param packet
     * @return
     */
    virtual int OnHandleServerMessage(uint64_t unLinkId, NFDataPackage& packet) override;

    /**
     * @brief 动态加载protobuf信息，并检查数据库，可能的话建立数据库，表格，列
     * @return
     */
    virtual bool LoadPbAndCheckDB();
public:
    /**
     * @brief 处理数据库请求
     * @param unLinkId
     * @param packet
     * @return
     */
	int OnHandleStoreReq(uint64_t unLinkId, NFDataPackage& packet);
public:
    /**
     * @brief Select Obj Rpc Service
     * @param request
     * @param respone
     * @return
     */
    int OnHandleSelectObjRpc(NFrame::storesvr_selobj& request, NFrame::storesvr_selobj_res& respone);

    /**
     * @brief Select Rpc Service
     * @param request
     * @param respone
     * @return
     */
    int OnHandleSelectRpc(NFrame::storesvr_sel& request, NFrame::storesvr_sel_res& respone, const std::function<void()>& cb);

    /**
     * @brief
     * @param request
     * @param respone
     * @return
     */
    int OnHandleInsertObjRpc(NFrame::storesvr_insertobj& request, NFrame::storesvr_insertobj_res& respone);

    /**
     * @brief
     * @param request
     * @param respone
     * @return
     */
    int OnHandleModifyObjRpc(NFrame::storesvr_modobj& request, NFrame::storesvr_modobj_res& respone);

    /**
     * @brief
     * @param request
     * @param respone
     * @return
     */
    int OnHandleModifyRpc(NFrame::storesvr_mod& request, NFrame::storesvr_mod_res& respone);

    /**
     * @brief
     * @param request
     * @param respone
     * @return
     */
    int OnHandleUpdateRpc(NFrame::storesvr_update& request, NFrame::storesvr_update_res& respone);

    /**
     * @brief
     * @param request
     * @param respone
     * @return
     */
    int OnHandleUpdateObjRpc(NFrame::storesvr_updateobj& request, NFrame::storesvr_updateobj_res& respone);

    /**
     * @brief
     * @param request
     * @param respone
     * @return
     */
    int OnHandleExecuteRpc(NFrame::storesvr_execute& request, NFrame::storesvr_execute_res& respone);

    /**
     * @brief Execute More Rpc Service
     * @param request
     * @param respone
     * @return
     */
    int OnHandleExecuteMoreRpc(NFrame::storesvr_execute_more& request, NFrame::storesvr_execute_more_res& respone, const std::function<void()>& cb);

    /**
     * @brief
     * @param request
     * @param respone
     * @return
     */
    int OnHandleDeleteRpc(NFrame::storesvr_del& request, NFrame::storesvr_del_res& respone);

    /**
     * @brief
     * @param request
     * @param respone
     * @return
     */
    int OnHandleDeleteObjRpc(NFrame::storesvr_delobj& request, NFrame::storesvr_delobj_res& respone);

public:
    bool m_useCache;
};
