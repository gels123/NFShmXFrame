// -------------------------------------------------------------------------
//    @FileName         :    NFEvppServer.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFNetPlugin
// -------------------------------------------------------------------------

#include "NFEvppNetMessage.h"

#include <cstdint>
#include <list>
#include <string>
#include <vector>
#include <NFCommPlugin/NFNetPlugin/NFPacketParseMgr.h>

#include "NFComm/NFCore/NFCommon.h"
#include "NFComm/NFPluginModule/NFCheck.h"
#include "NFComm/NFPluginModule/NFIConfigModule.h"

#include "NFEvppClient.h"
#include "NFEvppServer.h"
#include "NFComm/NFCore/NFStringUtility.h"
#include "NFComm/NFPluginModule/NFCodeQueue.h"
#include "NFComm/NFPluginModule/NFIMessageModule.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include "NFCommPlugin/NFNetPlugin/Encrypt.h"

NFEvppNetMessage::NFEvppNetMessage(NFIPluginManager* p, NF_SERVER_TYPE serverType) : NFINetMessage(p, serverType), m_netObjectPool(1000, false)
{
    auto pServerConfig = FindModule<NFIConfigModule>()->GetAppConfig(m_serverType);
    CHECK_EXPR_ASSERT_NOT_RET(pServerConfig, "m_serverType:{} Config Not Find", m_serverType);

    m_sendBuffer.AssureSpace(MAX_SEND_BUFFER_SIZE);
    m_recvBuffer.AssureSpace(MAX_RECV_BUFFER_SIZE);
#ifdef NF_DEBUG_MODE
    SetTimer(ENUM_SERVER_CLIENT_TIMER_HEART, ENUM_SERVER_CLIENT_TIMER_HEART_TIME_LONGTH * 3);
    SetTimer(ENUM_SERVER_TIMER_CHECK_HEART, ENUM_SERVER_TIMER_CHECK_HEART_TIME_LONGTH);
#else
    SetTimer(ENUM_SERVER_CLIENT_TIMER_HEART, ENUM_SERVER_CLIENT_TIMER_HEART_TIME_LONGTH*3);
	SetTimer(ENUM_SERVER_TIMER_CHECK_HEART, ENUM_SERVER_TIMER_CHECK_HEART_TIME_LONGTH);
#endif
    m_httpServer = nullptr;
#if defined(EVPP_HTTP_SERVER_SUPPORTS_SSL)
    m_httpServerEnableSSL = false;
#endif
    m_httpClient = nullptr;

    if (m_pObjPluginManager->IsLoadAllServer())
    {
        m_connectionThreadPool.reset(NF_NEW evpp::EventLoopThreadPool(nullptr, 1));
        m_connectionThreadPool->Start(true);
    }

    /**
     * @brief 0作废，作为一个错误处理，从1开始
     */
    m_netObjectArray.resize(MAX_CLIENT_INDEX);
    for (size_t i = 1; i < m_netObjectArray.size(); i++)
    {
        m_netObjectArray[i] = nullptr;
    }

    m_handleMsgNumPerFrame = pServerConfig->HandleMsgNumPerFrame;

    for (int i = 1; i < MAX_CLIENT_INDEX; i++)
    {
        uint64_t unlinkId = GetUnLinkId(NF_IS_NET, m_serverType, pServerConfig->BusId, i);
        while (!m_freeLinks.Enqueue(unlinkId))
        {
        }
    }

    m_handleMsgNumPerFrame = NF_NO_FIX_FAME_HANDLE_MAX_MSG_COUNT;

    m_curHandleMsgNum = 0;
    m_loopSendCount = 0;
}

NFEvppNetMessage::~NFEvppNetMessage()
{
    for (auto iter = m_netObjectMap.begin(); iter != m_netObjectMap.end(); ++iter)
    {
        auto pObject = iter->second;
        if (pObject)
        {
            m_netObjectPool.FreeObj(pObject);
        }
    }
    m_netObjectMap.clear();
    for (size_t i = 1; i < m_netObjectArray.size(); i++)
    {
        m_netObjectArray[i] = nullptr;
    }
}

void NFEvppNetMessage::ProcessMsgLogicThread()
{
    m_curHandleMsgNum = m_handleMsgNumPerFrame;

    while (!m_msgQueue.IsQueueEmpty() && m_curHandleMsgNum >= 0)
    {
        std::vector<MsgFromNetInfo> vecMsg;
        vecMsg.resize(200);

        m_msgQueue.TryDequeueBulk(vecMsg);

        for (size_t index = 0; index < vecMsg.size(); index++)
        {
            m_curHandleMsgNum--;
            MsgFromNetInfo* pMsg = &vecMsg[index];
            CHECK_EXPR_ASSERT_NOT_RET(pMsg->m_tcpConPtr != NULL, "");

            if (pMsg->m_type == eMsgType_SENDBUFFER)
            {
                if (pMsg->m_pRecvBuffer)
                {
                    m_recvCodeQueueList.push_back(pMsg->m_pRecvBuffer);
                    pMsg->m_tcpConPtr->loop()->RunEvery(evpp::Duration(2500000), std::bind(&NFEvppNetMessage::LoopSend, this, pMsg->m_tcpConPtr->loop()));
                }
            }
            else if (pMsg->m_type == eMsgType_CONNECTED)
            {
                for (size_t i = 0; i < m_connectionList.size(); i++)
                {
                    if (m_connectionList[i]->GetLinkId() == pMsg->m_serverLinkId)
                    {
                        if (m_connectionList[i]->GetConnectionType() == NF_CONNECTION_TYPE_TCP_CLIENT)
                        {
                            CHECK_EXPR_ASSERT_NOT_RET(m_connectionList[i]->GetLinkId() == pMsg->m_objectLinkId, "m_connectionList[i]->GetLinkId() != pMsg->m_objectLinkId, Error..........");

                            NetEvppObject* pObject = GetNetObject(pMsg->m_objectLinkId);
                            if (pObject == nullptr)
                            {
                                pObject = AddNetObject(pMsg->m_objectLinkId, pMsg->m_tcpConPtr, m_connectionList[i]->GetPacketParseType(), m_connectionList[i]->IsSecurity());
                                CHECK_EXPR_ASSERT_NOT_RET(pObject != NULL, "AddNetObject Failed");
                            }
                            CHECK_EXPR_ASSERT_NOT_RET(m_connectionList[i]->GetLinkId() == pObject->m_usLinkId, "m_connectionList[i]->GetLinkId() != pObject->m_usLinkId, Error..........");

                            pObject->SetConnPtr(pMsg->m_tcpConPtr);
                            pObject->SetIsServer(false);
                            NFDataPackage tmpPacket;
                            OnHandleMsgPeer(eMsgType_CONNECTED, m_connectionList[i]->GetLinkId(), pObject->m_usLinkId, tmpPacket);
                        }
                        else
                        {
                            NetEvppObject* pObject = GetNetObject(pMsg->m_objectLinkId);
                            CHECK_EXPR_ASSERT_NOT_RET(pObject == NULL, "GetNetObject(pMsg->m_objectLinkId:{}) Exist", pMsg->m_objectLinkId);
                            pObject = AddNetObject(pMsg->m_objectLinkId, pMsg->m_tcpConPtr, m_connectionList[i]->GetPacketParseType(), m_connectionList[i]->IsSecurity());
                            CHECK_EXPR_ASSERT_NOT_RET(pObject != NULL, "AddNetObject Failed");

                            NFDataPackage tmpPacket;
                            OnHandleMsgPeer(eMsgType_CONNECTED, m_connectionList[i]->GetLinkId(), pObject->m_usLinkId, tmpPacket);
                        }
                        break;
                    }
                }
            }
            else if (pMsg->m_type == eMsgType_DISCONNECTED)
            {
                if (pMsg->m_objectLinkId != 0)
                {
                    /**
                     * @brief 不允许出现pMsg->nObjectLinkId找不到的情况，说明代码设置有考虑不周到的情况
                     */
                    NetEvppObject* pObject = GetNetObject(pMsg->m_objectLinkId);
                    CHECK_EXPR_ASSERT_NOT_RET(pObject != NULL, "net disconnect, tcp context error, can't find the net object:{}", pMsg->m_objectLinkId);
                    if (pObject->GetNeedRemove() == false)
                    {
                        if (pObject->IsServer())
                        {
                            pObject->SetNeedRemove(true);
                        }
                    }

                    pObject->m_connPtr = nullptr;
                    NFDataPackage tmpPacket;
                    OnHandleMsgPeer(eMsgType_DISCONNECTED, pMsg->m_serverLinkId, pMsg->m_objectLinkId, tmpPacket);
                }
                else
                {
                    /**
                     * @brief   处理客户端连接服务器掉线, 这里相当于NFClient主动连接服务器，没有连接上
                     *         这里的conn其实是一个临时的对象
                     */
                    NFLogError(NF_LOG_DEFAULT, 0, "net client:{} disconnect, can't connect the server", pMsg->m_serverLinkId);

                    NFDataPackage tmpPacket;
                    OnHandleMsgPeer(eMsgType_DISCONNECTED, pMsg->m_serverLinkId, pMsg->m_serverLinkId, tmpPacket);
                }
            }
            else
            {
                NFLogError(NF_LOG_DEFAULT, 0, "net server  error");
            }
        }
    }
}

