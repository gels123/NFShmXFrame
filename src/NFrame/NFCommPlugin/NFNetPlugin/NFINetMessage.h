// -------------------------------------------------------------------------
//    @FileName         :    NFIServer.h
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

#include "NFComm/NFCore/NFMutex.h"
#include "NFComm/NFPluginModule/NFIDynamicModule.h"
#include "NFComm/NFPluginModule/NFIHttpHandle.h"
#include "NFComm/NFPluginModule/NFNetDefine.h"

enum EnumServerHeartTimer
{
    ENUM_SERVER_CLIENT_TIMER_HEART = 1, //定时发送心跳
    ENUM_SERVER_TIMER_CHECK_HEART = 2, //服务器定时坚持心跳包
    ENUM_SERVER_CLIENT_TIMER_HEART_TIME_LONGTH = 1000, //定时发送心跳时间长度 1000ms
    ENUM_SERVER_TIMER_CHECK_HEART_TIME_LONGTH = 1000, //定时发送心跳时间长度 3000ms
};

class NFINetMessage : public NFIDynamicModule
{
    friend class NFCNetMessageDriverModule;

public:
    /**
     * @brief 构造函数
     */
    NFINetMessage(NFIPluginManager* p, NF_SERVER_TYPE serverType) : NFIDynamicModule(p), m_serverType(serverType), m_netObjectMaxIndex(0)
    {
        NF_ASSERT(serverType > NF_ST_NONE && serverType < NF_ST_MAX);
    }

    /**
    * @brief 析构函数
    */
    ~NFINetMessage() override
    {
    }

    /**
     *@brief  设置接收回调.
     */
    template <typename BaseType>
    void SetRecvCb(BaseType* pBaseType,
                   void (BaseType::*handleRecv)(uint64_t connectLinkId, uint64_t objectLinkId, uint64_t valueId,
                                                uint32_t nMsgId, const char* msg, uint32_t nLen))
    {
        m_recvCb = std::bind(handleRecv, pBaseType, std::placeholders::_1, std::placeholders::_2,
                             std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6);
    }

    /**
     *@brief  设置连接事件回调.
     */
    template <typename BaseType>
    void SetEventCb(BaseType* pBaseType,
                    void (BaseType::*handleEvent)(eMsgType nEvent, uint64_t connectLinkId, uint64_t objectLinkId))
    {
        m_eventCb = std::bind(handleEvent, pBaseType, std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3);
    }

    /**
    *@brief  设置接收回调.
    */
    template <typename BaseType>
    void SetHttpRecvCb(BaseType* pBaseType, bool (BaseType::*handleRecv)(uint32_t, const NFIHttpHandle& req))
    {
        m_httpReceiveCb = std::bind(handleRecv, pBaseType, std::placeholders::_1, std::placeholders::_2);
    }

    /**
     *@brief  设置连接事件回调.
     */
    template <typename BaseType>
    void SetHttpFilterCb(BaseType* pBaseType, NFWebStatus (BaseType::*handleFilter)(uint32_t, const NFIHttpHandle& req))
    {
        m_httpFilter = std::bind(handleFilter, pBaseType, std::placeholders::_1, std::placeholders::_2);
    }

    /**
     *@brief  设置接收回调.
     */
    void SetRecvCb(const NET_CALLBACK_RECEIVE_FUNCTOR& recvCb)
    {
        m_recvCb = recvCb;
    }

    /**
     *@brief  设置连接事件回调.
     */
    void SetEventCb(const NET_CALLBACK_EVENT_FUNCTOR& eventCb)
    {
        m_eventCb = eventCb;
    }

    /**
     *@brief  设置接收回调.
     */
    void SetHttpRecvCb(const HTTP_RECEIVE_FUNCTOR& recvCb)
    {
        m_httpReceiveCb = recvCb;
    }

    /**
     *@brief  设置连接事件回调.
     */
    void SetHttpFilterCb(const HTTP_FILTER_FUNCTOR& eventCb)
    {
        m_httpFilter = eventCb;
    }

    /**
     * 纯虚函数，用于绑定服务器
     *
     * @param flag 消息标志，包含关于连接类型或协议的信息
     * @return 成功绑定后的链接ID，如果失败则返回0
     */
    virtual uint64_t BindServer(const NFMessageFlag& flag) = 0;

    /**
     * 纯虚函数，用于连接服务器
     *
     * @param flag 消息标志，包含关于连接类型或协议的信息
     * @return 成功连接后的链接ID，如果失败则返回0
     */
    virtual uint64_t ConnectServer(const NFMessageFlag& flag) = 0;

    /**
     * 纯虚函数，用于发送数据包通过指定链接
     *
     * @param linkId 链接ID，标识数据应发送到的链接
     * @param packet 数据包，包含要发送的数据
     * @param msg 附加消息，可以是任意字符串数据
     * @param nLen 附加消息的长度
     * @return 发送成功返回true，否则返回false
     */
    virtual bool Send(uint64_t linkId, NFDataPackage& packet, const char* msg, uint32_t nLen) = 0;

