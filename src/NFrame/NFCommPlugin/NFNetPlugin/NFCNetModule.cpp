// -------------------------------------------------------------------------
//    @FileName         :    NFCNetServerModule.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Module           :    NFCNetServerModule
// -------------------------------------------------------------------------

#include "NFCNetModule.h"

#include <enet/enet.h>
#include <NFComm/NFPluginModule/NFITaskModule.h>

#include "NFEmailSender.h"
#include "NFPacketParseMgr.h"
#include "Bus/NFCBusMessage.h"
#include "Enet/NFEnetMessage.h"
#include "Evpp/NFEvppNetMessage.h"
#include "evpp/httpc/ssl.h"
#include "NFComm/NFCore/NFServerIDUtil.h"
#include "NFComm/NFCore/NFSocketLibFunction.h"
#include "NFComm/NFPluginModule/NFIMessageModule.h"
#include "NFComm/NFPluginModule/NFIPlugin.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include "NFComm/NFPluginModule/NFServerDefine.h"

NFCNetModule::NFCNetModule(NFIPluginManager* p): NFINetModule(p)
{
	enet_initialize();
	NFSocketLibFunction::InitSocket();
#if defined(EVPP_HTTP_CLIENT_SUPPORTS_SSL)
	if (!evpp::httpc::GetSSLCtx())
    {
        evpp::httpc::InitSSL();
    }
#endif
	m_evppServerArray.resize(NF_ST_MAX);
	for (int i = 0; i < NF_ST_MAX; ++i)
	{
		m_evppServerArray[i] = nullptr;
	}
	m_busServerArray.resize(NF_ST_MAX);
	for (int i = 0; i < NF_ST_MAX; ++i)
	{
		m_busServerArray[i] = nullptr;
	}
	m_enetServerArray.resize(NF_ST_MAX);
	for (int i = 0; i < NF_ST_MAX; ++i)
	{
		m_enetServerArray[i] = nullptr;
	}

	NFPacketParseMgr::m_pPacketParse.resize(100);
}

NFCNetModule::~NFCNetModule()
{
	enet_deinitialize();
#if defined(EVPP_HTTP_CLIENT_SUPPORTS_SSL)
    if (evpp::httpc::GetSSLCtx())
    {
        evpp::httpc::CleanSSL();
    }
#endif
}

bool NFCNetModule::Awake()
{
	return true;
}

bool NFCNetModule::BeforeShut()
{
	return true;
}

bool NFCNetModule::Shut()
{
	for (size_t i = 0; i < m_evppServerArray.size(); i++)
	{
		if (m_evppServerArray[i] != nullptr)
		{
			m_evppServerArray[i]->Shut();
		}
	}
	for (size_t i = 0; i < m_busServerArray.size(); i++)
	{
		if (m_busServerArray[i] != nullptr)
		{
			m_busServerArray[i]->Shut();
		}
	}
	for (size_t i = 0; i < m_enetServerArray.size(); i++)
	{
		if (m_enetServerArray[i] != nullptr)
		{
			m_enetServerArray[i]->Shut();
		}
	}
	return true;
}

bool NFCNetModule::Finalize()
{
	for (size_t i = 0; i < m_evppServerArray.size(); i++)
	{
		if (m_evppServerArray[i] != nullptr)
		{
			m_evppServerArray[i]->Finalize();
			NF_SAFE_DELETE(m_evppServerArray[i]);
		}
	}
	for (size_t i = 0; i < m_busServerArray.size(); i++)
	{
		if (m_busServerArray[i] != nullptr)
		{
			m_busServerArray[i]->Finalize();
			NF_SAFE_DELETE(m_busServerArray[i]);
		}
	}
	for (size_t i = 0; i < m_enetServerArray.size(); i++)
	{
		if (m_enetServerArray[i] != nullptr)
		{
			m_enetServerArray[i]->Finalize();
			NF_SAFE_DELETE(m_enetServerArray[i]);
		}
	}
	m_evppServerArray.clear();
	m_busServerArray.clear();
	m_enetServerArray.clear();
	/**
	 * @brief 释放资源
	 */
	NFPacketParseMgr::ReleasePacketParse();
	return true;
}

