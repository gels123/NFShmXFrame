# NFShmXFrame 服务器运行流程详解

## 概述

NFShmXFrame是一个插件化的分布式游戏服务器框架，采用"插件管理器→插件→模块"的三层架构设计。本文档详细介绍服务器从启动到关闭的完整运行流程，包括各个关键阶段的执行顺序、核心功能和关键节点。

## 一、服务器启动入口

### 1.1 程序入口点

服务器程序从`NFPluginLoader.cpp`中的`c_main`函数开始执行：

```cpp
int c_main(int argc, char* argv[])
{
    // 1. 设置异常处理器
#if NF_PLATFORM == NF_PLATFORM_WIN
    SetUnhandledExceptionFilter(ApplicationCrashHandler);
#endif

    // 2. 处理启动参数
    ProcessParameter(argc, argv);

    // 3. 创建插件管理器实例
    auto vecPluginManager = NFGlobalSystem::Instance()->GetPluginManagerList();

    // 4. 初始化所有插件管理器
    for (auto* pPluginManager : vecPluginManager)
    {
        pPluginManager->Begin();
    }

    // 5. 主循环执行
    while (true)
    {
        // 执行每帧逻辑
        for (auto* pPluginManager : vecPluginManager)
        {
            pPluginManager->Execute();
        }

        // 检查重载请求
        if (NFGlobalSystem::Instance()->IsReloadApp())
        {
            // 配置重载处理
        }

        // 检查停服请求
        if (NFGlobalSystem::Instance()->IsServerStopping())
        {
            break;
        }
    }

    // 6. 清理和关闭
    for (auto* pPluginManager : vecPluginManager)
    {
        pPluginManager->End();
        NF_SAFE_DELETE(pPluginManager);
    }

    return 0;
}
```

### 1.2 启动参数处理

服务器支持多种启动参数配置：

```cpp
// 常用启动参数
--appid=1           // 应用程序ID
--config=./config/  // 配置文件路径
--plugin=./plugin/  // 插件文件路径
--log=./log/        // 日志文件路径
--daemon            // 守护进程模式（Linux）
--console           // 控制台模式
```

### 1.3 信号处理初始化

系统会初始化信号处理机制：

```cpp
void NFSignalHandleMgr::InitSignal()
{
    signal(SIGTERM, SignalHandler);   // 终止信号
    signal(SIGINT, SignalHandler);    // 中断信号(Ctrl+C)
    signal(SIGUSR1, SignalHandler);   // 用户信号1(重载配置)
    signal(SIGUSR2, SignalHandler);   // 用户信号2(热修复)
    signal(SIGPIPE, SIG_IGN);         // 忽略管道断开信号
}
```

## 二、插件管理器初始化流程

### 2.1 NFPluginManager::Begin() 完整流程

```cpp
bool NFCPluginManager::Begin()
{
    // === 第一阶段：插件加载 ===
    if (!LoadAllPlugin()) return false;
    
    // === 第二阶段：完整初始化流程 ===
    if (!AfterLoadAllPlugin()) return false;     // 所有插件加载完成
    if (!AfterInitShmMem()) return false;        // 共享内存初始化完成
    if (!Awake()) return false;                  // 唤醒所有模块
    if (!Init()) return false;                   // 初始化所有模块
    if (!CheckConfig()) return false;            // 检查配置正确性
    if (!ReadyExecute()) return false;           // 准备执行
    
    return true;
}
```

### 2.2 插件加载阶段详解

#### 2.2.1 LoadAllPlugin() 流程

```cpp
bool NFCPluginManager::LoadAllPlugin()
{
#ifndef NF_DYNAMIC_PLUGIN
    // 静态插件加载模式
    RegisterStaticPlugin();      // 注册静态插件函数
    LoadKernelPlugin();          // 优先加载内核插件
    LoadPluginConfig();          // 加载插件配置文件
    
    // 按配置顺序加载所有静态插件
    for (const auto& pluginName : m_nPluginNameVec)
    {
        LoadStaticPlugin(pluginName);
    }
#else
    // 动态插件加载模式
    LoadPluginConfig();          // 加载插件配置
    
    // 动态加载插件库文件
    for (const auto& pluginName : m_nPluginNameVec)
    {
        LoadPluginLibrary(pluginName + ".dll/.so");
    }
#endif
    return true;
}
```

