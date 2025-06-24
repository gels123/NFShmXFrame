// -------------------------------------------------------------------------
//    @FileName         :    NFIPluginManager.h
//    @Author           :    LvSheng.Huang
//    @Date             :   2022-09-18
//    @Module           :    NFIPluginManager
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFPluginModule/NFServerDefine.h"
#include "NFComm/NFPluginModule/NFGlobalSystem.h"
#include "NFSystemInfo.h"
#include <functional>
#include <list>

#define ALL_SERVER ("AllServer")

class NFIModule;

class NFIPlugin;

class NFIPluginManager;

typedef std::function<NFIModule *(NFIPluginManager *pMan)> CREATE_ALONE_MODULE;

typedef std::function<NFIPlugin *(NFIPluginManager *pMan)> CREATE_PLUGIN_FUNCTION;

template<typename T>
static T* FindModule()
{
	static T* pStaticModule = NULL;
	if (NFGlobalSystem::Instance()->IsMoreServer())
	{
		NFIModule* pLogicModule = NFGlobalSystem::Instance()->FindModule(typeid(T).name());
		if (pLogicModule)
		{
			if (!TIsDerived<T, NFIModule>::Result)
			{
				return nullptr;
			}

			//OSX上dynamic_cast返回了NULL
#if NF_PLATFORM == NF_PLATFORM_APPLE
			T* pT = (T*)pLogicModule;
#else
			T* pT = dynamic_cast<T *>(pLogicModule);
#endif

			pStaticModule = pT;
			return pT;
		}
		return nullptr;
	}
	if (pStaticModule == NULL)
	{
		NFIModule* pLogicModule = NFGlobalSystem::Instance()->FindModule(typeid(T).name());
		if (pLogicModule)
		{
			if (!TIsDerived<T, NFIModule>::Result)
			{
				return nullptr;
			}

			//OSX上dynamic_cast返回了NULL
#if NF_PLATFORM == NF_PLATFORM_APPLE
			T* pT = (T*)pLogicModule;
#else
			T* pT = dynamic_cast<T *>(pLogicModule);
#endif

			pStaticModule = pT;
			return pT;
		}
		return nullptr;
	}
	return pStaticModule;
}

class NFIPluginManager {
public:
	/**
	 * @brief NFIPluginManager 类的默认构造函数。
	 *
	 * 该构造函数使用默认实现，不执行任何特殊操作。
	 */
	NFIPluginManager() = default;

	/**
	 * @brief NFIPluginManager 类的虚析构函数。
	 *
	 * 该析构函数使用默认实现，确保派生类对象能够正确释放资源。
	 */
	virtual ~NFIPluginManager() = default;

	/**
	 * @brief 在所有插件加载完成后调用的虚函数。
	 *
	 * 该函数提供了一个钩子，允许在插件加载完成后执行一些自定义操作。
	 * 默认实现返回 true，表示操作成功。
	 *
	 * @return bool 返回 true 表示操作成功，false 表示操作失败。
	 */
	virtual bool AfterLoadAllPlugin() {
	    return true;
	}

	/**
	 * @brief 在共享内存初始化完成后调用的虚函数。
	 *
	 * 该函数提供了一个钩子，允许在共享内存初始化完成后执行一些自定义操作。
	 * 默认实现返回 true，表示操作成功。
	 *
	 * @return bool 返回 true 表示操作成功，false 表示操作失败。
	 */
	virtual bool AfterInitShmMem() {
	    return true;
	}

	/**
	 * @brief Awake函数用于在对象初始化之前执行一些准备工作。
	 * @return 返回true表示准备成功，false表示准备失败。
	 */
	virtual bool Awake() {
	    return true;
	}

	/**
	 * @brief Init函数用于初始化对象的状态或资源。
	 * @return 返回true表示初始化成功，false表示初始化失败。
	 */
	virtual bool Init() {
	    return true;
	}

	/**
	 * @brief CheckConfig函数用于检查配置是否正确或有效。
	 * @return 返回true表示配置检查通过，false表示配置检查失败。
	 */
	virtual bool CheckConfig() {
	    return true;
	}

