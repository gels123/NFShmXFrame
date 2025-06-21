// -------------------------------------------------------------------------
//    @FileName         :    NFCNetServerModule.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Module           :    NFCNetServerModule
//
//
//                    .::::.
//                  .::::::::.
//                 :::::::::::  FUCK YOU
//             ..:::::::::::'
//           '::::::::::::'
//             .::::::::::
//        '::::::::::::::..
//             ..::::::::::::.
//           ``::::::::::::::::
//            ::::``:::::::::'        .:::.
//           ::::'   ':::::'       .::::::::.
//         .::::'      ::::     .:::::::'::::.
//        .:::'       :::::  .:::::::::' ':::::.
//       .::'        :::::.:::::::::'      ':::::.
//      .::'         ::::::::::::::'         ``::::.
//  ...:::           ::::::::::::'              ``::.
// ```` ':.          ':::::::::'                  ::::..
//                    '.:::::'                    ':'````..
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFPluginModule/NFINetModule.h"
#include "NFComm/NFPluginModule/NFIPluginManager.h"
#include "NFINetMessage.h"
#include <NFComm/NFPluginModule/NFIPacketParse.h>

class NFCNetModule final : public NFINetModule
{
public:
	/**
	 * @brief 构造函数
	 */
	explicit NFCNetModule(NFIPluginManager* p);

	/**
	 * @brief 析构函数
	 */
	~NFCNetModule() override;

	/**
	 *@brief  设置接收回调.
	 */
	void SetRecvCB(const NET_CALLBACK_RECEIVE_FUNCTOR& recvCb) override
	{
		m_recvCb = recvCb;
	}

	/**
	 *@brief  设置连接事件回调.
	 */
	void SetEventCB(const NET_CALLBACK_EVENT_FUNCTOR& eventCb) override
	{
		m_eventCb = eventCb;
	}

	/**
	*@brief  设置接收回调.
	*/
	void SetHttpRecvCB(const HTTP_RECEIVE_FUNCTOR& recvCb) override
	{
		m_httpReceiveCb = recvCb;
	}

	/**
	 *@brief  设置连接事件回调.
	 */
	void SetHttpFilterCB(const HTTP_FILTER_FUNCTOR& eventCb) override
	{
		m_httpFilter = eventCb;
	}

	/**
	* @brief
	*
	* @return bool
	*/
	bool Awake() override;

	/**
	 * @brief
	 *
	 * @return bool
	 */
	bool BeforeShut() override;

	/**
	 * @brief
	 *
	 * @return bool
	 */
	bool Shut() override;

	/**
	 * @brief 释放数据
	 *
	 * @return bool
	 */
	bool Finalize() override;

	bool ReadyExecute() override;

	/**
	 * @brief
	 *
	 * @return bool
	 */
	bool Execute() override;

	/**
	 * 绑定服务器函数
	 *
	 * 本函数用于在特定类型的服务器上进行绑定操作它允许指定服务器类型、URL、网络线程数量、最大连接数、数据包解析类型和安全性选项
	 *
	 * @param serverType 服务器类型，表示要绑定的服务器的类型
	 * @param url 服务器的URL地址，用于网络连接
	 * @param netThreadNum 网络线程数量，默认为1，用于处理网络消息的线程数
	 * @param maxConnectNum 最大连接数，默认为100，表示服务器能同时处理的最大连接数
	 * @param packetParseType 数据包解析类型，默认为内部解析类型，指示如何解析接收到的数据包
	 * @param security 是否启用安全性，默认为否，表示是否需要在通信中启用安全措施
	 *
	 * @return 返回一个uint64_t类型的值，通常用于表示绑定操作的结果或生成的唯一标识符
	 */
	uint64_t BindServer(NF_SERVER_TYPE serverType, const std::string& url, uint32_t netThreadNum = 1, uint32_t maxConnectNum = 100, uint32_t packetParseType = PACKET_PARSE_TYPE_INTERNAL, bool security = false) override;

	/**
	 * 重置并初始化解析包。
	 *
	 * 本函数旨在根据指定的解析类型和解析包对象，进行重置和初始化操作，以确保数据包的解析过程正确进行。
	 *
	 * @param parseType 解析类型，一个无符号32位整数，用于指定解析的类型或模式。
	 * @param pPacketParse 指向NFIPacketParse对象的指针，表示要进行重置和初始化的数据包对象。
	 * @return 返回一个整数值，表示操作的结果，具体含义取决于实现。
	 */
	int ResetPacketParse(uint32_t parseType, NFIPacketParse* pPacketParse) override;

