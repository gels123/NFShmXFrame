// -------------------------------------------------------------------------
//    @FileName         :    NFCBusClient.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFCBusClient.h
//
// -------------------------------------------------------------------------

#pragma once

#include <map>
#include "NFBusShm.h"
#include "NFIBusConnection.h"
#include "../NFINetMessage.h"
#include "NFComm/NFCore/NFBuffer.h"
#include "NFComm/NFCore/NFSpinLock.h"
#include "NFComm/NFPluginModule/NFIBusModule.h"
#include "NFComm/NFPluginModule/NFNetDefine.h"

class NFCBusClient final : public NFIBusConnection
{
public:
    explicit NFCBusClient(NFIPluginManager* p, NF_SERVER_TYPE serverType, const NFMessageFlag& flag, const NFMessageFlag& bindFlag):NFIBusConnection(p, serverType, flag)
    {
        m_bindFlag = bindFlag;
        m_sendBuffer.AssureSpace(MAX_SEND_BUFFER_SIZE);
        m_isConnected = false;
    }

    ~NFCBusClient() override;

    bool Execute() override;

    bool Init() override;

    bool Shut() override;

    bool Finalize() override;

    bool IsConnected() override;

    void SetConnected(bool connected) override;

public:
    /**
    * @brief	初始化
    *
    * @return 是否成功
    */
    uint64_t ConnectServer(const NFMessageFlag& flag, const NFMessageFlag& bindFlag);

    /**
     * @brief	发送数据 不包含数据头
     *
     * @param packet
     * @param msg		发送的数据,
     * @param nLen	数据的大小
     * @return
     */
    bool Send(NFDataPackage& packet, const char* msg, uint32_t nLen) override;
    bool Send(NFDataPackage& packet, const google::protobuf::Message& xData) override;

    bool Send(NFShmChannel* pChannel, int packetParseType, const NFDataPackage& packet, const char* msg, uint32_t nLen);
private:
    NFBuffer m_sendBuffer;
    bool m_isConnected;
};
