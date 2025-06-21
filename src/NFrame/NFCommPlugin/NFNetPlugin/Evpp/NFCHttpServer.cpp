// -------------------------------------------------------------------------
//    @FileName         :    NFCHttpServer.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFCHttpServer.cpp
//
// -------------------------------------------------------------------------


#include <NFComm/NFPluginModule/NFCheck.h>
#include "NFCHttpServer.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include "NFComm/NFCore/NFCommon.h"
#include "evpp/http/context.h"
#include "evpp/libevent.h"
#include "NFComm/NFCore/NFTime.h"

NFServerHttpHandle::NFServerHttpHandle()
{
    m_type = NF_HTTP_REQ_GET;
    m_requestId = 0;
    m_timeOut = 0;
}

void NFServerHttpHandle::Reset()
{
    m_requestId = 0;
    m_timeOut = 0;
    m_ctx = nullptr;
    m_responseCb = nullptr;
}

void NFServerHttpHandle::AddResponseHeader(const std::string& key, const std::string& value) const
{
    if (m_ctx)
    {
        m_ctx->AddResponseHeader(key, value);
    }
}

bool NFServerHttpHandle::ResponseMsg(const std::string& strMsg, NFWebStatus code, const std::string& strReason) const
{
    AddResponseHeader("Content-Type", "application/json");
    AddResponseHeader("Access-Control-Allow-Origin", "*");

    if (m_ctx)
    {
        m_ctx->set_response_http_code(code);
    }

    if (m_responseCb)
    {
        m_responseCb(strMsg);
    }
    return true;
}

std::string NFServerHttpHandle::GetQuery(const std::string& queryKey) const
{
    if (m_ctx)
    {
        return m_ctx->GetQuery(queryKey);
    }
    return std::string();
}

NFEvppHttMsg::NFEvppHttMsg()
{
    Clear();
}

NFEvppHttMsg::~NFEvppHttMsg()
{
    Clear();
}

NFEvppHttMsg::NFEvppHttMsg(const NFEvppHttMsg& msg)
{
    if (this != &msg)
    {
        m_ctx = msg.m_ctx;
        m_responseCb = msg.m_responseCb;
    }
}

NFEvppHttMsg& NFEvppHttMsg::operator=(const NFEvppHttMsg& msg)
{
    if (this != &msg)
    {
        m_ctx = msg.m_ctx;
        m_responseCb = msg.m_responseCb;
    }
    return *this;
}

void NFEvppHttMsg::Clear()
{
    m_ctx = nullptr;
    m_responseCb = nullptr;
}

NFCHttpServer::NFCHttpServer(uint32_t serverType, uint32_t netThreadNum)
{
    m_serverType = serverType;
    m_port = 0;
    m_pHttpServer = new evpp::http::Server(netThreadNum);
    m_index = 0;
    m_listHttpRequestPool = NF_NEW NFObjectPool<NFServerHttpHandle>(1000, false);
    m_pHttpServer->RegisterDefaultHandler([this](evpp::EventLoop*,
                                                 const evpp::http::ContextPtr &ctx,
                                                 const evpp::http::HTTPSendResponseCallback& respCb)
    {
                                              NFEvppHttMsg msg;
                                              msg.m_ctx = ctx;
                                              msg.m_responseCb = respCb;
                                              while (!m_msgQueue.Enqueue(msg))
                                              {
                                              }
    });
}

NFCHttpServer::~NFCHttpServer()
{
    for (auto iter = m_httpRequestMap.begin(); iter != m_httpRequestMap.end(); ++iter)
    {
        NF_SAFE_DELETE(iter->second);
    }
    m_httpRequestMap.clear();

    if (m_listHttpRequestPool)
    {
        NF_SAFE_DELETE(m_listHttpRequestPool);
    }

    if (m_pHttpServer)
    {
        m_pHttpServer->Stop();
        NFSLEEP(1000*1000); // sleep a while to release the listening address and port
        NF_SAFE_DELETE(m_pHttpServer);
    }
}

