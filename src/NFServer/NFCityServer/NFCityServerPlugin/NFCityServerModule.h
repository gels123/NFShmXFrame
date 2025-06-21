// -------------------------------------------------------------------------
//    @FileName         :    NFCityServerModule.h
//    @Author           :    gaoyi
//    @Date             :    2024/12/18
//    @Email            :    445267987@qq.com
//    @Module           :    NFCityServerModule
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFPluginModule/NFServerDefine.h"
#include "NFServerComm/NFServerCommon/NFICityServerModule.h"

class NFCityServerModule : public NFICityServerModule
{
public:
    explicit NFCityServerModule(NFIPluginManager *p);

    virtual ~NFCityServerModule();

    virtual bool Awake() override;

    virtual bool Init() override;

    virtual bool Execute() override;

    virtual bool OnDynamicPlugin() override;

    /**
     * @brief 处理来自服务器的信息
     * @param unLinkId
     * @param packet
     * @return
     */
    virtual int OnHandleServerMessage(uint64_t unLinkId, NFDataPackage& packet) override;
};