bool NFCNetModule::ReadyExecute()
{
	for (size_t i = 0; i < m_evppServerArray.size(); i++)
	{
		if (m_evppServerArray[i] != nullptr)
		{
			m_evppServerArray[i]->ReadyExecute();
		}
	}
	for (size_t i = 0; i < m_busServerArray.size(); i++)
	{
		if (m_busServerArray[i] != nullptr)
		{
			m_busServerArray[i]->ReadyExecute();
		}
	}
	for (size_t i = 0; i < m_enetServerArray.size(); i++)
	{
		if (m_enetServerArray[i] != nullptr)
		{
			m_enetServerArray[i]->ReadyExecute();
		}
	}
	return true;
}

bool NFCNetModule::Execute()
{
	for (size_t i = 0; i < m_evppServerArray.size(); i++)
	{
		if (m_evppServerArray[i] != nullptr)
		{
			m_evppServerArray[i]->Execute();
		}
	}
	for (size_t i = 0; i < m_busServerArray.size(); i++)
	{
		if (m_busServerArray[i] != nullptr)
		{
			m_busServerArray[i]->Execute();
		}
	}
	for (size_t i = 0; i < m_enetServerArray.size(); i++)
	{
		if (m_enetServerArray[i] != nullptr)
		{
			m_enetServerArray[i]->Execute();
		}
	}
	return true;
}

NFINetMessage* NFCNetModule::GetServerByServerType(NF_SERVER_TYPE serverType) const
{
	if (serverType > NF_ST_NONE && serverType < NF_ST_MAX)
	{
		return m_evppServerArray[serverType];
	}
	return nullptr;
}

uint64_t NFCNetModule::ConnectServer(NF_SERVER_TYPE serverType, const std::string& url, uint32_t packetParseType, bool security)
{
	NFChannelAddress addr;
	if (!NFServerIDUtil::MakeAddress(url, addr))
	{
		NFLogError(NF_LOG_DEFAULT, 0, "usl:{} error", url);
		return 0;
	}

	if (serverType > NF_ST_NONE && serverType < NF_ST_MAX)
	{
		if (addr.mScheme == "tcp" || addr.mScheme == "http")
		{
			NFMessageFlag flag;
			flag.mStrIp = addr.mHost;
			flag.nPort = addr.mPort;
			flag.mPacketParseType = packetParseType;
			flag.mSecurity = security;

			NFINetMessage* pServer = m_evppServerArray[serverType];
			if (!pServer)
			{
#ifdef USE_NET_EVPP
				pServer = NF_NEW NFEvppNetMessage(m_pObjPluginManager, serverType);
#else

#endif
				pServer->SetRecvCb(m_recvCb);
				pServer->SetEventCb(m_eventCb);
				m_evppServerArray[serverType] = pServer;
			}


			uint64_t linkId = pServer->ConnectServer(flag);
			return linkId;
		}
		else if (addr.mScheme == "udp")
		{
			NFMessageFlag flag;
			flag.mStrIp = addr.mHost;
			flag.nPort = addr.mPort;
			flag.mPacketParseType = packetParseType;
			flag.mSecurity = security;

			NFINetMessage* pServer = m_enetServerArray[serverType];
			if (!pServer)
			{
#ifdef USE_NET_EVPP
				pServer = NF_NEW NFEnetMessage(m_pObjPluginManager, serverType);
#else

#endif
				pServer->SetRecvCb(m_recvCb);
				pServer->SetEventCb(m_eventCb);
				m_enetServerArray[serverType] = pServer;
			}

			uint64_t linkId = pServer->ConnectServer(flag);
			return linkId;
		}
		else if (addr.mScheme == "bus")
		{
			uint32_t busid = NFServerIDUtil::GetBusID(addr.mHost);
			if (busid <= 0)
			{
				NFLogError(NF_LOG_DEFAULT, 0, "BusAddrAton Failed! host:{}", addr.mHost);
				return 0;
			}
			NFMessageFlag flag;
			flag.mStrIp = addr.mHost;
			flag.nPort = addr.mPort;
			flag.mBusId = busid;
			flag.mBusLength = addr.mPort;
			flag.mPacketParseType = packetParseType;
			flag.mSecurity = security;

			NFINetMessage* pServer = m_busServerArray[serverType];
			if (!pServer)
			{
				pServer = NF_NEW NFCBusMessage(m_pObjPluginManager, serverType);
				pServer->SetRecvCb(m_recvCb);
				pServer->SetEventCb(m_eventCb);
				m_busServerArray[serverType] = pServer;
			}

			uint64_t linkId = pServer->ConnectServer(flag);
			return linkId;
		}
	}
	return 0;
}

