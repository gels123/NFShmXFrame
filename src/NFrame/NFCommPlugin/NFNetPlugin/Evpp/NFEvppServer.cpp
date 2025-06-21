
#include "NFEvppServer.h"

#include <evpp/tcp_server.h>


bool NFEvppServer::Init()
{
    m_eventLoop.reset(NF_NEW evpp::EventLoopThread());
    m_eventLoop->set_name(GetServerName(m_serverType));
    m_eventLoop->Start(true);

    std::string listenAddr;
    if (m_flag.mStrIp == "127.0.0.1")
    {
        listenAddr = NF_FORMAT("{}:{}", m_flag.mStrIp, m_flag.nPort);
    }
    else
    {
        listenAddr = NF_FORMAT("0.0.0.0:{}", m_flag.nPort);
    }

    m_tcpServer.reset(NF_NEW evpp::TCPServer(m_eventLoop->loop(), listenAddr, GetServerName(m_serverType), m_flag.nNetThreadNum, m_flag.mMaxConnectNum));
    if (!m_tcpServer)
    {
        return false;
    }

    m_connectionType = NF_CONNECTION_TYPE_TCP_SERVER;

    //链接回调是在别的线程里运行的
    m_tcpServer->SetConnectionCallback(m_connCallback);

    //消息回调是在别的线程里运行的
    m_tcpServer->SetMessageCallback(m_messageCallback);

    if (m_tcpServer->Init())
    {
        if (m_tcpServer->Start())
        {
            return true;
        }
    }
    return false;
}

bool NFEvppServer::Shut()
{
    m_eventLoop->loop()->RunAfter(100.0, [this]
    {
        m_tcpServer->Stop();
    });

    while (!m_tcpServer->IsStopped())
    {
        NFSLEEP(1);
    }

    m_eventLoop->Stop(true);

    return true;
}

bool NFEvppServer::Finalize()
{
    m_eventLoop.reset();
    m_tcpServer.reset();
    return true;
}

