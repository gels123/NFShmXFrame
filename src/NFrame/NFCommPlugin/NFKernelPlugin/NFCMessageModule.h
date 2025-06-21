﻿// -------------------------------------------------------------------------
//    @FileName         :    NFCMsgModule.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFPluginModule/NFIMessageModule.h"
#include "NFComm/NFPluginModule/NFINetModule.h"
#include "NFComm/NFPluginModule/NFIHttpHandle.h"
#include "NFComm/NFCore/NFCommMapEx.hpp"
#include "NFServerLinkData.h"
#include <stdint.h>
#include <unordered_set>

class NFCMessageModule : public NFIMessageModule
{
public:
	explicit NFCMessageModule(NFIPluginManager* p);

	virtual ~NFCMessageModule();

	virtual bool Awake() override;

	virtual bool Finalize() override;

	virtual bool Execute() override;

	virtual bool OnReloadConfig() override;

	/**
	 * @brief 添加服务器
	 *
	 * @param  eType		服务器类型
	 * @param  nServerID	服务器ID
	 * @param  nMaxClient	服务器最大连接客户端数
	 * @param  nPort		服务器监听端口
	 * @return int			返回0错误
	 */
	virtual uint64_t BindServer(NF_SERVER_TYPE eServerType, const std::string& url, uint32_t nNetThreadNum = 1, uint32_t nMaxConnectNum = 100,
								uint32_t nPacketParseType = PACKET_PARSE_TYPE_INTERNAL, bool bSecurity = false) override;

	/**
	 * 重置并初始化解析包。
	 *
	 * 本函数旨在根据指定的解析类型和解析包对象，进行重置和初始化操作，以确保数据包的解析过程正确进行。
	 *
	 * @param parseType 解析类型，一个无符号32位整数，用于指定解析的类型或模式。
	 * @param pPacketParse 指向NFIPacketParse对象的指针，表示要进行重置和初始化的数据包对象。
	 * @return 返回一个整数值，表示操作的结果，具体含义取决于实现。
	 */
	virtual int ResetPacketParse(uint32_t parseType, NFIPacketParse* pPacketParse) override;

	/**
	 * @brief 添加服务器
	 *
	 * @param  eType		服务器类型
	 * @param  nServerID	服务器ID
	 * @param  nMaxClient	服务器最大连接客户端数
	 * @param  nPort		服务器监听端口
	 * @return int			返回0错误
	 */
	virtual uint64_t ConnectServer(NF_SERVER_TYPE eServerType, const std::string& url, uint32_t nPacketParseType = 0, bool bSecurity = false) override;

	virtual int ResumeConnect(NF_SERVER_TYPE eServerType) override;

	virtual std::string GetLinkIp(uint64_t usLinkId) override;

	virtual uint32_t GetPort(uint64_t usLinkId) override;

	virtual void CloseLinkId(uint64_t usLinkId) override;

	virtual void CloseServer(NF_SERVER_TYPE eServerType, NF_SERVER_TYPE destServer, uint32_t busId, uint64_t usLinkId) override;

	virtual void TransPackage(uint64_t usLinkId, NFDataPackage& packet) override;

	virtual void OnHandleMessage(NFDataPackage& packet) override;

	virtual void Send(uint64_t usLinkId, uint32_t nModuleId, uint32_t nMsgID, const std::string& strData, uint64_t param1 = 0, uint64_t param2 = 0, uint64_t srcId = 0, uint64_t dstId = 0) override;

	virtual void Send(uint64_t usLinkId, uint32_t nModuleId, uint32_t nMsgID, const char* msg, uint32_t nLen, uint64_t param1 = 0, uint64_t param2 = 0, uint64_t srcId = 0, uint64_t dstId = 0) override;

	virtual void Send(uint64_t usLinkId, uint32_t nModuleId, uint32_t nMsgID, const google::protobuf::Message& xData, uint64_t param1 = 0, uint64_t param2 = 0, uint64_t srcId = 0, uint64_t dstId = 0) override;

	virtual void SendServer(uint64_t usLinkId, uint32_t nModuleId, uint32_t nMsgID, const std::string& strData, uint64_t param1 = 0, uint64_t param2 = 0, uint64_t srcId = 0, uint64_t dstId = 0) override;