int NFCNetModule::ResumeConnect(NF_SERVER_TYPE serverType)
{
	NFINetMessage* pServer = m_busServerArray[serverType];
	if (pServer)
	{
		return pServer->ResumeConnect();
	}
	return -1;
}

int NFCNetModule::ResetPacketParse(uint32_t parseType, NFIPacketParse* pPacketParse)
{
	return NFPacketParseMgr::ResetPacketParse(parseType, pPacketParse);
}

uint64_t NFCNetModule::BindServer(NF_SERVER_TYPE serverType, const std::string& url, uint32_t netThreadNum, uint32_t maxConnectNum, uint32_t packetParseType, bool security)
{
	NFChannelAddress addr;
	if (!NFServerIDUtil::MakeAddress(url, addr))
	{
		NFLogError(NF_LOG_DEFAULT, 0, "usl:{} error", url);
		return 0;
	}

	if (serverType > NF_ST_NONE && serverType < NF_ST_MAX)
	{
		if (addr.mScheme == "tcp" || addr.mScheme == "http")
		{
			NFMessageFlag flag;
			flag.mStrIp = addr.mHost;
			flag.nPort = addr.mPort;
			flag.mPacketParseType = packetParseType;
			flag.nNetThreadNum = netThreadNum;
			flag.mMaxConnectNum = maxConnectNum;
			flag.mSecurity = security;
			if (addr.mScheme == "http")
			{
				flag.bHttp = true;
			}

			NFINetMessage* pServer = m_evppServerArray[serverType];
			if (!pServer)
			{
				pServer = NF_NEW NFEvppNetMessage(m_pObjPluginManager, serverType);
				pServer->SetRecvCb(m_recvCb);
				pServer->SetEventCb(m_eventCb);
				pServer->SetHttpRecvCb(m_httpReceiveCb);
				pServer->SetHttpFilterCb(m_httpFilter);
				m_evppServerArray[serverType] = pServer;
			}

			uint64_t linkId = pServer->BindServer(flag);
			if (linkId > 0)
			{
				return linkId;
			}

			NFLogError(NF_LOG_DEFAULT, 0, "Add Server Failed!");
		}
		else if (addr.mScheme == "udp")
		{
			NFMessageFlag flag;
			flag.mStrIp = addr.mHost;
			flag.nPort = addr.mPort;
			flag.mPacketParseType = packetParseType;
			flag.nNetThreadNum = netThreadNum;
			flag.mMaxConnectNum = maxConnectNum;
			flag.mSecurity = security;

			NFINetMessage* pServer = m_enetServerArray[serverType];
			if (!pServer)
			{
				pServer = NF_NEW NFEnetMessage(m_pObjPluginManager, serverType);
				pServer->SetRecvCb(m_recvCb);
				pServer->SetEventCb(m_eventCb);
				pServer->SetHttpRecvCb(m_httpReceiveCb);
				pServer->SetHttpFilterCb(m_httpFilter);
				m_enetServerArray[serverType] = pServer;
			}

			uint64_t linkId = pServer->BindServer(flag);
			if (linkId > 0)
			{
				return linkId;
			}

			NFLogError(NF_LOG_DEFAULT, 0, "Add Server Failed!");
		}
		else if (addr.mScheme == "bus")
		{
			uint32_t busid = NFServerIDUtil::GetBusID(addr.mHost);
			if (busid <= 0)
			{
				NFLogError(NF_LOG_DEFAULT, 0, "BusAddrAton Failed! host:{}", addr.mHost);
				return 0;
			}
			NFMessageFlag flag;
			flag.mStrIp = addr.mHost;
			flag.nPort = addr.mPort;
			flag.mBusId = busid;
			flag.mBusLength = addr.mPort;
			flag.mPacketParseType = packetParseType;
			flag.nNetThreadNum = netThreadNum;
			flag.mMaxConnectNum = maxConnectNum;
			flag.mSecurity = security;

			NFINetMessage* pServer = m_busServerArray[serverType];
			if (!pServer)
			{
				pServer = NF_NEW NFCBusMessage(m_pObjPluginManager, serverType);

				pServer->SetRecvCb(m_recvCb);
				pServer->SetEventCb(m_eventCb);
				m_busServerArray[serverType] = pServer;
			}

			uint64_t linkId = pServer->BindServer(flag);
			if (linkId > 0)
			{
				return linkId;
			}

			NFLogError(NF_LOG_DEFAULT, 0, "Add Server Failed!");
		}
	}
	return 0;
}