	/**
	 * @brief ReadyExecute函数用于在执行前进行最后的准备工作。
	 * @return 返回true表示准备就绪，false表示准备未完成。
	 */
	virtual bool ReadyExecute() {
	    return true;
	}

	/**
	 * @brief Execute函数用于执行主要的逻辑或任务。
	 * @return 返回true表示执行成功，false表示执行失败。
	 */
	virtual bool Execute() {
	    return true;
	}

	/**
	 * @brief 在关闭之前执行的函数，通常用于执行一些关闭前的准备工作。
	 * @return 返回 true 表示准备成功，返回 false 表示准备失败。
	 */
	virtual bool BeforeShut() {
	    return true;
	}

	/**
	 * @brief 关闭函数，通常用于执行实际的关闭操作。
	 * @return 返回 true 表示关闭成功，返回 false 表示关闭失败。
	 */
	virtual bool Shut() {
	    return true;
	}

	/**
	 * @brief 最终化函数，通常用于执行一些关闭后的清理工作。
	 * @return 返回 true 表示最终化成功，返回 false 表示最终化失败。
	 */
	virtual bool Finalize() {
	    return true;
	}

	/**
	 * @brief 当配置重新加载时执行的函数，通常用于处理配置更新后的逻辑。
	 * @return 返回 true 表示处理成功，返回 false 表示处理失败。
	 */
	virtual bool OnReloadConfig() {
	    return true;
	}

	/**
	 * @brief 在配置重新加载后执行的函数，通常用于执行一些配置更新后的后续操作。
	 * @return 返回 true 表示后续操作成功，返回 false 表示后续操作失败。
	 */
	virtual bool AfterOnReloadConfig() {
	    return true;
	}

	/**
	 * @brief 服务器连接完成后
	 * @return
	 */
	virtual bool AfterAllConnectFinish() {
		return true;
	}

	/**
	 * @brief 加载完服务器数据，包过excel, 以及从数据拉取的数据
	 * @return
	 */
	virtual bool AfterAllDescStoreLoaded() {
		return true;
	}

	/**
	 * @brief 从db加载全局数据, 这个加载一定在完成连接后，有可能依赖descstore数据，也可能不依赖
	 * @return
	 */
	virtual bool AfterObjFromDBLoaded() {
		return true;
	}

	/**
	 * @brief 完成服务器之间的注册
	 * @return
	 */
	virtual bool AfterServerRegisterFinish() {
		return true;
	}

	/**
	 * @brief  服务器完成初始化之后
	 * @return
	 */
	virtual bool AfterAppInitFinish()
	{
		return true;
	}

	template<typename T>
	T* FindModule()
	{
		static T* pStaticModule = NULL;
		if (NFGlobalSystem::Instance()->IsMoreServer())
		{
			NFIModule* pLogicModule = FindModule(typeid(T).name());
			if (pLogicModule)
			{
				if (!TIsDerived<T, NFIModule>::Result)
				{
					return nullptr;
				}

				//OSX上dynamic_cast返回了NULL
#if NF_PLATFORM == NF_PLATFORM_APPLE
                T* pT = (T*)pLogicModule;
#else
				T* pT = dynamic_cast<T *>(pLogicModule);
#endif

				pStaticModule = pT;
				return pT;
			}
			return nullptr;
		}
		if (pStaticModule == NULL)
		{
			NFIModule* pLogicModule = FindModule(typeid(T).name());
			if (pLogicModule)
			{
				if (!TIsDerived<T, NFIModule>::Result)
				{
					return nullptr;
				}

				//OSX上dynamic_cast返回了NULL
#if NF_PLATFORM == NF_PLATFORM_APPLE
                    T* pT = (T*)pLogicModule;
#else
				T* pT = dynamic_cast<T *>(pLogicModule);
#endif

				pStaticModule = pT;
				return pT;
			}
			return nullptr;
		}
		return pStaticModule;
	}


