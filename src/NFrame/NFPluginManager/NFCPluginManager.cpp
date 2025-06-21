// -------------------------------------------------------------------------
//    @FileName         :    NFCPluginManager.cpp
//    @Author           :    LvSheng.Huang
//    @Date             :   2022-09-18
//    @Module           :    NFCPluginManager
//
// -------------------------------------------------------------------------

#include "NFCPluginManager.h"
#include "NFComm/NFPluginModule/NFIPlugin.h"
#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFPluginModule/NFProfiler.h"
#include "NFComm/NFPluginModule/NFServerDefine.h"
#include "NFComm/NFCore/NFFileUtility.h"
#include "NFComm/NFPluginModule/NFIConfigModule.h"
#include "NFPrintfLogo.h"
#include "NFComm/NFCore/NFServerTime.h"
#include "NFComm/NFPluginModule/NFIConsoleModule.h"
#include "NFComm/NFPluginModule/NFCheck.h"
#include "NFComm/NFKernelMessage/FrameMsg.pb.h"
#include "NFComm/NFPluginModule/NFIMessageModule.h"
#include "NFComm/NFCore/NFMD5.h"
#include "NFComm/NFCore/NFServerIDUtil.h"
#include "hwinfo/hw_info.h"
#include "NFSignalHandleMgr.h"

#include <csignal>
#include <utility>
#include <thread>
#include <chrono>

#include "sigar/sigar.h"

#ifdef __cplusplus
extern "C" {
#endif
int sigar_net_address_to_string(sigar_t* sigar,
                                sigar_net_address_t* address,
                                char* addr_str);
#ifdef __cplusplus
}
#endif

#if NF_PLATFORM == NF_PLATFORM_WIN
#elif NF_PLATFORM == NF_PLATFORM_LINUX
#include "NFComm/NFPluginModule/NFCheck.h"
#include "NFComm/NFCore/NFServerIDUtil.h"

#endif

NFCPluginManager::NFCPluginManager() : m_appInited(this)
{
	// 初始化帧计数器
	m_nCurFrameCount = 0;
	// 设置应用ID（默认为0）
	m_nAppID = 0;

	// 设置默认配置文件路径
	m_strConfigPath = "../../Config";
	// 设置默认插件加载路径
	m_strPluginPath = "../../Plugin";
	// 设置日志输出目录
	m_strLogPath = "logs";

	// 记录服务启动时间戳
	m_nInitTime = NF_ADJUST_TIMENOW_MS();
	// 初始化当前逻辑时间
	m_nNowTime = m_nInitTime;
	// 守护进程模式标记（默认关闭）
	m_isDaemon = false;
	// 共享内存初始化状态（默认未初始化）
	m_bInitShm = false;
	// 是否杀死前一个应用实例（默认关闭）
	m_isKillPreApp = false;
	// 配置重载状态（默认关闭）
	m_bReloadServer = false;
	// 停服流程进行中标记（默认关闭）
	m_bServerStopping = false;
	// 是否切换性能分析应用（默认关闭）
	m_bChangeProfileApp = false;
	// 热更新处理状态（默认关闭）
	m_bHotfixServer = false;
	// 固定帧率模式（默认开启）
	m_bFixedFrame = true;

	// 初始化完成状态（默认未完成）
	m_isInited = false;

	// 是否加载所有服务器（默认关闭）
	m_isAllServer = false;

	// 设置空闲状态休眠微秒数
	m_idleSleepUs = 1000;

	// 初始化服务器时间系统
	NFServerTime::Instance()->Init(static_cast<int>(m_nFrame));

	// 初始化随机数种子
	NFRandomSeed();
#ifdef NF_DEBUG_MODE
	// 调试模式下开启性能分析
	NFCPluginManager::SetOpenProfiler(true);
#endif
}


NFCPluginManager::~NFCPluginManager() = default;

bool NFCPluginManager::IsDaemon() const
{
	return m_isDaemon;
}

void NFCPluginManager::SetDaemon()
{
	m_isDaemon = true;
}

bool NFCPluginManager::IsInitShm() const
{
	return m_bInitShm;
}

void NFCPluginManager::SetInitShm()
{
	m_bInitShm = true;
}

bool NFCPluginManager::IsLoadAllServer() const
{
	return m_isAllServer;
}

void NFCPluginManager::SetLoadAllServer(bool b)
{
	m_isAllServer = b;
}

bool NFCPluginManager::AfterLoadAllPlugin()
{
	// 在所有插件加载完成后调用，用于执行插件间的依赖初始化
	// 遍历所有插件实例，调用其AfterLoadAllPlugin方法
	for (auto iter = m_nPluginInstanceList.begin(); iter != m_nPluginInstanceList.end(); ++iter)
	{
		(*iter)->AfterLoadAllPlugin();
	}

	return true;
}

bool NFCPluginManager::AfterInitShmMem()
{
	// 在共享内存初始化完成后调用，用于执行共享内存相关的初始化
	// 遍历所有插件实例，调用其AfterInitShmMem方法
	for (auto iter = m_nPluginInstanceList.begin(); iter != m_nPluginInstanceList.end(); ++iter)
	{
		(*iter)->AfterInitShmMem();
	}

	return true;
}

bool NFCPluginManager::Awake()
{
	// 插件系统启动时调用，用于执行插件的初始化逻辑
	// 遍历所有插件实例，调用其Awake方法
	for (auto iter = m_nPluginInstanceList.begin(); iter != m_nPluginInstanceList.end(); ++iter)
	{
		(*iter)->Awake();
	}

	return true;
}

