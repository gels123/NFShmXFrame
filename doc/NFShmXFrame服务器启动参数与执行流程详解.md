# NFShmXFrame 服务器启动参数与执行流程详解

## 概述

NFShmXFrame服务器支持丰富的命令行参数配置，可以灵活地控制服务器的启动模式、路径配置、运行状态等。本文档详细介绍服务器支持的所有启动参数以及完整的执行流程，帮助开发者和运维人员正确使用和管理服务器。

## 一、启动参数详解

### 1.1 基础配置参数

#### 1.1.1 服务器标识参数

| 参数 | 类型 | 必需 | 默认值 | 说明 |
|------|------|------|--------|------|
| `--Server` | string | 否 | "AllServer" | 服务器名称，支持多种模式 |
| `--ID` | string | 否 | "1.1.1.1" | 服务器ID，格式类似IP地址 |

**Server参数支持的值：**

| 值 | 模式 | 说明 |
|------|------|------|
| `AllServer` | 单进程多服务器（每种一个） | 在一个进程中启动多种服务器，每种类型只启动一个实例 |
| `AllMoreServer` | 单进程多服务器（每种多个） | 在一个进程中启动多种服务器，每种类型可以启动多个实例 |
| `GameServer` | 单一服务器 | 只启动游戏服务器 |
| `ProxyServer` | 单一服务器 | 只启动代理服务器 |
| `LogicServer` | 单一服务器 | 只启动逻辑服务器 |
| `...` | 单一服务器 | 其他具体的服务器类型 |

**使用示例：**
```bash
# 启动单个服务器
./NFPluginLoader --Server=GameServer --ID=1.1.1.1

# 启动多服务器模式（每种服务器一个实例）
./NFPluginLoader --Server=AllServer --ID=1.1.1.1

# 启动多服务器模式（每种服务器可多个实例）
./NFPluginLoader --Server=AllMoreServer --ID=1.1.1.1
```

#### 1.1.2 路径配置参数

| 参数 | 类型 | 必需 | 默认值 | 说明 |
|------|------|------|--------|------|
| `--Config` | string | 否 | "../../Config" | 配置文件路径 |
| `--Path` | string | 否 | "../../Config" | 配置文件路径（与Config等效） |
| `--Plugin` | string | 否 | "../../Plugin" | 插件文件路径 |
| `--LogPath` | string | 否 | "logs" | 日志文件路径 |
| `--LuaScript` | string | 否 | "../../LuaScript" | Lua脚本路径 |

**使用示例：**
```bash
./NFPluginLoader \
  --Server=GameServer \
  --ID=1.1.1.1 \
  --Config=./config \
  --Plugin=./plugin \
  --LogPath=./logs \
  --LuaScript=./scripts
```

#### 1.1.3 游戏配置参数

| 参数 | 类型 | 必需 | 默认值 | 说明 |
|------|------|------|--------|------|
| `--Game` | string | 否 | "MMO" | 游戏类型标识 |
| `--Param` | string | 否 | "Param" | 临时参数，可自定义使用 |

### 1.2 Windows平台专用参数

| 参数 | 短参数 | 说明 |
|------|--------|------|
| `--XButton` | `-x` | 禁用控制台窗口的"X"关闭按钮 |

**使用示例：**
```bash
# Windows下禁用X按钮
NFPluginLoader.exe --Server=GameServer --ID=1.1.1.1 --XButton
```

### 1.3 Linux平台专用参数

#### 1.3.1 运行模式参数

| 参数 | 短参数 | 说明 |
|------|--------|------|
| `--Daemon` | `-d` | 以守护进程模式运行 |
| `--Init` | - | 设置共享内存为初始化模式 |

#### 1.3.2 服务器控制参数

| 参数 | 说明 | 执行后行为 |
|------|------|------------|
| `--Start` | 启动服务器 | 正常启动流程 |
| `--Stop` | 停止运行中的服务器 | 发送停止信号后退出 |
| `--Reload` | 重载服务器配置 | 发送重载信号后退出 |
| `--Quit` | 退出服务器 | 发送退出信号后退出 |
| `--Restart` | 重启服务器 | 杀死旧进程并启动新进程 |
| `--Kill` | 强制杀死服务器进程 | 设置强制杀死标志 |

