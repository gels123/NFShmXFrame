// -------------------------------------------------------------------------
//    @FileName         :    EnetObject.cpp
//    @Author           :    gaoyi
//    @Date             :    2025-03-13
//    @Email			:    445267987@qq.com
//    @Module           :    EnetObject
//
// -------------------------------------------------------------------------

#include "EnetObject.h"

EnetObject::EnetObject(ENetPeer* pConn) : m_usLinkId(0), m_needRemove(false), m_connPtr(pConn)
{
    m_isServer = true;
    m_packetParseType = 0;
    m_lastHeartBeatTime = NFGetTime();
    m_port = 0;
    m_security = false;
}

EnetObject::~EnetObject()
{
}

std::string EnetObject::GetStrIp() const
{
    return m_strIp;
}

uint32_t EnetObject::GetPort() const
{
    return m_port;
}

void EnetObject::SetStrIp(const std::string& val)
{
    m_strIp = val;
}

void EnetObject::SetPort(uint32_t port)
{
    m_port = port;
}

uint64_t EnetObject::GetLinkId() const
{
    return m_usLinkId;
}

void EnetObject::SetLinkId(uint64_t linkId)
{
    m_usLinkId = linkId;
}

bool EnetObject::GetNeedRemove() const
{
    return m_needRemove;
}

void EnetObject::SetNeedRemove(bool val)
{
    m_needRemove = val;
}

void EnetObject::CloseObject() const
{
    if (m_connPtr && IsDisConnect())
    {
        enet_peer_disconnect(m_connPtr, 0);
    }
}

void EnetObject::SetIsServer(bool b)
{
    m_isServer = b;
}

bool EnetObject::IsServer() const
{
    return m_isServer;
}

bool EnetObject::IsConnect() const
{
    return !IsDisConnect();
}

bool EnetObject::IsDisConnect() const
{
    if (!m_connPtr) return true;
    if (m_connPtr->state == ENET_PEER_STATE_DISCONNECTING ||
        m_connPtr->state == ENET_PEER_STATE_DISCONNECTED ||
        m_connPtr->state == ENET_PEER_STATE_ACKNOWLEDGING_DISCONNECT ||
        m_connPtr->state == ENET_PEER_STATE_ZOMBIE)
    {
        return true;
    }
    return false;
}