bool NFCPluginManager::LoadAllPlugin()
{
#ifndef NF_DYNAMIC_PLUGIN
	// 静态插件加载流程
	RegisterStaticPlugin(); // 注册静态引擎插件
	LoadKernelPlugin(); // 优先加载内核插件（NFKernelPlugin）

	// 打印启动日志
	NFLogInfo(NF_LOG_DEFAULT, 0, "{}", PrintfLogo());
	NFLogInfo(NF_LOG_DEFAULT, 0, "NFPluginManager Awake................");
	NFLogWarning(NF_LOG_DEFAULT, 0, "LoadPlugin:NFKernelPlugin");

	// 加载插件配置文件plugin.xml
	LoadPluginConfig();
	// 遍历插件名称列表，加载每个静态插件
	for (auto it = m_nPluginNameVec.begin(); it != m_nPluginNameVec.end(); ++it)
	{
		LoadStaticPlugin(*it);
	}
#else
    // 动态插件加载流程
    LoadKernelPlugin(); // 优先加载内核插件

    // 打印启动日志
    NFLogInfo(NF_LOG_DEFAULT, 0, "{}", PrintfLogo());
    NFLogInfo(NF_LOG_DEFAULT, 0, "NFPluginManager Awake................");
    NFLogWarning(NF_LOG_DEFAULT, 0, "LoadPlugin:NFKernelPlugin");

    // 加载插件配置文件
    LoadPluginConfig();
    // 遍历插件名称列表，动态加载每个插件
    for (PluginNameVec::iterator it = m_nPluginNameVec.begin(); it != m_nPluginNameVec.end(); ++it)
    {
        LoadPluginLibrary(*it);
        NFIPlugin* pPlugin = FindPlugin(*it);
        // 如果是动态加载的插件，将其添加到控制台模块
        if (pPlugin->IsDynamicLoad())
        {
            ((NFIPluginManager*)this)->FindModule<NFIConsoleModule>()->AddDynamicPluginFile(*it);
        }
    }
#endif
	return true;
}

bool NFCPluginManager::Begin()
{
	// 启动插件管理器，按顺序执行以下步骤：
	LoadAllPlugin(); // 1. 加载所有插件
	AfterLoadAllPlugin(); // 2. 插件后加载处理
	AfterInitShmMem(); // 3. 共享内存初始化
	Awake(); // 4. 插件唤醒
	Init(); // 5. 插件初始化
	CheckConfig(); // 6. 配置检查
	ReadyExecute(); // 7. 准备执行
	return true;
}

bool NFCPluginManager::End()
{
	// 关闭插件管理器，按顺序执行以下步骤：
	BeforeShut(); // 1. 关闭前处理
	Shut(); // 2. 关闭插件
	Finalize(); // 3. 资源清理
	return true;
}

inline bool NFCPluginManager::Init()
{
	// 初始化插件管理器，调用所有已注册插件的Init方法
	NFLogInfo(NF_LOG_DEFAULT, 0, "NFPluginManager Init................");
	// 遍历插件实例列表，逐个调用插件的初始化方法
	for (auto iter = m_nPluginInstanceList.begin(); iter != m_nPluginInstanceList.end(); ++iter)
	{
		(*iter)->Init();
	}

	return true;
}

bool NFCPluginManager::LoadPluginConfig()
{
	// 加载插件配置文件，获取当前应用的插件配置
	NFPluginConfig* pConfig = NFIPluginManager::FindModule<NFIConfigModule>()->GetPluginConfig(m_strAppName);
	if (pConfig == nullptr)
	{
		// 如果找不到对应应用的插件配置，则报错
		NF_ASSERT_MSG(0, "There are no plugin:" + m_strAppName);
		return false;
	}

	// 加载框架插件
	for (size_t i = 0; i < pConfig->FramePlugins.size(); i++)
	{
		std::string strPluginName = pConfig->FramePlugins[i];
		m_nPluginNameVec.push_back(strPluginName);
	}

	// 加载服务器插件
	for (size_t i = 0; i < pConfig->ServerPlugins.size(); i++)
	{
		std::string strPluginName = pConfig->ServerPlugins[i];
		m_nPluginNameVec.push_back(strPluginName);
	}

	// 加载工作插件
	for (size_t i = 0; i < pConfig->WorkPlugins.size(); i++)
	{
		std::string strPluginName = pConfig->WorkPlugins[i];
		m_nPluginNameVec.push_back(strPluginName);
	}

	return true;
}


void NFCPluginManager::RegisteredStaticPlugin(const std::string& strPluginName, const CREATE_PLUGIN_FUNCTION& createFunc)
{
	// 注册静态插件，将插件名称和创建函数存入映射表
	m_nPluginFuncMap.emplace(strPluginName, createFunc);
}

void NFCPluginManager::Registered(NFIPlugin* pPlugin)
{
	// 注册插件实例
	if (pPlugin == nullptr) return;

	// 动态加载的情况下，直接使用FindPlugin(plugin->GetPluginName()) 将导致崩溃， 这到底是为啥呢
	std::string strPluginName = pPlugin->GetPluginName();
	// 检查插件是否已经注册
	if (!FindPlugin(strPluginName))
	{
		// 将插件实例存入映射表和列表
		m_nPluginInstanceMap.insert(PluginInstanceMap::value_type(strPluginName, pPlugin));
		m_nPluginInstanceList.push_back(pPlugin);
		// 调用插件的安装方法
		pPlugin->Install();
	}
	else
	{
		// 如果插件已经存在，则报错
		assert(0);
	}
}

void NFCPluginManager::UnRegistered(NFIPlugin* pPlugin)
{
	// 注销插件实例
	if (pPlugin == nullptr)
	{
		std::cerr << "UnRegistered, plugin == nullptr" << std::endl;
		return;
	}

	// 动态加载的情况下，直接使用mPluginInstanceMap.find(plugin->GetPluginName()) 将导致崩溃， 这到底是为啥呢
	std::string strPluginName = pPlugin->GetPluginName();
	// 查找插件实例
	auto it = m_nPluginInstanceMap.find(strPluginName);
	if (it != m_nPluginInstanceMap.end())
	{
		// 如果插件实例为空，直接移除
		if (it->second == nullptr)
		{
			m_nPluginInstanceMap.erase(it);
			return;
		}

		// 从插件实例列表中移除
		for (auto listIt = m_nPluginInstanceList.begin(); listIt != m_nPluginInstanceList.end(); ++listIt)
		{
			if (it->second == *listIt)
			{
				m_nPluginInstanceList.erase(listIt);
				break;
			}
		}

		// 调用插件的卸载方法并删除实例
		it->second->Uninstall();
		NF_SAFE_DELETE(it->second);
		it->second = nullptr;
		// 从插件实例映射表中移除
		m_nPluginInstanceMap.erase(it);
	}
}

NFIPlugin* NFCPluginManager::FindPlugin(const std::string& strPluginName)
{
	// 根据插件名称查找插件实例
	auto it = m_nPluginInstanceMap.find(strPluginName);
	if (it != m_nPluginInstanceMap.end())
	{
		// 如果找到则返回插件实例
		return it->second;
	}

	// 未找到返回空指针
	return nullptr;
}

