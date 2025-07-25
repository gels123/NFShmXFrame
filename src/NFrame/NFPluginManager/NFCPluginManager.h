// -------------------------------------------------------------------------
//    @FileName         :    NFCPluginManager.h
//    @Author           :    LvSheng.Huang
//    @Date             :   2022-09-18
//    @Module           :    NFCPluginManager
//
// -------------------------------------------------------------------------

#ifndef NFC_PLUGIN_MANAGER_H
#define NFC_PLUGIN_MANAGER_H

#include <map>
#include <string>
#include <list>
#include <atomic>
#include "NFComm/NFPluginModule/NFIPluginManager.h"
#include "NFComm/NFCore/NFRandom.hpp"
#include "NFCDynLib.h"
#include "NFComm/NFPluginModule/NFProfiler.h"
#include "NFCAppInited.h"

/**
 * @brief 系统的模块管理器，负责加载、初始化、执行和销毁插件及其模块。
 *
 * NFCPluginManager 类实现了 NFIPluginManager 接口，提供了对插件和模块的生命周期管理，
 * 包括插件的加载、初始化、执行和销毁等操作。它还处理服务器启动、停止、重新加载配置等任务。
 */
class NFCPluginManager final : public NFIPluginManager
{
public:
	NFCPluginManager();

	~NFCPluginManager() override;

	/**
	 * @brief 开始初始化插件管理器。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool Begin() override;

	/**
	 * @brief 结束插件管理器的运行。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool End() override;

	/**
	 * @brief 所有插件加载完成后调用。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool AfterLoadAllPlugin() override;

	/**
	 * @brief 共享内存初始化后调用。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool AfterInitShmMem() override;

	/**
	 * @brief 插件管理器的 Awake 阶段。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool Awake() override;

	/**
	 * @brief 插件管理器的 Init 阶段。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool Init() override;

	/**
	 * @brief 检查配置文件是否正确。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool CheckConfig() override;

	/**
	 * @brief 准备执行阶段。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool ReadyExecute() override;

	/**
	 * @brief 执行阶段。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool Execute() override;

	/**
	 * @brief 关闭前的准备工作。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool BeforeShut() override;

	/**
	 * @brief 关闭插件管理器。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool Shut() override;

	/**
	 * @brief 最终化阶段。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool Finalize() override;

	/**
	 * @brief 重新加载配置文件时调用。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool OnReloadConfig() override;

	/**
	 * @brief 重新加载配置文件后的处理。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool AfterOnReloadConfig() override;

	/**
	 * @brief 服务器连接完成后调用。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool AfterAllConnectFinish() override;

	/**
	 * @brief 加载完服务器数据（如 Excel 和数据库数据）后调用。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool AfterAllDescStoreLoaded() override;

	/**
	 * @brief
	 * @return AfterAllConnectFinish 和 AfterAllDescStoreLoaded 都完成后
	 */
	bool AfterAllConnectAndAllDescStore() override;

	/**
	 * @brief 从数据库加载全局数据后调用。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool AfterObjFromDBLoaded() override;

	/**
	 * @brief 完成服务器之间的注册后调用。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool AfterServerRegisterFinish() override;

	/**
	 * @brief 服务器完成初始化后调用。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool AfterAppInitFinish() override;

	/**
	 * @brief 服务器连接完成后调用。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool AfterAllConnectFinish(NF_SERVER_TYPE serverType) override;

	/**
	 * @brief 加载完服务器数据（如 Excel 和数据库数据）后调用。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool AfterAllDescStoreLoaded(NF_SERVER_TYPE serverType) override;

	/**
	 * @brief
	 * @return AfterAllConnectFinish 和 AfterAllDescStoreLoaded 都完成后
	 */
	bool AfterAllConnectAndAllDescStore(NF_SERVER_TYPE serverType) override;