#### 2.2.2 内核插件优先加载

```cpp
void NFCPluginManager::LoadKernelPlugin()
{
    // 内核插件必须最先加载，因为其他插件都依赖它
    CREATE_PLUGIN(this, NFKernelPlugin);
}
```

内核插件包含的核心模块：
- **NFILogModule**：日志系统
- **NFIConfigModule**：配置管理
- **NFITimerModule**：定时器系统
- **NFIEventModule**：事件系统
- **NFIKernelModule**：内核管理
- **NFIMessageModule**：消息系统
- **NFICoroutineModule**：协程系统
- **NFITaskModule**：任务系统

#### 2.2.3 插件配置文件加载

系统会读取配置文件确定需要加载的插件：

```xml
<plugins>
    <plugin name="NFKernelPlugin" main="1"/>        <!-- 内核插件 -->
    <plugin name="NFNetPlugin" main="0"/>           <!-- 网络插件 -->
    <plugin name="NFShmPlugin" main="0"/>           <!-- 共享内存插件 -->
    <plugin name="NFMemPlugin" main="0"/>           <!-- 正常内存插件 -->
    <plugin name="NFConfigPlugin" main="0"/>        <!-- 配置插件 -->
    <plugin name="NFLuaScriptPlugin" main="0"/>     <!-- Lua脚本插件 -->
</plugins>
```

### 2.3 生命周期初始化阶段

#### 2.3.1 AfterLoadAllPlugin() - 插件间依赖建立

```cpp
bool NFCPluginManager::AfterLoadAllPlugin()
{
    for (auto iter = m_nPluginInstanceList.begin(); 
         iter != m_nPluginInstanceList.end(); ++iter)
    {
        (*iter)->AfterLoadAllPlugin();
    }
    return true;
}
```

**此阶段的主要任务：**
- 所有插件都已创建完成
- 可以安全地获取其他插件的引用
- 建立插件间的依赖关系

#### 2.3.2 AfterInitShmMem() - 共享内存初始化

```cpp
bool NFCPluginManager::AfterInitShmMem()
{
    // 根据选择的内存池插件进行初始化
    if (使用NFShmPlugin)
    {
        // 初始化共享内存池
        InitSharedMemoryPool();
        RegisterShmObjects();     // 注册共享内存对象
    }
    else if (使用NFMemPlugin)
    {
        // 初始化正常内存池
        InitNormalMemoryPool();
    }
    
    // 通知所有插件共享内存已准备就绪
    for (auto iter = m_nPluginInstanceList.begin(); 
         iter != m_nPluginInstanceList.end(); ++iter)
    {
        (*iter)->AfterInitShmMem();
    }
    return true;
}
```

#### 2.3.3 Awake() - 模块唤醒

```cpp
bool NFCPluginManager::Awake()
{
    for (auto iter = m_nPluginInstanceList.begin(); 
         iter != m_nPluginInstanceList.end(); ++iter)
    {
        (*iter)->Awake();  // 转发给插件，插件再转发给模块
    }
    return true;
}
```

**模块在Awake阶段的典型任务：**
- 获取其他模块的引用（依赖注入）
- 设置基础参数
- 注册必要的回调函数

#### 2.3.4 Init() - 模块初始化

```cpp
bool NFCPluginManager::Init()
{
    for (auto iter = m_nPluginInstanceList.begin(); 
         iter != m_nPluginInstanceList.end(); ++iter)
    {
        if (!(*iter)->Init())
        {
            NFLogError(NF_LOG_DEFAULT, 0, "Plugin {} Init failed!", 
                      (*iter)->GetPluginName());
            return false;  // 任何插件初始化失败都会导致整体失败
        }
    }
    return true;
}
```

**模块在Init阶段的典型任务：**
- 读取配置文件
- 初始化数据结构
- 建立网络连接
- 注册消息处理函数
- 注册定时器任务

#### 2.3.5 CheckConfig() - 配置验证

```cpp
bool NFCPluginManager::CheckConfig()
{
    for (auto iter = m_nPluginInstanceList.begin(); 
         iter != m_nPluginInstanceList.end(); ++iter)
    {
        if (!(*iter)->CheckConfig())
        {
            NFLogError(NF_LOG_DEFAULT, 0, "Plugin {} CheckConfig failed!", 
                      (*iter)->GetPluginName());
            return false;
        }
    }
    return true;
}
```