	virtual void SendServer(uint64_t usLinkId, uint32_t nModuleId, uint32_t nMsgID, const char* msg, uint32_t nLen, uint64_t param1 = 0, uint64_t param2 = 0, uint64_t srcId = 0, uint64_t dstId = 0) override;

	virtual void SendServer(uint64_t usLinkId, uint32_t nModuleId, uint32_t nMsgID, const google::protobuf::Message& xData, uint64_t param1 = 0, uint64_t param2 = 0, uint64_t srcId = 0, uint64_t dstId = 0) override;

	virtual int SendMsgToServer(NF_SERVER_TYPE eSendType, NF_SERVER_TYPE recvType, uint32_t srcBusId, uint32_t dstBusId, uint32_t nModuleId, uint32_t nMsgId, const google::protobuf::Message& xData, uint64_t param1 = 0, uint64_t param2 = 0) override;

	virtual int SendMsgToServer(NF_SERVER_TYPE eSendType, NF_SERVER_TYPE recvType, uint32_t srcBusId, uint32_t dstBusId, uint32_t nModuleId, uint32_t nMsgId, const std::string& xData, uint64_t param1 = 0, uint64_t param2 = 0) override;

	virtual int SendTrans(NF_SERVER_TYPE eSendType, NF_SERVER_TYPE recvType, uint32_t srcBusId, uint32_t dstBusId, uint32_t nMsgID, const google::protobuf::Message& xData, uint32_t req_trans_id = 0, uint32_t rsp_trans_id = 0) override;

	virtual int SendTrans(NF_SERVER_TYPE eSendType, NF_SERVER_TYPE recvType, uint32_t srcBusId, uint32_t dstBusId, uint32_t nMsgID, const std::string& xData, uint32_t req_trans_id = 0, uint32_t rsp_trans_id = 0) override;

	virtual NF_SHARE_PTR<NFServerData> GetServerByServerId(NF_SERVER_TYPE eSendType, uint32_t busId) override;

	virtual NF_SHARE_PTR<NFServerData> GetServerByUnlinkId(NF_SERVER_TYPE eSendType, uint64_t unlinkId) override;

	virtual NF_SHARE_PTR<NFServerData> CreateServerByServerId(NF_SERVER_TYPE eSendType, uint32_t busId, NF_SERVER_TYPE busServerType, const NFrame::ServerInfoReport& data) override;

	virtual void CreateLinkToServer(NF_SERVER_TYPE eSendType, uint32_t busId,
									uint64_t linkId) override;

	virtual void DelServerLink(NF_SERVER_TYPE eSendType, uint64_t linkId) override;

	virtual NFServerData* GetRouteData(NF_SERVER_TYPE eSendType) override;

	virtual const NFServerData* GetRouteData(NF_SERVER_TYPE eSendType) const override;

	virtual NFServerData* GetMasterData(NF_SERVER_TYPE eSendType) override;

	virtual const NFServerData* GetMasterData(NF_SERVER_TYPE eSendType) const override;

	virtual void CloseAllLink(NF_SERVER_TYPE eSendType) override;

	virtual uint64_t GetServerLinkId(NF_SERVER_TYPE eSendType) const override;

	virtual void SetServerLinkId(NF_SERVER_TYPE eSendType, uint64_t linkId) override;

	virtual uint64_t GetClientLinkId(NF_SERVER_TYPE eSendType) const override;

	virtual void SetClientLinkId(NF_SERVER_TYPE eSendType, uint64_t linkId) override;

	virtual std::vector<NF_SHARE_PTR<NFServerData>> GetServerByServerType(NF_SERVER_TYPE eSendType, NF_SERVER_TYPE serverTypes) override;

	virtual NF_SHARE_PTR<NFServerData> GetFirstServerByServerType(NF_SERVER_TYPE eSendType, NF_SERVER_TYPE serverTypes) override;

	virtual NF_SHARE_PTR<NFServerData> GetFirstServerByServerType(NF_SERVER_TYPE eSendType, NF_SERVER_TYPE serverTypes, bool crossServer) override;

	virtual NF_SHARE_PTR<NFServerData> GetRandomServerByServerType(NF_SERVER_TYPE eSendType, NF_SERVER_TYPE serverTypes) override;

	virtual NF_SHARE_PTR<NFServerData> GetRandomServerByServerType(NF_SERVER_TYPE eSendType, NF_SERVER_TYPE serverTypes, bool crossServer) override;

