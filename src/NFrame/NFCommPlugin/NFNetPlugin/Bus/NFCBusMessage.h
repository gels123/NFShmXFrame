// -------------------------------------------------------------------------
//    @FileName         :    NFCBusModule.h
//    @Author           :    Yi.Gao
//    @Date             :   2022-09-18
//    @Module           :    NFBusPlugin
//    @Desc             :
// -------------------------------------------------------------------------

#pragma once

#include "NFIBusConnection.h"
#include "../NFINetMessage.h"
#include "NFComm/NFCore/NFCommMapEx.hpp"
#include "NFComm/NFPluginModule/NFNetDefine.h"

class NFCBusServer;
class NFCBusClient;

class NFCBusMessage final : public NFINetMessage
{
public:
	explicit NFCBusMessage(NFIPluginManager* p, NF_SERVER_TYPE serverType);

	~NFCBusMessage() override;

	bool ReadyExecute() override;

	bool Execute() override;

	bool Shut() override;

	bool Finalize() override;

public:
	/**
	* @brief	初始化
	*
	* @return 是否成功
	*/
	uint64_t BindServer(const NFMessageFlag& flag) override;

	/**
	* @brief	初始化
	*
	* @return 是否成功
	*/
	uint64_t ConnectServer(const NFMessageFlag& flag) override;

	/**
	 * @brief	发送数据 不包含数据头
	 *
	 * @param usLinkId
	 * @param packet
	 * @param msg		发送的数据,
	 * @param nLen	数据的大小
	 * @return
	 */
	bool Send(uint64_t usLinkId, NFDataPackage& packet, const char* msg, uint32_t nLen) override;
	bool Send(uint64_t usLinkId, NFDataPackage& packet, const google::protobuf::Message& xData) override;

	/**
	 * @brief 获得连接IP
	 *
	 * @param  usLinkId
	 * @return std::string
	 */
	std::string GetLinkIp(uint64_t usLinkId) override;
	uint32_t GetPort(uint64_t usLinkId) override;

	/**
	* @brief 关闭连接
	*
	* @param  usLinkId
	* @return
	*/
	void CloseLinkId(uint64_t usLinkId) override;

	void OnHandleMsgPeer(eMsgType type, uint64_t serverLinkId, uint64_t objectLinkId, NFDataPackage& package);

	int ResumeConnect() override;

	int OnTimer(uint32_t nTimerId) override;

	void SendHeartMsg();

	void CheckServerHeartBeat();

private:
	NFCommMapEx<uint64_t, NFIBusConnection> m_busConnectMap;
    NF_SHARE_PTR<NFIBusConnection> m_bindConnect;
};