bool NFCPluginManager::Execute()
{
	// 主循环执行函数
	bool bRet = true;
	// 更新当前时间
	m_nNowTime = NF_ADJUST_TIMENOW_MS();
	// 记录开始时间
	uint64_t startTime = NFGetTime();
	uint64_t endTime = 0;
	// 帧计数器递增
	m_nCurFrameCount++;

	// 更新服务器时间
	NFServerTime::Instance()->Update(startTime);

	// 开始性能分析
	BeginProfiler("MainLoop");

	// 如果未初始化，则执行初始化
	if (!IsInited())
	{
		m_appInited.Execute();
	}

	// 遍历所有插件实例并执行
	for (auto it = m_nPluginInstanceMap.begin(); it != m_nPluginInstanceMap.end(); ++it)
	{
		// 开始单个插件的性能分析
		BeginProfiler(it->first + "--Loop");
		// 执行插件逻辑
		bool tempRet = it->second->Execute();
		// 汇总执行结果
		bRet = bRet && tempRet;
		// 结束性能分析并获取耗时
		uint64_t useTime = EndProfiler();
		// 如果执行时间超过30ms（10毫秒）则记录错误日志
		if (useTime >= 30000) //>= 10毫秒
		{
			if (!IsLoadAllServer())
			{
				NFLogError(NF_LOG_DEFAULT, 0, "maintained:{} use time:{} ms", it->first + "--Loop", useTime / 1000);
			}
		}
	}

	// 结束主循环性能分析
	EndProfiler();

	// 采用固定帧率模式
	endTime = NFGetTime();
	auto cost = static_cast<uint32_t>(endTime > startTime ? endTime - startTime : 0);
	if (m_bFixedFrame)
	{
		// 计算需要休眠的时间
		uint32_t sleepTime = m_nFrameTime > cost ? m_nFrameTime - cost : 0;
		if (sleepTime > 0)
		{
			// 如果还有剩余时间则休眠
			NFSLEEP(sleepTime*1000);
		}
		else
		{
			// 如果超时则根据超时程度记录日志
			if (cost >= 40 && cost <= 200)
			{
				if (!IsLoadAllServer())
				{
					NFLogWarning(NF_LOG_DEFAULT, 0, "main thread frame timeout:{}", cost);
				}
			}
			else if (cost > 200)
			{
				if (!IsLoadAllServer())
				{
					NFLogError(NF_LOG_DEFAULT, 0, "main thread frame timeout:{}, something wrong", cost);
				}
			}
			else
			{
				// 如果时间过短则休眠1ms
				NFSLEEP(1000);
			}
		}
	}
	else
	{
		// 非固定帧率模式的处理逻辑
		if (cost >= 40 && cost <= 200)
		{
			if (!IsLoadAllServer())
			{
				NFLogWarning(NF_LOG_DEFAULT, 0, "main thread:{} frame timeout:{}", NFCommon::tostr(std::this_thread::get_id()), cost);
			}
		}
		else if (cost > 200)
		{
			if (!IsLoadAllServer())
			{
				NFLogError(NF_LOG_DEFAULT, 0, "main thread:{} frame timeout:{}, something wrong", NFCommon::tostr(std::this_thread::get_id()), cost);
			}
		}
		else
		{
			// 根据是否加载所有服务器决定休眠时间
			if (IsLoadAllServer())
			{
				NFSLEEP(1000);
			}
			else
			{
				NFSLEEP(m_idleSleepUs);
			}
		}
	}

	// 定期打印性能分析结果
	if (m_bFixedFrame)
	{
		if (m_nCurFrameCount % 10000 == 0)
		{
			PrintProfiler();
		}
	}
	else
	{
		if (m_nCurFrameCount % 1000000 == 0)
		{
			PrintProfiler();
		}
	}

	return bRet;
}


inline int NFCPluginManager::GetAppID() const
{
	return m_nAppID;
}

void NFCPluginManager::SetAppID(const int nAppID)
{
	m_nAppID = nAppID;
}

int NFCPluginManager::GetWorldID() const
{
	return static_cast<int>(NFServerIDUtil::GetWorldID(m_nAppID));
}


int NFCPluginManager::GetZoneID() const
{
	return static_cast<int>(NFServerIDUtil::GetZoneID(m_nAppID));
}


const std::string& NFCPluginManager::GetConfigPath() const
{
	return m_strConfigPath;
}

void NFCPluginManager::SetConfigPath(const std::string& strPath)
{
	if (strPath.empty()) return;

	m_strConfigPath = strPath;
}

const std::string& NFCPluginManager::GetPluginPath() const
{
	return m_strPluginPath;
}

void NFCPluginManager::SetPluginPath(const std::string& strPath)
{
	if (strPath.empty()) return;

	m_strPluginPath = strPath;
}

void NFCPluginManager::SetLuaScriptPath(const std::string& luaScriptPath)
{
	if (luaScriptPath.empty()) return;
	m_strLuaScriptPath = luaScriptPath;
}

void NFCPluginManager::SetGame(const std::string& game)
{
	if (game.empty()) return;
	m_strGame = game;
}

const std::string& NFCPluginManager::GetLuaScriptPath() const
{
	return m_strLuaScriptPath;
}

const std::string& NFCPluginManager::GetGame() const
{
	return m_strGame;
}

void NFCPluginManager::SetBusName(const std::string& busName)
{
	m_mStrBusName = busName;
}

const std::string& NFCPluginManager::GetBusName() const
{
	return m_mStrBusName;
}

const std::string& NFCPluginManager::GetAppName() const
{
	return m_strAppName;
}

const std::string& NFCPluginManager::GetStrParam() const
{
	return m_strParam;
}

void NFCPluginManager::SetStrParam(const std::string& strAppName)
{
	m_strParam = strAppName;
}


const std::string& NFCPluginManager::GetFullPath() const
{
	return m_strFullPath;
}

void NFCPluginManager::SetFullPath(const std::string& strFullPath)
{
	m_strFullPath = strFullPath;
	m_strExeName = NFFileUtility::GetFileName(strFullPath);
}

void NFCPluginManager::SetAppName(const std::string& strAppName)
{
	if (!m_strAppName.empty())
	{
		return;
	}

	m_strAppName = strAppName;
}