	/**
	 * @brief 从数据库加载全局数据后调用。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool AfterObjFromDBLoaded(NF_SERVER_TYPE serverType) override;

	/**
	 * @brief 完成服务器之间的注册后调用。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool AfterServerRegisterFinish(NF_SERVER_TYPE serverType) override;

	/**
	 * @brief 服务器完成初始化后调用。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool AfterAppInitFinish(NF_SERVER_TYPE serverType) override;

	//////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 注册静态插件。
	 * @param strPluginName 插件名称。
	 * @param createFunc 创建插件的函数指针。
	 */
	void RegisteredStaticPlugin(const std::string& strPluginName, const CREATE_PLUGIN_FUNCTION& createFunc) override;

	/**
	 * @brief 注册插件实例。
	 * @param pPlugin 插件实例指针。
	 */
	void Registered(NFIPlugin* pPlugin) override;

	/**
	 * @brief 取消注册插件实例。
	 * @param pPlugin 插件实例指针。
	 */
	void UnRegistered(NFIPlugin* pPlugin) override;

	//////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 查找指定名称的插件。
	 * @param strPluginName 插件名称。
	 * @return 返回找到的插件实例指针，未找到则返回 nullptr。
	 */
	NFIPlugin* FindPlugin(const std::string& strPluginName) override;

	/**
	 * @brief 添加模块到插件管理器。
	 * @param strModuleName 模块名称。
	 * @param pModule 模块实例指针。
	 * @return 成功返回模块 ID，失败返回 -1。
	 */
	int AddModule(const std::string& strModuleName, NFIModule* pModule) override;

	/**
	 * @brief 移除模块。
	 * @param strModuleName 模块名称。
	 */
	void RemoveModule(const std::string& strModuleName) override;

	/**
	 * @brief 查找指定名称的模块。
	 * @param strModuleName 模块名称。
	 * @return 返回找到的模块实例指针，未找到则返回 nullptr。
	 */
	NFIModule* FindModule(const std::string& strModuleName) override;

	/**
	 * @brief 加载所有插件。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool LoadAllPlugin() override;

	/**
	 * @brief 加载指定的插件库。
	 * @param strPluginDLLName 插件库名称。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool LoadPluginLibrary(const std::string& strPluginDLLName) override;

	/**
	 * @brief 卸载指定的插件库。
	 * @param strPluginDLLName 插件库名称。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool UnLoadPluginLibrary(const std::string& strPluginDLLName) override;

	/**
	 * @brief 动态加载指定的插件库。
	 * @param strPluginDLLName 插件库名称。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool DynamicLoadPluginLibrary(const std::string& strPluginDLLName) override;

	////////////////////////////////////////////////////////////////////////////////////////////////
	/**
	 * @brief 获取应用程序 ID。
	 * @return 返回应用程序 ID。
	 */
	int GetAppID() const override;

	/**
	 * @brief 设置应用程序 ID。
	 * @param nAppID 应用程序 ID。
	 */
	void SetAppID(int nAppID) override;

	/**
	 * @brief 获取世界 ID。
	 * @return 返回世界 ID。
	 */
	int GetWorldID() const override;

	/**
	 * @brief 获取区域 ID。
	 * @return 返回区域 ID。
	 */
	int GetZoneID() const override;

	int GetZoneAreaID() const override;

	/**
	 * @brief 获取配置文件路径。
	 * @return 返回配置文件路径。
	 */
	const std::string& GetConfigPath() const override;

	/**
	 * @brief 设置配置文件路径。
	 * @param strPath 配置文件路径。
	 */
	void SetConfigPath(const std::string& strPath) override;

	/**
	 * @brief 获取插件路径。
	 * @return 返回插件路径。
	 */
	const std::string& GetPluginPath() const override;

	/**
	 * @brief 设置插件路径。
	 * @param strPath 插件路径。
	 */
	void SetPluginPath(const std::string& strPath) override;

	/**
	 * @brief 获取完整路径。
	 * @return 返回完整路径。
	 */
	const std::string& GetFullPath() const override;

