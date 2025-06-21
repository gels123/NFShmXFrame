// -------------------------------------------------------------------------
//    @FileName         :    NFCMemOtherModule.h
//    @Author           :    gaoyi
//    @Date             :    23-9-8
//    @Email			:    445267987@qq.com
//    @Module           :    NFCMemOtherModule
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFPluginModule/NFIDynamicModule.h"

class NFCMemOtherModule final : public NFIDynamicModule
{
public:
    explicit NFCMemOtherModule(NFIPluginManager* p);

    ~NFCMemOtherModule() override;

public:
    bool Awake() override;

    int OnExecute(uint32_t serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const google::protobuf::Message* pMessage) override;

    bool CheckStopServer() override;

    bool StopServer() override;
};