**配置检查的主要内容：**
- 配置文件格式正确性
- 必要配置项存在性
- 配置值合理性验证
- 依赖服务可用性检查

#### 2.3.6 ReadyExecute() - 执行准备

```cpp
bool NFCPluginManager::ReadyExecute()
{
    for (auto iter = m_nPluginInstanceList.begin(); 
         iter != m_nPluginInstanceList.end(); ++iter)
    {
        if (!(*iter)->ReadyExecute())
        {
            NFLogError(NF_LOG_DEFAULT, 0, "Plugin {} ReadyExecute failed!", 
                      (*iter)->GetPluginName());
            return false;
        }
    }
    
    // 设置服务器状态为运行中
    m_isInited = true;
    
    return true;
}
```

## 三、服务器初始化任务系统

### 3.1 初始化任务分组

NFShmXFrame使用任务分组机制管理复杂的初始化流程：

```cpp
enum APP_INIT_TASK_GROUP
{
    APP_INIT_TASK_GROUP_SERVER_CONNECT = 0,    // 服务器连接组
    APP_INIT_TASK_GROUP_SERVER_LOAD_DESC,      // Excel配置加载组
    APP_INIT_TASK_GROUP_SERVER_LOAD_DB,        // 数据库数据加载组
    APP_INIT_MAX_TASK_GROUP,
};
```

### 3.2 任务注册和管理

```cpp
// 模块可以注册初始化任务
int RegisterAppTask(NF_SERVER_TYPE eServerType, uint32_t taskType, 
                   const std::string& desc, uint32_t taskGroup);

// 标记任务完成
int FinishAppTask(NF_SERVER_TYPE eServerType, uint32_t taskType, 
                 uint32_t taskGroup);

// 检查任务组是否完成
bool IsFinishAppTask(NF_SERVER_TYPE eServerType, uint32_t taskGroup) const;
```

### 3.3 典型的服务器初始化任务流程

```cpp
// 以LogicServer为例
bool LogicServerModule::Init()
{
    // 注册连接任务
    RegisterAppTask(NF_ST_LOGIC, TASK_CONNECT_WORLD, 
                   "Connect to world server", 
                   APP_INIT_TASK_GROUP_SERVER_CONNECT);
    
    RegisterAppTask(NF_ST_LOGIC, TASK_CONNECT_STORE, 
                   "Connect to store server", 
                   APP_INIT_TASK_GROUP_SERVER_CONNECT);
    
    // 注册配置加载任务
    RegisterAppTask(NF_ST_LOGIC, TASK_LOAD_DESC_STORE, 
                   "Load description data", 
                   APP_INIT_TASK_GROUP_SERVER_LOAD_DESC);
    
    return true;
}

// 连接建立后标记任务完成
void LogicServerModule::OnConnectWorldServer()
{
    FinishAppTask(NF_ST_LOGIC, TASK_CONNECT_WORLD, 
                 APP_INIT_TASK_GROUP_SERVER_CONNECT);
}
```

## 四、主循环运行阶段

### 4.1 主循环结构

```cpp
bool NFCPluginManager::Execute()
{
    // 1. 更新服务器时间和帧计数
    m_nNowTime = NF_ADJUST_TIMENOW_MS();
    ++m_nCurFrameCount;

    // 2. 执行所有插件的主逻辑（按注册顺序）
    for (auto iter = m_nPluginInstanceList.begin(); 
         iter != m_nPluginInstanceList.end(); ++iter)
    {
        // 性能监控开始
        BeginProfiler((*iter)->GetPluginName() + "--Execute");
        
        // 执行插件逻辑
        (*iter)->Execute();
        
        // 性能监控结束
        uint64_t useTime = EndProfiler();
        if (useTime >= 30000) // >= 30毫秒
        {
            NFLogWarning(NF_LOG_DEFAULT, 0, "Plugin {} execute time: {} ms", 
                        (*iter)->GetPluginName(), useTime / 1000);
        }
    }

    // 3. 执行初始化任务检查
    m_appInited.Execute();

    return true;
}
```

### 4.2 帧率控制机制

