// -------------------------------------------------------------------------
//    @FileName         :    NFCBusServer.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFCBusServer.cpp
//
// -------------------------------------------------------------------------


#include "NFCBusServer.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include "NFComm/NFPluginModule/NFIMessageModule.h"
#include "NFComm/NFPluginModule/NFNetPackagePool.h"
#include <string.h>
#include <NFCommPlugin/NFNetPlugin/NFPacketParseMgr.h>

#include "NFComm/NFPluginModule/NFIConfigModule.h"

NFCBusServer::NFCBusServer(NFIPluginManager* p, NF_SERVER_TYPE serverType, const NFMessageFlag& flag): NFIBusConnection(p, serverType, flag)
{
    m_handleMsgNumPerFrame = NF_NO_FIX_FAME_HANDLE_MAX_MSG_COUNT;
    auto pServerConfig = FindModule<NFIConfigModule>()->GetAppConfig(m_serverType);
    if (pServerConfig)
    {
        m_handleMsgNumPerFrame = pServerConfig->HandleMsgNumPerFrame;
    }
}

NFCBusServer::~NFCBusServer()
{
}

bool NFCBusServer::Execute()
{
    ProcessMsgLogicThread();
    return true;
}

bool NFCBusServer::Init()
{
    uint64_t linkId = BindServer(m_flag);
    if (linkId == 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "BindServer Failed!");
        return false;
    }

    return true;
}

bool NFCBusServer::Shut()
{
    m_busMsgPeerCb = nullptr;

    return true;
}

bool NFCBusServer::Finalize()
{
    return true;
}

/**
* @brief	初始化
*
* @return 是否成功
*/
uint64_t NFCBusServer::BindServer(const NFMessageFlag& flag)
{
    if (flag.mBusId <= 0 || flag.mBusLength <= 4096)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "busid:{} busLength:{} error!", flag.mBusId, flag.mBusLength);
        return 0;
    }

    int ret = AttachShm(static_cast<key_t>(flag.mBusId), flag.mBusLength);
    if (ret < 0)
    {
        ret = InitShm(static_cast<key_t>(flag.mBusId), flag.mBusLength);
    }

    if (ret < 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "bus init failed:{} ", ret);
        return 0;
    }

    NFShmRecordType* pShmRecord = GetShmRecord();
    if (pShmRecord == nullptr)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "GetShmRecord failed, busid:{} ", flag.mBusId);
        return 0;
    }

    pShmRecord->m_nOwner = true;
    pShmRecord->m_nBusId = flag.mBusId;
    pShmRecord->m_nBusLength = flag.mBusLength;
    pShmRecord->m_packetParseType = flag.mPacketParseType;
    pShmRecord->m_nUnLinkId = GetUnLinkId(NF_IS_BUS, m_serverType, flag.mBusId, 0);
    m_bindFlag = flag;
    m_bindFlag.mLinkId = pShmRecord->m_nUnLinkId;
    SetLinkId(pShmRecord->m_nUnLinkId);
    SetConnectionType(NF_CONNECTION_TYPE_TCP_SERVER);

    NFShmChannelHead* head = (NFShmChannelHead*)pShmRecord->m_nBuffer;
    if (head->m_nShmAddr.m_dstLinkId == 0)
    {
        head->m_nShmAddr.m_dstLinkId = pShmRecord->m_nUnLinkId;
    }
    else
    {
        if (head->m_nShmAddr.m_dstLinkId != pShmRecord->m_nUnLinkId)
        {
            NFLogError(NF_LOG_DEFAULT, 0, "shm dst linkId:{} != now linkId:{} ", head->m_nShmAddr.m_dstLinkId, pShmRecord->m_nUnLinkId);
            return 0;
        }
    }

    return pShmRecord->m_nUnLinkId;
}

/**
 * @brief 主线程处理消息队列
 */