void NFEvppNetMessage::ProcessCodeQueue(NFCodeQueue* pRecvQueue)
{
    NF_ASSERT_MSG(pRecvQueue != NULL, "pRecvQueue == NULL");
    while (pRecvQueue->HasCode() && m_curHandleMsgNum >= 0)
    {
        m_recvBuffer.Clear();
        int iCodeLen = 0;
        int iRet = pRecvQueue->Get(m_recvBuffer.WriteAddr(), m_recvBuffer.WritableSize(), iCodeLen);
        if (iRet || iCodeLen < static_cast<int>(sizeof(NFDataPackage)))
        {
            NFLogError(NF_LOG_DEFAULT, 0, "get code from pRecvQueue failed ret={}, codelen={}", iRet, iCodeLen);
            continue;
        }
        m_recvBuffer.Produce(iCodeLen);

        // 先获取NetHead
        auto pCodePackage = reinterpret_cast<NFDataPackage*>(m_recvBuffer.ReadAddr());
        if (iCodeLen != static_cast<int>(sizeof(NFDataPackage)) + static_cast<int>(pCodePackage->nMsgLen)) // 长度不一致
        {
            NFLogError(NF_LOG_DEFAULT, 0, "code length invalid. iCodeLen:{} != sizeof(NFDataPackage):{} + pCodePackage->nMsgLen:{}", iCodeLen,
                       sizeof(NFDataPackage), pCodePackage->nMsgLen);
            continue;
        }
        pCodePackage->nBuffer = m_recvBuffer.ReadAddr() + sizeof(NFDataPackage);

        NetEvppObject* pObject = GetNetObject(pCodePackage->nObjectLinkId);
        if (pObject)
        {
            OnHandleMsgPeer(eMsgType_RECIVEDATA, pCodePackage->nServerLinkId, pCodePackage->nObjectLinkId, *pCodePackage);
        }
        else
        {
            NFLogError(NF_LOG_DEFAULT, 0, "net server recv data, tcp context error");
        }
        m_curHandleMsgNum--;
    }
}


void NFEvppNetMessage::ProcessCodeQueue()
{
    m_curHandleMsgNum = m_handleMsgNumPerFrame;
    for (size_t i = 0; i < m_recvCodeQueueList.size(); i++)
    {
        NF_SHARE_PTR<NFBuffer> pRecvBuffer = m_recvCodeQueueList[i];
        if (pRecvBuffer)
        {
            auto pQueue = reinterpret_cast<NFCodeQueue*>(pRecvBuffer->ReadAddr());
            ProcessCodeQueue(pQueue);
        }
    }
}