```cpp
// 固定帧率模式（默认30FPS）
const uint32_t m_nFrame = 30;        // 目标帧率
bool m_bFixedFrame = true;           // 是否使用固定帧率
uint32_t m_idleSleepUs = 1000;       // 空闲时睡眠时间（微秒）

// 帧率计算
uint64_t expectedFrameTime = 1000000 / m_nFrame;  // 每帧预期时间（微秒）
uint64_t actualFrameTime = GetActualFrameTime();   // 实际帧时间

if (m_bFixedFrame && actualFrameTime < expectedFrameTime)
{
    usleep(expectedFrameTime - actualFrameTime);   // 睡眠补足时间
}
```

### 4.3 插件执行顺序

插件按照注册顺序执行，典型的执行顺序：

1. **NFKernelPlugin**：处理定时器、事件、任务等核心功能
2. **NFNetPlugin**：处理网络消息收发
3. **NFConfigPlugin**：处理配置更新
4. **NFMemPlugin/NFShmPlugin**：内存管理
5. **NFLuaScriptPlugin**：执行Lua脚本逻辑
6. **游戏业务插件**：处理具体游戏逻辑

### 4.4 模块执行示例

```cpp
// 典型的模块Execute实现
bool TimerModule::Execute()
{
    // 检查到期的定时器
    auto now = NFGetSecondTime();
    while (!m_timerQueue.empty() && m_timerQueue.top().expireTime <= now)
    {
        auto timer = m_timerQueue.top();
        m_timerQueue.pop();
        
        // 执行定时器回调
        timer.callback();
        
        // 如果是循环定时器，重新加入队列
        if (timer.interval > 0)
        {
            timer.expireTime = now + timer.interval;
            m_timerQueue.push(timer);
        }
    }
    
    return true;
}
```

## 五、配置热重载流程

### 5.1 重载触发机制

```cpp
// 信号触发重载
void SignalHandler(int signal)
{
    switch(signal)
    {
    case SIGUSR1:
        NFGlobalSystem::Instance()->SetReloadServer(true);
        break;
    }
}

// 主循环检查重载标志
if (NFGlobalSystem::Instance()->IsReloadApp())
{
    // 执行配置重载
    OnReloadConfig();
    AfterOnReloadConfig();
    
    // 重置重载标志
    NFGlobalSystem::Instance()->SetReloadServer(false);
}
```

### 5.2 配置重载流程

```cpp
bool NFCPluginManager::OnReloadConfig()
{
    NFLogInfo(NF_LOG_DEFAULT, 0, "Starting config reload...");
    
    // 通知所有插件重载配置
    for (auto iter = m_nPluginInstanceList.begin(); 
         iter != m_nPluginInstanceList.end(); ++iter)
    {
        (*iter)->OnReloadConfig();
    }
    
    return true;
}

bool NFCPluginManager::AfterOnReloadConfig()
{
    NFLogInfo(NF_LOG_DEFAULT, 0, "Starting post-reload processing...");
    
    // 通知所有插件执行重载后处理
    for (auto iter = m_nPluginInstanceList.begin(); 
         iter != m_nPluginInstanceList.end(); ++iter)
    {
        (*iter)->AfterOnReloadConfig();
    }
    
    return true;
}
```

### 5.3 模块级配置重载示例

```cpp
bool ConfigModule::OnReloadConfig()
{
    // 重新加载配置文件
    std::vector<std::string> changedFiles;
    if (ReloadConfigFiles(changedFiles))
    {
        // 通知其他模块配置已更新
        for (const auto& fileName : changedFiles)
        {
            NFEventMgr::Instance()->SendEvent<NFConfigChangedEvent>(fileName);
        }
        
        NFLogInfo(NF_LOG_DEFAULT, 0, "Config files reloaded: {}", 
                 fmt::join(changedFiles, ","));
        return true;
    }
    
    return false;
}
```

## 六、热修复机制

### 6.1 共享内存热修复（NFShmPlugin）

```cpp
bool NFShmPlugin::HotfixServer()
{
    // 1. 保存当前数据状态
    SaveCurrentState();
    
    // 2. 卸载旧的动态库
    UnloadOldLibraries();
    
    // 3. 加载新的动态库
    if (!LoadNewLibraries())
    {
        // 回滚到旧版本
        RollbackToOldLibraries();
        return false;
    }
    
    // 4. 重新注册插件和模块
    ReregisterPluginsAndModules();
    
    // 5. 恢复数据状态
    RestoreDataState();
    
    NFLogInfo(NF_LOG_DEFAULT, 0, "Hotfix completed successfully");
    return true;
}
```

