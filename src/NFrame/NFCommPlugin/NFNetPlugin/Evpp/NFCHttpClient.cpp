// -------------------------------------------------------------------------
//    @FileName         :    NFCHttpClient.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFCHttpClient.cpp
//
// -------------------------------------------------------------------------

#include "NFCHttpClient.h"
#include "NFComm/NFPluginModule/NFCheck.h"
#include "evpp/httpc/response.h"
#include "NFComm/NFCore/NFCommon.h"

NFHttpClientMsg::NFHttpClientMsg()
{
    m_code = 0;
    m_reqId = 0;
}

void NFHttpClientMsg::Clear()
{
    m_body.clear();
    m_code = 0;
    m_reqId = 0;
}

NFHttpClientMsg::NFHttpClientMsg(const NFHttpClientMsg& msg)
{
    if (this != &msg)
    {
        m_body = msg.m_body;
        m_code = msg.m_code;
        m_reqId = msg.m_reqId;
    }
}

NFHttpClientMsg& NFHttpClientMsg::operator=(const NFHttpClientMsg& msg)
{
    if (this != &msg)
    {
        m_body = msg.m_body;
        m_code = msg.m_code;
        m_reqId = msg.m_reqId;
    }
    return *this;
}

NFCHttpClientParam::NFCHttpClientParam(int id, const HTTP_CLIENT_RESPONE& func, uint32_t timeout): m_id(id), m_resp(func)
{
    m_timeout = NF_ADJUST_TIMENOW() + timeout * 10;
}

NFCHttpClientParam::~NFCHttpClientParam()
{
}

bool NFCHttpClientParam::IsTimeOut() const
{
    return NF_ADJUST_TIMENOW() > m_timeout;
}

NFCHttpClient::NFCHttpClient()
{
    m_threadLoop.Start();
    m_staticReqId = 10000;
    m_pHttpClientParamPool = NF_NEW NFObjectPool<NFCHttpClientParam>(1000, false);
}

NFCHttpClient::~NFCHttpClient()
{
    m_threadLoop.Stop(true);
    if (m_pHttpClientParamPool)
    {
        NF_SAFE_DELETE(m_pHttpClientParamPool);
    }
}

void NFCHttpClient::HandleHttpGetResponse(const std::shared_ptr<evpp::httpc::Response>& response,
                                          const evpp::httpc::GetRequest* request)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "HttpRespone url:http://{}{} code:{} body:{}", request->host(), request->uri(),
               response->http_code(), response->body().ToString());
    NF_ASSERT(request == response->request());

    NFHttpClientMsg msg;
    msg.m_code = response->http_code();
    msg.m_body = response->body().ToString();
    msg.m_reqId = request->GetId();
    while (!m_msgQueue.Enqueue(msg))
    {
    }

    NF_SAFE_DELETE(request); // The request MUST BE deleted in EventLoop thread.
}

int NFCHttpClient::HttpGet(const string& strUri, const HTTP_CLIENT_RESPONE& respone,
                           const map<std::string, std::string>& xHeaders, int timeout)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "HttpGet uri:{} headers:{}", strUri, NFCommon::tostr(xHeaders));

    auto req = new evpp::httpc::GetRequest(m_threadLoop.loop());
    CHECK_EXPR(req, -1, "new GetRequest Failed!");

    int ret = req->Init(strUri, "", evpp::Duration(timeout));
    CHECK_RET(ret, "req->Init uri:{} posData:{} Failed, ret:{}", strUri, "", ret);

    req->SetId(m_staticReqId++);

    for (auto iter = xHeaders.begin(); iter != xHeaders.end(); ++iter)
    {
        req->AddHeader(iter->first, iter->second);
    }

    NFCHttpClientParam* pParam = m_pHttpClientParamPool->MallocObjWithArgs(req->GetId(), respone, timeout);
    CHECK_EXPR_ASSERT(pParam, -1, "m_pHttpClientParamPool->MallocObj() Failed");

    m_httpClientMap.emplace(pParam->m_id, pParam);

    req->Execute(std::bind(&NFCHttpClient::HandleHttpGetResponse, this, std::placeholders::_1, req));
    return 0;
}

int NFCHttpClient::HttpPost(const string& strUri, const string& strPostData, const HTTP_CLIENT_RESPONE& respone,
                            const map<std::string, std::string>& xHeaders, int timeout)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "HttpPost uri:{} headers:{}", strUri, NFCommon::tostr(xHeaders));

    auto req = new evpp::httpc::PostRequest(m_threadLoop.loop());
    CHECK_EXPR(req, -1, "new PostRequest Failed!");

    int ret = req->Init(strUri, strPostData, evpp::Duration(timeout));
    CHECK_RET(ret, "req->Init uri:{} posData:{} Failed", strUri, strPostData);

    req->SetId(m_staticReqId++);

    for (auto iter = xHeaders.begin(); iter != xHeaders.end(); ++iter)
    {
        req->AddHeader(iter->first, iter->second);
    }

    NFCHttpClientParam* pParam = m_pHttpClientParamPool->MallocObjWithArgs(req->GetId(), respone, timeout);
    CHECK_EXPR_ASSERT(pParam, -1, "m_pHttpClientParamPool->MallocObj() Failed");
    m_httpClientMap.emplace(pParam->m_id, pParam);

    req->Execute(std::bind(&NFCHttpClient::HandleHttpPostResponse, this, std::placeholders::_1, req));
    return 0;
}

void NFCHttpClient::HandleHttpPostResponse(const shared_ptr<evpp::httpc::Response>& response,
                                           const evpp::httpc::PostRequest* request)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "HttpRespone url:http://{}{} code:{} body:{}", request->host(), request->uri(),
               response->http_code(), response->body().ToString());
    NF_ASSERT(request == response->request());

    NFHttpClientMsg msg;
    msg.m_code = response->http_code();
    msg.m_body = response->body().ToString();
    msg.m_reqId = request->GetId();
    while (!m_msgQueue.Enqueue(msg))
    {
    }

    NF_SAFE_DELETE(request); // The request MUST BE deleted in EventLoop thread.
}

void NFCHttpClient::ProcessMsgLogicThread()
{
    int maxTimes = 10000;
    while (!m_msgQueue.IsQueueEmpty() && --maxTimes >= 0)
    {
        std::vector<NFHttpClientMsg> vecMsg;
        vecMsg.resize(200);
        m_msgQueue.TryDequeueBulk(vecMsg);
        for (size_t i = 0; i < vecMsg.size(); i++)
        {
            NFHttpClientMsg* pMsg = &vecMsg[i];
            if (pMsg)
            {
                auto iter = m_httpClientMap.find(pMsg->m_reqId);
                if (iter != m_httpClientMap.end())
                {
                    NFCHttpClientParam* pParam = iter->second;
                    if (pParam)
                    {
                        pParam->m_resp(pMsg->m_code, pMsg->m_body);
                        m_pHttpClientParamPool->FreeObj(pParam);
                    }
                    m_httpClientMap.erase(iter);
                }
            }
        }
    }
}

bool NFCHttpClient::Execute()
{
    ProcessMsgLogicThread();

    for (auto iter = m_httpClientMap.begin(); iter != m_httpClientMap.end();)
    {
        NFCHttpClientParam* pParam = iter->second;
        if (pParam)
        {
            if (!pParam->IsTimeOut())
            {
                ++iter;
                continue;
            }
            m_pHttpClientParamPool->FreeObj(pParam);
        }
        iter = m_httpClientMap.erase(iter);
    }
    return true;
}

