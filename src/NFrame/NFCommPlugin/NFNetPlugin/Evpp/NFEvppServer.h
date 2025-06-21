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

#include <evpp/event_loop_thread.h>
#include <evpp/tcp_server.h>

#include "NFIConnection.h"


class NFEvppServer final : public NFIConnection
{
public:
	NFEvppServer(NFIPluginManager* p, NF_SERVER_TYPE serverType, const NFMessageFlag& flag):NFIConnection(p, serverType, flag)
	{
		m_eventLoop = nullptr;
		m_tcpServer = nullptr;
	}

	bool Init() override;

	bool Shut() override;

	bool Finalize() override;

private:
    std::unique_ptr<evpp::EventLoopThread> m_eventLoop;
    std::unique_ptr<evpp::TCPServer> m_tcpServer;
};