	/**
	 * @brief 设置完整路径。
	 * @param strFullPath 应用程序名称。
	 */
	void SetFullPath(const std::string& strFullPath) override;

	/**
	 * @brief 获取字符串参数。
	 * @return 返回字符串参数。
	 */
	const std::string& GetStrParam() const override;

	/**
	 * @brief 设置字符串参数。
	 * @param strAppName 字符串参数。
	 */
	void SetStrParam(const std::string& strAppName) override;

	/**
	 * @brief 获取应用程序名称。
	 * @return 返回应用程序名称。
	 */
	const std::string& GetAppName() const override;

	/**
	 * @brief 设置应用程序名称。
	 * @param strAppName 应用程序名称。
	 */
	void SetAppName(const std::string& strAppName) override;

	/**
	 * @brief 获取日志路径。
	 * @return 返回日志路径。
	 */
	const std::string& GetLogPath() const override;

	/**
	 * @brief 设置日志路径。
	 * @param strName 日志路径。
	 */
	void SetLogPath(const std::string& strName) override;

	/**
	 * @brief 设置 Lua 脚本路径。
	 * @param luaScriptPath Lua 脚本路径。
	 */
	void SetLuaScriptPath(const std::string& luaScriptPath) override;

	void SetGame(const std::string& game) override;

	const std::string& GetLuaScriptPath() const override;

	const std::string& GetGame() const override;

	/**
	 * @brief 设置总线名称。
	 * @param busName 总线名称。
	 */
	void SetBusName(const std::string& busName) override;

	/**
	 * @brief 获取总线名称。
	 * @return 返回总线名称。
	 */
	const std::string& GetBusName() const override;

	/**
	 * @brief 检查是否加载了所有服务器。
	 * @return 如果已加载所有服务器返回 true，否则返回 false。
	 */
	bool IsLoadAllServer() const override;

	/**
	 * @brief 设置是否加载了所有服务器。
	 * @param b 是否加载了所有服务器。
	 */
	void SetLoadAllServer(bool b) override;

	/**
	 * @brief 获取当前帧数。
	 * @return 返回当前帧数。
	 */
	uint32_t GetFrame() const override;

	/**
	 * @brief 获取每帧时间（微秒）。
	 * @return 返回每帧时间（微秒）。
	 */
	uint32_t GetFrameTime() const override;

	/**
	 * @brief 检查是否使用固定帧率。
	 * @return 如果使用固定帧率返回 true，否则返回 false。
	 */
	bool IsFixedFrame() const override;

	/**
	 * @brief 设置是否使用固定帧率。
	 * @param frame 是否使用固定帧率。
	 */
	void SetFixedFrame(bool frame) override;

	/**
	 * @brief 获取空闲睡眠时间（微秒）。
	 * @return 返回空闲睡眠时间（微秒）。
	 */
	uint32_t GetIdleSleepUs() const override;

	/**
	 * @brief 设置空闲睡眠时间（微秒）。
	 * @param time 空闲睡眠时间（微秒）。
	 */
	void SetIdleSleepUs(uint32_t time) override;

	/**
	 * @brief 获取服务器初始化时间（毫秒）。
	 * @return 返回服务器初始化时间（毫秒）。
	 */
	uint64_t GetInitTime() const override;

	/**
	 * @brief 获取当前服务器时间（毫秒）。
	 * @return 返回当前服务器时间（毫秒）。
	 */
	uint64_t GetNowTime() const override;

	/**
	 * @brief 检查服务器是否作为守护进程运行。
	 * @return 如果是守护进程返回 true，否则返回 false。
	 */
	bool IsDaemon() const override;

	/**
	 * @brief 设置服务器为守护进程。
	 */
	void SetDaemon() override;

	/**
	 * @brief 检查是否初始化共享内存。
	 * @return 如果已初始化共享内存返回 true，否则返回 false。
	 */
	bool IsInitShm() const override;

