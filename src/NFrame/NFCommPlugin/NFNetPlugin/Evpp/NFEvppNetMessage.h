// -------------------------------------------------------------------------
//    @FileName         :    NFEvppServer.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFNetPlugin
//
//
//                    .::::.
//                  .::::::::.
//                 :::::::::::  FUCK YOU
//             ..:::::::::::'
//           '::::::::::::'
//             .::::::::::
//        '::::::::::::::..
//             ..::::::::::::.
//           ``::::::::::::::::
//            ::::``:::::::::'        .:::.
//           ::::'   ':::::'       .::::::::.
//         .::::'      ::::     .:::::::'::::.
//        .:::'       :::::  .:::::::::' ':::::.
//       .::'        :::::.:::::::::'      ':::::.
//      .::'         ::::::::::::::'         ``::::.
//  ...:::           ::::::::::::'              ``::.
// ```` ':.          ':::::::::'                  ::::..
//                    '.:::::'                    ':'````..
//
// -------------------------------------------------------------------------
#pragma once

#include "NetEvppObject.h"
#include "NFCHttpClient.h"
#include "NFCHttpServer.h"
#include "NFIConnection.h"
#include "../NFINetMessage.h"
#include "evpp/buffer.h"
#include "evpp/tcp_conn.h"
#include "evpp/tcp_server.h"
#include "NFComm/NFCore/NFConcurrentQueue.h"
#include "NFComm/NFCore/NFQueue.hpp"
#include "NFComm/NFPluginModule/NFCodeQueue.h"
#include "NFComm/NFPluginModule/NFNetDefine.h"

#define EVPP_LOOP_CONTEXT_0_MAIN_THREAD_RECV 0
#define EVPP_LOOP_CONTEXT_1_MAIN_THREAD_SEND 1
#define EVPP_LOOP_CONTEXT_2_COMPRESS_BUFFER 2
#define EVPP_LOOP_CONTEXT_3_CONNPTR_MAP 3
#define EVPP_LOOP_CONTEXT_4_CODE_QUEUE_BUFFER 4


struct MsgFromNetInfo final
{
    MsgFromNetInfo()
    {
        m_type = eMsgType_Num;
        m_tcpConPtr = nullptr;
        m_serverLinkId = 0;
        m_objectLinkId = 0;
        m_pRecvBuffer = nullptr;
    }

    MsgFromNetInfo(const MsgFromNetInfo& info)
    {
        if (this != &info)
        {
            m_type = info.m_type;
            m_tcpConPtr = info.m_tcpConPtr;
            m_serverLinkId = info.m_serverLinkId;
            m_objectLinkId = info.m_objectLinkId;
            m_pRecvBuffer = info.m_pRecvBuffer;
        }
    }

    MsgFromNetInfo& operator=(const MsgFromNetInfo& info)
    {
        if (this != &info)
        {
            m_type = info.m_type;
            m_tcpConPtr = info.m_tcpConPtr;
            m_serverLinkId = info.m_serverLinkId;
            m_objectLinkId = info.m_objectLinkId;
            m_pRecvBuffer = info.m_pRecvBuffer;
        }
        return *this;
    }

    ~MsgFromNetInfo()
    {
        Clear();
    }

    void Clear()
    {
        m_type = eMsgType_Num;
        m_tcpConPtr = nullptr;
        m_serverLinkId = 0;
        m_objectLinkId = 0;
        m_pRecvBuffer = nullptr;
    }

    eMsgType m_type;
    evpp::TCPConnPtr m_tcpConPtr;
    uint64_t m_serverLinkId;
    uint64_t m_objectLinkId;
    NF_SHARE_PTR<NFBuffer> m_pRecvBuffer;
};

class NFCNetServerModule;

class NFEvppNetMessage final : public NFINetMessage
{
    friend NFCNetServerModule;

public:
    /**
     * @brief 构造函数
     */
    NFEvppNetMessage(NFIPluginManager* p, NF_SERVER_TYPE serverType);

    /**
    * @brief 析构函数
    */
    ~NFEvppNetMessage() override;

    /**
     * @brief 添加网络对象
     *
     * @return bool
     */
    NetEvppObject* AddNetObject(const evpp::TCPConnPtr& conn, uint32_t parseType, bool bSecurity);

    /**
     * @brief 添加网络对象
     *
     * @return bool
     */
    NetEvppObject* AddNetObject(uint64_t unLinkId, const evpp::TCPConnPtr& conn, uint32_t parseType, bool bSecurity);

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
    * @brief	初始化
    *
    * @return 是否成功
    */
    uint64_t BindHttpServer(uint32_t listenPort, uint32_t netThreadNum);

    /**
    * @brief 连接回调
    *
    * @return
    */
    void ConnectionCallback(const evpp::TCPConnPtr& conn, uint64_t serverLinkId);

    /**
    * @brief 消息回调
    *
    * @return 消息回调
    */
    void MessageCallback(const evpp::TCPConnPtr& conn, evpp::Buffer* msg, uint64_t serverLinkId, uint32_t packetParse, bool bSecurity);

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
     * @brief 在网络线程里运行
     * @param loop
     */
    void LoopSend(evpp::EventLoop* loop);

    /**
     * @brief 根据给定的链路ID获取网络对象
     *
     * @param linkId 链路ID，用于标识特定的网络连接
     * @return NetEvppObject* 返回与链路ID对应的网络对象指针，如果未找到则返回nullptr
     */
    NetEvppObject* GetNetObject(uint64_t linkId) const;

    /**
     * @brief 定时器回调函数，当定时器触发时调用
     *
     * @param timerId 定时器ID，用于标识特定的定时器
     * @return int 返回处理结果，通常为0表示成功，非0表示失败
     */
    int OnTimer(uint32_t timerId) override;