const std::string& NFCPluginManager::GetLogPath() const
{
	return m_strLogPath;
}

void NFCPluginManager::SetLogPath(const std::string& strName)
{
	if (strName.empty()) return;
	m_strLogPath = strName;
}

int NFCPluginManager::AddModule(const std::string& strModuleName, NFIModule* pModule)
{
	// 添加模块到模块实例映射表
	// 检查模块指针是否为空
	CHECK_EXPR_ASSERT(pModule != NULL, -1, "pModule != NULL name:{}", strModuleName);
	// 检查模块是否已经存在
	CHECK_EXPR_ASSERT(m_nModuleInstanceMap.find(strModuleName) == m_nModuleInstanceMap.end(), -1, "module exist name:{} --- {}", strModuleName, pModule->m_strName);

	// 将模块插入映射表
	m_nModuleInstanceMap.insert(ModuleInstanceMap::value_type(strModuleName, pModule));
	return 0;
}

void NFCPluginManager::RemoveModule(const std::string& strModuleName)
{
	// 从模块实例映射表中移除指定模块
	auto it = m_nModuleInstanceMap.find(strModuleName);
	if (it != m_nModuleInstanceMap.end())
	{
		m_nModuleInstanceMap.erase(it);
	}
}

NFIModule* NFCPluginManager::FindModule(const std::string& strModuleName)
{
	// 查找指定模块
	std::string strSubModuleName = strModuleName;

#if NF_PLATFORM == NF_PLATFORM_WIN
	// Windows平台处理：去除模块名前缀
	std::size_t position = strSubModuleName.find(' ');
	if (string::npos != position)
	{
		strSubModuleName = strSubModuleName.substr(position + 1, strSubModuleName.length());
	}
#else
    // 非Windows平台处理：解析模块名
    for (int i = 0; i < (int)strSubModuleName.length(); i++)
    {
        std::string s = strSubModuleName.substr(0, i + 1);
        int n = atof(s.c_str());
        if ((int)strSubModuleName.length() == i + 1 + n)
        {
            strSubModuleName = strSubModuleName.substr(i + 1, strSubModuleName.length());
            break;
        }
    }
#endif

	// 在模块实例映射表中查找模块
	auto it = m_nModuleInstanceMap.find(strSubModuleName);
	if (it != m_nModuleInstanceMap.end())
	{
		return it->second;
	}

	return nullptr;
}


bool NFCPluginManager::CheckConfig()
{
	NFLogInfo(NF_LOG_DEFAULT, 0, "NFPluginManager CheckConfig................");

	for (auto itCheckInstance = m_nPluginInstanceMap.begin(); itCheckInstance != m_nPluginInstanceMap.end(); ++itCheckInstance)
	{
		itCheckInstance->second->CheckConfig();
	}
	return true;
}

bool NFCPluginManager::ReadyExecute()
{
	NFLogInfo(NF_LOG_DEFAULT, 0, "NFPluginManager ReadyExecute................");

	if (!m_bFixedFrame)
	{
#ifdef NF_DEBUG_MODE
		m_profilerMgr.SetOpenProfiler(true);
#endif
	}

	for (auto itCheckInstance = m_nPluginInstanceMap.begin(); itCheckInstance != m_nPluginInstanceMap.end(); ++itCheckInstance)
	{
		itCheckInstance->second->ReadyExecute();
	}

	return true;
}

bool NFCPluginManager::BeforeShut()
{
	NFLogInfo(NF_LOG_DEFAULT, 0, "NFPluginManager BeforeShut................");

	for (auto itBeforeInstance = m_nPluginInstanceMap.begin(); itBeforeInstance != m_nPluginInstanceMap.end(); ++itBeforeInstance)
	{
		itBeforeInstance->second->BeforeShut();
	}

	return true;
}

bool NFCPluginManager::Shut()
{
	NFLogInfo(NF_LOG_DEFAULT, 0, "NFPluginManager Shut................");

	for (auto itInstance = m_nPluginInstanceMap.begin(); itInstance != m_nPluginInstanceMap.end(); ++itInstance)
	{
		itInstance->second->Shut();
	}

	return true;
}

bool NFCPluginManager::OnReloadConfig()
{
	NFLogInfo(NF_LOG_DEFAULT, 0, "NFPluginManager OnReloadConfig................");

	/*
	加载服务器配置
	*/
	FindModule<NFIConfigModule>()->LoadConfig();

	for (auto itInstance = m_nPluginInstanceMap.begin(); itInstance != m_nPluginInstanceMap.end(); ++itInstance)
	{
		itInstance->second->OnReloadConfig();
	}

	return true;
}


bool NFCPluginManager::AfterOnReloadConfig()
{
	NFLogInfo(NF_LOG_DEFAULT, 0, "NFPluginManager AfterOnReloadConfig................");

	for (auto itInstance = m_nPluginInstanceMap.begin(); itInstance != m_nPluginInstanceMap.end(); ++itInstance)
	{
		itInstance->second->AfterOnReloadConfig();
	}

	return true;
}

bool NFCPluginManager::Finalize()
{
	// 插件管理器最终化处理
	NFLogInfo(NF_LOG_DEFAULT, 0, "NFPluginManager Finalize................");

	// 遍历所有插件实例，调用其Finalize方法
	for (auto itInstance = m_nPluginInstanceMap.begin(); itInstance != m_nPluginInstanceMap.end(); ++itInstance)
	{
		itInstance->second->Finalize();
	}

#ifndef NF_DYNAMIC_PLUGIN
	// 静态插件卸载流程
	// 遍历插件名称列表，卸载除NFKernelPlugin外的所有插件
	for (auto it = m_nPluginNameVec.begin(); it != m_nPluginNameVec.end(); ++it)
	{
		if (*it != "NFKernelPlugin")
		{
			UnLoadStaticPlugin(*it);
		}
	}

	// 最后卸载内核插件NFKernelPlugin
	NFLogWarning(NF_LOG_DEFAULT, 0, "UnLoadPlugin:NFKernelPlugin");
	UnLoadStaticPlugin("NFKernelPlugin");
#else
    // 动态插件卸载流程
    // 遍历插件名称列表，卸载除NFKernelPlugin外的所有插件
    for (auto it = m_nPluginNameVec.begin(); it != m_nPluginNameVec.end(); ++it)
    {
        if (*it != "NFKernelPlugin")
        {
            UnLoadPluginLibrary(*it);
        }
    }

    // 最后卸载内核插件NFKernelPlugin
    NFLogWarning(NF_LOG_DEFAULT, 0, "UnLoadPlugin:NFKernelPlugin");
    UnLoadPluginLibrary("NFKernelPlugin");
#endif

	// 清空所有插件相关容器
	m_nPluginInstanceMap.clear();
	m_nPluginInstanceList.clear();
	m_nModuleInstanceMap.clear();
	m_nPluginNameVec.clear();
	return true;
}