**使用示例：**
```bash
# 以守护进程模式启动
./NFPluginLoader --Server=GameServer --ID=1.1.1.1 --Start --Daemon

# 停止服务器
./NFPluginLoader --Server=GameServer --ID=1.1.1.1 --Stop

# 重载配置
./NFPluginLoader --Server=GameServer --ID=1.1.1.1 --Reload

# 重启服务器
./NFPluginLoader --Server=GameServer --ID=1.1.1.1 --Restart --Daemon
```

## 二、参数处理执行流程

### 2.1 主入口处理流程

```cpp
void ProcessParameter(int argc, char* argv[])
{
    // 1. 创建命令行解析器
    NFCmdLine::NFParser cmdParser;
    
    // 2. 添加所有支持的命令行参数
    cmdParser.Add<std::string>("Server", 0, "Server Name", false, "xxAllServer or AllMoreServer");
    cmdParser.Add<std::string>("ID", 0, "Server ID", false, "1.1.1.1");
    // ... 添加其他参数
    
    // 3. 解析命令行参数
    cmdParser.ParseCheck(argc, argv);
    
    // 4. 获取服务器名称
    auto strAppName = cmdParser.Get<std::string>("Server");
    
    // 5. 根据服务器名称选择启动模式
    if (strAppName == "AllMoreServer")
    {
        // AllMoreServer模式：每种服务器类型可启动多个实例
        LaunchAllMoreServerMode();
    }
    else if (strAppName.find("AllServer") != std::string::npos)
    {
        // AllServer模式：每种服务器类型只启动一个实例
        LaunchAllServerMode();
    }
    else
    {
        // 单一服务器模式：只启动指定类型的服务器
        LaunchSingleServerMode();
    }
}
```

**三种启动模式对比：**

| 启动模式 | 参数示例 | 进程数 | 插件管理器数 | 服务器实例数 |
|----------|----------|--------|--------------|--------------|
| **单一服务器** | `--Server=GameServer` | 1 | 1 | 1 |
| **AllServer** | `--Server=AllServer` | 1 | 1 | 多个（每种类型1个） |
| **AllMoreServer** | `--Server=AllMoreServer` | 1 | 多个 | 多个（每种类型可多个） |

### 2.2 多服务器模式启动流程

NFShmXFrame支持两种多服务器启动模式，它们都可以在一个进程中启动多个服务器，方便跨服务器调试：

#### 2.2.1 两种多服务器模式对比

| 模式 | 参数值 | 特点 | 适用场景 |
|------|--------|------|----------|
| **AllServer模式** | `--Server=AllServer` | 每种服务器类型只能启动一个实例 | 标准单机调试环境 |
| **AllMoreServer模式** | `--Server=AllMoreServer` | 每种服务器类型可以启动多个实例 | 集群模拟和负载测试 |

#### 2.2.2 共同点

1. **单进程多服务器**：都在一个进程中启动多个不同类型的服务器
2. **跨服务器调试**：便于调试服务器间的通信和数据交互
3. **配置驱动**：都通过配置文件定义要启动的服务器列表
4. **统一管理**：使用同一个插件管理器系统进行统一管理

#### 2.2.3 AllServer模式处理流程

```cpp
// AllServer模式：每种服务器类型只启动一个实例
if (strAppName.find("AllServer") != std::string::npos && strAppName != "AllMoreServer")
{
    // 1. 设置单进程多服务器模式
    NFGlobalSystem::Instance()->SetMoreServer(false);
    
    // 2. 创建单个插件管理器
    std::vector<std::string> vecParam;
    for (int i = 0; i < argc; i++)
    {
        vecParam.push_back(argv[i]);
    }
    
    // 3. 创建并配置插件管理器
    NFIPluginManager* pPluginManager = new NFCPluginManager();
    ProcessParameter(pPluginManager, vecParam);
    
    // 4. 设置为全局插件管理器
    NFGlobalSystem::Instance()->SetGlobalPluginManager(pPluginManager);
    NFGlobalSystem::Instance()->AddPluginManager(pPluginManager);
    
    // 在这种模式下，一个插件管理器会加载所有类型的服务器插件
    // 但每种类型只会有一个实例：
    // - 一个ProxyServer
    // - 一个GameServer  
    // - 一个LogicServer
    // - 一个WorldServer
    // 等等...
}
```