	/**
	 * @brief 设置是否初始化共享内存。
	 */
	void SetInitShm() override;

	/**
	 * @brief 设置 PID 文件名。
	 */
	void SetPidFileName() override;

	/**
	 * @brief 获取 PID 文件名。
	 * @return 返回 PID 文件名。
	 */
	const std::string& GetPidFileName() override;

#if NF_PLATFORM == NF_PLATFORM_LINUX
	/**
	 * @brief 带超时等待子进程结束。
	 * @param pid 子进程 ID。
	 * @param sec 超时时间（秒）。
	 * @return 返回子进程退出状态码。
	 */
	virtual int TimedWait(pid_t pid, int sec) override;
#else
	/**
	 * @brief Windows版本的带超时等待进程结束。
	 * @param proc_id Windows进程 ID。
	 * @param sec 超时时间（秒）。
	 * @return 返回进程退出状态码。
	 */
	virtual int TimedWait(DWORD proc_id, int sec);
#endif

	/**
	 * @brief 检查 PID 文件是否存在。
	 * @return 如果存在返回 0，否则返回非零值。
	 */
	virtual int CheckPidFile() override;

	/**
	 * @brief 创建 PID 文件。
	 * @return 成功返回 0，失败返回非零值。
	 */
	virtual int CreatePidFile() override;

	/**
	 * @brief 杀死之前的应用程序实例。
	 * @return 成功返回 0，失败返回非零值。
	 */
	virtual int KillPreApp() override;

	/**
	 * @brief 停止应用程序。
	 */
	virtual void StopApp() override;

	/**
	 * @brief 重新加载应用程序。
	 */
	virtual void ReloadApp() override;

	/**
	 * @brief 退出应用程序。
	 */
	virtual void QuitApp() override;

	/**
	 * @brief 设置是否开启性能分析器。
	 * @param b 是否开启性能分析器。
	 */
	void SetOpenProfiler(bool b) override;

	/**
	 * @brief 检查是否开启性能分析器。
	 * @return 如果开启返回 true，否则返回 false。
	 */
	bool IsOpenProfiler() override;

	/**
	 * @brief 获取当前帧计数。
	 * @return 返回当前帧计数。
	 */
	uint32_t GetCurFrameCount() const override;

	/**
	 * @brief 检查是否已完成初始化。
	 * @return 如果已完成初始化返回 true，否则返回 false。
	 */
	bool IsInited() const override;

	/**
	 * @brief 检查指定类型的服务器是否已完成初始化。
	 * @param eServerType 服务器类型。
	 * @return 如果已完成初始化返回 true，否则返回 false。
	 */
	bool IsInited(NF_SERVER_TYPE eServerType) const override;


	// 设置初始化状态
	void SetIsInited(bool b) override;

	/**
	 * 注册应用任务
	 *
	 * @param eServerType 服务器类型
	 * @param taskType 任务类型
	 * @param desc 任务描述
	 * @param taskGroup 任务组，默认为服务器连接任务组
	 *
	 * @return 任务注册结果
	 */
	int RegisterAppTask(NF_SERVER_TYPE eServerType, uint32_t taskType, const std::string& desc, uint32_t taskGroup) override;

	/**
	 * 完成应用任务
	 *
	 * @param eServerType 服务器类型
	 * @param taskType 任务类型
	 * @param taskGroup 任务组，默认为服务器连接任务组
	 *
	 * @return 任务完成结果
	 */
	int FinishAppTask(NF_SERVER_TYPE eServerType, uint32_t taskType, uint32_t taskGroup) override;

	/**
	 * 检查应用任务是否完成
	 *
	 * @param eServerType 服务器类型
	 * @param taskGroup 任务组
	 *
	 * @return 如果任务完成返回true，否则返回false
	 */
	bool IsFinishAppTask(NF_SERVER_TYPE eServerType, uint32_t taskGroup) const override;