bool NFCPluginManager::LoadStaticPlugin(const std::string& strPluginName)
{
	// 加载静态插件
	// 在插件函数映射表中查找插件
	auto it = m_nPluginFuncMap.find(strPluginName);
	if (it == m_nPluginFuncMap.end())
	{
		// 如果插件未注册，记录错误日志并报错
		NFLogError(NF_LOG_DEFAULT, 0, " Load Static Plugin [{0}] Failed, The Plugin Not Registered, Please Register like this 'REGISTER_STATIC_PLUGIN(this, {0})' in the NFCLoadStaticPlugin.cpp", strPluginName);
		assert(0);
		return false;
	}

	// 如果插件已经加载，直接返回
	if (FindPlugin(strPluginName)) return true;

	// 创建插件实例
	NFIPlugin* pPlugin = it->second(this);
	if (pPlugin)
	{
		// 记录加载日志并注册插件
		NFLogWarning(NF_LOG_DEFAULT, 0, "LoadPlugin:{}", strPluginName);
		Registered(pPlugin);
	}
	return true;
}

bool NFCPluginManager::UnLoadStaticPlugin(const std::string& strPluginName)
{
	NFLogWarning(NF_LOG_DEFAULT, 0, "UnLoadPlugin:{}", strPluginName);
	UnRegistered(FindPlugin(strPluginName));
	return true;
}

uint32_t NFCPluginManager::GetFrame() const
{
	return m_nFrame;
}

uint32_t NFCPluginManager::GetFrameTime() const
{
	return m_nFrameTime;
}

bool NFCPluginManager::IsFixedFrame() const
{
	return m_bFixedFrame;
}

void NFCPluginManager::SetFixedFrame(bool frame)
{
	if (IsLoadAllServer()) return;

	m_bFixedFrame = frame;
}

uint32_t NFCPluginManager::GetIdleSleepUs() const
{
	return m_idleSleepUs;
}

void NFCPluginManager::SetIdleSleepUs(uint32_t time)
{
	if (IsLoadAllServer()) return;

	m_idleSleepUs = time;
}

uint64_t NFCPluginManager::GetInitTime() const
{
	return m_nInitTime;
}

uint64_t NFCPluginManager::GetNowTime() const
{
	return m_nNowTime;
}

int NFCPluginManager::GetMachineAddr(const std::string& str)
{
	sigar_t* pSigar;
	sigar_open(&pSigar);

	sigar_net_interface_config_t netConfig;
	if (sigar_net_interface_config_get(pSigar, nullptr, &netConfig) != SIGAR_OK)
	{
		return -1;
	}

	char name[SIGAR_FQDN_LEN];
	sigar_net_address_to_string(pSigar, &netConfig.hwaddr, name);
	std::string macAddr = name;

	std::string cpuId;
	std::string diskId;
	std::string boardSerial;
	if (get_cpu_id(cpuId) || get_disk_id(diskId) || get_board_serial_number(boardSerial))
	{
		std::string machineAddr = NF_FORMAT("{}|{}|{}|{}|{}", macAddr, str, cpuId, diskId, boardSerial);
		m_iMachineAddrMD5 = NFMD5::md5str(machineAddr);
	}
	else
	{
		std::string machineAddr = NF_FORMAT("{}|{}", macAddr, str);
		m_iMachineAddrMD5 = NFMD5::md5str(machineAddr);
	}

	sigar_close(pSigar);

	return 0;
}

bool NFCPluginManager::LoadPluginLibrary(const std::string& strPluginDLLName)
{
	// 加载动态链接库插件
	// 检查是否已经加载过该插件
	auto it = m_nPluginLibMap.find(strPluginDLLName);
	if (it == m_nPluginLibMap.end())
	{
		// 创建动态库对象
		auto* pLib = new NFCDynLib(strPluginDLLName);
		// 尝试加载动态库
		bool bLoad = pLib->Load();

		if (bLoad)
		{
			// 将加载成功的动态库加入映射表
			m_nPluginLibMap.insert(PluginLibMap::value_type(strPluginDLLName, pLib));

			// 获取DllStartPlugin函数指针
			auto pFunc = (DLL_START_PLUGIN_FUNC)(pLib->GetSymbol("DllStartPlugin"));
			if (!pFunc)
			{
				// 如果找不到DllStartPlugin函数，记录错误日志并报错
				NFLogError(NF_LOG_DEFAULT, 0, "Find function DllStartPlugin Failed in [{}]", pLib->GetName());
				assert(0);
				return false;
			}

			// 调用插件的启动函数
			pFunc(this);

			return true;
		}

#if NF_PLATFORM == NF_PLATFORM_LINUX || NF_PLATFORM == NF_PLATFORM_APPLE
        // Linux/Apple平台错误处理
        char* error = dlerror();
        if (error)
        {
            // 输出错误信息并退出程序
            std::cout << "Load shared lib failed: " << strPluginDLLName << std::endl;
            NFLogError(NF_LOG_DEFAULT, 0, " Load DLL[{0}] failed, ErrorNo. = [{1}] Load [{0}] failed", strPluginDLLName, error);
            NFSLEEP(1000);
            exit(0);
            return false;
        }
        else
        {
            // 未知错误处理
            std::cout << "Load shared lib failed: " << strPluginDLLName << std::endl;
            NFSLEEP(1000);
            exit(0);
            return false;
        }
#elif NF_PLATFORM == NF_PLATFORM_WIN
		// Windows平台错误处理
		NFLogError(NF_LOG_DEFAULT, 0, " Load DLL[{0}] failed, ErrorNo. = [{1}] Load [{0}] failed", pLib->GetName(), GetLastError());
		exit(0);
#endif // NF_PLATFORM
	}

	return false;
}