/**
* @brief 连接回调
*
* @return
*/
void NFEvppNetMessage::ConnectionCallback(const evpp::TCPConnPtr& conn, uint64_t serverLinkId)
{
    if (conn->loop()->context(EVPP_LOOP_CONTEXT_0_MAIN_THREAD_RECV).IsEmpty())
    {
        NF_SHARE_PTR<NFBuffer> pRecvBuffer = std::make_shared<NFBuffer>();
        pRecvBuffer->AssureSpace(MAX_CODE_QUEUE_SIZE);
        pRecvBuffer->Produce(MAX_CODE_QUEUE_SIZE);
        auto pQueue = reinterpret_cast<NFCodeQueue*>(pRecvBuffer->ReadAddr());
        pQueue->Init(pRecvBuffer->ReadableSize());
        conn->loop()->set_context(EVPP_LOOP_CONTEXT_0_MAIN_THREAD_RECV, evpp::Any(pRecvBuffer));

        MsgFromNetInfo msg;
        msg.m_type = eMsgType_SENDBUFFER;
        msg.m_tcpConPtr = conn;
        msg.m_serverLinkId = serverLinkId;
        msg.m_pRecvBuffer = pRecvBuffer;
        while (!m_msgQueue.Enqueue(msg))
        {
        }
    }

    if (conn->loop()->context(EVPP_LOOP_CONTEXT_1_MAIN_THREAD_SEND).IsEmpty())
    {
        NF_SHARE_PTR<NFBuffer> pSendBuffer = std::make_shared<NFBuffer>();
        pSendBuffer->AssureSpace(MAX_CODE_QUEUE_SIZE);
        pSendBuffer->Produce(MAX_CODE_QUEUE_SIZE);
        auto pQueue = reinterpret_cast<NFCodeQueue*>(pSendBuffer->ReadAddr());
        pQueue->Init(pSendBuffer->ReadableSize());
        conn->loop()->set_context(EVPP_LOOP_CONTEXT_1_MAIN_THREAD_SEND, evpp::Any(pSendBuffer));
    }

    if (conn->loop()->context(EVPP_LOOP_CONTEXT_2_COMPRESS_BUFFER).IsEmpty())
    {
        NF_SHARE_PTR<NFBuffer> pComBuffer = std::make_shared<NFBuffer>();
        pComBuffer->AssureSpace(MAX_RECV_BUFFER_SIZE);
        conn->loop()->set_context(EVPP_LOOP_CONTEXT_2_COMPRESS_BUFFER, evpp::Any(pComBuffer));
    }

    if (conn->loop()->context(EVPP_LOOP_CONTEXT_3_CONNPTR_MAP).IsEmpty())
    {
        NF_SHARE_PTR<std::unordered_map<uint64_t, evpp::TCPConnPtr>> pConnMap = std::make_shared<std::unordered_map<uint64_t, evpp::TCPConnPtr>>();
        conn->loop()->set_context(EVPP_LOOP_CONTEXT_3_CONNPTR_MAP, evpp::Any(pConnMap));
    }

    if (conn->loop()->context(EVPP_LOOP_CONTEXT_4_CODE_QUEUE_BUFFER).IsEmpty())
    {
        NF_SHARE_PTR<NFBuffer> pCodeQueueBuffer = std::make_shared<NFBuffer>();
        pCodeQueueBuffer->AssureSpace(MAX_RECV_BUFFER_SIZE);
        conn->loop()->set_context(EVPP_LOOP_CONTEXT_4_CODE_QUEUE_BUFFER, evpp::Any(pCodeQueueBuffer));
    }

    if (conn->IsConnected())
    {
        conn->SetTCPNoDelay(true);

        NF_SHARE_PTR<std::unordered_map<uint64_t, evpp::TCPConnPtr>> pConnMap = evpp::any_cast<NF_SHARE_PTR<std::unordered_map<uint64_t, evpp::TCPConnPtr>>>(conn->loop()->context(EVPP_LOOP_CONTEXT_3_CONNPTR_MAP));
        NF_ASSERT_MSG(pConnMap != NULL,
                      "evpp::any_cast<NF_SHARE_PTR<std::unordered_map<uint64_t, evpp::TCPConnPtr>>>(conn->loop()->context(EVPP_LOOP_CONTEXT_3_CONNPTR_MAP)) Failed");

        MsgFromNetInfo msg;
        msg.m_tcpConPtr = conn;
        msg.m_serverLinkId = serverLinkId;
        msg.m_type = eMsgType_CONNECTED;

        //client connect
        if (conn->type() == evpp::TCPConn::kOutgoing)
        {
            /**
             * @brief 客户端掉线，一定会清理调conn->context(), 不如程序有问题
             */
            CHECK_EXPR_ASSERT_NOT_RET(conn->context().IsEmpty(), "conn->context().IsEmpty() Error");
            msg.m_objectLinkId = serverLinkId;
            msg.m_tcpConPtr->set_context(evpp::Any(msg.m_objectLinkId));
            CHECK_EXPR_ASSERT_NOT_RET(pConnMap->find(serverLinkId) == pConnMap->end(), "pConnMap->find(connectLinkId) == pConnMap->end() Error");
            pConnMap->emplace(serverLinkId, msg.m_tcpConPtr);
        }
        else
        {
            CHECK_EXPR_ASSERT_NOT_RET(conn->context().IsEmpty(), "conn->context().IsEmpty() Error");
            msg.m_objectLinkId = GetFreeUnLinkId();
            if (msg.m_objectLinkId == 0)
            {
                NFLogError(NF_LOG_DEFAULT, 0, "GetFreeUnLinkId() Failed, No Free UnlinkId");
                msg.m_tcpConPtr->Close();

                /**
                 * @brief 用来区分GetFreeUnLinkId Failed
                 */
                msg.m_tcpConPtr->set_context(1, evpp::Any(msg.m_objectLinkId));

                return;
            }
            msg.m_tcpConPtr->set_context(evpp::Any(msg.m_objectLinkId));
            CHECK_EXPR_ASSERT_NOT_RET(pConnMap->find(msg.m_objectLinkId) == pConnMap->end(), "pConnMap->find(connectLinkId) == pConnMap->end() Error");
            pConnMap->emplace(msg.m_objectLinkId, msg.m_tcpConPtr);
        }

        while (!m_msgQueue.Enqueue(msg))
        {
        }
    }
    else
    {
        NF_SHARE_PTR<std::unordered_map<uint64_t, evpp::TCPConnPtr>> pConnMap = evpp::any_cast<NF_SHARE_PTR<std::unordered_map<uint64_t, evpp::TCPConnPtr>>>(conn->loop()->context(EVPP_LOOP_CONTEXT_3_CONNPTR_MAP));
        NF_ASSERT_MSG(pConnMap != NULL,
                      "evpp::any_cast<NF_SHARE_PTR<std::unordered_map<uint64_t, evpp::TCPConnPtr>>>(conn->loop()->context(EVPP_LOOP_CONTEXT_3_CONNPTR_MAP)) Failed");

        MsgFromNetInfo msg;
        msg.Clear();
        msg.m_tcpConPtr = conn;
        msg.m_serverLinkId = serverLinkId;
        msg.m_type = eMsgType_DISCONNECTED;
        /**
         * @brief 处理客户端连接服务器掉线
         */
        if (conn->type() == evpp::TCPConn::kOutgoing)
        {
            if (!conn->context().IsEmpty())
            {
                msg.m_objectLinkId = evpp::any_cast<uint64_t>(conn->context());
                msg.m_tcpConPtr->set_context(evpp::Any());
                pConnMap->erase(msg.m_objectLinkId);
            }
            else
            {
                /**
                 * @brief   处理NFClient客户端连接服务器掉线, 这里相当于NFClient主动连接服务器，没有连接上, 这里的conn其实是一个临时的对象.
                 */
                CHECK_EXPR_ASSERT_NOT_RET(pConnMap->find(msg.m_serverLinkId) == pConnMap->end(), "pConnMap->find(connectLinkId) == pConnMap->end() Error");
                msg.m_objectLinkId = 0;
            }
        }
        else
        {
            /**
             * @brief 处理服务器的连接掉线
             */
            if (!conn->context().IsEmpty())
            {
                msg.m_objectLinkId = evpp::any_cast<uint64_t>(conn->context());
                msg.m_tcpConPtr->set_context(evpp::Any());
                pConnMap->erase(msg.m_objectLinkId);
            }
            else
            {
                /**
                 * @brief  GetFreeUnLinkId() Failed, NFServer的连接被主动关闭导致
                 */
                if (!conn->context(1).IsEmpty())
                {
                    NFLogError(NF_LOG_DEFAULT, 0, "GetFreeUnLinkId() Failed, net client disconnect, can't find the net context!");
                    return;
                }

                NFLogError(NF_LOG_DEFAULT, 0, "Unknow Failed, net client disconnect, can't find the net context!");
                msg.m_objectLinkId = 0;
            }
        }

        while (!m_msgQueue.Enqueue(msg))
        {
        }
    }
}