#### 2.2.4 AllMoreServer模式处理流程

```cpp
// AllMoreServer模式：每种服务器类型可以启动多个实例
if (strAppName == "AllMoreServer")
{
    // 1. 设置多服务器模式标志
    NFGlobalSystem::Instance()->SetMoreServer(true);
    
    // 2. 加载配置文件
    NFGlobalSystem::Instance()->LoadConfig(strPlugin);
    
    // 3. 获取所有服务器配置
    const NFrame::pbPluginConfig* pPlugConfig = 
        NFGlobalSystem::Instance()->GetAllMoreServerConfig();
    
    // 4. 遍历每个服务器配置，可能包含同类型的多个服务器
    for (int i = 0; i < pPlugConfig->serverlist_size(); i++)
    {
        const NFrame::pbAllServerConfig& serverConfig = pPlugConfig->serverlist(i);
        
        // 5. 构造新的启动参数
        std::vector<std::string> vecParam;
        vecParam.push_back(argv[0]);
        vecParam.push_back("--Server=" + serverConfig.server());
        vecParam.push_back("--ID=" + serverConfig.id());
        vecParam.push_back("--Config=" + strConfigPath);
        vecParam.push_back("--Plugin=" + strPlugin);
        vecParam.push_back("--restart");
        
        // 6. 为每个服务器创建独立的插件管理器
        NFIPluginManager* pPluginManager = NF_NEW NFCPluginManager();
        ProcessParameter(pPluginManager, vecParam);
        
        // 7. 设置全局插件管理器（如果包含ALL_SERVER）
        if (NFStringUtility::Contains(serverConfig.server(), ALL_SERVER))
        {
            NFGlobalSystem::Instance()->SetGlobalPluginManager(pPluginManager);
        }
        
        // 8. 添加到全局系统
        NFGlobalSystem::Instance()->AddPluginManager(pPluginManager);
    }
    
    // 在这种模式下，可以启动同类型的多个服务器实例：
    // - ProxyServer1 (ID: 1.10.1.1)
    // - ProxyServer2 (ID: 1.10.1.2) 
    // - ProxyServer3 (ID: 1.10.1.3)
    // - GameServer1 (ID: 1.11.1.1)
    // - GameServer2 (ID: 1.11.1.2)
    // 等等...
}
```

#### 2.2.5 配置文件示例

**AllServer模式配置示例：**
```xml
<!-- AllServer模式：每种服务器只有一个实例 -->
<plugins>
    <plugin name="NFKernelPlugin" main="1"/>
    <plugin name="NFNetPlugin" main="0"/>
    <plugin name="NFProxyServerPlugin" main="0"/>     <!-- 只有一个Proxy -->
    <plugin name="NFGameServerPlugin" main="0"/>      <!-- 只有一个Game -->
    <plugin name="NFLogicServerPlugin" main="0"/>     <!-- 只有一个Logic -->
    <plugin name="NFWorldServerPlugin" main="0"/>     <!-- 只有一个World -->
</plugins>
```

**AllMoreServer模式配置示例：**
```xml
<!-- AllMoreServer模式：可以有多个同类型服务器实例 -->
<PluginConfig>
    <ServerList>
        <!-- 多个Proxy服务器 -->
        <Server server="ProxyServer" id="1.10.1.1"/>
        <Server server="ProxyServer" id="1.10.1.2"/>
        <Server server="ProxyServer" id="1.10.1.3"/>
        
        <!-- 多个Game服务器 -->
        <Server server="GameServer" id="1.11.1.1"/>
        <Server server="GameServer" id="1.11.1.2"/>
        
        <!-- 多个Logic服务器 -->
        <Server server="LogicServer" id="1.12.1.1"/>
        <Server server="LogicServer" id="1.12.1.2"/>
        <Server server="LogicServer" id="1.12.1.3"/>
        
        <!-- 其他服务器 -->
        <Server server="WorldServer" id="1.13.1.1"/>
        <Server server="MasterServer" id="1.1.1.1"/>
    </ServerList>
</PluginConfig>
```