bool NFCPluginManager::UnLoadPluginLibrary(const std::string& strPluginDLLName)
{
	// 卸载动态链接库插件
	// 查找要卸载的插件
	auto it = m_nPluginLibMap.find(strPluginDLLName);
	if (it != m_nPluginLibMap.end())
	{
		// 获取动态库对象
		NFCDynLib* pLib = it->second;
		// 获取DllStopPlugin函数指针
		auto pFunc = (DLL_STOP_PLUGIN_FUNC)(pLib->GetSymbol("DllStopPlugin"));

		if (pFunc)
		{
			// 调用插件的停止函数
			pFunc(this);
		}

		// 卸载动态库
		pLib->UnLoad();
		// 删除动态库对象
		delete pLib;
		// 从映射表中移除
		m_nPluginLibMap.erase(it);

		return true;
	}

	return false;
}

bool NFCPluginManager::DynamicLoadPluginLibrary(const std::string& strPluginDLLName)
{
#ifndef NF_DYNAMIC_PLUGIN
	NFLogError(NF_LOG_DEFAULT, 0, "can't load plugin:{}, you are static load!", strPluginDLLName);
#else
	NFIPlugin* pPlugin = FindPlugin(strPluginDLLName);
	if (pPlugin)
	{
		if (pPlugin->IsDynamicLoad() == false)
		{
			NFLogError(NF_LOG_DEFAULT, 0, "plugin:{} can't not dynamic load!", strPluginDLLName);
			return false;
		}

		/*
		**卸载动态库
		*/
		pPlugin->OnDynamicPlugin();
		pPlugin->BeforeShut();
		pPlugin->Shut();
		pPlugin->Finalize();
		pPlugin = nullptr;
		UnLoadPluginLibrary(strPluginDLLName);

		/*
		**重新加载动态库
		*/
		//EN_OBJ_MODE oldMode = NFShmMgr::Instance()->GetInitMode();
		//NFShmMgr::Instance()->SetInitMode(EN_OBJ_MODE_RECOVER);
		LoadPluginLibrary(strPluginDLLName);
		pPlugin = FindPlugin(strPluginDLLName);
		if (pPlugin)
		{
			pPlugin->Awake();
			pPlugin->Init();
			pPlugin->ReadyExecute();
		}
		else
		{
			NFLogError(NF_LOG_DEFAULT, 0, "dynamic load plugin:{} failed!", strPluginDLLName);
		}
		//NFShmMgr::Instance()->SetInitMode(oldMode);
	}
	else
	{
		NFLogError(NF_LOG_DEFAULT, 0, "plugin:{} is not exist!", strPluginDLLName);
		return false;
	}
#endif
	return true;
}

void NFCPluginManager::BeginProfiler(const std::string& funcName)
{
	m_profilerMgr.BeginProfiler(funcName);
}

uint64_t NFCPluginManager::EndProfiler()
{
	return m_profilerMgr.EndProfiler();
}

void NFCPluginManager::ClearProfiler()
{
	m_profilerMgr.ResetAllProfilerTimer();
}

void NFCPluginManager::PrintProfiler()
{
	if (m_profilerMgr.IsOpenProfiler())
	{
		std::string str = m_profilerMgr.OutputTopProfilerTimer();
		LOG_STATISTIC("{}", str);
	}
}

void NFCPluginManager::SetOpenProfiler(bool b)
{
	m_profilerMgr.SetOpenProfiler(b);
}

bool NFCPluginManager::IsOpenProfiler()
{
	return m_profilerMgr.IsOpenProfiler();
}

uint32_t NFCPluginManager::GetCurFrameCount() const
{
	return m_nCurFrameCount;
}

void NFCPluginManager::SetPidFileName()
{
	m_strPidFileName = NF_FORMAT("{}_{}.pid", m_strAppName, m_mStrBusName);
}

const std::string& NFCPluginManager::GetPidFileName()
{
	return m_strPidFileName;
}

#if NF_PLATFORM == NF_PLATFORM_LINUX
/**
 * @brief 检查PID文件是否存在，并验证对应的进程是否在运行。
 *
 * 该函数首先检查指定的PID文件是否存在。如果文件不存在，则返回0。
 * 如果文件存在，读取文件内容并解析为进程ID。然后，根据进程ID构建可执行文件路径，
 * 并检查该路径是否存在。如果路径不存在，则返回0；否则返回-1。
 *
 * @return int 返回值为0表示PID文件不存在或对应的进程未运行；返回-1表示PID文件存在且对应的进程正在运行。
 */
int NFCPluginManager::CheckPidFile()
{
    // 检查PID文件是否存在
    bool exist = false;
    exist = NFFileUtility::IsFileExist(m_strPidFileName);
    if (exist == false)
    {
        return 0;
    }

    // 读取PID文件内容并解析为进程ID
    std::string content;
    NFFileUtility::ReadFileContent(m_strPidFileName, content);
    pid_t proc_id = NFCommon::strto<pid_t>(content);

    // 构建可执行文件路径并检查是否存在
    std::string exe_path = NF_FORMAT("/proc/{}/cwd/{}", proc_id, m_strExeName);
    std::cout << "path = " << exe_path << std::endl;
    exist = NFFileUtility::IsFileExist(exe_path);
    if (exist == false)
    {
        return 0;
    }

    // 返回-1表示PID文件存在且对应的进程正在运行
    return -1;
}

#endif

#if NF_PLATFORM == NF_PLATFORM_LINUX
/**
 * @brief 创建PID文件并写入当前进程的PID
 *
 * 该函数用于创建一个PID文件，并将当前进程的PID写入该文件中。PID文件通常用于记录
 * 某个进程的ID，以便其他程序或脚本可以识别和管理该进程。
 *
 * @return int 返回值为0表示成功创建并写入PID文件，返回-1表示失败。
 */
int  NFCPluginManager::CreatePidFile()
{
    // 获取当前进程的PID
    pid_t proc_id = NFGetPID();
    std::cout << "pid = " << proc_id << std::endl;

    // 将PID转换为字符串
    std::string pidName = NFCommon::tostr(proc_id);

    // 打开或创建PID文件，并设置文件权限
    int32_t fd = ::open(m_strPidFileName.c_str(), O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );

    // 如果文件成功打开并且成功写入PID，则关闭文件并返回成功
    if (-1 != fd &&
        ::write( fd, (const void*)pidName.c_str(), pidName.length()))
    {
        close( fd );
        return 0;
    }

    // 如果文件打开或写入失败，则返回失败
    return -1;
}

