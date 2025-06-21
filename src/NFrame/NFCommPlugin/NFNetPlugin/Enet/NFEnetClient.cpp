// -------------------------------------------------------------------------
//    @FileName         :    NFEnetClient.h
//    @Author           :    gaoyi
//    @Date             :    2025-03-13
//    @Email			:    445267987@qq.com
//    @Module           :    NFEnetClient
//
// -------------------------------------------------------------------------

#include "NFEnetClient.h"
#include <NFComm/NFPluginModule/NFLogMgr.h>

bool NFEnetClient::Init()
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

    m_pHost = enet_host_create(NULL, 1, 1, 0, 0);
    if (nullptr == m_pHost)
    {
        LOG_ERR(0, -1, "enet_host_create, {}:{} failed", ipAddr, m_flag.nPort);
        return false;
    }

    ENetAddress srv_addr;
    if(enet_address_set_host(&srv_addr, ipAddr.c_str())){
        LOG_ERR(0, -1, "enet_address_set_host_ip {} fail", m_flag.mStrIp);
        return false;
    }
    srv_addr.port = m_flag.nPort;

    auto pPeer = enet_host_connect(m_pHost, &srv_addr, 1, 0);
    if(pPeer == nullptr){
        LOG_ERR(0, -1, "enet_host_connect {}:{} fail", m_flag.mStrIp, m_flag.nPort);
        return false;
    }

    m_connectionType = NF_CONNECTION_TYPE_TCP_CLIENT;

    return true;
}

bool NFEnetClient::Execute()
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

bool NFEnetClient::Shut()
{
    return true;
}

bool NFEnetClient::Finalize()
{
    if (m_pHost)
    {
        enet_host_destroy(m_pHost);
        m_pHost = nullptr;
    }
    return true;
}