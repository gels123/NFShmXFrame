// -------------------------------------------------------------------------
//    @FileName         :    NFPluginManager.cpp
//    @Author           :    LvSheng.Huang
//    @Date             :   2022-09-18
//    @Module           :    NFPluginManager
//
// -------------------------------------------------------------------------

//#include <crtdbg.h>
#include <time.h>
#include <stdio.h>
#include <iostream>
#include <utility>
#include <thread>
#include <chrono>
#include <future>
#include <functional>
#include <atomic>

#include "NFPluginManager/NFCPluginManager.h"
#include "NFComm/NFCore/NFPlatform.h"
#include "NFPluginManager/NFProcessParameter.h"
#include "NFComm/NFPluginModule/NFGlobalSystem.h"

#if NF_PLATFORM == NF_PLATFORM_WIN
#elif NF_PLATFORM == NF_PLATFORM_LINUX
#include <sys/syscall.h>
#endif


int main(int argc, char* argv[])
{
#if NF_PLATFORM == NF_PLATFORM_WIN
	//SetConsoleOutputCP(CP_UTF8);
	SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ApplicationCrashHandler);
#elif NF_PLATFORM == NF_PLATFORM_LINUX
#endif

	ProcessParameter(argc, argv);

	std::vector<NFIPluginManager*> vecPluginManager = NFGlobalSystem::Instance()->GetPluginManagerList();

	for(int i = 0; i < (int)vecPluginManager.size(); i++)
    {
        NFIPluginManager* pPluginManager = vecPluginManager[i];
        pPluginManager->Begin();
    }

	while (true)
	{
        for(int i = 0; i < (int)vecPluginManager.size(); i++)
        {
            NFIPluginManager *pPluginManager = vecPluginManager[i];
            pPluginManager->Execute();
        }

        if (NFGlobalSystem::Instance()->IsReloadApp())
        {
            for(int i = 0; i < (int)vecPluginManager.size(); i++)
            {
                NFIPluginManager *pPluginManager = vecPluginManager[i];
                pPluginManager->SetReloadServer(true);
                pPluginManager->OnReloadConfig();
                pPluginManager->SetReloadServer(false);
                pPluginManager->AfterOnReloadConfig();
            }
            NFGlobalSystem::Instance()->SetReloadServer(false);
        }

        /*
         * stop server，停服，意味着需要保存该保存的数据，共享内存可能后面会被清理，服务器会走正常的停服流程
         * */
        if (NFGlobalSystem::Instance()->IsServerStopping())
        {
            bool bExit = true;
            for(int i = 0; i < (int)vecPluginManager.size(); i++)
            {
                NFIPluginManager *pPluginManager = vecPluginManager[i];
                pPluginManager->SetServerStopping(true);
                if (pPluginManager->StopServer() == false)
                {
                    bExit = false;
                }
            }

            if (bExit)
            {
                break;
            }
        }

        /*
         * 热更退出app, 用于服务器需要热更app代码的情况，这时候会杀掉正在运行的的的app,重启新的服务器app
         * */
        if (NFGlobalSystem::Instance()->IsHotfixServer())
        {
            bool bExit = true;
            for(int i = 0; i < (int)vecPluginManager.size(); i++)
            {
                NFIPluginManager *pPluginManager = vecPluginManager[i];
                pPluginManager->SetHotfixServer(true);
                if (pPluginManager->HotfixServer() == false)
                {
                    bExit = false;
                }
            }

            if (bExit)
            {
                break;
            }
        }
	}

    for(int i = 0; i < (int)vecPluginManager.size(); i++)
    {
        NFIPluginManager* pPluginManager = vecPluginManager[i];
        pPluginManager->End();
        NF_SAFE_DELETE(pPluginManager);
    }

    NFGlobalSystem::Instance()->ReleaseSingleton();

	return 0;
}