	virtual NF_SHARE_PTR<NFServerData> GetSuitServerByServerType(NF_SERVER_TYPE eSendType, NF_SERVER_TYPE serverTypes, uint64_t value) override;

	virtual NF_SHARE_PTR<NFServerData> GetSuitServerByServerType(NF_SERVER_TYPE eSendType, NF_SERVER_TYPE serverTypes, uint64_t valu, bool crossServere) override;

	virtual NF_SHARE_PTR<NFServerData> GetSuitServerByServerType(NF_SERVER_TYPE eSendType, NF_SERVER_TYPE serverTypes, const std::string& value) override;

	virtual NF_SHARE_PTR<NFServerData> GetSuitServerByServerType(NF_SERVER_TYPE eSendType, NF_SERVER_TYPE serverTypes, const std::string& value, bool crossServer) override;

	virtual std::vector<NF_SHARE_PTR<NFServerData>> GetAllServer(NF_SERVER_TYPE eSendType) override;

	virtual std::vector<NF_SHARE_PTR<NFServerData>> GetAllServer(NF_SERVER_TYPE eSendType, NF_SERVER_TYPE serverTypes) override;

	virtual std::vector<NF_SHARE_PTR<NFServerData>> GetAllServer(NF_SERVER_TYPE eSendType, NF_SERVER_TYPE serverTypes, bool isCrossServer = false) override;

	virtual std::vector<std::string> GetDBNames(NF_SERVER_TYPE eSendType) override;

	virtual std::set<uint32_t> GetAllMsg(NF_SERVER_TYPE eSendType, uint32_t moduleId) override;

	virtual NF_SHARE_PTR<NFServerData> GetFirstDbServer(NF_SERVER_TYPE eSendType, const std::string& dbName) override;
	virtual NF_SHARE_PTR<NFServerData> GeRandomDbServer(NF_SERVER_TYPE eSendType, const std::string& dbName) override;
	virtual NF_SHARE_PTR<NFServerData> GetSuitDbServer(NF_SERVER_TYPE eSendType, const std::string& dbName, uint64_t value) override;
	virtual NF_SHARE_PTR<NFServerData> GetSuitDbServer(NF_SERVER_TYPE eSendType, const std::string& dbName, const std::string& value) override;

	virtual int BroadcastEventToServer(NF_SERVER_TYPE eType, NF_SERVER_TYPE recvType, uint32_t dstBusId, uint32_t nEventID, uint32_t bySrcType, uint64_t nSrcID, const google::protobuf::Message& message) override;
	virtual int BroadcastEventToServer(NF_SERVER_TYPE eType, NF_SERVER_TYPE recvType, uint32_t nEventID, uint32_t bySrcType, uint64_t nSrcID, const google::protobuf::Message& message) override;
	virtual int BroadcastEventToAllServer(NF_SERVER_TYPE eType, uint32_t nEventID, uint32_t bySrcType, uint64_t nSrcID, const google::protobuf::Message& message) override;
	virtual int BroadcastEventToAllServer(NF_SERVER_TYPE eType, uint32_t busId, uint32_t nEventID, uint32_t bySrcType, uint64_t nSrcID, const google::protobuf::Message &message) override;
public:
	/// @brief 设置通信驱动(通信库)，运行时只支持一种通信驱动，如rawudp，tbuspp或第3方网络库
	/// @param driver 对MessageDriver接口实现的网络库
	/// @return 0 表示成功
	/// @return -1 表示失败
	void SetNetModule(NFINetModule* driver);
public:
	/*
	 * 删除目标的所有注册的回调
	 * */
	virtual bool DelAllCallBack(NFIDynamicModule* pTarget) override;

	/*
	 * 删除一个连接的所有回调
	 * */
	virtual bool DelAllCallBack(NF_SERVER_TYPE eType, uint64_t unLinkId) override;

	/*
	 * 添加模块0, 消息ID的回调, 一个消息只能有一个处理函数
	 * */
	virtual bool AddMessageCallBack(NF_SERVER_TYPE eType, uint32_t nMsgID, NFIDynamicModule* pTarget,
									const NET_RECEIVE_FUNCTOR& cb, bool createCo) override;

	/*
	 * 添加模块moduleId, 消息ID的回调, 一个消息只能有一个处理函数
	 * */
	virtual bool AddMessageCallBack(NF_SERVER_TYPE eType, uint32_t nModuleId, uint32_t nMsgID, NFIDynamicModule* pTarget, const NET_RECEIVE_FUNCTOR& cb, bool createCo) override;