    /**
     * 纯虚函数，用于发送数据包通过指定链接，此版本用于发送protobuf消息
     *
     * @param linkId 链接ID，标识数据应发送到的链接
     * @param packet 数据包，包含要发送的数据
     * @param xData protobuf消息，包含结构化数据
     * @return 发送成功返回true，否则返回false
     */
    virtual bool Send(uint64_t linkId, NFDataPackage& packet, const google::protobuf::Message& xData) = 0;

    /**
     * 纯虚函数，用于获取指定链接的IP地址
     *
     * @param linkId 链接ID，标识链接
     * @return 返回链接的IP地址字符串，如果失败则返回空字符串
     */
    virtual std::string GetLinkIp(uint64_t linkId) = 0;

    /**
     * 纯虚函数，用于获取指定链接的端口号
     *
     * @param linkId 链接ID，标识链接
     * @return 返回链接的端口号，如果失败则返回0
     */
    virtual uint32_t GetPort(uint64_t linkId) = 0;

    /**
     * 纯虚函数，用于关闭与指定链接ID相关的连接或会话
     *
     * @param linkId 需要关闭的链接的唯一标识符
     */
    virtual void CloseLinkId(uint64_t linkId) = 0;

    /**
     * 获取服务器类型
     *
     * @return 服务器类型的整数值
     */
    virtual uint32_t GetServerType() const { return m_serverType; }

    /**
     * 响应HTTP消息，使用NFIHttpHandle对象
     *
     * @param req HTTP请求的处理对象
     * @param strMsg 响应的消息内容
     * @param code HTTP状态码，默认为WEB_OK
     * @param reason 状态码的描述，默认为"OK"
     * @return 响应是否成功
     */
    virtual bool ResponseHttpMsg(const NFIHttpHandle& req, const std::string& strMsg, NFWebStatus code = WEB_OK, const std::string& reason = "OK") { return false; }

    /**
     * 响应HTTP消息，使用请求ID
     *
     * @param requestId HTTP请求的唯一标识符
     * @param strMsg 响应的消息内容
     * @param code HTTP状态码，默认为WEB_OK
     * @param reason 状态码的描述，默认为"OK"
     * @return 响应是否成功
     */
    virtual bool ResponseHttpMsg(uint64_t requestId, const std::string& strMsg, NFWebStatus code = WEB_OK, const std::string& reason = "OK") { return false; }

    /**
     * 执行HTTP GET请求。
     *
     * @param strUri 请求的URI。
     * @param respone 回调函数，用于处理服务器的响应。
     * @param xHeaders 在请求中添加的额外HTTP头，默认为空。
     * @param timeout 请求的超时时间（秒），默认为3秒。
     * @return -1，表示未实现或出错。
     *
     * 注意：此函数提供了一个默认实现，返回-1，表示未实现或出错。
     */
    virtual int HttpGet(const std::string& strUri, const HTTP_CLIENT_RESPONE& respone, const std::map<std::string, std::string>& xHeaders = std::map<std::string, std::string>(), int timeout = 3) { return -1; }

    /**
     * 执行HTTP POST请求。
     *
     * @param strUri 请求的URI。
     * @param strPostData POST请求的数据。
     * @param respone 回调函数，用于处理服务器的响应。
     * @param xHeaders 在请求中添加的额外HTTP头，默认为空。
     * @param timeout 请求的超时时间（秒），默认为3秒。
     * @return -1，表示未实现或出错。
     *
     * 注意：此函数提供了一个默认实现，返回-1，表示未实现或出错。
     */
    virtual int HttpPost(const std::string& strUri, const std::string& strPostData, const HTTP_CLIENT_RESPONE& respone, const std::map<std::string, std::string>& xHeaders = std::map<std::string, std::string>(), int timeout = 3) { return -1; }

    /**
     * 尝试恢复连接。
     *
     * @return 0，表示未实现或默认行为。
     *
     * 注意：此函数提供了一个默认实现，返回0，表示未实现或默认行为。
     */
    virtual int ResumeConnect() { return 0; }

protected:
    /**
     * @brief	处理接受数据的回调
     */
    NET_CALLBACK_RECEIVE_FUNCTOR m_recvCb;

    /**
     * @brief	网络事件回调
     */
    NET_CALLBACK_EVENT_FUNCTOR m_eventCb;

    /**
    * @brief 服务器类型
    */
    NF_SERVER_TYPE m_serverType;

    /**
    * @brief 当前链接对象最大索引
    */
    uint32_t m_netObjectMaxIndex;

    /**
    * @brief	HTTP处理接受数据的回调
    */
    HTTP_RECEIVE_FUNCTOR m_httpReceiveCb;
    /**
    * @brief	HTTP处理接受数据的回调
    */
    HTTP_FILTER_FUNCTOR m_httpFilter;
};