#### 2.2.6 使用场景对比

**AllServer模式适用场景：**
```bash
# 标准开发调试环境
./NFPluginLoader --Server=AllServer --ID=1.1.1.1 --Config=./config

# 特点：
# - 启动速度快，资源占用少
# - 适合功能开发和基础调试
# - 模拟完整的服务器架构但规模较小
# - 每种服务器类型只有一个实例，逻辑简单
```

**AllMoreServer模式适用场景：**
```bash
# 集群模拟和负载测试环境
./NFPluginLoader --Server=AllMoreServer --ID=1.1.1.1 --Config=./config

# 特点：
# - 可以模拟真实的集群环境
# - 适合负载测试和性能调优
# - 测试服务器间的负载均衡
# - 验证多实例间的数据一致性
# - 模拟服务器故障和恢复场景
```

#### 2.2.7 实际运行效果

**AllServer模式运行效果：**
```
进程列表：
NFAllServer1.1.1.1    [包含所有服务器类型，每种一个实例]
├── ProxyServer (1.10.1.1)
├── GameServer (1.11.1.1)  
├── LogicServer (1.12.1.1)
├── WorldServer (1.13.1.1)
└── MasterServer (1.1.1.1)
```

**AllMoreServer模式运行效果：**
```
进程列表：
NFAllMoreServer1.1.1.1 [包含多个插件管理器，每个管理一个服务器实例]
├── PluginManager1 → ProxyServer (1.10.1.1)
├── PluginManager2 → ProxyServer (1.10.1.2)
├── PluginManager3 → ProxyServer (1.10.1.3)
├── PluginManager4 → GameServer (1.11.1.1)
├── PluginManager5 → GameServer (1.11.1.2)
├── PluginManager6 → LogicServer (1.12.1.1)
├── PluginManager7 → LogicServer (1.12.1.2)
└── PluginManager8 → WorldServer (1.13.1.1)
```

### 2.3 单服务器参数处理流程

```cpp
void ProcessParameter(NFIPluginManager* pPluginManager, const std::vector<std::string>& vecParam)
{
    // 1. 创建命令行解析器
    NFCmdLine::NFParser cmdParser;
    
    // 2. 添加所有命令行参数定义
    // ... 参数定义代码
    
    // 3. 解析命令行参数
    cmdParser.ParseCheck(vecParam);
    
    // 4. 设置基础参数
    SetBasicParameters(pPluginManager, cmdParser);
    
    // 5. 处理平台特定参数
    ProcessPlatformSpecificParameters(pPluginManager, cmdParser);
}
```

### 2.4 基础参数设置流程

```cpp
void SetBasicParameters(NFIPluginManager* pPluginManager, NFCmdLine::NFParser& cmdParser)
{
    // 1. 设置程序完整路径
    pPluginManager->SetFullPath(vecParam[0]);
    
    // 2. 设置临时参数
    auto strParam = cmdParser.Get<std::string>("Param");
    pPluginManager->SetStrParam(strParam);
    
    // 3. 设置服务器名称
    auto strAppName = cmdParser.Get<std::string>("Server");
    pPluginManager->SetAppName(strAppName);
    
    // 4. 检查是否为全服务器模式
    if (strAppName.find(ALL_SERVER) != std::string::npos)
    {
        pPluginManager->SetLoadAllServer(true);
    }
    
    // 5. 设置服务器ID
    auto strBusName = cmdParser.Get<std::string>("ID");
    uint32_t mBusId = NFServerIDUtil::GetBusID(strBusName);
    
    if (mBusId <= 0)
    {
        std::cerr << "Invalid ID:" << strBusName << std::endl;
        exit(0);
    }
    
    pPluginManager->SetBusName(strBusName);
    pPluginManager->SetAppID(static_cast<int>(mBusId));
    
    // 6. 设置各种路径
    SetPaths(pPluginManager, cmdParser);
}
```

