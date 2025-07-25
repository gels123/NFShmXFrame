//
// Created by gaoyi on 2022/9/18.
//

#include "NFProcessParameter.h"

#include <NFComm/NFCore/NFCmdLine.h>
#include <NFComm/NFCore/NFCommon.h>
#include <NFComm/NFCore/NFServerIDUtil.h>

#include "NFCPluginManager.h"
#include "NFSignalHandleMgr.h"
#include "NFComm/NFPluginModule/NFGlobalSystem.h"

void CloseXButton()
{
#if NF_PLATFORM == NF_PLATFORM_WIN
    HWND hWnd = GetConsoleWindow();
	if (hWnd)
	{
		HMENU hMenu = GetSystemMenu(hWnd, FALSE);
		EnableMenuItem(hMenu, SC_CLOSE, MF_DISABLED | MF_BYCOMMAND);
	}
#endif
}

void ignore_pipe_new()
{
#if	NF_PLATFORM == NF_PLATFORM_LINUX
	struct sigaction sig;

	sig.sa_handler = SIG_IGN;
	sig.sa_flags = 0;
	sigemptyset(&sig.sa_mask);
	sigaction(SIGPIPE, &sig,NULL);
#endif
}

//转变成守护进程后，会新建一个进程
void InitDaemon()
{
#if	NF_PLATFORM == NF_PLATFORM_LINUX
	pid_t pid;

	if ((pid = fork()) != 0)
	{
		exit(0);
	}

	setsid();

	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	ignore_pipe_new();

	if ((pid = fork()) != 0)
	{
		exit(0);
	}

	umask(0);
#endif
}

/**
 * @brief 处理命令行参数，并根据参数配置服务器环境。
 *
 * 该函数通过解析命令行参数，配置服务器的运行模式、路径、插件等设置。根据不同的参数，
 * 函数会启动单个服务器或多个服务器实例，并加载相应的配置文件和插件。
 *
 * @param argc 命令行参数的数量。
 * @param argv 命令行参数的数组，每个参数为一个字符串。
 * @return 无返回值。
 */
