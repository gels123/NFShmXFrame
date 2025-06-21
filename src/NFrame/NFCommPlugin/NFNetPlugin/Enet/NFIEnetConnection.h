// -------------------------------------------------------------------------
//    @FileName         :    NFIEnetConnection.h
//    @Author           :    gaoyi
//    @Date             :    2025-03-13
//    @Email			:    445267987@qq.com
//    @Module           :    NFIEnetConnection
//
// -------------------------------------------------------------------------

#pragma once

#include <enet/enet.h>
#include "NFComm/NFPluginModule/NFIModule.h"
#include "NFComm/NFPluginModule/NFNetDefine.h"

typedef std::function<void(ENetEventType eventType, ENetPeer* pConn, uint64_t serverLinkId)> ENET_CONNECT_CALLBACK;
typedef std::function<void(ENetPeer* pConn, ENetPacket* pPacket, uint64_t serverLinkId)> ENET_MESSAGE_CALLBACK;

class NFIEnetConnection : public NFIModule
{
public:
    NFIEnetConnection(NFIPluginManager* p, NF_SERVER_TYPE serverType, const NFMessageFlag& flag): NFIModule(p), m_connectionType(0), m_unLinkId(0), m_serverType(serverType), m_flag(flag)
    {
    }

    ~NFIEnetConnection() override
    {
    }

    virtual void SetConnCallback(const ENET_CONNECT_CALLBACK& back)
    {
        m_connCallback = back;
    }

    virtual void SetMessageCallback(const ENET_MESSAGE_CALLBACK& back)
    {
        m_messageCallback = back;
    }

    virtual uint64_t GetBusId() const { return m_flag.mBusId; }

    virtual uint64_t GetBusLength() const { return m_flag.mBusLength; }

    virtual uint32_t GetPacketParseType() const { return m_flag.mPacketParseType; }

    virtual bool IsSecurity() const { return m_flag.mSecurity; }

    virtual uint32_t GetConnectionType() { return m_connectionType; }

    virtual void SetConnectionType(uint32_t type) { m_connectionType = type; }

    virtual void SetLinkId(uint64_t id) { m_unLinkId = id; }
    virtual uint64_t GetLinkId() const { return m_unLinkId; }

    virtual bool IsActivityConnect() const { return m_flag.bActivityConnect; }

protected:
    ENET_CONNECT_CALLBACK m_connCallback;

    ENET_MESSAGE_CALLBACK m_messageCallback;

    uint32_t m_connectionType;

    uint64_t m_unLinkId;

    NF_SERVER_TYPE m_serverType;

    NFMessageFlag m_flag;
};