#endif

#if NF_PLATFORM == NF_PLATFORM_LINUX
/**
 * @brief 等待指定进程结束，直到超时。
 *
 * 该函数会每隔1秒检查一次指定进程的状态，如果在指定的秒数内进程结束，则返回0；
 * 如果超时进程仍未结束，则返回-1。
 *
 * @param pid 需要等待的进程ID。
 * @param sec 等待的最大秒数。
 * @return int 返回0表示进程在指定时间内结束，返回-1表示超时。
 */
int  NFCPluginManager::TimedWait(pid_t pid, int sec)
{
	int count = 0;
	do
	{
		// 每次循环等待1秒
		sleep(1);
		count++;

		// 如果等待时间超过指定秒数，返回超时
		if (count >= sec)
		{
			return -1;
		}

		// 检查进程状态
		if (kill(pid, 0) == 0 || errno == EPERM)
			continue;  // 进程仍然存在或没有权限，继续等待
		else if (errno == ESRCH)
			break;  // 进程不存在，退出循环
		else
			std::cout << "error checking pid:" << pid << " status" << std::endl;  // 其他错误，打印错误信息

	} while(true);
	return 0;  // 进程在指定时间内结束，返回成功
}

#endif

#if NF_PLATFORM == NF_PLATFORM_LINUX
// 终止之前的应用程序实例
int NFCPluginManager::KillPreApp()
{
    bool exist = false;
    // 检查预先记录的进程ID文件是否存在
    exist = NFFileUtility::IsFileExist(m_strPidFileName);
    if (exist)
    {
        std::string content;
        // 读取PID文件内容（包含之前实例的进程ID）
        NFFileUtility::ReadFileContent(m_strPidFileName, content);

        // 将字符串类型的进程ID转换为pid_t类型
        pid_t proc_id = NFCommon::strto<pid_t>(content);

        // 向目标进程发送SIGUNUSED信号（通常用于终止进程）
        kill(proc_id, SIGUNUSED);

        // 等待目标进程退出，最多等待10秒，返回等待结果
        return TimedWait(proc_id, 10);
    }

    // 如果不存在PID文件，说明没有之前的实例，直接返回0
    return 0;
}

#endif

#if NF_PLATFORM == NF_PLATFORM_LINUX
// 停止应用程序
void NFCPluginManager::StopApp()
{
    // 检查PID文件是否存在，该文件通常用于存储应用程序的进程ID
    bool exist = false;
    exist = NFFileUtility::IsFileExist(m_strPidFileName);

    if (exist)
    {
        // 读取PID文件内容
        std::string content;
        NFFileUtility::ReadFileContent(m_strPidFileName, content);

        // 将文件内容转换为进程ID（pid_t类型）
        pid_t proc_id = NFCommon::strto<pid_t>(content);

        // 向目标进程发送SIGUSR1信号
        // SIGUSR1通常用于自定义操作，这里可能用于触发进程的优雅退出
        kill(proc_id, SIGUSR1);
    }

    // 注意：代码假设PID文件存在且包含有效的进程ID
    // 未处理文件读取失败或转换错误的情况
}

#endif

#if NF_PLATFORM == NF_PLATFORM_LINUX
// 重新加载应用程序
void NFCPluginManager::ReloadApp()
{
    // 检查PID文件是否存在（PID文件通常用于存储正在运行进程的进程ID）
    bool exist = false;
    exist = NFFileUtility::IsFileExist(m_strPidFileName);

    if (exist)
    {
        // 从PID文件中读取内容（预期内容为进程ID的字符串形式）
        std::string content;
        NFFileUtility::ReadFileContent(m_strPidFileName, content);

        // 将字符串类型的进程ID转换为pid_t类型
        pid_t proc_id = NFCommon::strto<pid_t>(content);

        // 向指定进程发送SIGUSR2信号
        // SIGUSR2通常被定义为用户自定义信号，用于触发应用程序重载逻辑
        kill(proc_id, SIGUSR2);
    }
}

#endif

#if NF_PLATFORM == NF_PLATFORM_LINUX
// 退出应用程序
void NFCPluginManager::QuitApp()
{
    // 检查PID文件是否存在
    bool exist = false;
    exist = NFFileUtility::IsFileExist(m_strPidFileName);

    if (exist)
    {
        // 读取PID文件中存储的进程ID
        std::string content;
        NFFileUtility::ReadFileContent(m_strPidFileName, content);

        // 将字符串类型的进程ID转换为数值类型
        pid_t proc_id = NFCommon::strto<pid_t>(content);

        // 向指定进程发送SIGUNUSED信号（通常用于自定义进程通信）
        // 注：SIGUNUSED是保留信号，具体实现需结合业务场景
        kill(proc_id, SIGUNUSED);
    }
}

#endif

bool NFCPluginManager::IsInited() const
{
	return m_isInited;
}

bool NFCPluginManager::IsInited(NF_SERVER_TYPE eServerType) const
{
	return m_appInited.IsInited(eServerType);
}

bool NFCPluginManager::IsFinishAppTask(NF_SERVER_TYPE eServerType, uint32_t taskGroup) const
{
	return m_appInited.IsFinishAppTask(eServerType, taskGroup);
}

bool NFCPluginManager::IsHasAppTask(NF_SERVER_TYPE eServerType, uint32_t taskGroup) const
{
	return m_appInited.IsHasAppTask(eServerType, taskGroup);
}

bool NFCPluginManager::IsHasAppTask(NF_SERVER_TYPE eServerType, uint32_t taskGroup, uint32_t taskType) const
{
	return m_appInited.IsHasAppTask(eServerType, taskGroup, taskType);
}

void NFCPluginManager::SetIsInited(bool b)
{
	m_isInited = b;
}

int NFCPluginManager::RegisterAppTask(NF_SERVER_TYPE eServerType, uint32_t taskType, const std::string& desc, uint32_t taskGroup)
{
	return m_appInited.RegisterAppTask(eServerType, taskType, desc, taskGroup);
}

int NFCPluginManager::FinishAppTask(NF_SERVER_TYPE eServerType, uint32_t taskType, uint32_t taskGroup)
{
	return m_appInited.FinishAppTask(eServerType, taskType, taskGroup);
}