### 6.2 Lua脚本热更新

```cpp
bool LuaScriptModule::HotfixServer()
{
    // 1. 检查Lua文件变更
    std::vector<std::string> changedFiles;
    CheckLuaFileChanges(changedFiles);
    
    // 2. 重新加载变更的Lua文件
    for (const auto& luaFile : changedFiles)
    {
        if (!ReloadLuaFile(luaFile))
        {
            NFLogError(NF_LOG_DEFAULT, 0, "Failed to reload Lua file: {}", luaFile);
            return false;
        }
    }
    
    // 3. 调用Lua的热更新回调
    CallLuaHotfixCallback(changedFiles);
    
    NFLogInfo(NF_LOG_DEFAULT, 0, "Lua hotfix completed: {}", 
             fmt::join(changedFiles, ","));
    return true;
}
```

## 七、服务器关闭流程

### 7.1 关闭触发机制

```cpp
// 信号触发关闭
void SignalHandler(int signal)
{
    switch(signal)
    {
    case SIGTERM:
    case SIGINT:
        NFGlobalSystem::Instance()->SetServerStopping(true);
        break;
    }
}

// 主循环检查关闭标志
if (NFGlobalSystem::Instance()->IsServerStopping())
{
    // 开始关闭流程
    break;
}
```

### 7.2 四阶段关闭流程

```cpp
bool NFCPluginManager::End()
{
    // === 第一阶段：停服前准备 ===
    BeforeShut();
    
    // === 第二阶段：正式关闭 ===
    Shut();
    
    // === 第三阶段：最终清理 ===
    Finalize();
    
    // === 第四阶段：插件清理 ===
    CleanupPlugins();
    
    return true;
}
```

#### 7.2.1 第一阶段：停服前准备

```cpp
bool NFCPluginManager::BeforeShut()
{
    NFLogInfo(NF_LOG_DEFAULT, 0, "Server shutdown phase 1: Before shut");
    
    // 停止接受新连接
    for (auto iter = m_nPluginInstanceList.begin(); 
         iter != m_nPluginInstanceList.end(); ++iter)
    {
        (*iter)->BeforeShut();
    }
    
    return true;
}
```

**第一阶段主要任务：**
- 停止接受新的客户端连接
- 通知所有在线玩家服务器即将关闭
- 保存关键数据和状态
- 停止定时任务和后台作业

#### 7.2.2 第二阶段：正式关闭

```cpp
bool NFCPluginManager::Shut()
{
    NFLogInfo(NF_LOG_DEFAULT, 0, "Server shutdown phase 2: Shut");
    
    // 关闭所有网络连接和服务
    for (auto iter = m_nPluginInstanceList.begin(); 
         iter != m_nPluginInstanceList.end(); ++iter)
    {
        (*iter)->Shut();
    }
    
    return true;
}
```

**第二阶段主要任务：**
- 关闭所有网络连接
- 停止所有业务逻辑处理
- 保存玩家数据到数据库
- 释放网络资源

#### 7.2.3 第三阶段：最终清理

```cpp
bool NFCPluginManager::Finalize()
{
    NFLogInfo(NF_LOG_DEFAULT, 0, "Server shutdown phase 3: Finalize");
    
    // 最终清理和资源释放
    for (auto iter = m_nPluginInstanceList.begin(); 
         iter != m_nPluginInstanceList.end(); ++iter)
    {
        (*iter)->Finalize();
    }
    
    return true;
}
```

**第三阶段主要任务：**
- 释放所有内存资源
- 关闭文件句柄和数据库连接
- 清理临时文件
- 输出最终的统计信息

#### 7.2.4 第四阶段：插件清理

```cpp
void NFCPluginManager::CleanupPlugins()
{
    NFLogInfo(NF_LOG_DEFAULT, 0, "Server shutdown phase 4: Cleanup plugins");
    
    // 卸载所有插件
    for (auto it = m_nPluginInstanceList.begin(); 
         it != m_nPluginInstanceList.end(); ++it)
    {
        (*it)->Uninstall();
        delete (*it);
    }
    
    // 清理容器
    m_nPluginInstanceList.clear();
    m_nPluginInstanceMap.clear();
    m_nModuleInstanceMap.clear();
    
    // 卸载动态库
    for (auto& libPair : m_nPluginLibMap)
    {
        libPair.second->UnLoad();
        delete libPair.second;
    }
    m_nPluginLibMap.clear();
}
```

