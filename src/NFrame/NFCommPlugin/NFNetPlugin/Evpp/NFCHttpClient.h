// -------------------------------------------------------------------------
//    @FileName         :    NFCHttpClient.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFCHttpClient.h
//
// -------------------------------------------------------------------------

#pragma once

#include <evpp/event_loop_thread.h>
#include <evpp/httpc/request.h>
#include <unordered_map>
#include "NFComm/NFCore/NFConcurrentQueue.h"
#include "NFComm/NFPluginModule/NFIHttpHandle.h"
#include "NFComm/NFCore/NFTime.h"
#include "NFComm/NFPluginModule/NFObjectPool.hpp"

class NFCHttpClient;

class NFHttpClientMsg
{
public:
    NFHttpClientMsg();

    void Clear();

    NFHttpClientMsg(const NFHttpClientMsg& msg);

    NFHttpClientMsg& operator=(const NFHttpClientMsg& msg);

    std::string m_body;
    int m_code;
    int m_reqId;
};

class NFCHttpClientParam
{
public:
    NFCHttpClientParam(int id, const HTTP_CLIENT_RESPONE& func, uint32_t timeout = 3);

    ~NFCHttpClientParam();

    bool IsTimeOut() const;

public:
    int m_id = 0;
    HTTP_CLIENT_RESPONE m_resp;
    uint32_t m_timeout;
};

class NFCHttpClient final
{
public:
    NFCHttpClient();

    ~NFCHttpClient();

    bool Execute();

    void ProcessMsgLogicThread();

public:
    /**
     * 处理HTTP GET请求的响应
     *
     * @param response 共享的HTTP响应对象，包含响应的状态码、头信息和身体内容
     * @param request 发出的HTTP GET请求的指针，用于访问请求的详细信息
     */
    void HandleHttpGetResponse(const std::shared_ptr<evpp::httpc::Response>& response, const evpp::httpc::GetRequest* request);

    /**
     * 处理HTTP POST请求的响应
     *
     * @param response 共享的HTTP响应对象，包含响应的状态码、头信息和身体内容
     * @param request 发出的HTTP POST请求的指针，用于访问请求的详细信息
     */
    void HandleHttpPostResponse(const std::shared_ptr<evpp::httpc::Response>& response, const evpp::httpc::PostRequest* request);

    /**
     * 发起HTTP GET请求
     *
     * @param strUri 请求的URI地址
     * @param respone 回调函数，用于处理接收到的HTTP响应
     * @param xHeaders 请求的头信息，默认为空
     * @param timeout 请求的超时时间，默认为3秒
     *
     * @return 请求的结果代码
     */
    int HttpGet(const std::string& strUri,
                const HTTP_CLIENT_RESPONE& respone,
                const std::map<std::string, std::string>& xHeaders = std::map<std::string, std::string>(),
                int timeout = 3);

    /**
     * 发起HTTP POST请求
     *
     * @param strUri 请求的URI地址
     * @param strPostData POST请求的数据内容
     * @param respone 回调函数，用于处理接收到的HTTP响应
     * @param xHeaders 请求的头信息，默认为空
     * @param timeout 请求的超时时间，默认为3秒
     *
     * @return 请求的结果代码
     */
    int HttpPost(const std::string& strUri, const std::string& strPostData, const HTTP_CLIENT_RESPONE& respone,
                 const std::map<std::string, std::string>& xHeaders = std::map<std::string, std::string>(),
                 int timeout = 3);

private:
    // 存储HTTP客户端参数的哈希映射，键为请求ID，值为HTTP客户端参数指针
    std::unordered_map<int, NFCHttpClientParam*> m_httpClientMap;

    // 事件循环线程，用于处理HTTP客户端事件
    evpp::EventLoopThread m_threadLoop;

    // 消息队列，用于存储和处理HTTP客户端消息
    NFConcurrentQueue<NFHttpClientMsg> m_msgQueue;

    // 对象池，用于管理HTTP客户端参数对象的生命周期
    NFObjectPool<NFCHttpClientParam>* m_pHttpClientParamPool;

    // 静态请求ID，用于标识和追踪HTTP请求
    int m_staticReqId;
};
