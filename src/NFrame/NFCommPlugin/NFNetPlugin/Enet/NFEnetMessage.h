// -------------------------------------------------------------------------
//    @FileName         :    NFEnetMessage.h
//    @Author           :    gaoyi
//    @Date             :    2025-03-13
//    @Email			:    445267987@qq.com
//    @Module           :    NFEnetNetMessage
//
// -------------------------------------------------------------------------

#pragma once
#include <queue>
#include <enet/enet.h>
#include <NFComm/NFPluginModule/NFObjectPool.hpp>

#include "../NFINetMessage.h"

class NFCodeQueue;
class EnetObject;
class NFIEnetConnection;

struct ENetPeerData
{
    ENetPeerData()
    {
        m_objectLinkId = 0;
        m_packetParseType = 0;
        m_security = false;
    }

    uint64_t m_objectLinkId;
    uint32_t m_packetParseType;
    bool m_security;
};

class NFEnetMessage final : public NFINetMessage
{
public:
    /**
    * @brief 构造函数
    */
    NFEnetMessage(NFIPluginManager* p, NF_SERVER_TYPE serverType);

    /**
    * @brief 析构函数
    */
    ~NFEnetMessage() override;

public:
    /**
    * @brief	初始化
    *
    * @return 是否成功
    */
    uint64_t BindServer(const NFMessageFlag& flag) override;

    /**
    * @brief	初始化
    *
    * @return 是否成功
    */
    uint64_t ConnectServer(const NFMessageFlag& flag) override;

    /**
     * @brief 添加网络对象
     *
     * @return bool
     */
    EnetObject* AddNetObject(ENetPeer* pConn, uint32_t parseType, bool bSecurity);

    /**
     * @brief 添加网络对象
     *
     * @return bool
     */
    EnetObject* AddNetObject(uint64_t unLinkId, ENetPeer* pConn, uint32_t parseType, bool bSecurity);

    /**
     * @brief 根据给定的链路ID获取网络对象
     *
     * @param linkId 链路ID，用于标识特定的网络连接
     * @return NetEvppObject* 返回与链路ID对应的网络对象指针，如果未找到则返回nullptr
     */
    EnetObject* GetNetObject(uint64_t linkId) const;

public:
    /**
    * @brief 连接回调
    *
    * @return
    */
    void ConnectionCallback(ENetEventType eventType, ENetPeer* pConn, uint64_t serverLinkId);

    /**
    * @brief 消息回调
    *
    * @return 消息回调
    */
    void MessageCallback(const ENetPeer* pConn, const ENetPacket* pPacket, uint64_t serverLinkId);

    /**
     * @brief	对解析出来的数据进行处理
     *
     * @param type    数据类型，主要是为了和多线程统一处理, 主要有接受数据处理，连接成功处理，断开连接处理
     * @param serverLinkId
     * @param objectLinkId
     * @param packet
     * @return
     */
    void OnHandleMsgPeer(eMsgType type, uint64_t serverLinkId, uint64_t objectLinkId, NFDataPackage& packet);

    /**
     * @brief 处理代码队列的默认函数。
     *
     * 该函数用于处理默认的代码队列。它不接收任何参数，也不返回任何值。
     * 通常用于处理全局或默认的代码队列。
     */
    void ProcessCodeQueue();

    /**
     * @brief 处理指定的代码队列。
     *
     * 该函数用于处理传入的特定代码队列。它接收一个指向NFCodeQueue结构的指针作为参数，
     * 并对该队列中的代码进行处理。函数不返回任何值。
     *
     * @param pRecvQueue 指向要处理的NFCodeQueue结构的指针。该队列包含待处理的代码。
     */
    void ProcessCodeQueue(NFCodeQueue* pRecvQueue);

public:
    /**
    * @brief	关闭客户端
    *
    * @return  是否成功
    */
    bool Shut() override;

    /**
     * @brief 释放数据
     *
     * @return bool
     */
    bool Finalize() override;

    /**
    * @brief	服务器每帧执行
    *
    * @return	是否成功
    */
    bool Execute() override;

    /**
     * @brief 获得连接IP
     *
     * @param  usLinkId
     * @return std::string
     */
    std::string GetLinkIp(uint64_t usLinkId) override;
    uint32_t GetPort(uint64_t usLinkId) override;

    /**
    * @brief 关闭连接
    *
    * @param  usLinkId
    * @return
    */
    void CloseLinkId(uint64_t usLinkId) override;

    /**
     * @brief 获得一个可用的ID
     *
     * @return uint32_t
     */
    uint64_t GetFreeUnLinkId();

    /**
     * @brief  发送数据 不包含数据头
     * @param usLinkId
     * @param packet      数据包
     * @param msg
     * @param nLen
     * @return     true:Success false:Failure
     */
    bool Send(uint64_t usLinkId, NFDataPackage& packet, const char* msg, uint32_t nLen) override;
    bool Send(uint64_t usLinkId, NFDataPackage& packet, const google::protobuf::Message& xData) override;

    /**
     * @brief  发送数据 不包含数据头
     * @param pObject
     * @param packet  数据包
     * @param msg
     * @param nLen
     * @return true:Success false:Failure
     */
    bool Send(const EnetObject* pObject, NFDataPackage& packet, const char* msg, uint32_t nLen);

    /**
     * @brief 定时器回调函数，当定时器触发时调用
     *
     * @param timerId 定时器ID，用于标识特定的定时器
     * @return int 返回处理结果，通常为0表示成功，非0表示失败
     */
    int OnTimer(uint32_t timerId) override;
private:
    /**
     * @brief 存储所有连接的列表，每个连接由NFIConnection指针表示。
     *
     * 该列表用于管理所有活动的连接，允许对连接进行遍历、查找和操作。
     */
    std::vector<NFIEnetConnection*> m_connectionList;

    /**
     * @brief 用于存储空闲链接ID的并发队列。
     *
     * 该队列使用NFConcurrentQueue实现，支持多线程环境下的安全操作。
     * 空闲链接ID可以用于分配给新的连接，避免频繁的内存分配。
     */
    std::queue<uint64_t> m_freeLinks;
    /**
    * @brief 链接对象数组
    */
    std::vector<EnetObject*> m_netObjectArray;
    /**
     * @brief
     */
    std::unordered_map<uint64_t, EnetObject*> m_netObjectMap;

    /**
     * @brief 网络对象池
     */
    NFObjectPool<EnetObject> m_netObjectPool;

    /**
    * @brief 需要删除的连接对象
    */
    std::vector<uint64_t> m_removeObject;

    /**
    * @brief 发送BUFF
    */
    NFBuffer m_sendBuffer;
    NFBuffer m_sendComBuffer;

    /**
    * @brief recv BUFF
    */
    NFBuffer m_recvBuffer;
    NFBuffer m_recvCodeList;

    /**
     * @brief 服务器每一帧处理的消息数
     */
    uint32_t m_handleMsgNumPerFrame;

    /**
     * @brief 服务器当前帧处理的消息数
     */
    int32_t m_curHandleMsgNum;
};