/**
* @brief 消息回调
*
* @return 消息回调
*/
void NFEvppNetMessage::MessageCallback(const evpp::TCPConnPtr& conn, evpp::Buffer* msg, uint64_t serverLinkId, uint32_t packetParse, bool bSecurity)
{
    if (msg)
    {
        if (bSecurity)
        {
            Decryption(const_cast<char*>(msg->data()), msg->size());
        }

        while (true)
        {
            char* outData = nullptr;
            uint32_t outLen = 0;
            uint32_t allLen = 0;

            NFDataPackage codePackage;
            int ret = NFPacketParseMgr::DeCode(packetParse, msg->data(), msg->size(), outData, outLen, allLen, codePackage);
            if (ret < 0)
            {
                NFLogError(NF_LOG_DEFAULT, 0, "net server parse data failed!");
                msg->Reset();
                break;
            }
            else if (ret > 0)
            {
                break;
            }
            else
            {
                if (!NFGlobalSystem::Instance()->IsSpecialMsg(codePackage.mModuleId, codePackage.nMsgId))
                {
                    NFLogTrace(NF_LOG_DEFAULT, 0, "recv msg:{} ", codePackage.ToString());
                }

                if (codePackage.bCompress)
                {
                    CHECK_EXPR_ASSERT_NOT_RET(!conn->loop()->context(EVPP_LOOP_CONTEXT_2_COMPRESS_BUFFER).IsEmpty(), "conn->loop()->context(EVPP_LOOP_CONTEXT_2_COMPRESS_BUFFER).IsEmpty()");

                    NF_SHARE_PTR<NFBuffer> pComBuffer = evpp::any_cast<NF_SHARE_PTR<NFBuffer>>(conn->loop()->context(EVPP_LOOP_CONTEXT_2_COMPRESS_BUFFER));
                    pComBuffer->Clear();

                    int decompressLen = NFPacketParseMgr::Decompress(packetParse, outData, outLen, pComBuffer->WriteAddr(), static_cast<int>(pComBuffer->WritableSize()));
                    if (decompressLen < 0)
                    {
                        NFLogError(NF_LOG_DEFAULT, 0, "recv msg:{}, decompress failed!", codePackage.ToString());
                        msg->Skip(allLen);
                        continue;
                    }
                    pComBuffer->Produce(decompressLen);

                    CHECK_EXPR_ASSERT_NOT_RET(!conn->loop()->context(EVPP_LOOP_CONTEXT_0_MAIN_THREAD_RECV).IsEmpty(), "conn->loop()->context(EVPP_LOOP_CONTEXT_0_MAIN_THREAD_RECV).IsEmpty(), Recv Code Queue Not Exist, Can't Parse Data");
                    NF_SHARE_PTR<NFBuffer> pRecvBuffer = evpp::any_cast<NF_SHARE_PTR<NFBuffer>>(conn->loop()->context(EVPP_LOOP_CONTEXT_0_MAIN_THREAD_RECV));
                    NF_ASSERT(pRecvBuffer != NULL);
                    auto pRecvQueue = reinterpret_cast<NFCodeQueue*>(pRecvBuffer->ReadAddr());
                    NF_ASSERT(pRecvQueue != NULL);

                    codePackage.nMsgLen = pComBuffer->ReadableSize();
                    codePackage.nServerLinkId = serverLinkId;
                    if (!conn->context().IsEmpty())
                    {
                        codePackage.nObjectLinkId = evpp::any_cast<uint64_t>(conn->context());
                    }
                    else
                    {
                        NFLogError(NF_LOG_DEFAULT, 0, "net server diconnect, tcp context error");
                        codePackage.nObjectLinkId = 0;
                    }
                    int iRet = pRecvQueue->Put(reinterpret_cast<const char*>(&codePackage), sizeof(NFDataPackage), pComBuffer->ReadAddr(), pComBuffer->ReadableSize());
                    if (iRet != 0)
                    {
                        if (iRet == -1)
                        {
                            NFLogError(NF_LOG_DEFAULT, 0, "pRecvQueue->Put((const char*)&codePackage, sizeof(NFDataPackage), (const char*)outData, outLen) param error");
                        }
                        else if (iRet == -2)
                        {
                            NFLogError(NF_LOG_DEFAULT, 0, "Recv Queue Full error, can't put the error");
                        }
                    }
                }
                else
                {
                    CHECK_EXPR_ASSERT_NOT_RET(!conn->loop()->context(EVPP_LOOP_CONTEXT_0_MAIN_THREAD_RECV).IsEmpty(), "conn->loop()->context(EVPP_LOOP_CONTEXT_0_MAIN_THREAD_RECV).IsEmpty(), Recv Code Queue Not Exist, Can't Parse Data");
                    NF_SHARE_PTR<NFBuffer> pRecvBuffer = evpp::any_cast<NF_SHARE_PTR<NFBuffer>>(conn->loop()->context(EVPP_LOOP_CONTEXT_0_MAIN_THREAD_RECV));
                    NF_ASSERT(pRecvBuffer != NULL);
                    auto pRecvQueue = reinterpret_cast<NFCodeQueue*>(pRecvBuffer->ReadAddr());
                    NF_ASSERT(pRecvQueue != NULL);

                    codePackage.nMsgLen = outLen;
                    codePackage.nServerLinkId = serverLinkId;
                    if (!conn->context().IsEmpty())
                    {
                        codePackage.nObjectLinkId = evpp::any_cast<uint64_t>(conn->context());
                    }
                    else
                    {
                        NFLogError(NF_LOG_DEFAULT, 0, "net server disconnect, tcp context error");
                        codePackage.nObjectLinkId = 0;
                    }

                    int iRet = pRecvQueue->Put(reinterpret_cast<const char*>(&codePackage), sizeof(NFDataPackage), outData, outLen);
                    if (iRet != 0)
                    {
                        if (iRet == -1)
                        {
                            NFLogError(NF_LOG_DEFAULT, 0, "pRecvQueue->Put((const char*)&codePackage, sizeof(NFDataPackage), (const char*)outData, outLen) param error");
                        }
                        else if (iRet == -2)
                        {
                            NFLogError(NF_LOG_DEFAULT, 0, "Recv Queue Full error, Can't Parse Data");
                        }
                    }
                }

                msg->Skip(allLen);

                if (msg->length() <= 0)
                {
                    break;
                }
            }
        }
    }
}