	/**
	 * @brief 开始执行某个操作，通常用于初始化或启动某个过程。
	 * @return 返回一个布尔值，表示操作是否成功启动。
	 */
	virtual bool Begin() = 0;

	/**
	 * @brief 结束执行某个操作，通常用于清理或终止某个过程。
	 * @return 返回一个布尔值，表示操作是否成功结束。
	 */
	virtual bool End() = 0;

	/**
	 * @brief 注册一个静态插件，通常用于在系统中注册插件的创建函数。
	 * @param strPluginName 插件的名称。
	 * @param createFunc 插件的创建函数。
	 */
	virtual void RegisteredStaticPlugin(const std::string &strPluginName, const CREATE_PLUGIN_FUNCTION &createFunc) = 0;

	/**
	 * @brief 注册一个插件实例，通常用于在系统中注册已创建的插件。
	 * @param plugin 指向插件实例的指针。
	 */
	virtual void Registered(NFIPlugin *plugin) = 0;

	/**
	 * @brief 注销一个插件实例，通常用于从系统中移除已注册的插件。
	 * @param plugin 指向插件实例的指针。
	 */
	virtual void UnRegistered(NFIPlugin *plugin) = 0;

	/**
	 * @brief 根据插件名称查找已注册的插件。
	 * @param strPluginName 插件的名称。
	 * @return 返回指向找到的插件实例的指针，如果未找到则返回nullptr。
	 */
	virtual NFIPlugin *FindPlugin(const std::string &strPluginName) = 0;

	/**
	 * @brief 向系统中添加一个模块。
	 * @param strModuleName 模块的名称。
	 * @param pModule 指向模块实例的指针。
	 * @return 返回一个整数值，表示操作的结果或状态。
	 */
	virtual int AddModule(const std::string &strModuleName, NFIModule *pModule) = 0;

	/**
	 * @brief 从系统中移除一个模块。
	 * @param strModuleName 模块的名称。
	 */
	virtual void RemoveModule(const std::string &strModuleName) = 0;

	/**
	 * @brief 根据模块名称查找已注册的模块。
	 * @param strModuleName 模块的名称。
	 * @return 返回指向找到的模块实例的指针，如果未找到则返回nullptr。
	 */
	virtual NFIModule *FindModule(const std::string &strModuleName) = 0;

	/**
	 * @brief 加载所有已注册的插件。
	 * @return 返回一个布尔值，表示所有插件是否成功加载。
	 */
	virtual bool LoadAllPlugin() = 0;

	/**
	 * @brief 加载指定的插件库。
	 * @param strPluginDLLName 插件库的文件名或路径。
	 * @return 返回一个布尔值，表示插件库是否成功加载。
	 */
	virtual bool LoadPluginLibrary(const std::string &strPluginDLLName) = 0;

	/**
	 * @brief 卸载指定的插件库。
	 * @param strPluginDLLName 插件库的文件名或路径。
	 * @return 返回一个布尔值，表示插件库是否成功卸载。
	 */
	virtual bool UnLoadPluginLibrary(const std::string &strPluginDLLName) = 0;

	/**
	 * @brief 动态加载指定的插件库。
	 * @param strPluginDLLName 插件库的文件名或路径。
	 * @return 返回一个布尔值，表示插件库是否成功动态加载。
	 */
	virtual bool DynamicLoadPluginLibrary(const std::string &strPluginDLLName) = 0;

	/**
	 * @brief 获取完整路径
	 * @return 返回当前对象的完整路径
	 */
	virtual const std::string &GetFullPath() const = 0;

	/**
	 * @brief 设置完整路径
	 * @param strFullPath 要设置的完整路径
	 */
	virtual void SetFullPath(const std::string &strFullPath) = 0;

	/**
	 * @brief 获取应用程序ID
	 * @return 返回当前对象的应用程序ID
	 */
	virtual int GetAppID() const = 0;

	/**
	 * @brief 设置应用程序ID
	 * @param nAppID 要设置的应用程序ID
	 */
	virtual void SetAppID(int nAppID) = 0;

