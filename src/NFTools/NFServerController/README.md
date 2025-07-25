# NFServerController v2.0

NFServerController 是一个用于管理 NFrame 服务器集群的工具，支持通配符模式匹配和批量操作。

## 功能特性

- 支持通配符模式匹配服务器ID
- 单次命令执行，无需交互式模式
- 批量操作多个服务器
- 跨平台支持（Windows/Linux）

## 命令格式

```bash
NFServerController [选项] "命令 目标"
```

### 选项

- `-c, --config <文件>` - 指定配置文件路径
- `-v, --verbose` - 详细输出
- `-q, --quiet` - 静默模式
- `-h, --help` - 显示帮助信息

### 命令

- `start <目标>` - 启动指定服务器
- `stop <目标>` - 停止指定服务器
- `restart <目标>` - 重启指定服务器（使用独立的 --restart 参数）
- `reload <目标>` - 重载指定服务器配置（热更新配置，无需重启）
- `check [目标]` - 显示服务器状态


### 目标格式

服务器ID格式：`WorldID.RegionID.ServerType.Index`

使用 `*` 作为通配符匹配任意组件。

### 通配符示例

- `*.*.*.*` - 所有服务器
- `*.*.5.*` - 所有ServerType为5的服务器
- `1.13.*.*` - World 1, Region 13中的所有服务器
- `1.13.1.1` - 特定服务器

### 服务器类型

| 类型 | 服务器名称 |
|------|------------|
| 1 | MasterServer |
| 3 | RouteAgentServer |
| 5 | ProxyServer |
| 6 | StoreServer |
| 7 | LoginServer |
| 8 | WorldServer |
| 9 | LogicServer |
| 10 | GameServer |

## 使用示例

### 启动所有服务器
```bash
NFServerController "start *.*.*.*"
```

### 重启所有游戏服务器（类型10）
```bash
NFServerController "restart *.*.10.*"
```

### 停止特定服务器
```bash
NFServerController "stop 1.13.1.1"
```

### 重载服务器配置（热更新）
```bash
NFServerController "reload *.*.10.*"  # 重载所有游戏服务器配置
NFServerController "reload 1.13.1.1"  # 重载特定服务器配置
```

### 清理共享内存
```bash
NFServerController "clear"             # 清理所有服务器的共享内存
NFServerController "clear *.*.*.*"     # 清理所有服务器的共享内存
NFServerController "clear *.*.5.*"     # 清理类型5的所有服务器共享内存
NFServerController "clear 1.13.1.1"   # 清理特定服务器的共享内存
```

### 查看所有服务器状态
```bash
NFServerController "check"
```

### 查看特定区域的服务器状态
```bash
NFServerController "check 1.13.*.*"
```

## 重启命令特性

`restart` 命令具有以下特性：
- **独立重启**：直接使用 `--restart` 参数启动服务器进程
- **不依赖停止**：不需要先执行停止操作
- **原子操作**：服务器内部处理重启逻辑，确保数据一致性
- **更快速度**：相比先停止再启动的方式更加高效

## 重载命令特性

`reload` 命令具有以下特性：
- **热更新**：直接使用 `--reload` 参数重载服务器配置
- **无需重启**：服务器进程保持运行，只重载配置文件
- **配置生效**：动态加载新的配置参数、Lua脚本等
- **最小影响**：对服务器运行状态影响最小，适合生产环境使用

### 重启方式对比

**旧方式（不推荐）**：
1. 停止服务器进程
2. 等待进程完全退出
3. 启动新的服务器进程

**新方式（推荐）**：
1. 直接发送 `--restart` 命令给服务器
2. 服务器内部处理重启逻辑

## 共享内存清理特性

`clear` 命令具有以下特性：
- **安全清理**：在清理共享内存前会先停止相关服务器
- **精确定位**：通过 NFServerIDUtil::GetBusID 和 NFServerIDUtil::GetShmObjKey 获取服务器专用的共享内存 key
- **原生实现**：使用纯 C++ 系统调用（shmctl、shmget）执行清理操作，不依赖外部脚本
- **跨平台**：Linux/Unix 系统原生支持，Windows 系统需要 WSL
- **模式匹配**：支持通配符模式清理特定服务器的共享内存
- **确认对话**：Qt 界面提供确认对话框防止误操作
- **详细日志**：显示找到的共享内存段信息（ID、Key、大小、连接数）和清理结果

### 清理方式

- **全部清理**：`clear` 或 `clear *.*.*.*` - 停止所有服务器并基于 serverId 精确清理共享内存
- **模式清理**：`clear *.*.5.*` - 清理匹配模式的服务器共享内存（基于 serverId）
- **单个清理**：`clear 1.13.1.1` - 根据 serverId 精确清理指定服务器的共享内存

### 工作原理

