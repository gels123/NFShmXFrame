// -------------------------------------------------------------------------
//    @FileName         :    NFIBusConnect.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFIBusConnect.h
//
// -------------------------------------------------------------------------

#pragma once

#include <map>
#include "NFBusDefine.h"
#include "NFBusShm.h"
#include "../NFINetMessage.h"
#include "NFComm/NFCore/NFBuffer.h"
#include "NFComm/NFCore/NFSpinLock.h"
#include "NFComm/NFPluginModule/NFIBusModule.h"
#include "NFComm/NFPluginModule/NFNetDefine.h"

struct MsgFromBusInfo;
typedef std::function<void(eMsgType type, uint64_t connectLinkId, uint64_t objectLinkId, NFDataPackage& package)> BusMsgPeerCallback;

struct MsgFromBusInfo
{
    MsgFromBusInfo()
    {
        m_type = eMsgType_Num;
    }

    void Clear()
    {
        m_type = eMsgType_Num;
        m_packet.Clear();
    }

    eMsgType m_type;
    NFDataPackage m_packet;
};

class NFIBusConnection : public NFIModule
{
public:
    NFIBusConnection(NFIPluginManager* p, NF_SERVER_TYPE serverType, const NFMessageFlag& flag): NFIModule(p), m_connectionType(0), m_unLinkId(0), m_serverType(serverType), m_flag(flag)
    {
        m_pShmRecord = nullptr;
        m_nLastActionChannelPtr = nullptr;
        m_buffer.AssureSpace(MAX_SEND_BUFFER_SIZE);
        m_connectBuffer.AssureSpace(MAX_SEND_BUFFER_SIZE);
        m_lastHeartBeatTime = 0;
    }

    ~NFIBusConnection() override
    {
        if (m_pShmRecord)
        {
            NF_SAFE_DELETE(m_pShmRecord);
        }
    }

public:
    const NFMessageFlag& GetBindFlag() const;

    int SendBusConnectMsg(uint64_t busId, uint64_t busLength);
    int SendBusConnectRspMsg(uint64_t busId, uint64_t busLength);
    int SendBusHeartBeatMsg(uint64_t busId, uint64_t busLength);
    int SendBusHeartBeatRspMsg(uint64_t busId, uint64_t busLength);

    virtual void SetLastHeartBeatTime(uint64_t updateTime) { m_lastHeartBeatTime = updateTime; }

    virtual uint64_t GetLastHeartBeatTime() const { return m_lastHeartBeatTime; }

    /**
     * @brief 获得连接IP
     * @return std::string
     */
    virtual std::string GetLinkIp();

    /**
    * @brief 关闭连接
    * @return
    */
    virtual void CloseLinkId();

    virtual bool IsConnected() { return true; }

    virtual void SetConnected(bool connected)
    {
    }

    /**
    * @brief 通过key找到相应的共享内存
    */
    virtual NFShmChannel* GetShmChannel();

    /**
     * @brief 通过key找到相应的共享内存
     */
    virtual NFShmRecordType* GetShmRecord();

    /**
     * @brief 关闭一块共享内存
     * @return int
     */
    int CloseShmBuffer();

    /**
     * @brief 创建一块共享内存
     * @param shmKey 共享内存Key
     * @param len 共享内存长度
     * @param data
     * @param realSize
     * @param create
     * @return int
     */
    int OpenShmBuffer(key_t shmKey, size_t len, void** data, size_t* realSize, bool create);

    /**
     * @brief attach上一块共享内存
     * @param shmKey 共享内存Key
     * @param len 共享内存长度
     * @return int
     */
    virtual int AttachShm(key_t shmKey, size_t len);

    /**
     * @brief 对attach的内存进行检查
     * @param buffer
     * @param len
     * @return
     */
    virtual int AttachShmCheck(void* buffer, size_t len);

    /**
     * @brief 初始化上一块共享内存
     * @param shmKey 共享内存Key
     * @param len 共享内存长度
     * @return int
     */
    virtual int InitShm(key_t shmKey, size_t len);

    /**
     * @brief 初始化上一块共享内存
     * @param buffer
     * @param len
     * @return
     */
    virtual int InitShmBuffer(void* buffer, size_t len);

    /**
     * @brief 关闭一块共享内存
     * @return int
     */
    virtual int CloseShm();

    virtual void SetMsgPeerCallback(const BusMsgPeerCallback& cb);

    /**
     * @brief	发送数据
     *
     * @param packet
     * @param msg		发送的数据, 这里的数据已经包含了数据头
     * @param nLen	数据的大小
     * @return
     */
    virtual bool Send(NFDataPackage& packet, const char* msg, uint32_t nLen) = 0;
    virtual bool Send(NFDataPackage& packet, const google::protobuf::Message& xData) = 0;

protected:
    /**
     * @brief 发送数据
     * @return int
     */
    int ShmSend(NFShmChannel* channel, const void* buf, size_t len);

    /**
     * @brief 接受数据
     * @return int
     */
    int ShmRecv(NFShmChannel* channel, void* buf, size_t len, size_t* recvSize);

    /**
     * @brief 发送数据
     * @return int
     */
    int ShmRealSend(NFShmChannel* channel, const void* buf, size_t len);

protected:
    std::pair<size_t, size_t> LastAction();