	/**
	 * @brief 获取世界ID
	 * @return 返回当前对象的世界ID
	 */
	virtual int GetWorldID() const = 0;

	/**
	 * @brief 获取区域ID
	 * @return 返回当前对象的区域ID
	 */
	virtual int GetZoneID() const = 0;

	/**
	 * @brief 获取配置文件路径
	 * @return 返回当前对象的配置文件路径
	 */
	virtual const std::string &GetConfigPath() const = 0;

	/**
	 * @brief 设置配置文件路径
	 * @param strPath 要设置的配置文件路径
	 */
	virtual void SetConfigPath(const std::string &strPath) = 0;

	/**
	 * @brief 获取插件路径
	 * @return 返回当前对象的插件路径
	 */
	virtual const std::string &GetPluginPath() const = 0;

	/**
	 * @brief 设置插件路径
	 * @param strPath 要设置的插件路径
	 */
	virtual void SetPluginPath(const std::string &strPath) = 0;

	/**
	 * @brief 获取应用程序名称
	 * @return 返回当前对象的应用程序名称
	 */
	virtual const std::string &GetAppName() const = 0;

	/**
	 * @brief 设置应用程序名称
	 * @param strAppName 要设置的应用程序名称
	 */
	virtual void SetAppName(const std::string &strAppName) = 0;

	/**
	 * @brief 获取字符串参数
	 * @return 返回当前对象的字符串参数
	 */
	virtual const std::string &GetStrParam() const = 0;

	/**
	 * @brief 设置字符串参数
	 * @param strAppName 要设置的字符串参数
	 */
	virtual void SetStrParam(const std::string &strAppName) = 0;

	/**
	 * @brief 获取日志路径
	 * @return 返回当前对象的日志路径
	 */
	virtual const std::string &GetLogPath() const = 0;

	/**
	 * @brief 设置日志路径
	 * @param strName 要设置的日志路径
	 */
	virtual void SetLogPath(const std::string &strName) = 0;

	/**
	 * @brief 判断是否加载所有服务器
	 * @return 返回是否加载所有服务器的布尔值
	 */
	virtual bool IsLoadAllServer() const = 0;

	/**
	 * @brief 设置是否加载所有服务器
	 * @param b 要设置的布尔值
	 */
	virtual void SetLoadAllServer(bool b) = 0;

	/**
	 * @brief 设置Lua脚本路径
	 * @param luaScriptPath 要设置的Lua脚本路径
	 */
	virtual void SetLuaScriptPath(const std::string &luaScriptPath) = 0;

	/**
	 * @brief 设置游戏名称
	 * @param game 要设置的游戏名称
	 */
	virtual void SetGame(const std::string &game) = 0;

	/**
	 * @brief 获取Lua脚本路径
	 * @return 返回当前对象的Lua脚本路径
	 */
	virtual const std::string &GetLuaScriptPath() const = 0;

	/**
	 * @brief 获取游戏名称
	 * @return 返回当前对象的游戏名称
	 */
	virtual const std::string &GetGame() const = 0;

	/**
	 * @brief 获取初始化时间
	 * @return 返回当前对象的初始化时间
	 */
	virtual uint64_t GetInitTime() const = 0;

	/**
	 * @brief 获取当前时间
	 * @return 返回当前对象的当前时间
	 */
	virtual uint64_t GetNowTime() const = 0;

	/**
	 * @brief 判断是否为守护进程
	 * @return 返回是否为守护进程的布尔值
	 */
	virtual bool IsDaemon() const = 0;

	/**
	 * @brief 设置为守护进程
	 */
	virtual void SetDaemon() = 0;

	/**
	 * @brief 设置是否开启性能分析器
	 * @param b 要设置的布尔值
	 */
	virtual void SetOpenProfiler(bool b) = 0;

	/**
	 * @brief 判断是否开启性能分析器
	 * @return 返回是否开启性能分析器的布尔值
	 */
	virtual bool IsOpenProfiler() = 0;