uint64_t NFEvppNetMessage::BindServer(const NFMessageFlag& flag)
{
    if (flag.bHttp)
    {
        int iRet = BindHttpServer(flag.nPort, flag.nNetThreadNum);
        if (iRet == 0)
        {
            NFLogError(NF_LOG_DEFAULT, 0, "BindHttpServer Failed! port:{}", flag.nPort);
        }
        return iRet;
    }
    else
    {
        auto pServer = NF_NEW NFEvppServer(m_pObjPluginManager, m_serverType, flag);

        uint64_t unLinkId = GetFreeUnLinkId();
        pServer->SetLinkId(unLinkId);
        pServer->SetConnCallback(
            std::bind(&NFEvppNetMessage::ConnectionCallback, this, std::placeholders::_1, unLinkId));
        pServer->SetMessageCallback(
            std::bind(&NFEvppNetMessage::MessageCallback, this, std::placeholders::_1, std::placeholders::_2,
                      unLinkId, flag.mPacketParseType, flag.mSecurity));
        if (pServer->Init())
        {
            m_connectionList.push_back(pServer);
            return unLinkId;
        }
    }

    return 0;
}

uint64_t NFEvppNetMessage::BindHttpServer(uint32_t listenPort, uint32_t netThreadNum)
{
    auto pServer = NF_NEW NFCHttpServer(m_serverType, netThreadNum);
    if (pServer)
    {
        pServer->SetRecvCb(m_httpReceiveCb);
        pServer->SetFilterCb(m_httpFilter);
#if defined(EVPP_HTTP_SERVER_SUPPORTS_SSL)
        pServer->SetPortSSLOption(listenPort, m_httpServerEnableSSL, m_httpServerCertificateChainFile.c_str(), m_httpServerPrivateKeyFile.c_str());
#endif
        if (pServer->InitServer(listenPort))
        {
            m_httpServer = pServer;
            return 1;
        }
    }
    return 0;
}

/**
* @brief	初始化
*
* @return 是否成功
*/
uint64_t NFEvppNetMessage::ConnectServer(const NFMessageFlag& flag)
{
    auto pClient = NF_NEW NFEvppClient(m_pObjPluginManager, m_serverType, flag);

    if (pClient)
    {
        uint64_t unLinkId = GetFreeUnLinkId();
        pClient->SetLinkId(unLinkId);
        pClient->SetConnCallback(std::bind(&NFEvppNetMessage::ConnectionCallback, this, std::placeholders::_1, unLinkId));
        pClient->SetMessageCallback(std::bind(&NFEvppNetMessage::MessageCallback, this, std::placeholders::_1, std::placeholders::_2, unLinkId, flag.mPacketParseType, flag.mSecurity));

        if (m_pObjPluginManager->IsLoadAllServer() && m_connectionThreadPool)
        {
            if (pClient->Init(m_connectionThreadPool->GetNextLoop()))
            {
                m_connectionList.push_back(pClient);

                return unLinkId;
            }
        }
        else
        {
            if (pClient->Init(nullptr))
            {
                m_connectionList.push_back(pClient);

                return unLinkId;
            }
        }
    }
    return 0;
}

std::string NFEvppNetMessage::GetLinkIp(uint64_t usLinkId)
{
    NetEvppObject* pObject = GetNetObject(usLinkId);
    if (pObject)
    {
        return pObject->GetStrIp();
    }
    else
    {
        NFLogError(NF_LOG_DEFAULT, 0, "GetNetObject Failed, usLinkId:{}", usLinkId);
    }

    return std::string("");
}

uint32_t NFEvppNetMessage::GetPort(uint64_t usLinkId)
{
    NetEvppObject* pObject = GetNetObject(usLinkId);
    if (pObject)
    {
        return pObject->GetPort();
    }
    else
    {
        NFLogError(NF_LOG_DEFAULT, 0, "GetNetObject Failed, usLinkId:{}", usLinkId);
    }
    return 0;
}

NetEvppObject* NFEvppNetMessage::AddNetObject(const evpp::TCPConnPtr& conn, uint32_t parseType, bool bSecurity)
{
    uint64_t usLinkId = GetFreeUnLinkId();
    if (usLinkId == 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "GetFreeUnLinkId Failed, connected count:{}  Can't add connect", m_netObjectArray.size());
        return nullptr;
    }

    return AddNetObject(usLinkId, conn, parseType, bSecurity);
}

NetEvppObject* NFEvppNetMessage::AddNetObject(uint64_t unLinkId, const evpp::TCPConnPtr& conn, uint32_t parseType, bool bSecurity)
{
    int index = GetServerIndexFromUnlinkId(unLinkId);
    CHECK_EXPR_ASSERT(index > 0 && index < (int)m_netObjectArray.size(), NULL, "unLinkId:{} index:{} > 0 && index < m_netObjectArray.size()", unLinkId, index);
    CHECK_EXPR_ASSERT(m_netObjectArray[index] == NULL, NULL, "unLinkId:{} index:{} Exist", unLinkId, index);
    CHECK_EXPR_ASSERT(m_netObjectMap.find(unLinkId) == m_netObjectMap.end(), NULL, "unLinkId:{} index:{} Exist", unLinkId, index);

    auto pObject = m_netObjectPool.MallocObjWithArgs(conn);
    CHECK_EXPR_ASSERT(pObject, NULL, "m_netObjectPool.Alloc() Failed");

    m_netObjectArray[index] = pObject;
    m_netObjectMap.emplace(unLinkId, pObject);

    pObject->SetLinkId(unLinkId);

    if (conn)
    {
        std::string remoteAddr = conn->remote_addr();
        std::vector<std::string> vec;
        NFStringUtility::Split(remoteAddr, ":", &vec);
        if (vec.size() >= 2)
        {
            pObject->SetStrIp(vec[0]);
            pObject->SetPort(NFCommon::strto<uint32_t>(vec[1]));
        }
    }

    pObject->SetPacketParseType(parseType);
    pObject->SetSecurity(bSecurity);

    return pObject;
}

NetEvppObject* NFEvppNetMessage::GetNetObject(uint64_t linkId) const
{
    uint32_t serverType = GetServerTypeFromUnlinkId(linkId);
    CHECK_EXPR(serverType == m_serverType, NULL, "serverType != m_serverType, this usLinkId:{} is not of the server:{}", linkId, GetServerName(m_serverType).c_str());

    int index = GetServerIndexFromUnlinkId(linkId);
    CHECK_EXPR_ASSERT(index > 0 && index < (int)m_netObjectArray.size(), NULL, "unLinkId:{} index:{} > 0 && index < m_netObjectArray.size()", linkId, index);

    auto pObject = m_netObjectArray[index];

    if (pObject)
    {
        return pObject;
    }

    return nullptr;
}