### 2.5 路径设置流程

```cpp
void SetPaths(NFIPluginManager* pPluginManager, NFCmdLine::NFParser& cmdParser)
{
    // 1. 设置配置文件路径（优先级：Config > Path > 默认值）
    if (cmdParser.Exist("Config"))
    {
        auto strDataPath = cmdParser.Get<std::string>("Config");
        pPluginManager->SetConfigPath(strDataPath);
    }
    else if (cmdParser.Exist("Path"))
    {
        auto strDataPath = cmdParser.Get<std::string>("Path");
        pPluginManager->SetConfigPath(strDataPath);
    }
    else
    {
        auto strDataPath = cmdParser.Get<std::string>("Config");
        pPluginManager->SetConfigPath(strDataPath);
    }
    
    // 2. 设置插件路径
    auto strPlugin = cmdParser.Get<std::string>("Plugin");
    pPluginManager->SetPluginPath(strPlugin);
    
    // 3. 设置Lua脚本路径
    auto luaScript = cmdParser.Get<std::string>("LuaScript");
    pPluginManager->SetLuaScriptPath(luaScript);
    
    // 4. 设置日志路径
    auto logPath = cmdParser.Get<std::string>("LogPath");
    pPluginManager->SetLogPath(logPath);
    
    // 5. 设置游戏类型
    auto gameStr = cmdParser.Get<std::string>("Game");
    pPluginManager->SetGame(gameStr);
    
    // 6. 设置PID文件名
    pPluginManager->SetPidFileName();
}
```

## 三、平台特定处理流程

### 3.1 Windows平台处理

```cpp
#if NF_PLATFORM == NF_PLATFORM_WIN
if (cmdParser.Exist("XButton"))
{
    CloseXButton();  // 禁用控制台窗口的X按钮
}
#endif

void CloseXButton()
{
    HWND hWnd = GetConsoleWindow();
    if (hWnd)
    {
        HMENU hMenu = GetSystemMenu(hWnd, FALSE);
        EnableMenuItem(hMenu, SC_CLOSE, MF_DISABLED | MF_BYCOMMAND);
    }
}
```

### 3.2 Linux平台处理流程

Linux平台支持更丰富的服务器控制功能：

#### 3.2.1 初始化和杀死处理

```cpp
// 1. 共享内存初始化模式
if (cmdParser.Exist("Init"))
{
    pPluginManager->SetInitShm();
}

// 2. 强制杀死标志
if (cmdParser.Exist("Kill"))
{
    pPluginManager->SetKillPreApp(true);
}
```

#### 3.2.2 服务器控制命令处理

```cpp
// 停止服务器
if (cmdParser.Exist("Stop"))
{
    pPluginManager->StopApp();
    exit(0);
}
// 重载配置
else if (cmdParser.Exist("Reload"))
{
    pPluginManager->ReloadApp();
    exit(0);
}
// 退出服务器
else if (cmdParser.Exist("Quit"))
{
    pPluginManager->QuitApp();
    exit(0);
}
// 重启服务器
else if (cmdParser.Exist("Restart"))
{
    ProcessRestartCommand(pPluginManager, cmdParser);
}
// 启动服务器
else if (cmdParser.Exist("Start"))
{
    ProcessStartCommand(pPluginManager, cmdParser);
}
```

#### 3.2.3 重启命令处理流程

```cpp
void ProcessRestartCommand(NFIPluginManager* pPluginManager, NFCmdLine::NFParser& cmdParser)
{
    // 1. 检查是否需要守护进程模式
    if (cmdParser.Exist("Daemon"))
    {
        pPluginManager->SetDaemon();
        InitDaemon();  // 初始化守护进程
    }
    
    // 2. 初始化信号处理
    InitSignal();
    
    // 3. 杀死前一个应用程序
    if (pPluginManager->KillPreApp() < 0)
    {
        std::cout << "kill pre app failed!" << std::endl;
        exit(0);
    }
    
    // 4. 创建PID文件
    if (pPluginManager->CreatePidFile() < 0)
    {
        std::cout << "create pid file failed!" << std::endl;
        exit(0);
    }
}
```