std::string NFCNetModule::GetLinkIp(uint64_t linkId)
{
	uint32_t serverType = GetServerTypeFromUnlinkId(linkId);
	if (serverType > NF_ST_NONE && serverType < NF_ST_MAX)
	{
		uint32_t isServer = GetServerLinkModeFromUnlinkId(linkId);
		if (isServer == NF_IS_NET)
		{
			auto pServer = m_evppServerArray[serverType];
			if (pServer)
			{
				return pServer->GetLinkIp(linkId);
			}
			else
			{
				return std::string();
			}
		}
		else if (isServer == NF_IS_ENET)
		{
			auto pServer = m_enetServerArray[serverType];
			if (pServer)
			{
				return pServer->GetLinkIp(linkId);
			}
			else
			{
				return std::string();
			}
		}
		else
		{
			auto pServer = m_busServerArray[serverType];
			if (pServer)
			{
				return pServer->GetLinkIp(linkId);
			}
			else
			{
				return std::string();
			}
		}
	}
	return std::string();
}

uint32_t NFCNetModule::GetPort(uint64_t linkId)
{
	uint32_t serverType = GetServerTypeFromUnlinkId(linkId);
	if (serverType > NF_ST_NONE && serverType < NF_ST_MAX)
	{
		uint32_t isServer = GetServerLinkModeFromUnlinkId(linkId);
		if (isServer == NF_IS_NET)
		{
			auto pServer = m_evppServerArray[serverType];
			if (pServer)
			{
				return pServer->GetPort(linkId);
			}
			else
			{
				return 0;
			}
		}
		else if (isServer == NF_IS_ENET)
		{
			auto pServer = m_enetServerArray[serverType];
			if (pServer)
			{
				return pServer->GetPort(linkId);
			}
			else
			{
				return 0;
			}
		}
		else
		{
			auto pServer = m_busServerArray[serverType];
			if (pServer)
			{
				return pServer->GetPort(linkId);
			}
			else
			{
				return 0;
			}
		}
	}
	return 0;
}

