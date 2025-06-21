// -------------------------------------------------------------------------
//    @FileName         :    NFCBusMessage.cpp
//    @Author           :    Yi.Gao
//    @Date             :   2022-09-18
//    @Module           :    NFBusPlugin
//    @Desc             :
// -------------------------------------------------------------------------

#include "NFCBusMessage.h"
#include <sstream>
#include <string.h>
#include "NFCBusClient.h"
#include "NFCBusServer.h"
#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFCore/NFServerIDUtil.h"
#include "NFComm/NFPluginModule/NFCheck.h"
#include "NFComm/NFPluginModule/NFIMessageModule.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include "NFComm/NFPluginModule/NFNetPackagePool.h"

NFCBusMessage::NFCBusMessage(NFIPluginManager* p, NF_SERVER_TYPE serverType) : NFINetMessage(p, serverType)
{
    m_bindConnect = nullptr;
#ifdef NF_DEBUG_MODE
    SetTimer(ENUM_SERVER_CLIENT_TIMER_HEART, ENUM_SERVER_CLIENT_TIMER_HEART_TIME_LONGTH * 3);
    SetTimer(ENUM_SERVER_TIMER_CHECK_HEART, ENUM_SERVER_TIMER_CHECK_HEART_TIME_LONGTH);
#else
    SetTimer(ENUM_SERVER_CLIENT_TIMER_HEART, ENUM_SERVER_CLIENT_TIMER_HEART_TIME_LONGTH*3);
    SetTimer(ENUM_SERVER_TIMER_CHECK_HEART, ENUM_SERVER_TIMER_CHECK_HEART_TIME_LONGTH);
#endif
}

NFCBusMessage::~NFCBusMessage()
{
}

bool NFCBusMessage::Execute()
{
    if (m_bindConnect)
    {
        m_bindConnect->Execute();
    }
    return true;
}

bool NFCBusMessage::Shut()
{
    auto pConn = m_busConnectMap.First();
    while (pConn)
    {
        pConn->Shut();
        pConn = m_busConnectMap.Next();
    }

    return true;
}

bool NFCBusMessage::Finalize()
{
    auto pConn = m_busConnectMap.First();
    while (pConn)
    {
        pConn->Finalize();
        pConn = m_busConnectMap.Next();
    }
    m_bindConnect = nullptr;
    m_busConnectMap.ClearAll();

    return true;
}

bool NFCBusMessage::ReadyExecute()
{
    return true;
}

/**
* @brief	初始化
*
* @return 是否成功
*/
uint64_t NFCBusMessage::BindServer(const NFMessageFlag& flag)
{
    CHECK_EXPR(m_bindConnect == NULL, 0, "BindServer Failed!");
    NF_SHARE_PTR<NFCBusServer> pServer = std::make_shared<NFCBusServer>(m_pObjPluginManager, m_serverType, flag);
    NF_ASSERT(pServer);

    pServer->SetMsgPeerCallback(std::bind(&NFCBusMessage::OnHandleMsgPeer, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                                          std::placeholders::_4));


    if (pServer->Init())
    {
        m_busConnectMap.AddElement(pServer->GetLinkId(), pServer);
    }
    else
    {
        NFLogError(NF_LOG_DEFAULT, 0, "NFCBusServer Init Failed!");
        return 0;
    }

    m_bindConnect = pServer;
    return pServer->GetLinkId();
}

uint64_t NFCBusMessage::ConnectServer(const NFMessageFlag& flag)
{
    CHECK_EXPR(m_bindConnect, 0, "ConnectServer Failed, muset bindserver");

    NF_SHARE_PTR<NFCBusClient> pConn = std::make_shared<NFCBusClient>(m_pObjPluginManager, m_serverType, flag, m_bindConnect->GetBindFlag());
    NF_ASSERT(pConn);

    if (pConn->Init())
    {
        m_busConnectMap.AddElement(pConn->GetLinkId(), pConn);
    }
    else
    {
        NFLogError(NF_LOG_DEFAULT, 0, "NFCBusClient Init Failed")
        return 0;
    }

    return pConn->GetLinkId();
}

bool NFCBusMessage::Send(uint64_t usLinkId, NFDataPackage& packet, const char* msg, uint32_t nLen)
{
    auto pConn = m_busConnectMap.GetElement(usLinkId);

    if (pConn)
    {
        return pConn->Send(packet, msg, nLen);
    }

    NFLogError(NF_LOG_DEFAULT, 0, "usLinkId:{} not find", usLinkId);
    return false;
}

bool NFCBusMessage::Send(uint64_t usLinkId, NFDataPackage& packet, const google::protobuf::Message& xData)
{
    auto pConn = m_busConnectMap.GetElement(usLinkId);

    if (pConn)
    {
        return pConn->Send(packet, xData);
    }

    NFLogError(NF_LOG_DEFAULT, 0, "usLinkId:{} not find", usLinkId);
    return false;
}