	/**
	 * @brief 开始性能分析
	 * @param funcName 要分析的函数名称
	 */
	virtual void BeginProfiler(const std::string &funcName) = 0;

	/**
	 * @brief 结束性能分析
	 * @return 返回一个64位无符号整数，表示从开始性能分析到结束所经过的时间或其他性能指标。
	 */
	virtual uint64_t EndProfiler() = 0;

	/*
	 * stop server，停服，意味着需要保存该保存的数据，共享内存可能后面会被清理，服务器会走正常停服流程
	 * */
	virtual bool IsServerStopping() const = 0;

	/*
	 * stop server，停服，意味着需要保存该保存的数据，共享内存可能后面会被清理，服务器会走正常停服流程
	 * */
	virtual void SetServerStopping(bool exitApp) = 0;

	/*
	 * stop server，停服，意味着需要保存该保存的数据，共享内存可能后面会被清理，服务器会走正常停服流程
	 * */
	virtual bool StopServer() = 0;

	/*
	 * 停服之前，检查服务器是否满足停服条件
	 * */
	virtual bool CheckStopServer() = 0;

	/*
	 * 停服之前，做一些操作，满足停服条件
	 * */
	virtual bool OnStopServer() = 0;

	/*
	 * 服务器被杀掉，服务器退出
	 * */
	virtual bool OnServerKilling() = 0;

	/*
	 * reload server 重新加载服务器的配置数据
	 * */
	virtual bool IsReloadServer() const = 0;

	/*
	 * SetReloadServer - 重新加载服务器的配置数据
	 *
	 * 该函数是一个纯虚函数，用于通知服务器重新加载其配置数据。具体实现由派生类提供。
	 *
	 * @param exitApp: 布尔值，指示是否在重新加载配置后退出应用程序。
	 *                 - true: 重新加载配置后退出应用程序。
	 *                 - false: 仅重新加载配置，不退出应用程序。
	 *
	 * @return: 无返回值。
	 */
	virtual void SetReloadServer(bool exitApp) = 0;

	/**
	 * @brief 获取是否在更改配置文件时退出应用程序。
	 *
	 * 该函数用于查询当前是否在更改配置文件时退出应用程序。
	 *
	 * @return bool 返回值为 true 表示在更改配置文件时退出应用程序，false 表示不退出。
	 */
	virtual bool GetChangeProfileApp() const = 0;

	/**
	 * @brief 设置是否在更改配置文件时退出应用程序。
	 *
	 * 该函数用于设置是否在更改配置文件时退出应用程序。
	 *
	 * @param exitApp 如果为 true，表示在更改配置文件时退出应用程序；如果为 false，表示不退出。
	 */
	virtual void SetChangeProfileApp(bool exitApp) = 0;

	/**
	 * @brief 获取是否在启动新应用程序时终止前一个应用程序。
	 *
	 * 该函数用于查询当前是否在启动新应用程序时终止前一个应用程序。
	 *
	 * @return bool 返回值为 true 表示在启动新应用程序时终止前一个应用程序，false 表示不终止。
	 */
	virtual bool GetKillPreApp() const = 0;

	/**
	 * @brief 设置是否在启动新应用程序时终止前一个应用程序。
	 *
	 * 该函数用于设置是否在启动新应用程序时终止前一个应用程序。
	 *
	 * @param exitApp 如果为 true，表示在启动新应用程序时终止前一个应用程序；如果为 false，表示不终止。
	 */
	virtual void SetKillPreApp(bool exitApp) = 0;

	/*
	 * 热更退出app, 用于服务器需要热更app代码的情况，这时候会杀掉正在运行的的的app,重启新的服务器app
	 * */
	virtual bool IsHotfixServer() const = 0;

	/*
	 * 热更退出app, 用于服务器需要热更app代码的情况，这时候会杀掉正在运行的的的app,重启新的服务器app
	 * */
	virtual void SetHotfixServer(bool exitApp) = 0;

