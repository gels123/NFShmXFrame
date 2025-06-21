// -------------------------------------------------------------------------
//    @FileName         :    NFEvppClient.h
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

#include "NFIConnection.h"

class NFEvppObject;

class NFEvppClient final : public NFIConnection
{
public:
	NFEvppClient(NFIPluginManager* p, NF_SERVER_TYPE serverType, const NFMessageFlag& flag):NFIConnection(p, serverType, flag)
	{
		m_eventLoop = nullptr;
		m_tcpClient = nullptr;
	}

	bool Init(evpp::EventLoop* loop);

	bool Shut() override;

	bool Finalize() override;

private:
    std::unique_ptr<evpp::EventLoopThread> m_eventLoop;
    std::shared_ptr<evpp::TCPClient> m_tcpClient;
};