void NFEvppNetMessage::CloseLinkId(uint64_t usLinkId)
{
    auto pObject = GetNetObject(usLinkId);
    if (pObject)
    {
        if (pObject->m_isServer == false)
        {
            for (auto iter = m_connectionList.begin(); iter != m_connectionList.end(); ++iter)
            {
                NFIConnection* pConnection = *iter;
                if (pConnection->GetConnectionType() == NF_CONNECTION_TYPE_TCP_CLIENT)
                {
                    if (pConnection->GetLinkId() == usLinkId)
                    {
                        pConnection->Shut();
                        pConnection->Finalize();
                        NF_SAFE_DELETE(pConnection);
                        m_connectionList.erase(iter);
                        break;
                    }
                }
            }
        }

        pObject->SetNeedRemove(true);
        pObject->CloseObject();
    }
    else
    {
        NFLogError(NF_LOG_DEFAULT, 0, "can't find the unLinkId:{}", usLinkId);
    }
}

uint64_t NFEvppNetMessage::GetFreeUnLinkId()
{
    if (!m_freeLinks.IsQueueEmpty())
    {
        uint64_t unlinkId = 0;
        if (m_freeLinks.TryDequeue(unlinkId))
        {
            return unlinkId;
        }
    }

    NFLogError(NF_LOG_DEFAULT, 0, "GetFreeUnLinkId failed!");
    return 0;
}

bool NFEvppNetMessage::Shut()
{
    for (size_t i = 0; i < m_connectionList.size(); i++)
    {
        NFIConnection* pConn = m_connectionList[i];
        if (pConn)
        {
            pConn->Shut();
        }
    }


    return true;
}

bool NFEvppNetMessage::Finalize()
{
    for (size_t i = 0; i < m_connectionList.size(); i++)
    {
        NFIConnection* pConn = m_connectionList[i];
        if (pConn)
        {
            pConn->Finalize();
        }
    }

    for (size_t i = 0; i < m_connectionList.size(); i++)
    {
        NFIConnection* pConn = m_connectionList[i];
        if (pConn)
        {
            NF_SAFE_DELETE(pConn);
        }
    }

    if (m_connectionThreadPool)
    {
        m_connectionThreadPool->Stop(true);
        NF_ASSERT(m_connectionThreadPool->IsStopped());
        m_connectionThreadPool->Join();
        m_connectionThreadPool.reset();
    }

    m_recvCodeQueueList.clear();

    return true;
}

bool NFEvppNetMessage::Execute()
{
    ProcessMsgLogicThread();
    ProcessCodeQueue();
    if (m_httpServer)
    {
        m_httpServer->Execute();
    }
    if (m_httpClient)
    {
        m_httpClient->Execute();
    }

    return true;
}

bool NFEvppNetMessage::Send(uint64_t usLinkId, NFDataPackage& packet, const char* msg, uint32_t nLen)
{
    NetEvppObject* pObject = GetNetObject(usLinkId);
    if (pObject)
    {
        return Send(pObject, packet, msg, nLen);
    }
    else
    {
        NFLogErrorIf(pObject == NULL, NF_LOG_DEFAULT, 0, "GetNetObject Failed, usLinkId:{}", usLinkId);
    }

    return false;
}

bool NFEvppNetMessage::Send(uint64_t usLinkId, NFDataPackage& packet, const google::protobuf::Message& xData)
{
    NetEvppObject* pObject = GetNetObject(usLinkId);
    if (pObject)
    {
        m_sendBuffer.Clear();
        int byteSize = xData.ByteSize();
        CHECK_EXPR(static_cast<int>(m_sendBuffer.WritableSize()) >= byteSize, false, "m_sendBuffer.WritableSize():{} < byteSize:{} msg:{}", m_sendBuffer.WritableSize(), byteSize, xData.DebugString());

        auto start = reinterpret_cast<uint8_t*>(m_sendBuffer.WriteAddr());
        uint8_t* end = xData.SerializeWithCachedSizesToArray(start);
        CHECK_EXPR(end - start == byteSize, false, "xData.SerializeWithCachedSizesToArray Failed:{}", xData.DebugString());
        m_sendBuffer.Produce(byteSize);

        return Send(pObject, packet, m_sendBuffer.ReadAddr(), m_sendBuffer.ReadableSize());
    }
    else
    {
        NFLogErrorIf(pObject == NULL, NF_LOG_DEFAULT, 0, "GetNetObject Failed, usLinkId:{}", usLinkId);
    }

    return false;
}

void NFEvppNetMessage::OnHandleMsgPeer(eMsgType type, uint64_t serverLinkId, uint64_t objectLinkId, NFDataPackage& packet)
{
    switch (type)
    {
    case eMsgType_RECIVEDATA:
        {
            if (packet.mModuleId == NF_MODULE_FRAME)
            {
                if (packet.nMsgId == NFrame::NF_SERVER_TO_SERVER_HEART_BEAT)
                {
                    NetEvppObject* pObject = GetNetObject(objectLinkId);
                    if (pObject && pObject->m_isServer)
                    {
                        pObject->SetLastHeartBeatTime(NFGetTime());
                        NFDataPackage tempPacket;
                        tempPacket.mModuleId = NF_MODULE_FRAME;
                        tempPacket.nMsgId = NFrame::NF_SERVER_TO_SERVER_HEART_BEAT_RSP;
                        Send(pObject->GetLinkId(), tempPacket, nullptr, 0);
                        return;
                    }
                    else
                    {
                        NFLogErrorIf(pObject == NULL, NF_LOG_DEFAULT, 0, "GetNetObject Failed, usLinkId:{}", objectLinkId);
                    }
                }

                if (packet.nMsgId == NFrame::NF_SERVER_TO_SERVER_HEART_BEAT_RSP)
                {
                    NetEvppObject* pObject = GetNetObject(objectLinkId);
                    if (pObject && pObject->m_isServer == false)
                    {
                        pObject->SetLastHeartBeatTime(NFGetTime());
                        return;
                    }
                    else
                    {
                        NFLogErrorIf(pObject == NULL, NF_LOG_DEFAULT, 0, "GetNetObject Failed, usLinkId:{}", objectLinkId);
                    }
                }
            }

            if (m_recvCb)
            {
                m_recvCb(serverLinkId, objectLinkId, packet);
            }
        }
        break;
    case eMsgType_CONNECTED:
        {
            if (m_eventCb)
            {
                m_eventCb(type, serverLinkId, objectLinkId);
            }
        }
        break;
    case eMsgType_DISCONNECTED:
        {
            if (m_eventCb)
            {
                m_eventCb(type, serverLinkId, objectLinkId);
            }

            if (objectLinkId > 0)
            {
                uint32_t serverType = GetServerTypeFromUnlinkId(objectLinkId);
                NF_ASSERT_MSG(serverType == m_serverType, "the unlinkId is not of the server");

                uint32_t index = GetServerIndexFromUnlinkId(objectLinkId);
                /**
                 * @brief 处理特殊情况，比如客户端主动连接服务器，连接不上
                 */
                if (index == 0)
                {
                    return;
                }

                auto pObject = GetNetObject(objectLinkId);
                if (pObject && pObject->GetNeedRemove())
                {
                    CHECK_EXPR_ASSERT_NOT_RET(index > 0 && index < m_netObjectArray.size(), "unLinkId:{} index:{} Error", objectLinkId, index);
                    CHECK_EXPR_ASSERT_NOT_RET(m_netObjectMap.find(objectLinkId) != m_netObjectMap.end(), "unLinkId:{} index:{} Error", objectLinkId, index);
                    m_netObjectArray[index] = nullptr;
                    m_netObjectPool.FreeObj(pObject);
                    m_netObjectMap.erase(objectLinkId);
                    while (!m_freeLinks.Enqueue(objectLinkId))
                    {
                    }
                }
                else
                {
                    NFLogErrorIf(pObject == NULL, NF_LOG_DEFAULT, 0, "GetNetObject Failed, usLinkId:{}", objectLinkId);
                }
            }
        }
        break;
    default:
        break;
    }
}