/**
 * @brief 获得连接IP
 *
 * @param  usLinkId
 * @return std::string
 */
std::string NFCBusMessage::GetLinkIp(uint64_t usLinkId)
{
    auto pConn = m_busConnectMap.GetElement(usLinkId);
    CHECK_EXPR(pConn, "", "usLinkId:{} not find", usLinkId);

    return pConn->GetLinkIp();
}

uint32_t NFCBusMessage::GetPort(uint64_t usLinkId)
{
    return 0;
}

/**
* @brief 关闭连接
*
* @param  usLinkId
* @return
*/
void NFCBusMessage::CloseLinkId(uint64_t usLinkId)
{
    auto pConn = m_busConnectMap.GetElement(usLinkId);
    CHECK_EXPR(pConn, , "usLinkId:{} not find", usLinkId);

    return pConn->CloseLinkId();
}

void NFCBusMessage::OnHandleMsgPeer(eMsgType type, uint64_t serverLinkId, uint64_t objectLinkId, NFDataPackage& package)
{
    if (!NFGlobalSystem::Instance()->IsSpecialMsg(package.mModuleId, package.nMsgId))
    {
        NFLogTrace(NF_LOG_DEFAULT, 0, "recv msg:{} ", package.ToString());
    }

    uint32_t fromBusId = (key_t)GetBusIdFromUnlinkId(objectLinkId);
    uint64_t fromLinkId = GetUnLinkId(NF_IS_BUS, m_serverType, fromBusId, 0);
    switch (type)
    {
    case eMsgType_RECIVEDATA:
        {
            if (package.mModuleId == NF_MODULE_FRAME)
            {
                if (package.nMsgId == NFrame::NF_SERVER_TO_SERVER_HEART_BEAT)
                {
                    auto pConn = m_busConnectMap.GetElement(fromLinkId);
                    if (pConn)
                    {
                        pConn->SetLastHeartBeatTime(NFGetTime());
                        pConn->SendBusHeartBeatRspMsg(m_bindConnect->GetBusId(), m_bindConnect->GetBusLength());
                        return;
                    }
                    else
                    {
                        NFLogError(NF_LOG_DEFAULT, 0, "BusMessage OnHandleMsgPeer Error, busId:{} can't find",
                                   NFServerIDUtil::GetBusNameFromBusID(fromBusId));
                    }
                }

                if (package.nMsgId == NFrame::NF_SERVER_TO_SERVER_HEART_BEAT_RSP)
                {
                    auto pConn = m_busConnectMap.GetElement(fromLinkId);
                    if (pConn)
                    {
                        pConn->SetLastHeartBeatTime(NFGetTime());
                        return;
                    }
                    else
                    {
                        NFLogError(NF_LOG_DEFAULT, 0, "BusMessage OnHandleMsgPeer Error, busId:{} can't find",
                                   NFServerIDUtil::GetBusNameFromBusID(fromBusId));
                    }
                }
            }

            if (m_recvCb)
            {
                auto pConn = m_busConnectMap.GetElement(fromLinkId);
                if (pConn)
                {
                    package.nServerLinkId = m_bindConnect->GetLinkId();
                    package.nObjectLinkId = pConn->GetLinkId();
                    m_recvCb(m_bindConnect->GetLinkId(), pConn->GetLinkId(), package);
                }
                else
                {
                    NFLogError(NF_LOG_DEFAULT, 0, "BusMessage OnHandleMsgPeer Error, busId:{} can't find",
                               NFServerIDUtil::GetBusNameFromBusID(fromBusId));
                }
            }
        }
        break;
    case eMsgType_CONNECTED:
        {
            NFMessageFlag flag;
            flag.mBusId = package.nParam1;
            flag.mBusLength = package.nParam2;
            flag.bActivityConnect = false;
            if (static_cast<uint64_t>(fromBusId) != flag.mBusId)
            {
                NFLogError(NF_LOG_DEFAULT, 0, "BusMessage fromBusId:{} != busId:{}", fromBusId, flag.mBusId);
            }

            auto pConn = m_busConnectMap.GetElement(fromLinkId);
            if (pConn == nullptr)
            {
                ConnectServer(flag);
                pConn = m_busConnectMap.GetElement(fromLinkId);
                CHECK_EXPR_MSG(pConn, "m_busConnectMap.GetElement busId:{} == NULL", flag.mBusId);
            }

            if (pConn)
            {
                pConn->SetConnected(true);
                if (package.mModuleId == NF_MODULE_FRAME && package.nMsgId == NFrame::NF_SERVER_TO_SERVER_BUS_CONNECT_REQ)
                {
                    pConn->SendBusConnectRspMsg(m_bindConnect->GetBusId(), m_bindConnect->GetBusLength());
                    if (m_eventCb)
                    {
                        package.nServerLinkId = m_bindConnect->GetLinkId();
                        package.nObjectLinkId = pConn->GetLinkId();
                        m_eventCb(type, m_bindConnect->GetLinkId(), pConn->GetLinkId());
                    }
                }
                else
                {
                    if (m_eventCb)
                    {
                        package.nServerLinkId = m_bindConnect->GetLinkId();
                        package.nObjectLinkId = pConn->GetLinkId();
                        m_eventCb(type, pConn->GetLinkId(), pConn->GetLinkId());
                    }
                }
            }
        }
        break;
    case eMsgType_DISCONNECTED:
        {
            auto pConn = m_busConnectMap.GetElement(fromLinkId);
            if (pConn)
            {
                if (m_eventCb)
                {
                    package.nServerLinkId = m_bindConnect->GetLinkId();
                    package.nObjectLinkId = pConn->GetLinkId();
                    m_eventCb(type, serverLinkId, pConn->GetLinkId());
                }
            }
        }
        break;
    default:
        break;
    }
}