void NFCNetModule::CloseLinkId(uint64_t linkId)
{
	if (linkId == 0) return;

	uint32_t serverType = GetServerTypeFromUnlinkId(linkId);

	if (serverType > NF_ST_NONE && serverType < NF_ST_MAX)
	{
		uint32_t isServer = GetServerLinkModeFromUnlinkId(linkId);
		if (isServer == NF_IS_NET)
		{
			auto pServer = m_evppServerArray[serverType];
			if (pServer)
			{
				pServer->CloseLinkId(linkId);
				return;
			}
			else
			{
				NFLogError(NF_LOG_DEFAULT, 0, "CloseLinkId error, usLinkId:{} not exist!", linkId);
			}
		}
		else if (isServer == NF_IS_ENET)
		{
			auto pServer = m_enetServerArray[serverType];
			if (pServer)
			{
				pServer->CloseLinkId(linkId);
				return;
			}
			else
			{
				NFLogError(NF_LOG_DEFAULT, 0, "CloseLinkId error, usLinkId:{} not exist!", linkId);
			}
		}
		else
		{
			auto pServer = m_busServerArray[serverType];
			if (pServer)
			{
				pServer->CloseLinkId(linkId);
				return;
			}
			else
			{
				NFLogError(NF_LOG_DEFAULT, 0, "CloseLinkId error, usLinkId:{} not exist!", linkId);
			}
		}
	}
	NFLogError(NF_LOG_DEFAULT, 0, "CloseLinkId error, usLinkId:{} not exist!", linkId);
}

void NFCNetModule::Send(uint64_t linkId, uint32_t moduleId, uint32_t msgId, const std::string& strData, uint64_t param1, uint64_t param2, uint64_t srcId, uint64_t dstId)
{
	NFDataPackage packet;
	packet.mModuleId = moduleId;
	packet.nMsgId = msgId;
	packet.nParam1 = param1;
	packet.nParam2 = param2;
	packet.nSrcId = srcId;
	packet.nDstId = dstId;

	Send(linkId, packet, strData.data(), strData.length());
}

void NFCNetModule::Send(uint64_t linkId, uint32_t moduleId, uint32_t msgId, const char* msg, uint32_t len, uint64_t param1, uint64_t param2, uint64_t srcId, uint64_t dstId)
{
	NFDataPackage packet;
	packet.mModuleId = moduleId;
	packet.nMsgId = msgId;
	packet.nParam1 = param1;
	packet.nParam2 = param2;
	packet.nSrcId = srcId;
	packet.nDstId = dstId;

	Send(linkId, packet, msg, len);
}

void NFCNetModule::Send(uint64_t linkId, uint32_t moduleId, uint32_t msgId, const google::protobuf::Message& data, uint64_t param1, uint64_t param2, uint64_t srcId, uint64_t dstId)
{
	NFDataPackage packet;
	packet.mModuleId = moduleId;
	packet.nMsgId = msgId;
	packet.nParam1 = param1;
	packet.nParam2 = param2;
	packet.nSrcId = srcId;
	packet.nDstId = dstId;

	Send(linkId, packet, data);
}

void NFCNetModule::SendServer(uint64_t linkId, uint32_t moduleId, uint32_t msgId, const std::string& strData, uint64_t param1, uint64_t param2, uint64_t srcId, uint64_t dstId)
{
	NFDataPackage packet;
	packet.mModuleId = moduleId;
	packet.nMsgId = msgId;
	packet.nParam1 = param1;
	packet.nParam2 = param2;
	packet.nSrcId = srcId;
	packet.nDstId = dstId;

	Send(linkId, packet, strData.data(), strData.length());
}

void NFCNetModule::SendServer(uint64_t linkId, uint32_t moduleId, uint32_t msgId, const char* msg, uint32_t len, uint64_t param1, uint64_t param2, uint64_t srcId, uint64_t dstId)
{
	NFDataPackage packet;
	packet.mModuleId = moduleId;
	packet.nMsgId = msgId;
	packet.nParam1 = param1;
	packet.nParam2 = param2;
	packet.nSrcId = srcId;
	packet.nDstId = dstId;

	Send(linkId, packet, msg, len);
}

