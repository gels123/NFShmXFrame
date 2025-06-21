// -------------------------------------------------------------------------
//    @FileName         :    NetEvppObject.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFNetPlugin
// -------------------------------------------------------------------------

#include "NetEvppObject.h"
#include "NFComm/NFCore/NFPlatform.h"

#define MAGIC_KEY "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

NetEvppObject::NetEvppObject(const evpp::TCPConnPtr& conn) : m_usLinkId(0), m_needRemove(false), m_connPtr(conn)
{
	m_isServer = true;
	m_packetParseType = 0;
	m_lastHeartBeatTime = NFGetTime();
	m_port = 0;
	m_security = false;
}

NetEvppObject::~NetEvppObject()
{
}

std::string NetEvppObject::GetStrIp() const
{
	return m_strIp;
}

uint32_t NetEvppObject::GetPort() const
{
    return m_port;
}

void NetEvppObject::SetStrIp(const std::string& val)
{
	m_strIp = val;
}

void NetEvppObject::SetPort(uint32_t port)
{
    m_port = port;
}

void NetEvppObject::SetIsServer(bool b)
{
	m_isServer = b;
}

bool NetEvppObject::IsServer() const
{
    return m_isServer;
}

uint64_t NetEvppObject::GetLinkId() const
{
	return m_usLinkId;
}

void NetEvppObject::SetLinkId(uint64_t linkId)
{
	m_usLinkId = linkId;
}

bool NetEvppObject::GetNeedRemove() const
{
	return m_needRemove;
}

void NetEvppObject::SetNeedRemove(bool val)
{
	m_needRemove = val;
}

void NetEvppObject::CloseObject() const
{
	if (m_connPtr)
	{
		if (m_connPtr->IsConnected() || m_connPtr->IsConnecting())
		{
			m_connPtr->Close();
		}
	}
}