void ProcessParameter(int argc, char* argv[])
{
	try
	{
		// 创建命令行解析器对象
		NFCmdLine::NFParser cmdParser;

		// 添加命令行参数选项
		cmdParser.Add<std::string>("Server", 0, "Server Name", true, "xxAllServer or AllMoreServer(more server, must use different loaded server)");
		cmdParser.Add<std::string>("ID", 0, "Server ID", true, "1.1.1.1");
		cmdParser.Add<std::string>("Config", 0, "Config Path", true, "../../Config");
		cmdParser.Add<std::string>("Plugin", 0, "Plugin Path", false, "../../Plugin");
		cmdParser.Add<std::string>("Log", 0, "Log Path", false, "logs");
		cmdParser.Add<std::string>("LuaScript", 0, "Lua Script Path", false, "../LuaScript");
		cmdParser.Add<std::string>("Game", 0, "Game", true, "MMO");
		cmdParser.Add("XButton", 'x', "Close the 'X' button, only on windows");
		cmdParser.Add("Daemon", 'd', "Run it as daemon mode, only on linux");
		cmdParser.Add("Stop", 0, "Stop the run server, only on linux");
		cmdParser.Add("Reload", 0, "Reload the run server, only on linux");
		cmdParser.Add("Quit", 0, "Quit the run server, only on linux");
		cmdParser.Add("Restart", 0, "Close the run server, restart new proc, only on linux");
		cmdParser.Add("Start", 0, "Start the run server, only on linux");
		cmdParser.Add("Init", 0, "Change shm mode to init, only on linux");
		cmdParser.Add("Kill", 0, "Kill the run server, only on linux");
		cmdParser.Add<std::string>("Param", 0, "Temp Param, You love to use it", false, "Param");


		// 打印命令行参数的使用说明
		std::cout << cmdParser.Usage() << std::endl;

		// 解析并检查命令行参数
		cmdParser.ParseCheck(argc, argv);

		// 获取服务器名称参数
		auto strAppName = cmdParser.Get<std::string>("Server");

		// 如果服务器名称为 "AllMoreServer"，则启动多个服务器实例
		if (strAppName == "AllMoreServer")
		{
			// 设置多服务器模式
			NFGlobalSystem::Instance()->SetMoreServer(true);

			// 获取服务器ID、配置路径和插件路径
			auto strBusName = cmdParser.Get<std::string>("ID");
			auto strConfigPath = cmdParser.Get<std::string>("Config");
			auto strPlugin = cmdParser.Get<std::string>("Plugin");
			auto strGame = cmdParser.Get<std::string>("Game");

			// 加载配置文件
			NFGlobalSystem::Instance()->LoadConfig(strPlugin);

			// 获取所有服务器的配置信息
			const NFrame::pbPluginConfig* pPlugConfig = NFGlobalSystem::Instance()->GetAllMoreServerConfig();

			// 遍历每个服务器配置，启动相应的服务器实例
			for (int i = 0; i < pPlugConfig->serverlist_size(); i++)
			{
				const NFrame::pbAllServerConfig& serverConfig = pPlugConfig->serverlist(i);
				std::vector<std::string> vecParam;
				vecParam.push_back(argv[0]);
				vecParam.push_back("--Server=" + serverConfig.server());
				vecParam.push_back("--ID=" + serverConfig.id());
				vecParam.push_back("--Config=" + strConfigPath);
				vecParam.push_back("--Plugin=" + strPlugin);
				vecParam.push_back("--Game=" + strGame);
				vecParam.push_back("--restart");

				// 创建新的插件管理器并处理参数
				NFIPluginManager* pPluginManager = NF_NEW NFCPluginManager();
				ProcessParameter(pPluginManager, vecParam);

				// 如果服务器配置包含 "ALL_SERVER"，则设置为全局插件管理器
				if (NFStringUtility::Contains(serverConfig.server(), ALL_SERVER))
				{
					NFGlobalSystem::Instance()->SetGlobalPluginManager(pPluginManager);
				}

				// 将插件管理器添加到全局系统中
				NFGlobalSystem::Instance()->AddPluginManager(pPluginManager);
			}

			// 如果没有设置全局插件管理器，则使用第一个插件管理器作为全局管理器
			if (NFGlobalSystem::Instance()->GetGlobalPluginManager() == nullptr)
			{
				if (!NFGlobalSystem::Instance()->GetPluginManagerList().empty())
				{
					NFGlobalSystem::Instance()->SetGlobalPluginManager(NFGlobalSystem::Instance()->GetPluginManagerList()[0]);
				}
			}
		}
		else
		{
			// 如果服务器名称不是 "AllMoreServer"，则启动单个服务器实例
			std::vector<std::string> vecParam;
			for (int i = 0; i < argc; i++)
			{
				vecParam.push_back(argv[i]);
			}

			// 创建新的插件管理器并处理参数
			NFIPluginManager* pPluginManager = new NFCPluginManager();
			ProcessParameter(pPluginManager, vecParam);

			// 设置全局插件管理器并添加到全局系统中
			NFGlobalSystem::Instance()->SetGlobalPluginManager(pPluginManager);
			NFGlobalSystem::Instance()->AddPluginManager(pPluginManager);
		}
	}
	catch (NFCmdLine::NFCmdLine_Error& e)
	{
		std::cout << e.what() << std::endl;
		NFSLEEP(1000);
		exit(0);
	}
}

/**
 * @brief 处理命令行参数并初始化插件管理器
 *
 * 该函数负责解析命令行参数，并根据参数配置插件管理器。支持多种命令行选项，
 * 包括服务器名称、ID、配置文件路径、插件路径、日志路径等。还支持特定平台的
 * 操作，如关闭Windows的"X"按钮，或在Linux上以守护进程模式运行。
 *
 * @param pPluginManager 插件管理器的指针，用于配置和初始化插件管理器。
 * @param vecParam 命令行参数列表，包含用户输入的命令行参数。
 */