    /**
     * @brief 获取共享内存状态
     * @return int
     */
    void ShowShmChannel(NFShmChannel* channel, std::ostream& out, bool needNodeStatus, size_t needNodeData);

    /**
     * @brief 获取共享内存状态
     * @return int
     */
    void GetShmStats(NFShmChannel* channel, NFShmStatsBlockError& out);

protected:
    /**
     * @brief 设置写超时
     */
    static int SetWriteTimeout(NFShmChannel* channel, uint64_t ms);

    /*shm_key*
     * @brief 获取写超时
     */
    static uint64_t GetWriteTimeout(NFShmChannel* channel);

    /**
     * @brief 写重复次数
     */
    static int SetWriteRetryTimes(NFShmChannel* channel, size_t times);

    /**
     * @brief 获取写重复次数
     */
    static size_t GetWriteRetryTimes(NFShmChannel* channel);

    /**
     * @brief 配置
     * @param dst
     * @param src
     */
    static void CopyConf(NFShmConf& dst, const NFShmConf& src);

    /**
     * @brief 检查标记位
     * @param flag 待操作的flag
     * @param checked 检查项
     * @return 检查结果flag
     */
    static bool CheckFlag(uint32_t flag, NFShmFlag checked) { return !!(flag & checked); }

    /**
     * @brief 设置标记位
     * @param flag 待操作的flag
     * @param checked 设置项flag
     * @return 设置结果flag
     */
    static uint32_t SetFlag(uint32_t flag, NFShmFlag checked) { return flag | checked; }

    /**
     * @brief 生存默认配置
     * @param channel
     */
    static void CreateDefaultConf(NFShmChannel* channel);

    /**
     * @brief 获取数据节点head
     * @param channel 内存通道
     * @param index 节点索引
     * @param data 数据区起始地址
     * @param dataLen 到缓冲区末尾的长度
     * @return 节点head指针
     */
    static volatile NFShmNodeHead* GetNodeHead(NFShmChannel* channel, size_t index, void** data, size_t* dataLen);

    /**
     * @brief 获取数据块head
     * @param channel 内存通道
     * @param index 节点索引
     * @param data
     * @param dataLen
     * @return 数据块head指针
     */
    static NFShmBlockHead* GetBlockHead(NFShmChannel* channel, size_t index, void** data, size_t* dataLen);

    /**
     * @brief 获取后面的数据块index
     * @param channel 内存通道
     * @param index 节点索引
     * @param offset 索引偏移
     * @return 数据块head指针
     */
    static size_t GetNextIndex(NFShmChannel* channel, size_t index, size_t offset);

    /**
     * @brief 获取可用的数据节点数量
     * @param channel 内存通道
     * @param readCur 当前读游标
     * @param writeCur 当前写游标
     * @return 可用的节点数量
     */
    static size_t GetAvailableNodeCount(NFShmChannel* channel, size_t readCur, size_t writeCur);

    /**
     * @brief 获取使用的数据节点数量
     * @param channel 内存通道
     * @param beginCur 起始游标
     * @param endCur 结束游标
     * @return 使用的数据节点数量
     */
    static size_t GetNodeRangeCount(NFShmChannel* channel, size_t beginCur, size_t endCur);

    /**
     * @brief 获取前面的的数据块index
     * @param channel 内存通道
     * @return 数据块head指针
     */
    static uint32_t FetchOperationSeq(NFShmChannel* channel);

    /**
     * @brief 计算一定长度数据需要的数据node数量
     * @param channel
     * @param len 数据长度
     * @return 数据长度需要的数据块数量
     */
    static size_t CalcNodeNum(NFShmChannel* channel, size_t len);

    /**
     * @brief 生成校验码
     * @param src 源数据
     * @param len 数据长度
     * @note Hash 快速校验
     */
    static NFDataAlignType FastCheck(const void* src, size_t len);

    // 对齐单位的大小必须是2的N次方
    static_assert(0 == (sizeof(NFDataAlignType) & (sizeof(NFDataAlignType) - 1)), "data align size must be 2^N");
    // 节点大小必须是2的N次
    static_assert(0 == ((NFShmBlock::NODE_DATA_SIZE - 1) & NFShmBlock::NODE_DATA_SIZE), "node size must be 2^N");
    // 节点大小必须是对齐单位的2的N次方倍
    static_assert(0 == (NFShmBlock::NODE_DATA_SIZE & (NFShmBlock::NODE_DATA_SIZE - sizeof(NFDataAlignType))),
                  "node size must be [data align size] * 2^N");
public:
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
    NFShmRecordType* m_pShmRecord = nullptr;
    NFBuffer m_buffer;
    NFBuffer m_connectBuffer;
    NFMessageFlag m_bindFlag;
    BusMsgPeerCallback m_busMsgPeerCb;

    /**
    * @brief 心跳包更新时间
    */
    uint64_t m_lastHeartBeatTime;

    /**
     * @brief 调试辅助信息
     * @note 不再使用tls记录调试信息，以防跨线程dump拿不到数据
     */
    size_t m_nLastActionChannelEndNodeIndex = 0;
    size_t m_nLastActionChannelBeginNodeIndex = 0;
    NFShmChannel* m_nLastActionChannelPtr = nullptr;

protected:
    uint32_t m_connectionType;

    uint64_t m_unLinkId;

    NF_SERVER_TYPE m_serverType;

    NFMessageFlag m_flag;
};