bool NFCHttpServer::Execute()
{
    ProcessMsgLogicThread();
    std::vector<NFServerHttpHandle*> vec;
    for (auto iter = m_httpRequestMap.begin(); iter != m_httpRequestMap.end(); ++iter)
    {
        auto pRequest = iter->second;
        if (pRequest->m_timeOut + 30 <= static_cast<uint64_t>(NFGetSecondTime()))
        {
            vec.push_back(pRequest);
        }
    }

    for (size_t i = 0; i < vec.size(); i++)
    {
        NFServerHttpHandle *pRequest = vec[i];
        ResponseMsg(*pRequest, "TimeOut Error", WEB_TIMEOUT);
    }

    return true;
}

uint32_t NFCHttpServer::GetServerType() const
{
    return m_serverType;
}

bool NFCHttpServer::InitServer(int listenPort) const
{
    if (m_pHttpServer->Init(listenPort))
    {
        if (m_pHttpServer->Start())
        {
            return true;
        }
        NFLogError(NF_LOG_DEFAULT, 0, "Start Listen Port:{} Failed!", listenPort);
    }

    NFLogError(NF_LOG_DEFAULT, 0, "Init Listen Port:{} Failed!", listenPort);
    return false;
}

bool NFCHttpServer::InitServer(const std::vector<int>& listenPorts) const
{
    if (m_pHttpServer->Init(listenPorts))
    {
        if (m_pHttpServer->Start())
        {
            return true;
        }
        NFLogError(NF_LOG_DEFAULT, 0, "Start Listen Port:{} Failed!", NFCommon::tostr(listenPorts));
    }

    NFLogError(NF_LOG_DEFAULT, 0, "Init Listen Port:{} Failed!", NFCommon::tostr(listenPorts));
    return false;
}

bool NFCHttpServer::InitServer(const std::string& listenPorts/*like "80,8080,443"*/) const
{
    if (m_pHttpServer->Init(listenPorts))
    {
        if (m_pHttpServer->Start())
        {
            return true;
        }
        NFLogError(NF_LOG_DEFAULT, 0, "Start Listen Port:{} Failed!", listenPorts);
    }

    NFLogError(NF_LOG_DEFAULT, 0, "Init Listen Port:{} Failed!", listenPorts);
    return false;
}

void NFCHttpServer::ProcessMsgLogicThread()
{
    int maxTimes = 10000;
    while (!m_msgQueue.IsQueueEmpty() && maxTimes >= 0)
    {
        std::vector<NFEvppHttMsg> vecMsg;
        vecMsg.resize(200);

        m_msgQueue.TryDequeueBulk(vecMsg);
        for (size_t i = 0; i < vecMsg.size(); i++)
        {
            maxTimes--;
            NFEvppHttMsg *pMsg = &vecMsg[i];
            if (pMsg == nullptr) continue;

            NFServerHttpHandle *pRequest = AllocHttpRequest();
            pRequest->m_ctx = pMsg->m_ctx;
            pRequest->m_responseCb = pMsg->m_responseCb;
            pRequest->m_type = static_cast<NFHttpType>(pMsg->m_ctx->req()->type);
            pRequest->m_timeOut = NF_ADJUST_TIMENOW();

            m_httpRequestMap.emplace(pRequest->m_requestId, pRequest);

            bool flag = true;
            if (m_filter)
            {
                //return 401
                try
                {
                    NFWebStatus xWebStatus = m_filter(m_serverType, *pRequest);
                    if (xWebStatus != WEB_OK)
                    {
                        //401
                        ResponseMsg(*pRequest, "Filter error", xWebStatus);
                        flag = false;
                    }
                }
                catch (std::exception &e)
                {
                    ResponseMsg(*pRequest, e.what(), WEB_ERROR);
                    flag = false;
                }
                catch (...)
                {
                    ResponseMsg(*pRequest, "UNKNOW ERROR", WEB_ERROR);
                    flag = false;
                }
            }

            if (flag)
            {
                // call cb
                try
                {
                    if (m_receiveCb)
                    {
                        m_receiveCb(m_serverType, *pRequest);
                    } else
                    {
                        ResponseMsg(*pRequest, "NO PROCESSER", WEB_ERROR);
                    }
                }
                catch (std::exception &e)
                {
                    ResponseMsg(*pRequest, e.what(), WEB_ERROR);
                }
                catch (...)
                {
                    ResponseMsg(*pRequest, "UNKNOW ERROR", WEB_ERROR);
                }
            }
        }
    }
}