1. **Key 生成**：使用 `NFServerIDUtil::GetBusID(serverId)` 和 `NFServerIDUtil::GetShmObjKey(serverId)` 获取共享内存 key
2. **Shmid 查找**：通过 `shmget(key, 0, 0666)` 获取对应的共享内存段 ID
3. **信息获取**：使用 `shmctl(shmid, IPC_STAT, &info)` 获取段信息（大小、连接数等）
4. **安全删除**：使用 `shmctl(shmid, IPC_RMID, nullptr)` 删除共享内存段

## 注意事项

1. 命令和目标必须包含在引号中，特别是包含通配符时
2. **服务器按配置文件中的顺序启动**，停止时按反序执行
3. 配置文件中的服务器顺序很重要，请确保依赖服务器在前面
4. 批量操作时，工具会在操作间添加适当的延迟
5. 使用 `-v` 选项可以查看详细的操作日志

## 配置文件顺序

服务器在配置文件中的顺序决定了启动顺序：
- 启动：按配置文件从上到下的顺序
- 停止：按配置文件从下到上的顺序（反序）

建议的配置顺序：
```
MasterServer|1.13.1.1|...        # 1. 首先启动主服务器
RouteAgentServer|1.13.3.1|...    # 2. 路由代理服务器  
StoreServer|1.13.6.1|...         # 3. 数据存储服务器
LoginServer|1.13.7.1|...         # 4. 登录服务器
WorldServer|1.13.8.1|...         # 5. 世界服务器
LogicServer|1.13.9.1|...         # 6. 逻辑服务器
GameServer|1.13.10.1|...         # 7. 游戏服务器
ProxyServer|1.13.5.1|...         # 8. 最后启动代理服务器
```

## 配置文件格式

配置文件格式（每行一个服务器）：
```
ServerName|ServerID|ConfigPath|PluginPath|LuaScriptPath|LogPath|GameName|ExecutablePath|WorkingDir
```

示例：
```
MasterServer|1.13.1.1|../Install/Config|../Install/LieRenPlugin|../Install/LuaScript|../Install/logs|LieRen|../Install/Bin/NFServerStatic|../Install
GameServer|1.13.10.1|../Install/Config|../Install/LieRenPlugin|../Install/LuaScript|../Install/logs|LieRen|../Install/Bin/NFServerStatic|../Install
```

## 故障排除

### 常见错误及解决方案

#### 1. "CreateProcessA failed for restart, error code: 2"

**错误原因**: 找不到可执行文件（ERROR_FILE_NOT_FOUND）

**解决方案**:
1. **检查可执行文件路径**: 确保 `ExecutablePath` 相对于 `WorkingDir` 是正确的
   ```
   如果 WorkingDir = "../../Install/Bin/Debug"
   那么 ExecutablePath 应该是 "../NFServerStatic" 而不是 "../../Install/Bin/NFServerStatic"
   ```

2. **检查文件是否存在**: 确认可执行文件确实存在
   - Windows: 确保 `NFServerStatic.exe` 存在
   - Linux: 确保 `NFServerStatic` 存在且有执行权限

3. **使用绝对路径**: 如果相对路径有问题，可以使用绝对路径
   ```
   ExecutablePath = "E:/nfshm-xframe/Install/Bin/NFServerStatic"
   ```

4. **启用详细日志**: 使用 `-v` 参数查看详细的启动信息
   ```bash
   NFServerController -v "start 1.13.1.1"
   ```

#### 2. 路径配置最佳实践

**推荐配置结构**:
```
项目根目录/
├── Install/
│   ├── Bin/
│   │   ├── Debug/          # 工作目录
│   │   └── NFServerStatic  # 可执行文件
│   ├── Config/
│   ├── LuaScript/
│   └── logs/
└── src/NFTools/NFServerController/
    └── servers.conf        # 配置文件位置
```

**对应的配置**:
```
# 从 NFServerController 目录看的路径
WorkingDir = "../../Install/Bin/Debug"
ExecutablePath = "../NFServerStatic"  # 相对于 WorkingDir
```

#### 3. 权限问题

**Windows**:
- 以管理员身份运行命令提示符
- 确保可执行文件没有被杀毒软件阻止

**Linux**:
```bash
chmod +x /path/to/NFServerStatic
```

#### 4. 调试步骤

1. **测试路径组合**:
   ```bash
   # 测试是否能找到可执行文件
   NFServerController -v "start 1.13.1.1"
   ```

2. **检查日志输出**:
   查看以下日志信息：
   - "Start command line: ..."
   - "Working directory: ..."
   - "Executable path: ..."
   - "Executable file does not exist: ..."

3. **手动验证路径**:
   ```bash
   # 手动检查文件是否存在
   cd ../../Install/Bin/Debug
   ls -la ../NFServerStatic     # Linux
   dir ..\NFServerStatic.exe    # Windows
   ```

### 日志级别说明

- `-q, --quiet`: 仅显示错误信息
- 默认: 显示基本操作信息  
- `-v, --verbose`: 显示详细调试信息，包括完整的命令行和路径信息 