	/**
	 * 检查是否存在指定任务组的任务
	 *
	 * @param eServerType 服务器类型
	 * @param taskGroup 任务组
	 *
	 * @return 如果存在任务返回true，否则返回false
	 */
	bool IsHasAppTask(NF_SERVER_TYPE eServerType, uint32_t taskGroup) const override;

	/**
	 * 检查是否存在指定任务组和任务类型的任务
	 *
	 * @param eServerType 服务器类型
	 * @param taskGroup 任务组
	 * @param taskType 任务类型
	 *
	 * @return 如果存在任务返回true，否则返回false
	 */
	bool IsHasAppTask(NF_SERVER_TYPE eServerType, uint32_t taskGroup, uint32_t taskType) const override;

	/**
	 * 发送转储信息
	 *
	 * @param dumpInfo 转储信息
	 *
	 * @return 发送结果
	 */
	int SendDumpInfo(const std::string& dumpInfo) override;

	// 获取插件列表
	std::list<NFIPlugin*> GetListPlugin() override;

	// 获取机器地址的MD5值
	std::string GetMachineAddrMD5() override;

	template <typename T>
	T* FindModule()
	{
		return NFIPluginManager::FindModule<T>();
	}

protected:
	/**
	 * 加载静态插件
	 *
	 * @param strPluginName 插件的DLL名称
	 * @return 成功加载返回true，否则返回false
	 */
	bool LoadStaticPlugin(const std::string& strPluginName);

	/**
	 * 卸载静态插件
	 *
	 * @param strPluginName 插件的DLL名称
	 * @return 成功卸载返回true，否则返回false
	 */
	bool UnLoadStaticPlugin(const std::string& strPluginName);

	/**
	 * 加载插件配置
	 *
	 * @return 成功加载配置返回true，否则返回false
	 */
	bool LoadPluginConfig();

	/**
	 * 注册静态插件
	 *
	 * @return 成功注册返回true，否则返回false
	 */
	bool RegisterStaticPlugin();

	/**
	 * 加载内核插件
	 *
	 * @return 成功加载内核插件返回true，否则返回false
	 */
	bool LoadKernelPlugin();

	/**
	 * 开始性能分析器
	 *
	 * @param funcName 函数名称，用于标识性能分析的开始
	 */
	void BeginProfiler(const std::string& funcName) override;

	/**
	 * 结束性能分析器
	 *
	 * @return 返回本次性能分析花费的时间（微秒）
	 */
	uint64_t EndProfiler() override;

	/**
	 * 清除性能分析器的所有数据
	 */
	void ClearProfiler();

	/**
	 * 打印性能分析器的数据
	 */
	void PrintProfiler();

	/**
	 * 获取机器地址
	 *
	 * @param str 用于计算机器地址的字符串
	 * @return 返回计算得到的机器地址
	 */
	int GetMachineAddr(const std::string& str);

private:
	const uint32_t m_nFrame = 30; //服务器帧率，一秒30帧
	const uint32_t m_nFrameTime = 1000 / m_nFrame; //一帧多少时间
	uint32_t m_nCurFrameCount = 0;
	bool m_bFixedFrame;
	uint32_t m_idleSleepUs;

private:
	int m_nAppID;
	uint64_t m_nInitTime; //服务器开始时间，ms
	uint64_t m_nNowTime; //服务器帧时间，ms
	std::string m_mStrBusName;
	std::string m_strConfigPath;
	std::string m_strPluginPath;
	std::string m_strAppName;
	std::string m_strLogPath;
	std::string m_strLuaScriptPath;
	std::string m_strGame;
	std::string m_strFullPath;
	std::string m_strExeName;
	std::string m_strPidFileName;
	std::string m_strParam;
	std::string m_iMachineAddrMD5;

