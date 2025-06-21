// -------------------------------------------------------------------------
//    @FileName         :    InternalPacketParse.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFNetPlugin
//
// -------------------------------------------------------------------------
#include "InternalPacketParse.h"
#include "NFComm/NFPluginModule/NFNetDefine.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"

#pragma pack(push)
#pragma pack(1)

//高16位：1位压缩 + 15位模块号
//低16位：16位命令号
struct InternalMsg
{
public:
    InternalMsg() : m_cmdAndFlag(0), m_length(0), m_param1(0), m_param2(0), m_srcId(0), m_dstId(0), m_sendBusLinkId(0), m_errCode(0)
    {
    }

    void SetLength(uint32_t len)
    {
        m_length = len;
    }

    uint32_t GetLength() const
    {
        return m_length;
    }

    void SetModule(uint16_t module)
    {
        uint16_t hiWord = MMO_HIWORD(m_cmdAndFlag);
        hiWord = module | (hiWord & (1 << s_compressBitPos));
        m_cmdAndFlag = MMO_MAKELONG(MMO_LOWORD(m_cmdAndFlag), hiWord);
    }

    void SetCmd(uint16_t cmd)
    {
        m_cmdAndFlag = MMO_MAKELONG(cmd, MMO_HIWORD(m_cmdAndFlag));
    }

    uint16_t GetModule() const
    {
        uint16_t hiWord = MMO_HIWORD(m_cmdAndFlag);
        return (hiWord & (~(1 << s_compressBitPos)));
    }

    uint16_t GetCmd() const
    {
        return MMO_LOWORD(m_cmdAndFlag);
    }

    void SetCompress()
    {
        uint16_t hiWord = MMO_HIWORD(m_cmdAndFlag);
        hiWord |= (1 << s_compressBitPos);
        m_cmdAndFlag = MMO_MAKELONG(MMO_LOWORD(m_cmdAndFlag), hiWord);
    }

    void ClearCompress()
    {
        uint16_t hiWord = MMO_HIWORD(m_cmdAndFlag);
        hiWord &= ~(1 << s_compressBitPos);
        m_cmdAndFlag = MMO_MAKELONG(MMO_LOWORD(m_cmdAndFlag), hiWord);
    }

    bool IsCompressed() const
    {
        uint16_t hiWord = MMO_HIWORD(m_cmdAndFlag);
        return 0 != (hiWord & (1 << s_compressBitPos));
    }

    uint32_t m_cmdAndFlag;
    uint32_t m_length;
    uint64_t m_param1;
    uint64_t m_param2;
    uint64_t m_srcId;
    uint64_t m_dstId;
    uint64_t m_sendBusLinkId; //bus message need
    int32_t m_errCode; //主要是路由错误
};

#pragma pack(pop)

InternalPacketParse::InternalPacketParse()
{
}

int InternalPacketParse::DeCodeImpl(const char* strData, uint32_t unLen, char*& outData, uint32_t& outLen, uint32_t& allLen, NFDataPackage& recvPackage)
{
    if (strData == nullptr || unLen == 0) return 1;

    InternalMsg* packHead = nullptr;

    if (unLen < static_cast<uint32_t>(sizeof(InternalMsg)))
    {
        return 1;
    }

    packHead = (InternalMsg*)strData; //-V519
    uint32_t msgSize = packHead->m_length;
    uint32_t moduleId = packHead->GetModule();
    uint32_t tmpMsgId = packHead->GetCmd();

    if (sizeof(InternalMsg) + msgSize >= MAX_RECV_BUFFER_SIZE) //-V560
    {
        NFLogError(NF_LOG_DEFAULT, 0, "net server parse data failed, msgSize:{}, moduleId:{} msgId:{}", msgSize, moduleId, tmpMsgId);
        return -1;
    }

    if (sizeof(InternalMsg) + msgSize > unLen)
    {
        return 1;
    }

    outData = const_cast<char*>(strData + sizeof(InternalMsg));
    outLen = msgSize;
    recvPackage.mModuleId = moduleId;
    recvPackage.nMsgId = tmpMsgId;
    recvPackage.nParam1 = packHead->m_param1;
    recvPackage.nParam2 = packHead->m_param2;
    recvPackage.nSrcId = packHead->m_srcId;
    recvPackage.nDstId = packHead->m_dstId;
    recvPackage.nErrCode = packHead->m_errCode;
    recvPackage.nSendBusLinkId = packHead->m_sendBusLinkId;
    allLen = sizeof(InternalMsg) + msgSize;
    return 0;
}

int InternalPacketParse::EnCodeImpl(const NFDataPackage& recvPackage, const char* strData, uint32_t unLen, NFBuffer& buffer, uint64_t nSendBusLinkId)
{
    InternalMsg packHead;
    packHead.SetModule(recvPackage.mModuleId);
    packHead.SetCmd(recvPackage.nMsgId);
    packHead.SetLength(unLen);
    packHead.m_param1 = recvPackage.nParam1;
    packHead.m_param2 = recvPackage.nParam2;
    packHead.m_srcId = recvPackage.nSrcId;
    packHead.m_dstId = recvPackage.nDstId;
    packHead.m_sendBusLinkId = nSendBusLinkId;
    packHead.m_errCode = recvPackage.nErrCode;

    buffer.PushData(&packHead, sizeof(InternalMsg));
    buffer.PushData(strData, unLen);

    return packHead.m_length;
}