	/*
	 * 未没有注册过的消息，添加一个统一处理的回调函数
	 * */
	virtual bool AddOtherCallBack(NF_SERVER_TYPE eType, uint64_t linkId, NFIDynamicModule* pTarget, const NET_RECEIVE_FUNCTOR& cb, bool createCo) override;

	/*
	 * 添加连接事件，掉线事件的处理函数
	 * */
	virtual bool AddEventCallBack(NF_SERVER_TYPE eType, uint64_t linkId, NFIDynamicModule* pTarget, const NET_EVENT_FUNCTOR& cb, bool createCo) override;

	/*
	* 对所有的消息添加一个统一的回调， 同过判断返回true表示将处理这个消息，false将不处理这个消息
	* */
	virtual bool AddAllMsgCallBack(NF_SERVER_TYPE eType, NFIDynamicModule* pTaraget, const NET_RECEIVE_FUNCTOR& cb, bool createCo) override;

	/**
	 * @brief 添加rpc服务
	 * @param serverType
	 * @param pBase
	 * @param pRpcService
	 * @return
	 */
	virtual bool AddRpcService(NF_SERVER_TYPE serverType, uint32_t nMsgID, NFIDynamicModule* pBase, NFIRpcService* pRpcService, bool createCo = false) override;

	virtual bool AddRpcService(NF_SERVER_TYPE serverType, uint32_t nModuleID, uint32_t nMsgID, NFIDynamicModule* pBase, NFIRpcService* pRpcService, bool createCo = false) override;

	int OnReceiveNetPack(uint64_t connectionLink, uint64_t objectLinkId, NFDataPackage& packet);

	int OnHandleReceiveNetPack(uint64_t connectionLink, uint64_t objectLinkId, NFDataPackage& packet);

	int OnSocketNetEvent(eMsgType nEvent, uint64_t serverLinkId, uint64_t objectLinkId);

	int OnHandleRpcService(uint64_t connectionLink, uint64_t objectLinkId, const NFrame::Proto_FramePkg& svrPkg, uint64_t param1, uint64_t param2);
public:
	virtual bool ResponseHttpMsg(NF_SERVER_TYPE serverType, const NFIHttpHandle& req, const std::string& strMsg,
								 NFWebStatus code = NFWebStatus::WEB_OK, const std::string& reason = "OK");

	virtual bool ResponseHttpMsg(NF_SERVER_TYPE serverType, uint64_t requestId, const std::string& strMsg,
								 NFWebStatus code = NFWebStatus::WEB_OK,
								 const std::string& reason = "OK");

	virtual int HttpGet(NF_SERVER_TYPE serverType, const std::string& strUri,
						const HTTP_CLIENT_RESPONE& respone,
						const std::map<std::string, std::string>& xHeaders = std::map<std::string, std::string>(),
						int timeout = 3);

	virtual int HttpPost(NF_SERVER_TYPE serverType, const std::string& strUri, const std::string& strPostData, const HTTP_CLIENT_RESPONE& respone,
						 const std::map<std::string, std::string>& xHeaders = std::map<std::string, std::string>(),
						 int timeout = 3);

	virtual int SendEmail(NF_SERVER_TYPE serverType, const std::string& title, const std::string& subject, const string& content) override;

	virtual int SendWxWork(NF_SERVER_TYPE serverType, const string& content) override;
protected:
	virtual bool AddHttpMsgCB(NF_SERVER_TYPE serverType, const std::string& strCommand, const NFHttpType eRequestType,
							  const HTTP_RECEIVE_FUNCTOR& cb);

	virtual bool AddHttpOtherMsgCB(NF_SERVER_TYPE serverType, const NFHttpType eRequestType, const HTTP_RECEIVE_FUNCTOR& cb);

	virtual bool AddHttpFilterCB(NF_SERVER_TYPE serverType, const std::string& strCommand, const HTTP_FILTER_FUNCTOR& cb);

	virtual bool OnHttpReceiveNetPack(uint32_t serverType, const NFIHttpHandle& req);

	virtual NFWebStatus OnHttpFilterPack(uint32_t serverType, const NFIHttpHandle& req);
protected:
	NFINetModule* m_netModule;

	std::vector<CallBack> mxCallBack;

	std::vector<ServerLinkData> mServerLinkData;
};