bool NFEvppNetMessage::Send(NetEvppObject* pObject, NFDataPackage& packet, const char* msg, uint32_t nLen)
{
    if (pObject && !pObject->GetNeedRemove() && pObject->m_connPtr && pObject->m_connPtr->IsConnected())
    {
        packet.nPacketParseType = pObject->m_packetParseType;
        packet.isSecurity = pObject->IsSecurity();
        packet.nObjectLinkId = pObject->GetLinkId();
        packet.nMsgLen = nLen;
        CHECK_EXPR_ASSERT(!pObject->m_connPtr->loop()->context(EVPP_LOOP_CONTEXT_1_MAIN_THREAD_SEND).IsEmpty(), false, "pConn->loop()->context(EVPP_LOOP_CONTEXT_1_MAIN_THREAD_SEND).IsEmpty() ERROR");
        NF_SHARE_PTR<NFBuffer> pSendBuffer = evpp::any_cast<NF_SHARE_PTR<NFBuffer>>(pObject->m_connPtr->loop()->context(EVPP_LOOP_CONTEXT_1_MAIN_THREAD_SEND));
        CHECK_EXPR_ASSERT(pSendBuffer != NULL, false, "evpp::any_cast<NF_SHARE_PTR<NFBuffer>>(pObject->mConnPtr->loop()->context(EVPP_LOOP_CONTEXT_1_MAIN_THREAD_SEND) NULL");
        auto pSendQueue = reinterpret_cast<NFCodeQueue*>(pSendBuffer->ReadAddr());
        CHECK_EXPR_ASSERT(pSendQueue != NULL, false, "(NFCodeQueue*)pSendBuffer->ReadAddr() NULL");

        int iRet = pSendQueue->Put(reinterpret_cast<const char*>(&packet), sizeof(NFDataPackage), msg, nLen);
        if (iRet != 0)
        {
            if (iRet == -1)
            {
                NFLogError(NF_LOG_DEFAULT, 0, "pSendQueue->Put((const char*)&codePackage, sizeof(NFDataPackage), (const char*)msg, nLen) param error, package:({}) drop msg", packet.ToString());
            }
            else if (iRet == -2)
            {
                NFLogError(NF_LOG_DEFAULT, 0, "Send Queue Full error, can't put the error, package:({}) drop msg", packet.ToString());
            }
            pObject->m_connPtr->loop()->RunInLoop(std::bind(&NFEvppNetMessage::LoopSend, this, pObject->m_connPtr->loop()));
        }

        if (m_loopSendCount.load() <= 0)
        {
            ++m_loopSendCount;
            pObject->m_connPtr->loop()->RunInLoop(std::bind(&NFEvppNetMessage::LoopSend, this, pObject->m_connPtr->loop()));
        }

        return true;
    }

    return false;
}

int NFEvppNetMessage::OnTimer(uint32_t timerId)
{
    if (timerId == ENUM_SERVER_CLIENT_TIMER_HEART)
    {
        SendHeartMsg();
    }
    else if (timerId == ENUM_SERVER_TIMER_CHECK_HEART)
    {
        CheckServerHeartBeat();
    }
    return 0;
}

void NFEvppNetMessage::SendHeartMsg()
{
    for (size_t i = 0; i < m_connectionList.size(); i++)
    {
        if (m_connectionList[i] && m_connectionList[i]->GetConnectionType() == NF_CONNECTION_TYPE_TCP_CLIENT && GetNetObject(m_connectionList[i]->GetLinkId()) != nullptr)
        {
            NFDataPackage packet;
            packet.mModuleId = NF_MODULE_FRAME;
            packet.nMsgId = NFrame::NF_SERVER_TO_SERVER_HEART_BEAT;
            Send(m_connectionList[i]->GetLinkId(), packet, nullptr, 0);
        }
    }
}

void NFEvppNetMessage::CheckServerHeartBeat()
{
    uint64_t nowTime = NFGetTime();
    for (auto iter = m_netObjectMap.begin(); iter != m_netObjectMap.end(); ++iter)
    {
        NetEvppObject* pObject = iter->second;
        if (pObject && pObject->m_isServer && pObject->m_packetParseType == PACKET_PARSE_TYPE_INTERNAL) //服务器与服务器之间有这个需求，协议必须是内网协议
        {
            //debug 30min
#ifdef NF_DEBUG_MODE
            if (pObject->m_lastHeartBeatTime > 0 && nowTime - pObject->m_lastHeartBeatTime > ENUM_SERVER_CLIENT_TIMER_HEART_TIME_LONGTH * 20 * 60)
            {
                pObject->CloseObject();
            }
#else
            if (pObject->mLastHeartBeatTime > 0 && nowTime - pObject->mLastHeartBeatTime > ENUM_SERVER_CLIENT_TIMER_HEART_TIME_LONGTH * 20)
			{
				pObject->CloseObject();
			}
#endif
        }
    }
}

bool NFEvppNetMessage::ResponseHttpMsg(const NFIHttpHandle& req, const string& strMsg,
                                       NFWebStatus code, const string& reason)
{
    if (m_httpServer)
    {
        return m_httpServer->ResponseMsg(req, strMsg, code, reason);
    }
    return false;
}