NFServerHttpHandle *NFCHttpServer::AllocHttpRequest()
{
    NFServerHttpHandle* pRequest = m_listHttpRequestPool->MallocObj();
    CHECK_EXPR_ASSERT(pRequest, NULL, "mListHttpRequestPool->MallocObj() Failed");

    pRequest->Reset();

    pRequest->m_requestId = ++m_index;

    return pRequest;
}

bool NFCHttpServer::ResponseMsg(const NFIHttpHandle &req, const std::string &strMsg, NFWebStatus code,
                                const std::string &strReason)
{
    req.ResponseMsg(strMsg, code, strReason);

    auto it = m_httpRequestMap.find(req.GetRequestId());
    if (it != m_httpRequestMap.end())
    {
        it->second->Reset();
        m_listHttpRequestPool->FreeObj(it->second);
        m_httpRequestMap.erase(it);
    }
    return true;
}

bool NFCHttpServer::ResponseMsg(uint64_t requestId, const std::string &strMsg, NFWebStatus code,
                                const std::string &strReason)
{
    NFServerHttpHandle* req = nullptr;
    auto it = m_httpRequestMap.find(requestId);
    if (it == m_httpRequestMap.end())
    {
        NFLogError(NF_LOG_DEFAULT, 0, "Response Msg Timeout........ requestId:{}, mStrMsg:{}", requestId, strMsg);
        return false;
    }

    req = it->second;

    bool ret = req->ResponseMsg(strMsg, code, strReason);
    if (!ret)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "Response Msg error........ requestId:{}, mStrMsg:{}", requestId, strMsg);
    }

    req->Reset();
    m_listHttpRequestPool->FreeObj(it->second);
    m_httpRequestMap.erase(it);
    return true;
}

void NFCHttpServer::SetRecvCb(const HTTP_RECEIVE_FUNCTOR& recvCb)
{
    m_receiveCb = recvCb;
}

void NFCHttpServer::SetFilterCb(const HTTP_FILTER_FUNCTOR& eventCb)
{
    m_filter = eventCb;
}

#if defined(EVPP_HTTP_SERVER_SUPPORTS_SSL)
/* berif 对指定监听端口设置SSL选项
 * param listen_port 监听的端口
 * param enable_ssl 是否开启SSL支持
 * param certificate_chain_file 证书链文件
 * param private_key_file 私钥文件
 */
void NFCHttpServer::SetPortSSLOption(int listen_port,
                      bool enable_ssl,
                      const char* certificate_chain_file,
                      const char* private_key_file)
{
    m_pHttpServer->setPortSSLOption(listen_port, enable_ssl, certificate_chain_file, private_key_file);
}

/* berif 设置端口默认SSL配置选项
 * param enable_ssl 是否开启SSL支持
 * param certificate_chain_file 证书链文件
 * param private_key_file 私钥文件
 */
void NFCHttpServer::SetPortSSLDefaultOption(
        bool enable_ssl,
        const char* certificate_chain_file,
        const char* private_key_file)
{
    m_pHttpServer->setPortSSLDefaultOption( enable_ssl, certificate_chain_file, private_key_file);
}
#endif