#### 3.2.4 启动命令处理流程

```cpp
void ProcessStartCommand(NFIPluginManager* pPluginManager, NFCmdLine::NFParser& cmdParser)
{
    // 1. 守护进程模式处理
    if (cmdParser.Exist("Daemon"))
    {
        pPluginManager->SetDaemon();
        InitDaemon();
    }
    
    // 2. 初始化信号处理
    InitSignal();
    
    // 3. 可选的杀死前一个应用程序
    if (pPluginManager->GetKillPreApp())
    {
        if (pPluginManager->KillPreApp() < 0)
        {
            std::cout << "kill pre app failed!" << std::endl;
            exit(0);
        }
    }
    
    // 4. 检查PID文件
    if (pPluginManager->CheckPidFile() < 0)
    {
        std::cout << "check pid file failed!" << std::endl;
        exit(0);
    }
    
    // 5. 创建PID文件
    if (pPluginManager->CreatePidFile() < 0)
    {
        std::cout << "create pid file failed!" << std::endl;
        exit(0);
    }
}
```

### 3.3 守护进程初始化

```cpp
void InitDaemon()
{
#if NF_PLATFORM == NF_PLATFORM_LINUX
    pid_t pid;
    
    // 1. 第一次fork，父进程退出
    if ((pid = fork()) != 0)
    {
        exit(0);
    }
    
    // 2. 创建新的会话
    setsid();
    
    // 3. 忽略各种信号
    signal(SIGINT, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    ignore_pipe_new();
    
    // 4. 第二次fork，确保不能重新获得控制终端
    if ((pid = fork()) != 0)
    {
        exit(0);
    }
    
    // 5. 设置文件权限掩码
    umask(0);
#endif
}
```

## 四、进程标题设置

系统会根据服务器名称和ID设置进程标题，便于识别和管理：

```cpp
// 构造进程标题
std::string strTitleName = "NF" + strAppName + NFCommon::tostr(strBusName);

#if NF_PLATFORM == NF_PLATFORM_WIN
    // Windows: 设置控制台窗口标题
    // SetConsoleTitle(NFStringUtility::char2wchar(strTitleName.c_str(), NULL));
#elif NF_PLATFORM == NF_PLATFORM_LINUX
    // Linux: 设置进程名称
    prctl(PR_SET_NAME, strTitleName.c_str());
#endif
```

## 五、常用启动场景示例

### 5.1 开发调试模式

```bash
# 单一服务器调试（Windows）
NFPluginLoader.exe --Server=GameServer --ID=1.1.1.1 --Config=./config --XButton

# 单一服务器调试（Linux）
./NFPluginLoader --Server=GameServer --ID=1.1.1.1 --Config=./config

# AllServer模式调试（Windows）- 每种服务器一个实例
NFPluginLoader.exe --Server=AllServer --ID=1.1.1.1 --Config=./config --XButton

# AllServer模式调试（Linux）- 每种服务器一个实例
./NFPluginLoader --Server=AllServer --ID=1.1.1.1 --Config=./config
```

### 5.2 生产环境部署

```bash
# 单一服务器生产部署
./NFPluginLoader --Server=GameServer --ID=1.11.1.1 \
  --Config=/opt/gameserver/config \
  --Plugin=/opt/gameserver/plugin \
  --LogPath=/var/log/gameserver \
  --Start --Daemon

# AllServer模式生产部署（小规模环境）
./NFPluginLoader --Server=AllServer --ID=1.1.1.1 \
  --Config=/opt/gameserver/config \
  --Plugin=/opt/gameserver/plugin \
  --LogPath=/var/log/gameserver \
  --Start --Daemon
```

### 5.3 集群和测试环境