int NFCBusMessage::ResumeConnect()
{
    CHECK_NULL(0, m_bindConnect);
    CHECK_NULL(0, m_bindConnect->GetShmRecord());

    auto head = (NFShmChannelHead*)m_bindConnect->GetShmRecord()->m_nBuffer;
    CHECK_NULL(0, head);
    CHECK_EXPR(head->m_nShmAddr.m_dstLinkId == m_bindConnect->GetShmRecord()->m_nUnLinkId, -1,
               "head->m_nShmAddr.mDstLinkId == m_nShmBindRecord.m_nUnLinkId");

    for (size_t i = 0; i < ARRAYSIZE(head->m_nShmAddr.m_srcLinkId); i++)
    {
        if (head->m_nShmAddr.m_srcLinkId[i] > 0)
        {
            uint64_t linkId = head->m_nShmAddr.m_srcLinkId[i];
            NFMessageFlag flag;
            flag.mBusId = GetBusIdFromUnlinkId(linkId);
            flag.mBusLength = head->m_nShmAddr.m_srcBusLength[i];
            flag.mPacketParseType = head->m_nShmAddr.m_srcParseType[i];
            bool bActivityConnect = head->m_nShmAddr.m_bActiveConnect[i];
            flag.bActivityConnect = !bActivityConnect;
            uint32_t serverType = GetServerTypeFromUnlinkId(linkId);

            NF_SHARE_PTR<NFServerData> pServerData = FindModule<NFIMessageModule>()->GetServerByServerId(m_serverType, flag.mBusId);
            if (!pServerData)
            {
                NFrame::ServerInfoReport xData;
                xData.set_bus_id(flag.mBusId);
                xData.set_bus_length(flag.mBusLength);
                xData.set_link_mode("bus");
                xData.set_server_type(serverType);
                std::string busName = NFServerIDUtil::GetBusNameFromBusID(flag.mBusId);
                std::string url = NF_FORMAT("bus://{}:{}", busName, flag.mBusLength);
                xData.set_url(url);
                pServerData = FindModule<NFIMessageModule>()->CreateServerByServerId(m_serverType, flag.mBusId, static_cast<NF_SERVER_TYPE>(serverType), xData);

                pServerData->mUnlinkId = GetUnLinkId(NF_IS_BUS, m_serverType, flag.mBusId, 0);
                FindModule<NFIMessageModule>()->CreateLinkToServer(m_serverType, flag.mBusId, pServerData->mUnlinkId);
                if (bActivityConnect)
                {
                    NFDataPackage packet;
                    packet.nSendBusLinkId = GetUnLinkId(NF_IS_BUS, serverType, flag.mBusId, 0);
                    packet.mModuleId = NF_MODULE_FRAME;
                    packet.nMsgId = NFrame::NF_SERVER_TO_SERVER_BUS_CONNECT_REQ;
                    packet.nParam1 = flag.mBusId;
                    packet.nParam2 = flag.mBusLength;

                    OnHandleMsgPeer(eMsgType_CONNECTED, pServerData->mUnlinkId, pServerData->mUnlinkId, packet);
                }
                else
                {
                    uint64_t connectLinkId = ConnectServer(flag);
                    if (connectLinkId != pServerData->mUnlinkId)
                    {
                        NFLogError(NF_LOG_DEFAULT, 0, "ReConnect Server Error, connectLinkId:{} != pServerData->mUnlinkId:{}", connectLinkId,
                                   pServerData->mUnlinkId);
                    }
                }
            }
        }
    }

    return 0;
}

