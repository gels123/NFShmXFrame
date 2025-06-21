// -------------------------------------------------------------------------
//    @FileName         :    NFCBusServer.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFCBusServer.h
//
// -------------------------------------------------------------------------

#pragma once

#include "NFIBusConnection.h"
#include "../NFINetMessage.h"
#include "NFComm/NFPluginModule/NFNetDefine.h"

class NFCBusServer final : public NFIBusConnection
{
public:
    explicit NFCBusServer(NFIPluginManager* p, NF_SERVER_TYPE serverType, const NFMessageFlag& flag);

    ~NFCBusServer() override;

    bool Execute() override;

    bool Init() override;

    bool Shut() override;

    bool Finalize() override;

public:
    /**
    * @brief	初始化
    *
    * @return 是否成功
    */
    uint64_t BindServer(const NFMessageFlag& flag);

    /**
     * @brief 主线程处理消息队列
     */
    void ProcessMsgLogicThread();

    bool Send(NFDataPackage& packet, const char* msg, uint32_t nLen) override;
    bool Send(NFDataPackage& packet, const google::protobuf::Message& xData) override;
private:
    /**
     * @brief 服务器每一帧处理的消息数
     */
    uint32_t m_handleMsgNumPerFrame;
};