void NFCBusServer::ProcessMsgLogicThread()
{
    size_t maxTimes = NF_NO_FIX_FAME_HANDLE_MAX_MSG_COUNT;
    if (!m_pObjPluginManager->IsLoadAllServer() && m_pObjPluginManager->IsFixedFrame())
    {
        maxTimes = NF_FIX_FRAME_HANDLE_MAX_MSG_COUNT;
    }

    NFShmRecordType* pShmRecord = GetShmRecord();
    if (pShmRecord->m_nOwner)
    {
        NFShmChannelHead* head = (NFShmChannelHead*)pShmRecord->m_nBuffer;
        NFShmChannel* pConnectChannel = &head->m_nConnectChannel;
        NFShmChannel* pChannel = &head->m_nShmChannel;
        size_t leftTimes = maxTimes;
        m_connectBuffer.Clear();
        while (true)
        {
            size_t recvLen = 0;
            int iRecvRet = ShmRecv(pConnectChannel, m_connectBuffer.WriteAddr(), m_connectBuffer.WritableSize(), &recvLen);

            if (iRecvRet == NFrame::ERR_CODE_NFBUS_ERR_NO_DATA)
            {
                break;
            }

            m_connectBuffer.Produce(recvLen);

            // 回调收到数据事件
            if (iRecvRet < 0)
            {
                NFLogError(NF_LOG_DEFAULT, 0, "Shm Recv Error:{}", GetErrorStr(iRecvRet));
                break;
            }
            else
            {
                while (true)
                {
                    char* outData = nullptr;
                    uint32_t outLen = 0;
                    uint32_t allLen = 0;
                    NFDataPackage dataPacket;
                    int iRetCode = NFPacketParseMgr::DeCode(pShmRecord->m_packetParseType, m_connectBuffer.ReadAddr(), m_connectBuffer.ReadableSize(), outData, outLen, allLen, dataPacket);
                    if (iRetCode < 0)
                    {
                        NFLogError(NF_LOG_DEFAULT, 0, "nfbus parse data failed!");
                        m_connectBuffer.Clear();
                        break;
                    }
                    else if (iRetCode > 0)
                    {
                        break;
                    }
                    else
                    {
                        m_connectBuffer.Consume(allLen);

                        dataPacket.nBuffer = outData;
                        dataPacket.nMsgLen = outLen;


                        if (dataPacket.mModuleId == NF_MODULE_FRAME && dataPacket.nMsgId == NFrame::NF_SERVER_TO_SERVER_BUS_CONNECT_REQ)
                        {
                            m_busMsgPeerCb(eMsgType_CONNECTED, dataPacket.nSendBusLinkId, dataPacket.nSendBusLinkId, dataPacket);
                        }
                        else if (dataPacket.mModuleId == NF_MODULE_FRAME && dataPacket.nMsgId == NFrame::NF_SERVER_TO_SERVER_BUS_CONNECT_RSP)
                        {
                            m_busMsgPeerCb(eMsgType_CONNECTED, dataPacket.nSendBusLinkId, dataPacket.nSendBusLinkId, dataPacket);
                        }
                        else
                        {
                            m_busMsgPeerCb(eMsgType_RECIVEDATA, dataPacket.nSendBusLinkId, dataPacket.nSendBusLinkId, dataPacket);
                        }
                    }
                }
            }
        }

        m_buffer.Clear();
        bool inited = m_pObjPluginManager->IsFinishAppTask(m_serverType, APP_INIT_TASK_GROUP_SERVER_CONNECT);
        while (leftTimes-- > 0 && inited)
        {
            size_t recvLen = 0;
            int iRecvRet = ShmRecv(pChannel, m_buffer.WriteAddr(), m_buffer.WritableSize(), &recvLen);

            if (iRecvRet == NFrame::ERR_CODE_NFBUS_ERR_NO_DATA)
            {
                break;
            }

            m_buffer.Produce(recvLen);

            // 回调收到数据事件
            if (iRecvRet < 0)
            {
                NFLogError(NF_LOG_DEFAULT, 0, "Shm Recv Error:{}", GetErrorStr(iRecvRet))
                break;
            }
            else
            {
                while (true)
                {
                    char* outData = nullptr;
                    uint32_t outLen = 0;
                    uint32_t allLen = 0;
                    NFDataPackage dataPacket;
                    int iDecodeRet = NFPacketParseMgr::DeCode(pShmRecord->m_packetParseType, m_buffer.ReadAddr(), m_buffer.ReadableSize(), outData, outLen, allLen, dataPacket);
                    if (iDecodeRet < 0)
                    {
                        NFLogError(NF_LOG_DEFAULT, 0, "nfbus parse data failed!");
                        m_buffer.Clear();
                        break;
                    }
                    else if (iDecodeRet > 0)
                    {
                        break;
                    }
                    else
                    {
                        m_buffer.Consume(allLen);

                        dataPacket.nBuffer = outData;
                        dataPacket.nMsgLen = outLen;


                        if (dataPacket.mModuleId == NF_MODULE_FRAME && dataPacket.nMsgId == NFrame::NF_SERVER_TO_SERVER_BUS_CONNECT_REQ)
                        {
                            m_busMsgPeerCb(eMsgType_CONNECTED, dataPacket.nSendBusLinkId, dataPacket.nSendBusLinkId, dataPacket);
                        }
                        else if (dataPacket.mModuleId == NF_MODULE_FRAME && dataPacket.nMsgId == NFrame::NF_SERVER_TO_SERVER_BUS_CONNECT_RSP)
                        {
                            m_busMsgPeerCb(eMsgType_CONNECTED, dataPacket.nSendBusLinkId, dataPacket.nSendBusLinkId, dataPacket);
                        }
                        else
                        {
                            m_busMsgPeerCb(eMsgType_RECIVEDATA, dataPacket.nSendBusLinkId, dataPacket.nSendBusLinkId, dataPacket);
                        }
                    }
                }
            }
        }
    }
}

bool NFCBusServer::Send(NFDataPackage& packet, const char* msg, uint32_t nLen)
{
    NFLogError(NF_LOG_DEFAULT, 0, "Bus Server Can't Send Data............., packet:{}", packet.ToString());
    return false;
}

bool NFCBusServer::Send(NFDataPackage& packet, const google::protobuf::Message& xData)
{
    NFLogError(NF_LOG_DEFAULT, 0, "Bus Server Can't Send Data............., packet:{}", packet.ToString());
    return false;
}