```bash
# AllMoreServer模式 - 集群模拟和负载测试
./NFPluginLoader --Server=AllMoreServer --ID=1.1.1.1 \
  --Config=/opt/gameserver/config \
  --Plugin=/opt/gameserver/plugin \
  --Start --Daemon

# 特点：可以启动多个同类型服务器，如：
# - 3个ProxyServer实例用于负载均衡测试
# - 2个GameServer实例用于游戏逻辑分布
# - 多个LogicServer实例用于业务处理
```

### 5.4 不同规模的部署对比

```bash
# 小规模开发环境：AllServer模式
./NFPluginLoader --Server=AllServer --ID=1.1.1.1 --Config=./config
# 优点：资源占用少，启动快，适合功能开发

# 中规模测试环境：AllMoreServer模式
./NFPluginLoader --Server=AllMoreServer --ID=1.1.1.1 --Config=./config
# 优点：可以模拟集群，测试负载均衡和容错

# 大规模生产环境：分布式部署
# 在不同机器上启动不同的单一服务器
./NFPluginLoader --Server=ProxyServer --ID=1.10.1.1 --Start --Daemon
./NFPluginLoader --Server=GameServer --ID=1.11.1.1 --Start --Daemon
./NFPluginLoader --Server=LogicServer --ID=1.12.1.1 --Start --Daemon
# 优点：真正的分布式，高可用，易扩展
```

### 5.5 服务器维护操作

```bash
# 停止服务器
./NFPluginLoader --Server=GameServer --ID=1.1.1.1 --Stop

# 重载配置
./NFPluginLoader --Server=GameServer --ID=1.1.1.1 --Reload

# 优雅退出
./NFPluginLoader --Server=GameServer --ID=1.1.1.1 --Quit

# 强制杀死
./NFPluginLoader --Server=GameServer --ID=1.1.1.1 --Kill
```

## 六、错误处理和异常情况

### 6.1 参数验证

系统会对关键参数进行验证：

```cpp
// 服务器ID验证
uint32_t mBusId = NFServerIDUtil::GetBusID(strBusName);
if (mBusId <= 0)
{
    std::cerr << "ID:" << strBusName << std::endl;
    std::cerr << cmdParser.Usage() << std::endl;
    exit(0);
}
```

### 6.2 异常捕获

```cpp
try
{
    // 参数处理逻辑
}
catch (NFCmdLine::NFCmdLine_Error& e)
{
    std::cout << e.what() << std::endl;
    NFSLEEP(1000);
    exit(0);
}
```

### 6.3 PID文件操作失败

```cpp
// PID文件创建失败
if (pPluginManager->CreatePidFile() < 0)
{
    std::cout << "create " << pPluginManager->GetFullPath() 
              << " pid " << pPluginManager->GetPidFileName() 
              << " failed!" << std::endl;
    exit(0);
}
```

## 七、总结

NFShmXFrame的启动参数系统具有以下特点：

### 7.1 设计优势

1. **参数丰富**：支持20+个命令行参数，覆盖各种使用场景
2. **平台适配**：Windows和Linux平台都有针对性的参数支持
3. **模式灵活**：支持单服务器、多服务器、守护进程等多种运行模式
4. **操作便捷**：提供完整的服务器生命周期管理命令
5. **错误处理**：完善的参数验证和异常处理机制

### 7.2 核心流程

1. **参数解析**：使用NFCmdLine解析器处理命令行参数
2. **模式判断**：根据Server参数决定单服务器或多服务器模式
3. **配置设置**：将解析的参数设置到插件管理器中
4. **平台处理**：根据平台执行特定的初始化操作
5. **进程管理**：处理PID文件、守护进程、信号等

### 7.3 最佳实践

1. **开发环境**：使用简单参数，便于调试
2. **生产环境**：使用守护进程模式，配置完整路径
3. **集群部署**：使用多服务器模式，统一配置管理
4. **运维管理**：使用控制命令进行服务器生命周期管理

这套启动参数系统为NFShmXFrame提供了强大而灵活的服务器管理能力，满足了从开发到生产的各种需求。 