	typedef std::unordered_map<std::string, NFCDynLib*> PluginLibMap;
	typedef std::vector<std::string> PluginNameVec;
	typedef std::unordered_map<std::string, NFIPlugin*> PluginInstanceMap;
	typedef std::list<NFIPlugin*> PluginInstanceList;
	typedef std::unordered_map<std::string, NFIModule*> ModuleInstanceMap;
	typedef std::unordered_map<std::string, CREATE_PLUGIN_FUNCTION> PluginFuncMap; //静态加载Plugin, 先注册创建函数

	typedef void (*DLL_START_PLUGIN_FUNC)(NFIPluginManager* pm);

	typedef void (*DLL_STOP_PLUGIN_FUNC)(NFIPluginManager* pm);

	PluginLibMap m_nPluginLibMap;
	PluginNameVec m_nPluginNameVec;
	PluginInstanceMap m_nPluginInstanceMap;
	PluginInstanceList m_nPluginInstanceList;
	ModuleInstanceMap m_nModuleInstanceMap;
	PluginFuncMap m_nPluginFuncMap; ////静态加载Plugin, 先注册创建和销毁函数

	NFProfiler m_profilerMgr;

public:
	/*
	 * stop server，停服，意味着需要保存该保存的数据，共享内存可能后面会被清理，服务器会走正常停服流程
	 * */
	bool IsServerStopping() const override { return m_bServerStopping; }

	/*
	 * stop server，停服，意味着需要保存该保存的数据，共享内存可能后面会被清理，服务器会走正常停服流程
	 * */
	void SetServerStopping(bool exitApp) override { m_bServerStopping = exitApp; }

	/*
	 * 停服之前，检查服务器是否满足停服条件
	 * */
	bool CheckStopServer() override;

	/*
	 * 停服之前，做一些操作，满足停服条件
	 * */
	bool OnStopServer() override;

	/*
	 * stop server，停服，意味着需要保存该保存的数据，共享内存可能后面会被清理，服务器会走正常停服流程
	 * */
	bool StopServer() override;

	/*
	 * 停服之前保存需要的数据
	 * */
	bool OnServerKilling() override;

	/*
	 * reload server 重新加载服务器的配置数据
	 * */
	bool IsReloadServer() const override { return m_bReloadServer; }

	/*
	 * reload server 重新加载服务器的配置数据
	 * */
	void SetReloadServer(bool exitApp) override { m_bReloadServer = exitApp; }

	bool GetChangeProfileApp() const override { return m_bChangeProfileApp; }

	void SetChangeProfileApp(bool exitApp) override { m_bChangeProfileApp = exitApp; }

	bool GetKillPreApp() const override { return m_isKillPreApp; }

	void SetKillPreApp(bool exitApp) override { m_isKillPreApp = exitApp; }

	/*
	 * 热更退出app, 用于服务器需要热更app代码的情况，这时候会杀掉正在运行的的的app,重启新的服务器app
	 * */
	bool IsHotfixServer() const override { return m_bHotfixServer; }

	/*
	 * 热更退出app, 用于服务器需要热更app代码的情况，这时候会杀掉正在运行的的的app,重启新的服务器app
	 * */
	void SetHotfixServer(bool exitApp) override { m_bHotfixServer = exitApp; }

	/*
	 * 热更退出app, 用于服务器需要热更app代码的情况，这时候会杀掉正在运行的的的app,重启新的服务器app
	 * */
	bool HotfixServer() override;

private:
	//通过console控制服务器变量
	std::atomic_bool m_bServerStopping{};
	//通过console控制服务器变量重新加载配置
	std::atomic_bool m_bReloadServer{};
	//通过console控制服务器变量计算服务器效率
	std::atomic_bool m_bChangeProfileApp{};
	//通过console控制服务器, 直接关闭服务器，不做处理
	std::atomic_bool m_bHotfixServer{};
	//是否初始化共享内存
	bool m_bInitShm;
	bool m_isKillPreApp; //是否杀掉上一个应用程序，
	bool m_isDaemon;

	bool m_isAllServer;
	//程序初始化流程
	bool m_isInited;
	NFCAppInited m_appInited;
};

#endif