### 7.3 优雅关闭示例

```cpp
// 游戏逻辑模块的优雅关闭
bool GameLogicModule::BeforeShut()
{
    // 1. 通知所有玩家服务器即将关闭
    NotifyAllPlayersServerShutdown();
    
    // 2. 停止接受新的游戏请求
    SetAcceptingNewGames(false);
    
    // 3. 等待当前游戏结束
    WaitForCurrentGamesToEnd();
    
    return true;
}

bool GameLogicModule::Shut()
{
    // 1. 强制结束所有游戏
    ForceEndAllGames();
    
    // 2. 保存所有玩家数据
    SaveAllPlayerData();
    
    // 3. 断开所有连接
    DisconnectAllPlayers();
    
    return true;
}

bool GameLogicModule::Finalize()
{
    // 1. 释放游戏资源
    ReleaseGameResources();
    
    // 2. 清理内存数据
    CleanupMemoryData();
    
    // 3. 输出统计报告
    PrintStatisticsReport();
    
    return true;
}
```

## 八、异常处理和故障恢复

### 8.1 异常处理机制

```cpp
// Windows平台异常处理
#if NF_PLATFORM == NF_PLATFORM_WIN
LONG ApplicationCrashHandler(EXCEPTION_POINTERS* pException)
{
    CreateMiniDump(pException);     // 创建崩溃转储文件
    NFLogError(NF_LOG_DEFAULT, 0, "Application crashed, dump file created");
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

// Linux平台信号处理
void CrashSignalHandler(int signal)
{
    switch(signal)
    {
    case SIGSEGV:
        NFLogError(NF_LOG_DEFAULT, 0, "Segmentation fault occurred");
        CreateCoreDump();
        break;
    case SIGABRT:
        NFLogError(NF_LOG_DEFAULT, 0, "Abort signal received");
        break;
    }
    
    // 执行紧急清理
    EmergencyCleanup();
    exit(1);
}
```

### 8.2 故障恢复机制

```cpp
// 插件恢复机制
bool NFCPluginManager::RecoverFromError()
{
    try
    {
        // 1. 停止当前所有操作
        StopAllOprations();
        
        // 2. 检查插件状态
        CheckPluginHealth();
        
        // 3. 重新初始化出错的插件
        ReloadFailedPlugins();
        
        // 4. 恢复服务
        RestoreServices();
        
        return true;
    }
    catch (const std::exception& e)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "Recovery failed: {}", e.what());
        return false;
    }
}

// 内存恢复（共享内存模式）
bool NFShmPlugin::RecoverFromCrash()
{
    // 1. 检查共享内存完整性
    if (!CheckShmIntegrity())
    {
        NFLogError(NF_LOG_DEFAULT, 0, "Shared memory integrity check failed");
        return false;
    }
    
    // 2. 恢复数据结构
    if (!RestoreDataStructures())
    {
        NFLogError(NF_LOG_DEFAULT, 0, "Failed to restore data structures");
        return false;
    }
    
    // 3. 重新建立连接
    if (!ReestablishConnections())
    {
        NFLogError(NF_LOG_DEFAULT, 0, "Failed to reestablish connections");
        return false;
    }
    
    NFLogInfo(NF_LOG_DEFAULT, 0, "Successfully recovered from crash");
    return true;
}
```

## 九、性能监控和调优

### 9.1 性能监控系统

```cpp
class NFProfiler
{
public:
    void BeginProfiler(const std::string& funcName)
    {
        if (m_bOpenProfiler)
        {
            auto& profilerData = m_mapProfilerData[funcName];
            profilerData.nBeginTime = NFGetMicroSecondTime();
        }
    }
    
    uint64_t EndProfiler()
    {
        if (m_bOpenProfiler && !m_vecProfilerData.empty())
        {
            ProfilerData& data = m_vecProfilerData.back();
            uint64_t useTime = NFGetMicroSecondTime() - data.nBeginTime;
            data.nTotalTime += useTime;
            data.nCallCount++;
            
            return useTime;
        }
        return 0;
    }
    
    void PrintReport()
    {
        NFLogInfo(NF_LOG_DEFAULT, 0, "=== Performance Report ===");
        for (const auto& pair : m_mapProfilerData)
        {
            const ProfilerData& data = pair.second;
            double avgTime = data.nTotalTime / (double)data.nCallCount;
            
            NFLogInfo(NF_LOG_DEFAULT, 0, 
                     "Function: {}, Calls: {}, Total: {}ms, Avg: {:.2f}ms",
                     pair.first, data.nCallCount, 
                     data.nTotalTime / 1000, avgTime / 1000);
        }
    }
};
```