int NFCBusMessage::OnTimer(uint32_t nTimerId)
{
    if (nTimerId == ENUM_SERVER_CLIENT_TIMER_HEART)
    {
        SendHeartMsg();
    }
    else if (nTimerId == ENUM_SERVER_TIMER_CHECK_HEART)
    {
        CheckServerHeartBeat();
    }
    return 0;
}

void NFCBusMessage::SendHeartMsg()
{
    auto pConn = m_busConnectMap.First();
    while (pConn)
    {
        if (pConn->IsActivityConnect() && pConn->GetConnectionType() == NF_CONNECTION_TYPE_TCP_CLIENT)
        {
            pConn->SendBusHeartBeatMsg(m_bindConnect->GetBusId(), m_bindConnect->GetBusLength());
        }
        pConn = m_busConnectMap.Next();
    }
}

void NFCBusMessage::CheckServerHeartBeat()
{
    uint64_t nowTime = NFGetTime();
    auto pConn = m_busConnectMap.First();
    while (pConn)
    {
        if (pConn->GetConnectionType() == NF_CONNECTION_TYPE_TCP_CLIENT)
        {
            if (pConn->IsActivityConnect())
            {
                //debug 30min
#ifdef NF_DEBUG_MODE
                if (pConn->GetLastHeartBeatTime() > 0 && nowTime - pConn->GetLastHeartBeatTime() > ENUM_SERVER_CLIENT_TIMER_HEART_TIME_LONGTH * 20 * 60)
                {
                    pConn->CloseLinkId();

                    NFDataPackage packet;
                    packet.nSendBusLinkId = GetUnLinkId(NF_IS_BUS, m_serverType, pConn->GetBusId(), 0);
                    packet.nServerLinkId = GetUnLinkId(NF_IS_BUS, m_serverType, m_bindConnect->GetBusId(), 0);
                    packet.nObjectLinkId = GetUnLinkId(NF_IS_BUS, m_serverType, pConn->GetBusId(), 0);
                    OnHandleMsgPeer(eMsgType_DISCONNECTED, packet.nObjectLinkId, packet.nObjectLinkId, packet);
                }
#else
                if (pConn->GetLastHeartBeatTime() > 0 && nowTime - pConn->GetLastHeartBeatTime() > ENUM_SERVER_CLIENT_TIMER_HEART_TIME_LONGTH * 20)
                {
                    pConn->CloseLinkId();

                    NFDataPackage packet;
                    packet.nSendBusLinkId = GetUnLinkId(NF_IS_BUS, m_serverType, pConn->GetBusId(), 0);
                    packet.nServerLinkId = GetUnLinkId(NF_IS_BUS, m_serverType, m_bindConnect->GetBusId(), 0);
                    packet.nObjectLinkId = GetUnLinkId(NF_IS_BUS, m_serverType, pConn->GetBusId(), 0);
                    OnHandleMsgPeer(eMsgType_DISCONNECTED, packet.nObjectLinkId, packet.nObjectLinkId, packet);
                }
#endif
            }
            else
            {
#ifdef NF_DEBUG_MODE
                if (pConn->GetLastHeartBeatTime() > 0 && nowTime - pConn->GetLastHeartBeatTime() > ENUM_SERVER_CLIENT_TIMER_HEART_TIME_LONGTH * 20 * 60)
                {
                    pConn->CloseLinkId();

                    NFDataPackage packet;
                    packet.nSendBusLinkId = GetUnLinkId(NF_IS_BUS, m_serverType, pConn->GetBusId(), 0);
                    packet.nServerLinkId = GetUnLinkId(NF_IS_BUS, m_serverType, m_bindConnect->GetBusId(), 0);
                    packet.nObjectLinkId = GetUnLinkId(NF_IS_BUS, m_serverType, pConn->GetBusId(), 0);
                    OnHandleMsgPeer(eMsgType_DISCONNECTED, packet.nServerLinkId, packet.nObjectLinkId, packet);
                }
#else
                if (pConn->GetLastHeartBeatTime() > 0 && nowTime - pConn->GetLastHeartBeatTime() > ENUM_SERVER_CLIENT_TIMER_HEART_TIME_LONGTH * 20)
                {
                    pConn->CloseLinkId();

                    NFDataPackage packet;
                    packet.nSendBusLinkId = GetUnLinkId(NF_IS_BUS, m_serverType, pConn->GetBusId(), 0);
                    packet.nServerLinkId = GetUnLinkId(NF_IS_BUS, m_serverType, m_bindConnect->GetBusId(), 0);
                    packet.nObjectLinkId = GetUnLinkId(NF_IS_BUS, m_serverType, pConn->GetBusId(), 0);
                    OnHandleMsgPeer(eMsgType_DISCONNECTED, packet.nServerLinkId, packet.nObjectLinkId, packet);
                }
#endif
            }
        }
        pConn = m_busConnectMap.Next();
    }
}