	/**
	 * 连接服务器函数
	 *
	 * @param serverType 服务器类型，表示要连接的服务器的种类
	 * @param url 服务器的URL地址，格式如"ip:port"，用于建立网络连接
	 * @param packetParseType 数据包解析类型，默认为0，用于指定数据包的解析方式
	 * @param security 是否使用安全连接，默认为false，表示不使用安全连接
	 *
	 * @return 返回uint64_t类型的连接ID，用于后续的操作和管理
	 *
	 * 此函数用于建立与特定类型服务器的连接，可以根据需要选择不同的数据包解析方式和是否使用安全连接
	 * 它是异步操作，通过回调函数处理连接结果
	 */
	uint64_t ConnectServer(NF_SERVER_TYPE serverType, const std::string& url, uint32_t packetParseType = 0, bool security = false) override;

	int ResumeConnect(NF_SERVER_TYPE serverType) override;

	/**
	 * @brief 获得服务器
	 *
	 * @param  serverType
	 * @return NFIServer
	 */
	NFINetMessage* GetServerByServerType(NF_SERVER_TYPE serverType) const;

	/**
	 * @brief 获得连接的IP
	 *
	 * @param  linkId
	 * @return std::string
	 */
	std::string GetLinkIp(uint64_t linkId) override;
	uint32_t GetPort(uint64_t linkId) override;

	/**
	* @brief 关闭连接
	*
	* @param linkId
	* @return
	*/
	void CloseLinkId(uint64_t linkId) override;

	void Send(uint64_t linkId, uint32_t moduleId, uint32_t msgId, const std::string& strData, uint64_t param1, uint64_t param2 = 0, uint64_t srcId = 0, uint64_t dstId = 0) override;

	void Send(uint64_t linkId, uint32_t moduleId, uint32_t msgId, const char* msg, uint32_t len, uint64_t param1, uint64_t param2 = 0, uint64_t srcId = 0, uint64_t dstId = 0) override;

	void Send(uint64_t linkId, uint32_t moduleId, uint32_t msgId, const google::protobuf::Message& data, uint64_t param1, uint64_t param2 = 0, uint64_t srcId = 0, uint64_t dstId = 0) override;

	void SendServer(uint64_t linkId, uint32_t moduleId, uint32_t msgId, const std::string& strData, uint64_t param1, uint64_t param2 = 0, uint64_t srcId = 0, uint64_t dstId = 0) override;

	void SendServer(uint64_t linkId, uint32_t moduleId, uint32_t msgId, const char* msg, uint32_t len, uint64_t param1, uint64_t param2 = 0, uint64_t srcId = 0, uint64_t dstId = 0) override;

	void SendServer(uint64_t linkId, uint32_t moduleId, uint32_t msgId, const google::protobuf::Message& data, uint64_t param1, uint64_t param2 = 0, uint64_t srcId = 0, uint64_t dstId = 0) override;

	void TransPackage(uint64_t linkId, NFDataPackage& packet) override;

	bool Send(uint64_t linkId, NFDataPackage& packet, const char* msg, uint32_t len);
	bool Send(uint64_t linkId, NFDataPackage& packet, const google::protobuf::Message& data);

	bool ResponseHttpMsg(NF_SERVER_TYPE serverType, const NFIHttpHandle& req, const std::string& strMsg, NFWebStatus code = WEB_OK, const std::string& reason = "OK") override;

	bool ResponseHttpMsg(NF_SERVER_TYPE serverType, uint64_t requestId, const std::string& strMsg, NFWebStatus code = WEB_OK, const std::string& reason = "OK") override;

	int HttpGet(NF_SERVER_TYPE serverType, const std::string& strUri, const HTTP_CLIENT_RESPONE& respone, const std::map<std::string, std::string>& xHeaders = std::map<std::string, std::string>(), int timeout = 3) override;

	int HttpPost(NF_SERVER_TYPE serverType, const std::string& strUri, const std::string& strPostData, const HTTP_CLIENT_RESPONE& respone, const std::map<std::string, std::string>& xHeaders = std::map<std::string, std::string>(), int timeout = 3) override;

	int SendEmail(NF_SERVER_TYPE serverType, const std::string& title, const std::string& subject, const string& content) override;

protected:
	/**
	 * @brief 将消息编码后通过pServer发送出去
	 *
	 */
	bool Send(NFINetMessage* pServer, uint64_t linkId, NFDataPackage& packet, const char* msg, uint32_t len);
	bool Send(NFINetMessage* pServer, uint64_t linkId, NFDataPackage& packet, const google::protobuf::Message& data);

private:
	/**
	 * @brief	处理接受数据的回调
	 */
	NET_CALLBACK_RECEIVE_FUNCTOR m_recvCb;

	/**
	 * @brief	网络事件回调
	 */
	NET_CALLBACK_EVENT_FUNCTOR m_eventCb;

	/**
	* @brief	HTTP处理接受数据的回调
	*/
	HTTP_RECEIVE_FUNCTOR m_httpReceiveCb;
	/**
	* @brief	HTTP处理接受数据的回调
	*/
	HTTP_FILTER_FUNCTOR m_httpFilter;

	std::vector<NFINetMessage*> m_evppServerArray;
	std::vector<NFINetMessage*> m_enetServerArray;
	std::vector<NFINetMessage*> m_busServerArray;
};
