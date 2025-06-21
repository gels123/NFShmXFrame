// -------------------------------------------------------------------------
//    @FileName         :    NFEnetServer.cpp
//    @Author           :    gaoyi
//    @Date             :    2025-03-13
//    @Email			:    445267987@qq.com
//    @Module           :    NFEnetServer
//
// -------------------------------------------------------------------------

#include "NFEnetServer.h"

#include <NFComm/NFPluginModule/NFLogMgr.h>
#include <NFCommPlugin/NFNetPlugin/Evpp/NFEvppNetMessage.h>

NFEnetServer::NFEnetServer(NFIPluginManager* p, NF_SERVER_TYPE serverType, const NFMessageFlag& flag): NFIEnetConnection(p, serverType, flag)
{
    m_pHost = nullptr;
}

NFEnetServer::~NFEnetServer()
{
}

bool NFEnetServer::Init()
{
    std::string ipAddr;
    if (m_flag.mStrIp == "127.0.0.1")
    {
        ipAddr = m_flag.mStrIp;
    }
    else
    {
        ipAddr = "0.0.0.0";
    }

    ENetAddress address;
    if (enet_address_set_host(&address, ipAddr.c_str()))
    {
        LOG_ERR(0, -1, "enet_address_set_host_ip:{} failed", ipAddr);
        return false;
    }

    address.port = m_flag.nPort;
    m_pHost = enet_host_create(&address, 1, 1, 0, 0);
    if (nullptr == m_pHost)
    {
        LOG_ERR(0, -1, "enet_host_create, {}:{} failed", address.host, address.port);
        return false;
    }

    m_connectionType = NF_CONNECTION_TYPE_TCP_SERVER;

    return true;
}

bool NFEnetServer::Execute()
{
    if (m_pHost == nullptr) return true;

    int ret = 0;
    ENetEvent event;
    ret = enet_host_service(m_pHost, &event, 0);
    if (ret > 0)
    {
        if (event.type == ENET_EVENT_TYPE_CONNECT || event.type == ENET_EVENT_TYPE_DISCONNECT)
        {
            if (m_connCallback)
            {
                m_connCallback(event.type, event.peer, m_unLinkId);
            }
        }
        else if (event.type == ENET_EVENT_TYPE_RECEIVE)
        {
            if (m_messageCallback)
            {
                m_messageCallback(event.peer, event.packet, m_unLinkId);
            }
            enet_packet_destroy(event.packet);
        }
    }
    else if (0 == ret)
    {
        return true;
    }
    else
    {
        LOG_ERR(0, -1, "enet_host_service fail");
        return true;
    }
    return true;
}

bool NFEnetServer::Shut()
{
    return true;
}

bool NFEnetServer::Finalize()
{
    if (m_pHost)
    {
        enet_host_destroy(m_pHost);
        m_pHost = nullptr;
    }
    return true;
}
