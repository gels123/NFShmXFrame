// -------------------------------------------------------------------------
//    @FileName         :    NFPluginManager.cpp
//    @Author           :    LvSheng.Huang
//    @Date             :   2022-09-18
//    @Module           :    NFPluginManager
//
// -------------------------------------------------------------------------

#include <cstdio>
#include <iostream>
#include <functional>
#include <atomic>

#include "NFCrashHandlerMgr.h"
#include "NFPluginManager/NFCPluginManager.h"
#include "NFComm/NFCore/NFPlatform.h"
#include "NFPluginManager/NFProcessParameter.h"
#include "NFComm/NFPluginModule/NFGlobalSystem.h"
#include "NFSignalHandleMgr.h"

#if NF_PLATFORM == NF_PLATFORM_WIN
#elif NF_PLATFORM == NF_PLATFORM_LINUX
#include <sys/syscall.h>
#endif


int c_main(int argc, char* argv[])
{
    // 平台相关初始化
#if NF_PLATFORM == NF_PLATFORM_WIN
    // 设置未处理异常过滤器（Windows专用崩溃处理）
    SetUnhandledExceptionFilter(ApplicationCrashHandler);
#elif NF_PLATFORM == NF_PLATFORM_LINUX
    // Linux平台预留初始化位置（未来可扩展系统信号处理等）
#endif

    // 处理命令行参数（解析服务器ID、配置文件路径等关键参数）
    ProcessParameter(argc, argv);

    // 从全局系统获取所有插件管理器实例
    std::vector<NFIPluginManager*> vecPluginManager = NFGlobalSystem::Instance()->GetPluginManagerList();

    // 初始化所有插件管理器
    for (int i = 0; i < static_cast<int>(vecPluginManager.size()); i++)
    {
        NFIPluginManager* pPluginManager = vecPluginManager[i];
        pPluginManager->Begin(); // 执行插件管理器的初始化流程
    }

    // 主服务循环
    while (true)
    {
        // 执行所有插件管理器的每帧逻辑
        for (int i = 0; i < static_cast<int>(vecPluginManager.size()); i++)
        {
            NFIPluginManager* pPluginManager = vecPluginManager[i];
            pPluginManager->Execute(); // 驱动插件模块的主逻辑
        }

        // 配置重载处理
        if (NFGlobalSystem::Instance()->IsReloadApp())
        {
            for (int i = 0; i < static_cast<int>(vecPluginManager.size()); i++)
            {
                NFIPluginManager* pPluginManager = vecPluginManager[i];
                // 配置重载三部曲：设置标记->执行重载->清理标记
                pPluginManager->SetReloadServer(true);
                pPluginManager->OnReloadConfig(); // 加载新配置
                pPluginManager->SetReloadServer(false);
                pPluginManager->AfterOnReloadConfig(); // 重载后处理
            }
            NFGlobalSystem::Instance()->SetReloadServer(false); // 重置全局重载标记
        }

        // 服务停止处理（正常停服流程）
        if (NFGlobalSystem::Instance()->IsServerStopping() || NFGlobalSystem::Instance()->IsServerKilling())
        {
            bool bExit = true;
            // 正常停止流程
            if (NFGlobalSystem::Instance()->IsServerStopping() && !NFGlobalSystem::Instance()->IsServerKilling())
            {
                NFLogInfo(NF_LOG_DEFAULT, 0, "Main Loop Stop................");
                for (int i = 0; i < static_cast<int>(vecPluginManager.size()); i++)
                {
                    NFIPluginManager* pPluginManager = vecPluginManager[i];
                    pPluginManager->SetServerStopping(true);
                    if (!pPluginManager->StopServer())
                    {
                        // 执行停服逻辑
                        bExit = false; // 存在未完成停服的插件管理器
                    }
                }
            }
            // 强制终止流程
            else
            {
                NFLogInfo(NF_LOG_DEFAULT, 0, "Main Loop Killed................");
                if (NFGlobalSystem::Instance()->IsServerKilling())
                {
                    for (int i = 0; i < static_cast<int>(vecPluginManager.size()); i++)
                    {
                        NFIPluginManager* pPluginManager = vecPluginManager[i];
                        if (!pPluginManager->OnServerKilling())
                        {
                            // 执行强制终止
                            bExit = false;
                        }
                    }
                }
            }

            // 全部插件管理器完成停服后退出循环
            if (bExit)
            {
                NFLogInfo(NF_LOG_DEFAULT, 0, "Main Loop Exit, Will Release All................");
                break;
            }
        }

        // 热更新处理（动态代码替换）
        if (NFGlobalSystem::Instance()->IsHotfixServer())
        {
            NFLogInfo(NF_LOG_DEFAULT, 0, "Main Hotfix Server................");
            bool bHotFail = false;
            for (int i = 0; i < static_cast<int>(vecPluginManager.size()); i++)
            {
                NFIPluginManager* pPluginManager = vecPluginManager[i];
                pPluginManager->SetHotfixServer(true);
                if (!pPluginManager->HotfixServer())
                {
                    // 执行热更新操作
                    bHotFail = true; // 记录热更新失败
                }
            }

            // 热更新失败时触发停服流程
            if (bHotFail)
            {
                NFLogInfo(NF_LOG_DEFAULT, 0, "Main Hotfix Fail To Stop Server................");
                NFGlobalSystem::Instance()->SetServerStopping(true);
            }
        }
    }

#if NF_PLATFORM == NF_PLATFORM_WIN
    // 关闭Windows事件处理管理器
    NFSignalHandlerMgr::Instance()->Shutdown();
#endif

    // 清理资源
    for (int i = 0; i < static_cast<int>(vecPluginManager.size()); i++)
    {
        NFIPluginManager* pPluginManager = vecPluginManager[i];
        pPluginManager->End(); // 执行插件管理器终止逻辑
        NF_SAFE_DELETE(pPluginManager); // 安全删除插件管理器实例
    }
#if NF_PLATFORM == NF_PLATFORM_WIN
    if (NFGlobalSystem::Instance()->IsServerKilling())
    {
        NFSignalHandlerMgr::Instance()->SendKillSuccess();
    }

    // 释放全局系统单例
    NFSignalHandlerMgr::Instance()->ReleaseInstance();
#endif
    NFGlobalSystem::Instance()->ReleaseSingleton();

    return 0;
}