	/*
	 * 热更退出app, 用于服务器需要热更app代码的情况，这时候会杀掉正在运行的的的app,重启新的服务器app
	 * */
	virtual bool HotfixServer() = 0;

	/**
	 * @brief 检查共享内存是否已初始化。
	 *
	 * 该函数用于判断共享内存是否已经成功初始化。
	 *
	 * @return bool 返回true表示共享内存已初始化，false表示未初始化。
	 */
	virtual bool IsInitShm() const = 0;

	/**
	 * @brief 设置共享内存为已初始化状态。
	 *
	 * 该函数用于标记共享内存已经成功初始化。
	 */
	virtual void SetInitShm() = 0;

	/**
	 * @brief 设置总线名称。
	 *
	 * 该函数用于设置当前对象关联的总线名称。
	 *
	 * @param busName 总线名称，类型为std::string。
	 */
	virtual void SetBusName(const std::string &busName) = 0;

	/**
	 * @brief 获取总线名称。
	 *
	 * 该函数用于获取当前对象关联的总线名称。
	 *
	 * @return const std::string& 返回总线名称的引用。
	 */
	virtual const std::string &GetBusName() const = 0;

	/**
	 * @brief 设置PID文件名。
	 *
	 * 该函数用于设置当前对象关联的PID文件名。
	 */
	virtual void SetPidFileName() = 0;

	/**
	 * @brief 获取PID文件名。
	 *
	 * 该函数用于获取当前对象关联的PID文件名。
	 *
	 * @return const std::string& 返回PID文件名的引用。
	 */
	virtual const std::string &GetPidFileName() = 0;

#if NF_PLATFORM == NF_PLATFORM_LINUX
	virtual int TimedWait(pid_t pid, int sec) = 0;
#else
	/**
	 * @brief Windows版本的带超时等待进程结束。
	 * @param proc_id Windows进程 ID。
	 * @param sec 超时时间（秒）。
	 * @return 返回进程退出状态码。
	 */
	virtual int TimedWait(DWORD proc_id, int sec) = 0;
#endif

	/**
	* @brief Windows版本的检查 PID 文件是否存在。
	* @return 如果存在返回 0，否则返回非零值。
	*/
	virtual int CheckPidFile() = 0;

	/**
	 * @brief Windows版本的创建 PID 文件。
	 * @return 成功返回 0，失败返回非零值。
	 */
	virtual int CreatePidFile() = 0;

	/**
	 * @brief Windows版本的杀死之前的应用程序实例。
	 * @return 成功返回 0，失败返回非零值。
	 */
	virtual int KillPreApp() = 0;

	/**
	 * @brief Windows版本的停止应用程序。
	 */
	virtual void StopApp() = 0;

	/**
	 * @brief Windows版本的重新加载应用程序。
	 */
	virtual void ReloadApp() = 0;

	/**
	 * @brief Windows版本的退出应用程序。
	 */
	virtual void QuitApp() = 0;

	/**
	 * @brief 获取当前帧的编号。
	 * @return 返回当前帧的编号。
	 */
	virtual uint32_t GetFrame() const = 0;

	/**
	 * @brief 获取当前帧的时间。
	 * @return 返回当前帧的时间，单位为毫秒。
	 */
	virtual uint32_t GetFrameTime() const = 0;

	/**
	 * @brief 获取当前帧的计数。
	 * @return 返回当前帧的计数。
	 */
	virtual uint32_t GetCurFrameCount() const = 0;

	/**
	 * @brief 判断当前帧是否为固定帧。
	 * @return 返回true表示当前帧为固定帧，否则返回false。
	 */
	virtual bool IsFixedFrame() const = 0;

	/**
	 * @brief 设置当前帧是否为固定帧。
	 * @param frame 设置为true表示当前帧为固定帧，false表示非固定帧。
	 */
	virtual void SetFixedFrame(bool frame) = 0;

	/**
	 * @brief 获取空闲时的睡眠时间。
	 * @return 返回空闲时的睡眠时间，单位为微秒。
	 */
	virtual uint32_t GetIdleSleepUs() const = 0;