void NFCNetModule::SendServer(uint64_t linkId, uint32_t moduleId, uint32_t msgId, const google::protobuf::Message& data, uint64_t param1, uint64_t param2, uint64_t srcId, uint64_t dstId)
{
	NFDataPackage packet;
	packet.mModuleId = moduleId;
	packet.nMsgId = msgId;
	packet.nParam1 = param1;
	packet.nParam2 = param2;
	packet.nSrcId = srcId;
	packet.nDstId = dstId;

	Send(linkId, packet, data);
}

void NFCNetModule::TransPackage(uint64_t linkId, NFDataPackage& packet)
{
	Send(linkId, packet, packet.GetBuffer(), packet.GetSize());
}

bool NFCNetModule::Send(uint64_t linkId, NFDataPackage& packet, const char* msg, uint32_t len)
{
	uint32_t serverType = GetServerTypeFromUnlinkId(linkId);

	if (serverType > NF_ST_NONE && serverType < NF_ST_MAX)
	{
		uint32_t isServer = GetServerLinkModeFromUnlinkId(linkId);
		if (isServer == NF_IS_NET)
		{
			auto pServer = m_evppServerArray[serverType];
			if (pServer)
			{
				return Send(pServer, linkId, packet, msg, len);
			}
			else
			{
				NFLogError(NF_LOG_DEFAULT, 0, "SendByServerID error, usLinkId:{} not exist!", linkId);
			}
		}
		else if (isServer == NF_IS_ENET)
		{
			auto pServer = m_enetServerArray[serverType];
			if (pServer)
			{
				return Send(pServer, linkId, packet, msg, len);
			}
			else
			{
				NFLogError(NF_LOG_DEFAULT, 0, "SendByServerID error, usLinkId:{} not exist!", linkId);
			}
		}
		else
		{
			auto pServer = m_busServerArray[serverType];
			if (pServer)
			{
				return Send(pServer, linkId, packet, msg, len);
			}
			else
			{
				NFLogError(NF_LOG_DEFAULT, 0, "bus SendByServerID error, usLinkId:{} not exist!", linkId);
			}
		}
	}

	if (linkId != 0)
	{
		NFLogError(NF_LOG_DEFAULT, 0, "SendByServerID error, usLinkId:{} not exist!", linkId);
	}

	return false;
}

bool NFCNetModule::Send(uint64_t linkId, NFDataPackage& packet, const google::protobuf::Message& data)
{
	uint32_t serverType = GetServerTypeFromUnlinkId(linkId);

	if (serverType > NF_ST_NONE && serverType < NF_ST_MAX)
	{
		uint32_t isServer = GetServerLinkModeFromUnlinkId(linkId);
		if (isServer == NF_IS_NET)
		{
			auto pServer = m_evppServerArray[serverType];
			if (pServer)
			{
				return Send(pServer, linkId, packet, data);
			}
			else
			{
				NFLogError(NF_LOG_DEFAULT, 0, "SendByServerID error, usLinkId:{} not exist!", linkId);
			}
		}
		else if (isServer == NF_IS_ENET)
		{
			auto pServer = m_enetServerArray[serverType];
			if (pServer)
			{
				return Send(pServer, linkId, packet, data);
			}
			else
			{
				NFLogError(NF_LOG_DEFAULT, 0, "SendByServerID error, usLinkId:{} not exist!", linkId);
			}
		}
		else
		{
			auto pServer = m_busServerArray[serverType];
			if (pServer)
			{
				return Send(pServer, linkId, packet, data);
			}
			else
			{
				NFLogError(NF_LOG_DEFAULT, 0, "bus SendByServerID error, usLinkId:{} not exist!", linkId);
			}
		}
	}

	if (linkId != 0)
	{
		NFLogError(NF_LOG_DEFAULT, 0, "SendByServerID error, usLinkId:{} not exist!", linkId);
	}

	return false;
}