    /**
     * @brief 发送心跳消息，用于维持与服务器的连接
     */
    void SendHeartMsg();

    /**
     * @brief 检查服务器的心跳状态，确保连接正常
     */
    void CheckServerHeartBeat();

    /**
     * @brief 响应HTTP请求，发送指定的消息
     *
     * @param req HTTP请求句柄，包含请求的相关信息
     * @param strMsg 要发送的消息内容
     * @param code HTTP状态码，默认为WEB_OK（200）
     * @param reason HTTP状态原因短语，默认为"OK"
     * @return bool 返回响应是否成功，true表示成功，false表示失败
     */
    bool ResponseHttpMsg(const NFIHttpHandle& req, const std::string& strMsg, NFWebStatus code = WEB_OK, const std::string& reason = "OK") override;

    /**
     * @brief 响应HTTP请求，发送指定的消息
     *
     * @param requestId 请求ID，用于标识特定的HTTP请求
     * @param strMsg 要发送的消息内容
     * @param code HTTP状态码，默认为WEB_OK（200）
     * @param reason HTTP状态原因短语，默认为"OK"
     * @return bool 返回响应是否成功，true表示成功，false表示失败
     */
    bool ResponseHttpMsg(uint64_t requestId, const std::string& strMsg, NFWebStatus code = WEB_OK, const std::string& reason = "OK") override;

    /**
     * @brief 发送HTTP GET请求
     *
     * @param strUri 请求的URI
     * @param respone 响应回调函数，用于处理服务器返回的数据
     * @param xHeaders 请求头，默认为空
     * @param timeout 请求超时时间，默认为3秒
     * @return int 返回请求结果，通常为0表示成功，非0表示失败
     */
    int HttpGet(const std::string& strUri, const HTTP_CLIENT_RESPONE& respone, const std::map<std::string, std::string>& xHeaders = std::map<std::string, std::string>(), int timeout = 3) override;

    /**
     * @brief 发送HTTP POST请求
     *
     * @param strUri 请求的URI
     * @param strPostData POST请求的数据
     * @param respone 响应回调函数，用于处理服务器返回的数据
     * @param xHeaders 请求头，默认为空
     * @param timeout 请求超时时间，默认为3秒
     * @return int 返回请求结果，通常为0表示成功，非0表示失败
     */
    int HttpPost(const std::string& strUri, const std::string& strPostData, const HTTP_CLIENT_RESPONE& respone, const std::map<std::string, std::string>& xHeaders = std::map<std::string, std::string>(), int timeout = 3) override;

protected:
    /**
     * @brief 主线程处理消息队列
     */
    void ProcessMsgLogicThread();

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
     * @brief  发送数据 不包含数据头
     * @param pObject
     * @param packet  数据包
     * @param msg
     * @param nLen
     * @return true:Success false:Failure
     */
    bool Send(NetEvppObject* pObject, NFDataPackage& packet, const char* msg, uint32_t nLen);

private:
    /**
     * @brief 存储所有连接的列表，每个连接由NFIConnection指针表示。
     *
     * 该列表用于管理所有活动的连接，允许对连接进行遍历、查找和操作。
     */
    std::vector<NFIConnection*> m_connectionList;

    /**
     * @brief 用于存储空闲链接ID的并发队列。
     *
     * 该队列使用NFConcurrentQueue实现，支持多线程环境下的安全操作。
     * 空闲链接ID可以用于分配给新的连接，避免频繁的内存分配。
     */
    NFConcurrentQueue<uint64_t> m_freeLinks;

    /**
     * @brief 事件循环线程池的共享指针，用于管理多个事件循环线程。
     *
     * 该线程池基于evpp::EventLoopThreadPool实现，用于处理连接的异步事件。
     * 通过共享指针管理，确保线程池的生命周期与使用它的对象一致。
     */
    std::shared_ptr<evpp::EventLoopThreadPool> m_connectionThreadPool;

    /**
     * @brief 存储接收数据缓冲区的列表，每个缓冲区由NF_SHARE_PTR<NFBuffer>表示。
     *
     * 该列表用于管理接收到的数据缓冲区，允许对数据进行异步处理。
     * 每个缓冲区通过智能指针管理，确保资源的自动释放。
     */
    std::vector<NF_SHARE_PTR<NFBuffer>> m_recvCodeQueueList;

private:
    NFCHttpServer* m_httpServer;
#if defined(EVPP_HTTP_SERVER_SUPPORTS_SSL)
    bool m_httpServerEnableSSL;
    std::string m_httpServerCertificateChainFile;
    std::string m_httpServerPrivateKeyFile;
#endif

private:
    NFCHttpClient* m_httpClient;

private:
    /**
    * @brief 链接对象数组
    */
    std::vector<NetEvppObject*> m_netObjectArray;
    /**
     * @brief
     */
    std::unordered_map<uint64_t, NetEvppObject*> m_netObjectMap;

    /**
     * @brief 网络对象池
     */
    NFObjectPool<NetEvppObject> m_netObjectPool;

    /**
    * @brief 需要删除的连接对象
    */
    std::vector<uint64_t> m_removeObject;

    /**
    * @brief 需要消息队列
    */
    NFConcurrentQueue<MsgFromNetInfo> m_msgQueue;

    /**
    * @brief 发送BUFF
    */
    NFBuffer m_sendBuffer;

    /**
    * @brief recv BUFF
    */
    NFBuffer m_recvBuffer;

    /**
     * @brief 服务器每一帧处理的消息数
     */
    uint32_t m_handleMsgNumPerFrame;

    /**
     * @brief 服务器当前帧处理的消息数
     */
    int32_t m_curHandleMsgNum;

    std::atomic<int> m_loopSendCount;
};