/**
 * @brief 发送dump信息到服务器
 *
 * 该函数用于将dump信息发送到指定的服务器。如果当前已经加载了所有服务器，则直接返回0。
 *
 * @param dumpInfo 需要发送的dump信息，类型为std::string
 * @return int 返回0表示成功，返回-1表示失败
 */
int NFCPluginManager::SendDumpInfo(const std::string& dumpInfo)
{
	// 检查是否已经加载了所有服务器，如果是则直接返回0
	if (IsLoadAllServer()) return 0;

	// 获取服务器配置信息
	NFServerConfig* pConfig = NFIPluginManager::FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_NONE);
	CHECK_NULL(0, pConfig);

	// 创建dump信息通知对象，并设置dump信息和bus_id
	NFrame::Proto_ServerDumpInfoNtf ntf;
	ntf.set_dump_info(dumpInfo);
	ntf.set_bus_id(pConfig->BusId);

	// 获取主服务器数据，如果获取失败则返回-1
	auto pServerData = FindModule<NFIMessageModule>()->GetMasterData(static_cast<NF_SERVER_TYPE>(pConfig->ServerType));
	CHECK_EXPR(pServerData, -1, "pServerData == NULL, eType error:{}", (int) pConfig->ServerType);

	// 发送dump信息通知到指定的服务器
	FindModule<NFIMessageModule>()->Send(pServerData->mUnlinkId, NF_MODULE_FRAME, NFrame::NF_STS_SEND_DUMP_INFO_NTF, ntf);
	return 0;
}


bool NFCPluginManager::HotfixServer()
{
	bool ret = true;
	for (auto iter = m_nPluginInstanceList.begin(); iter != m_nPluginInstanceList.end(); ++iter)
	{
		if ((*iter)->HotfixServer() == false)
		{
			ret = false;
		}
	}

	return ret;
}

std::list<NFIPlugin*> NFCPluginManager::GetListPlugin()
{
	return m_nPluginInstanceList;
}

std::string NFCPluginManager::GetMachineAddrMD5()
{
	return m_iMachineAddrMD5;
}

bool NFCPluginManager::LoadKernelPlugin()
{
#ifndef NF_DYNAMIC_PLUGIN
	m_nPluginNameVec.emplace_back("NFKernelPlugin");
	LoadStaticPlugin("NFKernelPlugin");
#else
    m_nPluginNameVec.push_back("NFKernelPlugin");
    LoadPluginLibrary("NFKernelPlugin");
#endif

	/*
	    log 系统第一个启动，然后是配置
	*/
	FindModule<NFILogModule>()->InitLogSystem();
	/*
	    加载服务器配置
	*/
	FindModule<NFIConfigModule>()->LoadConfig();

	return true;
}

bool NFCPluginManager::OnServerKilling()
{
	NFLogInfo(NF_LOG_DEFAULT, 0, "NFPluginManager OnServerKilling................");
	bool ret = true;
	for (auto iter = m_nPluginInstanceList.begin(); iter != m_nPluginInstanceList.end(); ++iter)
	{
		if ((*iter)->OnServerKilling() == false)
		{
			ret = false;
		}
	}

	return ret;
}

bool NFCPluginManager::OnStopServer()
{
	NFLogInfo(NF_LOG_DEFAULT, 0, "NFPluginManager StopServer................");
	bool ret = true;
	for (auto iter = m_nPluginInstanceList.begin(); iter != m_nPluginInstanceList.end(); ++iter)
	{
		if ((*iter)->StopServer() == false)
		{
			ret = false;
		}
	}

	return ret;
}

bool NFCPluginManager::CheckStopServer()
{
	NFLogInfo(NF_LOG_DEFAULT, 0, "NFPluginManager CheckStopServer................");
	bool ret = true;
	for (auto iter = m_nPluginInstanceList.begin(); iter != m_nPluginInstanceList.end(); ++iter)
	{
		if ((*iter)->CheckStopServer() == false)
		{
			ret = false;
		}
	}

	return ret;
}

bool NFCPluginManager::StopServer()
{
	NFLogInfo(NF_LOG_DEFAULT, 0, "NFPluginManager StopServer................");
	bool ret = CheckStopServer();
	if (ret == false)
	{
		OnStopServer();
		return false;
	}

	return true;
}

bool NFCPluginManager::AfterAllConnectFinish()
{
	NFLogInfo(NF_LOG_DEFAULT, 0, "NFPluginManager AfterAllConnectFinish................");
	for (auto iter = m_nPluginInstanceList.begin(); iter != m_nPluginInstanceList.end(); ++iter)
	{
		(*iter)->AfterAllConnectFinish();
	}

	return true;
}

bool NFCPluginManager::AfterAllDescStoreLoaded()
{
	NFLogInfo(NF_LOG_DEFAULT, 0, "NFPluginManager AfterAllDescStoreLoaded................");
	for (auto iter = m_nPluginInstanceList.begin(); iter != m_nPluginInstanceList.end(); ++iter)
	{
		(*iter)->AfterAllDescStoreLoaded();
	}

	return true;
}

bool NFCPluginManager::AfterObjFromDBLoaded()
{
	NFLogInfo(NF_LOG_DEFAULT, 0, "NFPluginManager AfterObjFromDBLoaded................");
	for (auto iter = m_nPluginInstanceList.begin(); iter != m_nPluginInstanceList.end(); ++iter)
	{
		(*iter)->AfterObjFromDBLoaded();
	}

	return true;
}

bool NFCPluginManager::AfterServerRegisterFinish()
{
	NFLogInfo(NF_LOG_DEFAULT, 0, "NFPluginManager AfterServerRegisterFinish................");
	for (auto iter = m_nPluginInstanceList.begin(); iter != m_nPluginInstanceList.end(); ++iter)
	{
		(*iter)->AfterServerRegisterFinish();
	}

	return true;
}

bool NFCPluginManager::AfterAppInitFinish()
{
	NFLogInfo(NF_LOG_DEFAULT, 0, "NFPluginManager AfterAppInitFinish................");
	for (auto iter = m_nPluginInstanceList.begin(); iter != m_nPluginInstanceList.end(); ++iter)
	{
		(*iter)->AfterAppInitFinish();
	}

	return true;
}