bool NFCNetModule::Send(NFINetMessage* pServer, uint64_t linkId, NFDataPackage& packet, const char* msg, uint32_t len)
{
	if (pServer)
	{
		return pServer->Send(linkId, packet, msg, len);
	}
	return false;
}

bool NFCNetModule::Send(NFINetMessage* pServer, uint64_t linkId, NFDataPackage& packet, const google::protobuf::Message& data)
{
	if (pServer)
	{
		return pServer->Send(linkId, packet, data);
	}
	return false;
}

bool
NFCNetModule::ResponseHttpMsg(NF_SERVER_TYPE serverType, const NFIHttpHandle& req, const string& strMsg, NFWebStatus code, const string& reason)
{
	if (serverType > NF_ST_NONE && serverType < NF_ST_MAX)
	{
		NFINetMessage* pServer = m_evppServerArray[serverType];
		if (pServer)
		{
			return pServer->ResponseHttpMsg(req, strMsg, code, reason);
		}
	}
	return false;
}

bool NFCNetModule::ResponseHttpMsg(NF_SERVER_TYPE serverType, uint64_t requestId, const string& strMsg, NFWebStatus code, const string& reason)
{
	if (serverType > NF_ST_NONE && serverType < NF_ST_MAX)
	{
		NFINetMessage* pServer = m_evppServerArray[serverType];
		if (pServer)
		{
			return pServer->ResponseHttpMsg(requestId, strMsg, code, reason);
		}
	}
	return false;
}

int NFCNetModule::HttpGet(NF_SERVER_TYPE serverType, const string& strUri, const HTTP_CLIENT_RESPONE& respone, const map<std::string, std::string>& xHeaders, int timeout)
{
	if (serverType > NF_ST_NONE && serverType < NF_ST_MAX)
	{
		NFINetMessage* pServer = m_evppServerArray[serverType];
		if (pServer)
		{
			return pServer->HttpGet(strUri, respone, xHeaders, timeout);
		}
	}
	return -1;
}

int
NFCNetModule::HttpPost(NF_SERVER_TYPE serverType, const string& strUri, const string& strPostData, const HTTP_CLIENT_RESPONE& respone, const map<std::string, std::string>& xHeaders, int timeout)
{
	if (serverType > NF_ST_NONE && serverType < NF_ST_MAX)
	{
		NFINetMessage* pServer = m_evppServerArray[serverType];
		if (pServer)
		{
			return pServer->HttpPost(strUri, strPostData, respone, xHeaders, timeout);
		}
	}
	return -1;
}

int NFCNetModule::SendEmail(NF_SERVER_TYPE serverType, const std::string& title, const std::string& subject, const string& content)
{
#if NF_PLATFORM == NF_PLATFORM_LINUX
    NFServerConfig* pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_MASTER_SERVER);
    CHECK_NULL(0, pConfig);

    NFSmtpSendMail sendMail;
    sendMail.SetSmtpServer(pConfig->sendEmail, pConfig->sendEmailPass,pConfig->sendEmailUrl, pConfig->sendEmailPort);
    sendMail.SetSendName(title);
    sendMail.SetSendMail(pConfig->sendEmail);
    sendMail.AddRecvMail(pConfig->recvEmail);
    sendMail.SetSubject(subject);
    std::string dumpInfo = content;
    NFStringUtility::Replace(dumpInfo, "\n", "<br/>");
    sendMail.SetBodyContent(dumpInfo);
    if (!sendMail.SendMail())
    {
        NFLogError(NF_LOG_DEFAULT, 0, "Send Message(title:{} subject:{}) To Email:{} Failed", title, subject, pConfig->recvEmail);
        return -1;
    }
    else
    {
        NFLogError(NF_LOG_DEFAULT, 0, "Send Message(title:{} subject:{}) To Email:{} Success", title, subject, pConfig->recvEmail);
        return 0;
    }
#else
	return 0;
#endif
}