	/**
	 * @brief 设置空闲时的睡眠时间。
	 * @param time 空闲时的睡眠时间，单位为微秒。
	 */
	virtual void SetIdleSleepUs(uint32_t time) = 0;

	/**
	 * @brief 判断是否已经初始化。
	 * @return 返回true表示已经初始化，否则返回false。
	 */
	virtual bool IsInited() const = 0;

	/**
	 * @brief 判断指定服务器类型是否已经初始化。
	 * @param eServerType 服务器类型。
	 * @return 返回true表示指定服务器类型已经初始化，否则返回false。
	 */
	virtual bool IsInited(NF_SERVER_TYPE eServerType) const = 0;

	/**
	 * @brief 设置初始化状态。
	 * @param b 设置为true表示已初始化，false表示未初始化。
	 */
	virtual void SetIsInited(bool b) = 0;

	/**
	 * @brief 注册应用程序任务。
	 * @param eServerType 服务器类型。
	 * @param taskType 任务类型。
	 * @param desc 任务描述。
	 * @param taskGroup 任务组。
	 * @return 返回注册结果，0表示成功，非0表示失败。
	 */
	virtual int RegisterAppTask(NF_SERVER_TYPE eServerType, uint32_t taskType, const std::string &desc,
	                            uint32_t taskGroup) = 0;

	/**
	 * @brief 完成应用程序任务。
	 * @param eServerType 服务器类型。
	 * @param taskType 任务类型。
	 * @param taskGroup 任务组。
	 * @return 返回完成结果，0表示成功，非0表示失败。
	 */
	virtual int FinishAppTask(NF_SERVER_TYPE eServerType, uint32_t taskType,
	                          uint32_t taskGroup) = 0;

	/**
	 * @brief 判断指定服务器类型的任务组是否已完成。
	 * @param eServerType 服务器类型。
	 * @param taskGroup 任务组。
	 * @return 返回true表示任务组已完成，否则返回false。
	 */
	virtual bool IsFinishAppTask(NF_SERVER_TYPE eServerType, uint32_t taskGroup) const = 0;

	/**
	 * @brief 判断指定服务器类型的任务组是否存在。
	 * @param eServerType 服务器类型。
	 * @param taskGroup 任务组。
	 * @return 返回true表示任务组存在，否则返回false。
	 */
	virtual bool IsHasAppTask(NF_SERVER_TYPE eServerType, uint32_t taskGroup) const = 0;

	/**
	 * @brief 判断指定服务器类型的任务组中是否存在指定类型的任务。
	 * @param eServerType 服务器类型。
	 * @param taskGroup 任务组。
	 * @param taskType 任务类型。
	 * @return 返回true表示任务存在，否则返回false。
	 */
	virtual bool IsHasAppTask(NF_SERVER_TYPE eServerType, uint32_t taskGroup, uint32_t taskType) const = 0;

	/**
	 * @brief 发送转储信息。
	 * @param dmpInfo 转储信息。
	 * @return 返回发送结果，0表示成功，非0表示失败。
	 */
	virtual int SendDumpInfo(const std::string &dmpInfo) = 0;

	/**
	 * @brief 获取插件列表
	 *
	 * 该函数是一个纯虚函数，用于获取当前系统中所有插件的列表。
	 * 子类必须实现该函数以返回具体插件列表。
	 *
	 * @return std::list<NFIPlugin*> 返回一个包含所有插件的列表，列表中的每个元素都是指向NFIPlugin对象的指针。
	 */
	virtual std::list<NFIPlugin *> GetListPlugin() = 0;

	/**
	 * @brief 获取机器地址的MD5值
	 *
	 * 该函数是一个纯虚函数，用于获取当前机器地址的MD5哈希值。
	 * 子类必须实现该函数以返回具体的MD5值。
	 *
	 * @return std::string 返回一个字符串，表示当前机器地址的MD5哈希值。
	 */
	virtual std::string GetMachineAddrMD5() = 0;
};