void ProcessParameter(NFIPluginManager* pPluginManager, const std::vector<std::string>& vecParam)
{
	try
	{
		// 初始化命令行解析器
		NFCmdLine::NFParser cmdParser;

		// 添加命令行选项
		cmdParser.Add<std::string>("Server", 0, "Server Name", true, "AllServer");
		cmdParser.Add<std::string>("ID", 0, "Server ID", true, "1.1.1.1");
		cmdParser.Add<std::string>("Config", 0, "Config Path", true, "../../Config");
		cmdParser.Add<std::string>("Plugin", 0, "Plugin Path", true, "../../Plugin");
		cmdParser.Add<std::string>("Log", 0, "Log Path", false, "logs");
		cmdParser.Add<std::string>("LuaScript", 0, "Lua Script Path", false, "../../LuaScript");
		cmdParser.Add<std::string>("Game", 0, "Game", true, "MMO");
		cmdParser.Add("XButton", 'x', "Close the 'X' button, only on windows");
		cmdParser.Add("Daemon", 'd', "Run it as daemon mode, only on linux");
		cmdParser.Add("Stop", 0, "Stop the run server, only on linux");
		cmdParser.Add("Reload", 0, "Reload the run server, only on linux");
		cmdParser.Add("Quit", 0, "Quit the run server, only on linux");
		cmdParser.Add("Restart", 0, "Close the run server, restart new proc, only on linux");
		cmdParser.Add("Start", 0, "Start the run server, only on linux");
		cmdParser.Add("Init", 0, "Change shm mode to init, only on linux");
		cmdParser.Add("Kill", 0, "Kill the run server, only on linux");
		cmdParser.Add<std::string>("Param", 0, "Temp Param, You love to use it", false, "Param");

		// 解析并检查命令行参数
		cmdParser.ParseCheck(vecParam);

		// 设置插件管理器的完整路径
		pPluginManager->SetFullPath(vecParam[0]);

		// 获取并设置临时参数
		auto strParam = cmdParser.Get<std::string>("Param");
		pPluginManager->SetStrParam(strParam);

		// 获取并设置服务器名称
		auto strAppName = cmdParser.Get<std::string>("Server");
		pPluginManager->SetAppName(strAppName);

		// 如果服务器名称为"AllServer"，则设置加载所有服务器
		if (strAppName.find(ALL_SERVER) != std::string::npos)
		{
			pPluginManager->SetLoadAllServer(true);
		}

		// 获取并设置服务器ID
		auto strBusName = cmdParser.Get<std::string>("ID");
		uint32_t mBusId = NFServerIDUtil::GetBusID(strBusName);

		// 如果ID无效，则输出错误信息并退出
		if (mBusId <= 0)
		{
			std::cerr << "ID:" << strBusName << std::endl;
			std::cerr << cmdParser.Usage() << std::endl;
			exit(0);
		}

		// 设置总线名称和应用ID
		pPluginManager->SetBusName(strBusName);
		pPluginManager->SetAppID(static_cast<int>(mBusId));

		// 设置配置文件路径
		if (cmdParser.Exist("Config"))
		{
			auto strDataPath = cmdParser.Get<std::string>("Config");
			pPluginManager->SetConfigPath(strDataPath);
		}

		// 设置插件路径
		auto strPlugin = cmdParser.Get<std::string>("Plugin");
		pPluginManager->SetPluginPath(strPlugin);

		// 设置Lua脚本路径
		auto luaScript = cmdParser.Get<std::string>("LuaScript");
		pPluginManager->SetLuaScriptPath(luaScript);
		auto logPath = cmdParser.Get<std::string>("Log");
		pPluginManager->SetLogPath(logPath);

		auto gameStr = cmdParser.Get<std::string>("Game");
		pPluginManager->SetGame(gameStr);

		pPluginManager->SetPidFileName();
#if NF_PLATFORM == NF_PLATFORM_WIN
        if (cmdParser.Exist("XButton"))
		{
			CloseXButton();
		}

        // Windows平台的命令行参数处理
        // 检查命令行参数中是否存在 "Kill" 选项
        if (cmdParser.Exist("Kill"))
        {
            // 如果存在，则设置插件管理器杀死前一个应用程序的标志为 true
            pPluginManager->SetKillPreApp(true);
        }

        // 检查命令行参数中是否存在 "Stop" 选项
        if (cmdParser.Exist("Stop"))
        {
            // 如果存在，则调用插件管理器的 StopApp 方法停止应用程序
            pPluginManager->StopApp();
            // 停止应用程序后，退出当前进程
            exit(0);
        }
        // 若 "Stop" 选项不存在，检查是否存在 "Reload" 选项
        else if (cmdParser.Exist("Reload"))
        {
            // 如果存在，则调用插件管理器的 ReloadApp 方法重新加载应用程序
            pPluginManager->ReloadApp();
            // 重新加载应用程序后，退出当前进程
            exit(0);
        }
        // 若 "Stop" 和 "Reload" 选项都不存在，检查是否存在 "Quit" 选项
        else if (cmdParser.Exist("Quit"))
        {
            // 如果存在，则调用插件管理器的 QuitApp 方法退出应用程序
            pPluginManager->QuitApp();
            // 退出应用程序后，退出当前进程
            exit(0);
        }
        // 若 "Stop"、"Reload" 和 "Quit" 选项都不存在，检查是否存在 "Restart" 选项
        else if (cmdParser.Exist("Restart"))
        {
            // Windows上不支持守护进程，但可以处理Daemon参数（忽略）
            if (cmdParser.Exist("Daemon"))
            {
                // Windows上忽略守护进程设置，但设置标记
                pPluginManager->SetDaemon();
            }

        	// 初始化Windows事件处理管理器
        	NFSignalHandlerMgr::Instance()->Initialize();

            // Windows不需要信号处理，但设置标记
            pPluginManager->SetKillPreApp(true);

            // 尝试杀死前一个应用程序
            if (pPluginManager->KillPreApp() < 0)
            {
                // 若杀死失败，输出错误信息
                std::cout << "kill pre app failed!" << std::endl;
                // 退出当前进程
                exit(0);
            }

            // 尝试创建 PID 文件
            if (pPluginManager->CreatePidFile() < 0)
            {
                // 若创建失败，输出错误信息
                std::cout << "create " << pPluginManager->GetFullPath() << " pid " << pPluginManager->GetPidFileName() << " failed!" << std::endl;
                // 退出当前进程
                exit(0);
            }
        }
        // 若上述选项都不存在，检查是否存在 "Start" 选项
        else if (cmdParser.Exist("Start"))
        {
            // Windows上不支持守护进程，但可以处理Daemon参数（忽略）
            if (cmdParser.Exist("Daemon"))
            {
                // Windows上忽略守护进程设置，但设置标记
                pPluginManager->SetDaemon();
            }

        	// 初始化Windows事件处理管理器
        	NFSignalHandlerMgr::Instance()->Initialize();

            // 检查是否需要杀死前一个应用程序
            if (pPluginManager->GetKillPreApp())
            {
                // 尝试杀死前一个应用程序
                if (pPluginManager->KillPreApp() < 0)
                {
                    // 若杀死失败，输出错误信息
                    std::cout << "kill pre app failed!" << std::endl;
                    // 退出当前进程
                    exit(0);
                }
            }

            // 检查 PID 文件是否存在
            if (pPluginManager->CheckPidFile() < 0)
            {
                // 若检查失败，输出错误信息
                std::cout << "check " << pPluginManager->GetFullPath() << " pid " << pPluginManager->GetPidFileName() << " failed!" << std::endl;
                // 退出当前进程
                exit(0);
            }

            // 尝试创建 PID 文件
            if (pPluginManager->CreatePidFile() < 0)
            {
                // 若创建失败，输出错误信息
                std::cout << "create " << pPluginManager->GetFullPath() << " pid " << pPluginManager->GetPidFileName() << " failed!" << std::endl;
                // 退出当前进程
                exit(0);
            }
        }
#else
        // 检查命令行参数中是否存在 "Init" 选项
        if (cmdParser.Exist("Init"))
        {
            // 如果存在，则设置插件管理器的共享内存为初始化状态
            pPluginManager->SetInitShm();
        }

        // 检查命令行参数中是否存在 "Kill" 选项
        if (cmdParser.Exist("Kill"))
        {
            // 如果存在，则设置插件管理器杀死前一个应用程序的标志为 true
            pPluginManager->SetKillPreApp(true);
        }

        // 检查命令行参数中是否存在 "Stop" 选项
        if (cmdParser.Exist("Stop"))
        {
            // 如果存在，则调用插件管理器的 StopApp 方法停止应用程序
            pPluginManager->StopApp();
            // 停止应用程序后，退出当前进程
            exit(0);
        }
        // 若 "Stop" 选项不存在，检查是否存在 "Reload" 选项
        else if (cmdParser.Exist("Reload"))
        {
            // 如果存在，则调用插件管理器的 ReloadApp 方法重新加载应用程序
            pPluginManager->ReloadApp();
            // 重新加载应用程序后，退出当前进程
            exit(0);
        }
        // 若 "Stop" 和 "Reload" 选项都不存在，检查是否存在 "Quit" 选项
        else if (cmdParser.Exist("Quit"))
        {
            // 如果存在，则调用插件管理器的 QuitApp 方法退出应用程序
            pPluginManager->QuitApp();
            // 退出应用程序后，退出当前进程
            exit(0);
        }
        // 若 "Stop"、"Reload" 和 "Quit" 选项都不存在，检查是否存在 "Restart" 选项
        else if (cmdParser.Exist("Restart"))
        {
            // 如果存在 "Daemon" 选项，以守护进程模式运行
            if (cmdParser.Exist("Daemon"))
            {
                // 设置插件管理器为守护进程模式
                pPluginManager->SetDaemon();
                // 初始化守护进程
                InitDaemon();
            }

            // 初始化信号处理
            InitSignal();

            // 尝试杀死前一个应用程序
            if (pPluginManager->KillPreApp() < 0)
            {
                // 若杀死失败，输出错误信息
                std::cout << "kill pre app failed!" << std::endl;
                // 退出当前进程
                exit(0);
            }

            // 尝试创建 PID 文件
            if (pPluginManager->CreatePidFile() < 0)
            {
                // 若创建失败，输出错误信息
                std::cout << "create " << pPluginManager->GetFullPath() << " pid " << pPluginManager->GetPidFileName() << "failed!" << std::endl;
                // 退出当前进程
                exit(0);
            }
        }
        // 若上述选项都不存在，检查是否存在 "Start" 选项
        else if (cmdParser.Exist("Start"))
        {
            // 如果存在 "Daemon" 选项，以守护进程模式运行
            if (cmdParser.Exist("Daemon"))
            {
                // 设置插件管理器为守护进程模式
                pPluginManager->SetDaemon();
                // 初始化守护进程
                InitDaemon();
            }

            // 初始化信号处理
            InitSignal();

            // 检查是否需要杀死前一个应用程序
            if (pPluginManager->GetKillPreApp())
            {
                // 尝试杀死前一个应用程序
                if (pPluginManager->KillPreApp() < 0)
                {
                    // 若杀死失败，输出错误信息
                    std::cout << "kill pre app failed!" << std::endl;
                    // 退出当前进程
                    exit(0);
                }
            }

            // 检查 PID 文件是否存在
            if (pPluginManager->CheckPidFile() < 0)
            {
                // 若检查失败，输出错误信息
                std::cout << "check " << pPluginManager->GetFullPath() << " pid " << pPluginManager->GetPidFileName() << " failed!" << std::endl;
                // 退出当前进程
                exit(0);
            }

            // 尝试创建 PID 文件
            if (pPluginManager->CreatePidFile() < 0)
            {
                // 若创建失败，输出错误信息
                std::cout << "create " << pPluginManager->GetFullPath() << " pid " << pPluginManager->GetPidFileName() << " failed!" << std::endl;
                // 退出当前进程
                exit(0);
            }
        }

#endif

		// 构造窗口或进程的标题名称
		std::string strTitleName = "NF" + strAppName + NFCommon::tostr(strBusName); // +" PID" + NFGetPID();
#if NF_PLATFORM == NF_PLATFORM_WIN
        // 在 Windows 平台设置控制台窗口标题
		SetConsoleTitle(NFStringUtility::char2wchar(strTitleName.c_str(), NULL));

#elif NF_PLATFORM == NF_PLATFORM_LINUX
        // 在 Linux 平台设置进程名称
        prctl(PR_SET_NAME, strTitleName.c_str());
        //setproctitle(strTitleName.c_str());
#endif
	}
	catch (NFCmdLine::NFCmdLine_Error& e)
	{
		std::cout << e.what() << std::endl;
		NFSLEEP(1000);
		exit(0);
	}
}