### 9.2 内存监控

```cpp
class NFMemoryMonitor
{
public:
    void MonitorMemoryUsage()
    {
        // 获取当前内存使用情况
        MemoryInfo memInfo;
        GetMemoryInfo(memInfo);
        
        // 记录内存使用历史
        m_memoryHistory.push_back(memInfo);
        
        // 检查内存泄漏
        if (DetectMemoryLeak(memInfo))
        {
            NFLogWarning(NF_LOG_DEFAULT, 0, "Potential memory leak detected");
        }
        
        // 内存使用报警
        if (memInfo.usedMemory > m_memoryThreshold)
        {
            NFLogWarning(NF_LOG_DEFAULT, 0, 
                        "High memory usage: {}MB", memInfo.usedMemory / 1024 / 1024);
        }
    }
    
    void PrintMemoryReport()
    {
        if (!m_memoryHistory.empty())
        {
            const MemoryInfo& current = m_memoryHistory.back();
            NFLogInfo(NF_LOG_DEFAULT, 0, 
                     "Memory Usage - Total: {}MB, Used: {}MB, Free: {}MB",
                     current.totalMemory / 1024 / 1024,
                     current.usedMemory / 1024 / 1024,
                     current.freeMemory / 1024 / 1024);
        }
    }
};
```

### 9.3 性能调优建议

#### 9.3.1 框架级优化

- **模块执行顺序优化**：将高频执行的模块放在前面
- **内存池调优**：根据业务需求选择合适的内存池类型
- **日志级别控制**：生产环境关闭调试日志
- **性能监控开关**：生产环境可关闭详细的性能监控

#### 9.3.2 业务级优化

- **数据结构优化**：使用高效的数据结构和算法
- **数据库访问优化**：使用连接池和批量操作
- **网络通信优化**：合并小数据包，减少网络开销
- **缓存策略优化**：合理使用缓存减少重复计算

## 十、服务器运行流程总结

### 10.1 完整流程图

```
启动阶段
├── c_main入口
├── 参数处理
├── 信号处理初始化
├── 插件管理器创建
└── Begin()初始化
    ├── LoadAllPlugin()
    ├── AfterLoadAllPlugin()
    ├── AfterInitShmMem()
    ├── Awake()
    ├── Init()
    ├── CheckConfig()
    └── ReadyExecute()

运行阶段
├── 主循环Execute()
├── 帧率控制(30FPS)
├── 插件逻辑执行
├── 初始化任务检查
├── 热更新检查
└── 停服检查

关闭阶段
├── 停服信号处理
└── End()清理
    ├── BeforeShut()
    ├── Shut()
    ├── Finalize()
    └── 插件清理
```

### 10.2 关键时机总结

| 阶段 | 时机 | 主要任务 |
|------|------|----------|
| 启动 | c_main开始 | 系统初始化、插件加载 |
| 初始化 | Begin()调用 | 15个生命周期阶段执行 |
| 运行 | 主循环Execute() | 业务逻辑处理、性能监控 |
| 热更新 | 信号触发 | 配置重载、插件更新 |
| 关闭 | 停服信号 | 四阶段优雅关闭 |

### 10.3 核心特性

1. **15阶段生命周期管理**：精确控制初始化顺序
2. **30FPS固定帧率**：稳定的服务器运行节奏
3. **优雅关闭机制**：四阶段安全关闭流程
4. **热更新支持**：配置和插件的热更新能力
5. **异常恢复**：完善的错误处理和恢复机制
6. **性能监控**：实时的性能分析和优化建议

NFShmXFrame服务器运行流程体现了现代游戏服务器架构的最佳实践，通过精心设计的生命周期管理、模块化架构和完善的监控体系，实现了高性能、高可用、易维护的游戏服务器解决方案。 
NFShmXFrame通过精心设计的运行流程，实现了一个稳定、高效、可扩展的游戏服务器框架，为游戏开发提供了坚实的技术基础。 