bool NFEvppNetMessage::ResponseHttpMsg(uint64_t requestId, const string& strMsg,
                                       NFWebStatus code, const string& reason)
{
    if (m_httpServer)
    {
        return m_httpServer->ResponseMsg(requestId, strMsg, code, reason);
    }
    return false;
}

int NFEvppNetMessage::HttpGet(const string& strUri, const HTTP_CLIENT_RESPONE& respone,
                              const map<std::string, std::string>& xHeaders, int timeout)
{
    if (!m_httpClient)
    {
        m_httpClient = NF_NEW NFCHttpClient();
    }

    return m_httpClient->HttpGet(strUri, respone, xHeaders, timeout);
}

int NFEvppNetMessage::HttpPost(const string& strUri, const string& strPostData, const HTTP_CLIENT_RESPONE& respone,
                               const map<std::string, std::string>& xHeaders, int timeout)
{
    if (!m_httpClient)
    {
        m_httpClient = NF_NEW NFCHttpClient();
    }

    return m_httpClient->HttpPost(strUri, strPostData, respone, xHeaders, timeout);
}

void NFEvppNetMessage::LoopSend(evpp::EventLoop* loop)
{
    --m_loopSendCount;
    CHECK_EXPR_ASSERT_NOT_RET(loop != NULL, "loop == NULL ERROR");
    CHECK_EXPR_ASSERT_NOT_RET(!loop->context(EVPP_LOOP_CONTEXT_1_MAIN_THREAD_SEND).IsEmpty(), "loop->context(EVPP_LOOP_CONTEXT_1_MAIN_THREAD_SEND).IsEmpty() ERROR");
    NF_SHARE_PTR<NFBuffer> pSendBuffer = evpp::any_cast<NF_SHARE_PTR<NFBuffer>>(loop->context(EVPP_LOOP_CONTEXT_1_MAIN_THREAD_SEND));
    CHECK_EXPR_ASSERT_NOT_RET(pSendBuffer != NULL, "evpp::any_cast<NF_SHARE_PTR<NFBuffer>>(loop->context(EVPP_LOOP_CONTEXT_1_MAIN_THREAD_SEND) NULL");
    auto pSendQueue = reinterpret_cast<NFCodeQueue*>(pSendBuffer->ReadAddr());
    CHECK_EXPR_ASSERT_NOT_RET(pSendQueue != NULL, "(NFCodeQueue*)pSendBuffer->ReadAddr() NULL");

    CHECK_EXPR_ASSERT_NOT_RET(!loop->context(EVPP_LOOP_CONTEXT_2_COMPRESS_BUFFER).IsEmpty(), "loop->context(EVPP_LOOP_CONTEXT_2_COMPRESS_BUFFER).IsEmpty() ERROR");
    NF_SHARE_PTR<NFBuffer> pComBuffer = evpp::any_cast<NF_SHARE_PTR<NFBuffer>>(loop->context(EVPP_LOOP_CONTEXT_2_COMPRESS_BUFFER));
    CHECK_EXPR_ASSERT_NOT_RET(pComBuffer != NULL, "pComBuffer NULL");

    CHECK_EXPR_ASSERT_NOT_RET(!loop->context(EVPP_LOOP_CONTEXT_4_CODE_QUEUE_BUFFER).IsEmpty(), "loop->context(EVPP_LOOP_CONTEXT_4_CODE_QUEUE_BUFFER).IsEmpty() ERROR");
    NF_SHARE_PTR<NFBuffer> pCodeQueueBuffer = evpp::any_cast<NF_SHARE_PTR<NFBuffer>>(loop->context(EVPP_LOOP_CONTEXT_4_CODE_QUEUE_BUFFER));
    CHECK_EXPR_ASSERT_NOT_RET(pCodeQueueBuffer != NULL, "pCodeQueueBuffer NULL");

    while (pSendQueue->HasCode())
    {
        pCodeQueueBuffer->Clear();
        int iCodeLen = 0;
        int iRet = pSendQueue->Get(pCodeQueueBuffer->WriteAddr(), pCodeQueueBuffer->WritableSize(), iCodeLen);
        if (iRet || iCodeLen < static_cast<int>(sizeof(NFDataPackage)))
        {
            NFLogError(NF_LOG_DEFAULT, 0, "get code from pRecvQueue failed ret={}, codelen={}", iRet, iCodeLen);
            continue;
        }
        pCodeQueueBuffer->Produce(iCodeLen);

        // 先获取NetHead
        auto pCodePackage = reinterpret_cast<NFDataPackage*>(pCodeQueueBuffer->ReadAddr());
        if (iCodeLen != static_cast<int>(sizeof(NFDataPackage)) + static_cast<int>(pCodePackage->nMsgLen)) // 长度不一致
        {
            NFLogError(NF_LOG_DEFAULT, 0, "code length invalid. iCodeLen:{} != sizeof(NFDataPackage):{} + pCodePackage->nMsgLen:{}", iCodeLen,
                       sizeof(NFDataPackage), pCodePackage->nMsgLen);
            continue;
        }

        uint32_t parsePackageType = pCodePackage->nPacketParseType;
        bool isSecurity = pCodePackage->isSecurity;

        NF_SHARE_PTR<std::unordered_map<uint64_t, evpp::TCPConnPtr>> pConnMap = evpp::any_cast<NF_SHARE_PTR<std::unordered_map<uint64_t, evpp::TCPConnPtr>>>(loop->context(EVPP_LOOP_CONTEXT_3_CONNPTR_MAP));
        CHECK_EXPR_ASSERT_NOT_RET(pConnMap != NULL, "evpp::any_cast<NF_SHARE_PTR<std::unordered_map<uint64_t, evpp::TCPConnPtr>>>(loop->context(EVPP_LOOP_CONTEXT_3_CONNPTR_MAP)) Failed");

        auto iter = pConnMap->find(pCodePackage->nObjectLinkId);
        if (iter == pConnMap->end())
        {
            NFLogError(NF_LOG_DEFAULT, 0, "pConnMap->find(pCodePackage->m_objectLinkId) Failed, objectLinkId:{} maybe disconnect", pCodePackage->nObjectLinkId);
            continue;
        }

        evpp::TCPConnPtr pConn = iter->second;

        pComBuffer->Clear();
        NFPacketParseMgr::EnCode(parsePackageType, *pCodePackage, pCodeQueueBuffer->ReadAddr() + sizeof(NFDataPackage), pCodePackage->nMsgLen, *pComBuffer);
        pCodeQueueBuffer->Clear();

        if (isSecurity)
        {
            Encryption(pComBuffer->ReadAddr(), pComBuffer->ReadableSize());
        }

        pConn->Send(pComBuffer->ReadAddr(), pComBuffer->ReadableSize());

        pComBuffer->Clear();
    }
}
