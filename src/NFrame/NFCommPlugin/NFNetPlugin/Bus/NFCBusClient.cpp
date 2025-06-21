// -------------------------------------------------------------------------
//    @FileName         :    NFCBusClient.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFCBusClient.cpp
//
// -------------------------------------------------------------------------

#include "NFCBusClient.h"
#include <sstream>
#include <string.h>
#include <NFCommPlugin/NFNetPlugin/NFPacketParseMgr.h>
#include "NFComm/NFCore/NFServerIDUtil.h"
#include "NFComm/NFPluginModule/NFIMessageModule.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include "NFComm/NFPluginModule/NFNetPackagePool.h"

#include "NFComm/NFPluginModule/NFCheck.h"

NFCBusClient::~NFCBusClient()
{

}

bool NFCBusClient::Execute()
{
    return true;
}

bool NFCBusClient::Init()
{
    uint64_t linkId = ConnectServer(m_flag, m_bindFlag);
    if (linkId == 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "ConnectServer Failed!");
        return false;
    }
    return true;
}

bool NFCBusClient::Shut()
{
    return true;
}

bool NFCBusClient::Finalize()
{
    return true;
}

/**
* @brief	初始化
*
* @return 是否成功
*/
uint64_t NFCBusClient::ConnectServer(const NFMessageFlag& flag, const NFMessageFlag&)
{
    if (flag.mBusId <= 0 || flag.mBusLength <= 4096)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "busid:{} busLength:{} error!", NFServerIDUtil::GetBusNameFromBusID(flag.mBusId), flag.mBusLength);
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

    NFShmRecordType * pShmRecord = GetShmRecord();
    if (pShmRecord == nullptr)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "GetShmRecord failed, busid:{} ", NFServerIDUtil::GetBusNameFromBusID(flag.mBusId));
        return 0;
    }

    pShmRecord->m_nOwner = false;
    pShmRecord->m_nBusId = flag.mBusId;
    pShmRecord->m_nBusLength = flag.mBusLength;
    pShmRecord->m_packetParseType = flag.mPacketParseType;
    pShmRecord->m_nUnLinkId = GetUnLinkId(NF_IS_BUS, m_serverType, flag.mBusId, 0);
    SetLinkId(pShmRecord->m_nUnLinkId);
    SetConnectionType(NF_CONNECTION_TYPE_TCP_CLIENT);

    bool find = false;
    auto head = (NFShmChannelHead*)pShmRecord->m_nBuffer;
    for (size_t i = 0; i < ARRAYSIZE(head->m_nShmAddr.m_srcLinkId); i++)
    {
        if (head->m_nShmAddr.m_srcLinkId[i] == m_bindFlag.mLinkId)
        {
            find = true;
            break;
        }
    }

    if (!find)
    {
        for (size_t i = 0; i < ARRAYSIZE(head->m_nShmAddr.m_srcLinkId); i++)
        {
            if (head->m_nShmAddr.m_srcLinkId[i] == m_bindFlag.mLinkId)
            {
                break;
            }

            uint64_t curValue = 0;
            bool f = head->m_nShmAddr.m_srcLinkId[i].compare_exchange_strong(curValue, m_bindFlag.mLinkId);
            if (f)
            {
                find = true;
                head->m_nShmAddr.m_srcBusLength[i] = m_bindFlag.mBusLength;
                head->m_nShmAddr.m_srcParseType[i] = m_bindFlag.mPacketParseType;
                if (flag.bActivityConnect)
                {
                    head->m_nShmAddr.m_bActiveConnect[i] = true;
                }
                else
                {
                    head->m_nShmAddr.m_bActiveConnect[i] = false;
                }
                break;
            }
        }
    }

    if (!find)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "ConnectServer:{} failed! the bus no seat, too many connection!", NFServerIDUtil::GetBusNameFromBusID(flag.mBusId));
    }

    if (flag.bActivityConnect)
    {
        SendBusConnectMsg(m_bindFlag.mBusId, m_bindFlag.mBusLength);
    }

    return static_cast<int64_t>(pShmRecord->m_nUnLinkId);
}

bool NFCBusClient::Send(NFShmChannel* pChannel, int packetParseType, const NFDataPackage& packet, const char* msg, uint32_t nLen)
{
    m_buffer.Clear();
    NFPacketParseMgr::EnCode(packetParseType, packet, msg, nLen, m_buffer, m_bindFlag.mLinkId);

    int iRet = ShmSend(pChannel, m_buffer.ReadAddr(), m_buffer.ReadableSize());
    if (iRet == 0)
    {
        return true;
    }
    else
    {
        NFLogError(NF_LOG_DEFAULT, 0, "ShmSend from:{} to:{} error:{}", NFServerIDUtil::GetBusNameFromBusID(m_bindFlag.mBusId), NFServerIDUtil::GetBusNameFromBusID(m_flag.mBusId), iRet);
    }
    return false;
}

bool NFCBusClient::Send(NFDataPackage& packet, const char* msg, uint32_t nLen)
{
    NFShmRecordType * pShmRecord = GetShmRecord();
    if (pShmRecord == nullptr)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "GetShmRecord failed,");
        return false;
    }

    if (pShmRecord->m_nOwner == true)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "bus owner can't send data, unlinkId:{} ", pShmRecord->m_nUnLinkId);
        return false;
    }

    if (pShmRecord->m_nBuffer == nullptr)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "buffer = null, unlinkId:{} ", pShmRecord->m_nUnLinkId);
        return false;
    }


    auto head = (NFShmChannelHead*)pShmRecord->m_nBuffer;
    NFShmChannel* pChannel = nullptr; //&head->m_nShmChannel;
    if (NFGlobalSystem::Instance()->IsSpecialMsg(packet.mModuleId, packet.nMsgId))
    {
        pChannel = &head->m_nConnectChannel;
    }
    else {
        pChannel = &head->m_nShmChannel;
    }

    if (pChannel)
    {
        return Send(pChannel, pShmRecord->m_packetParseType, packet, msg, nLen);
    }

    return false;
}

bool NFCBusClient::Send(NFDataPackage& packet, const google::protobuf::Message& xData)
{
    m_sendBuffer.Clear();
    int byteSize = xData.ByteSize();
    CHECK_EXPR(static_cast<int>(m_sendBuffer.WritableSize()) >= byteSize, false, "mxSendBuffer.WritableSize():{} < byteSize:{} msg:{}", m_sendBuffer.WritableSize(), byteSize, xData.DebugString());

    auto start = reinterpret_cast<uint8_t*>(m_sendBuffer.WriteAddr());
    uint8_t* end = xData.SerializeWithCachedSizesToArray(start);
    CHECK_EXPR(end - start == byteSize, false, "xData.SerializeWithCachedSizesToArray Failed:{}", xData.DebugString());
    m_sendBuffer.Produce(byteSize);

    return Send(packet, m_sendBuffer.ReadAddr(), m_sendBuffer.ReadableSize());
}

bool NFCBusClient::IsConnected()
{
    return m_isConnected;
}

void NFCBusClient::SetConnected(bool connected)
{
    m_isConnected = connected;
}