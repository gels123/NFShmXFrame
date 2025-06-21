# NFServerController

NFServerController是NF框架的服务器管理工具，支持Windows和Linux平台。

## 功能特性

- 服务器进程管理（启动/停止/重启/重载）
- 服务器状态监控
- 服务器依赖管理
- 跨平台支持（Windows/Linux）
- 可配置的服务器参数
- 日志管理

## 服务器类型

系统支持以下服务器类型：

1. **MasterServer** (ID: 1.13.1.1)
   - 核心服务器，管理其他服务器
   - 必须首先启动
   - 无依赖关系

2. **RouteAgentServer** (ID: 1.13.3.1)
   - 处理路由和代理管理
   - 依赖于MasterServer
   - 必须第二个启动

3. **RouteServer** (ID: 1.13.4.1)
   - 处理服务器间路由
   - 依赖于MasterServer和RouteAgentServer

4. **ProxyServer** (ID: 1.13.5.1)
   - 管理代理连接
   - 依赖于MasterServer和RouteAgentServer

5. **StoreServer** (ID: 1.13.6.1)
   - 处理数据存储
   - 依赖于MasterServer和RouteAgentServer

6. **LoginServer** (ID: 1.13.7.1)
   - 管理用户登录
   - 依赖于MasterServer和RouteAgentServer

7. **WorldServer** (ID: 1.13.8.1)
   - 管理世界实例
   - 依赖于MasterServer和RouteAgentServer

8. **LogicServer** (ID: 1.13.9.1)
   - 处理游戏逻辑
   - 依赖于MasterServer和RouteAgentServer

9. **GameServer** (ID: 1.13.10.1)
   - 管理游戏会话
   - 依赖于MasterServer和RouteAgentServer

10. **MatchServer** (ID: 1.13.11.1)
    - 处理游戏匹配
    - 依赖于MasterServer和RouteAgentServer

11. **NavMeshServer** (ID: 1.13.12.1)
    - 处理寻路网格
    - 依赖于MasterServer和RouteAgentServer

12. **OnlineServer** (ID: 1.13.13.1)
    - 管理在线状态
    - 依赖于MasterServer和RouteAgentServer

13. **SnsServer** (ID: 1.13.14.1)
    - 处理社交网络服务
    - 依赖于MasterServer和RouteAgentServer

14. **WebServer** (ID: 1.13.15.1)
    - 提供Web服务接口
    - 依赖于MasterServer和RouteAgentServer

## 配置

服务器配置文件格式如下：
```
ServerName|ServerID|ConfigPath|PluginPath|LuaScriptPath|LogPath|GameName|ExecutablePath|WorkingDir
```

示例：
```
MasterServer|1.13.1.1|../Install/Config|../Install/LieRenPlugin|../Install/LuaScript|../Install/logs|LieRen|../Install/Bin/NFServerStatic.exe|../Install
```

## 使用方法

### 命令行选项

```
-c, --config <文件>     指定配置文件路径
-v, --verbose           详细输出模式
-q, --quiet             静默模式
-h, --help              显示帮助信息
```

### 命令

```
start [服务器ID]        启动指定服务器或所有服务器
stop [服务器ID]         停止指定服务器或所有服务器
restart [服务器ID]      重启指定服务器或所有服务器
reload [服务器ID]       重载指定服务器配置
status [服务器ID]       查看服务器状态
monitor                 启动监控模式
list                    列出所有服务器
```

### 示例

```bash
# 启动所有服务器
NFServerController start

# 启动指定服务器
NFServerController start 1.13.1.1

# 停止所有服务器
NFServerController stop

# 停止指定服务器
NFServerController stop 1.13.1.1

# 查看所有服务器状态
NFServerController status

# 查看指定服务器状态
NFServerController status 1.13.1.1

# 启动监控模式
NFServerController monitor
```

## 服务器依赖关系

服务器按以下顺序启动：
1. MasterServer
2. RouteAgentServer
3. StoreServer
4. LoginServer
5. WorldServer
6. LogicServer
7. GameServer
8. ProxyServer

服务器按相反顺序停止。

## 日志级别

- 0: 仅错误信息
- 1: 错误和警告信息
- 2: 所有信息（默认）

## 编译

### Windows
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Linux
```bash
mkdir build
cd build
cmake ..
make
```

## 注意事项

- 服务器ID必须唯一
- 服务器名称可以重复
- 系统自动管理服务器依赖关系
- 服务器状态每5秒监控一次
- 进程终止先尝试优雅关闭，然后强制终止

## 错误处理

程序提供完善的错误处理机制：

- **配置文件错误**: 自动使用默认配置
- **进程启动失败**: 详细错误信息和回滚
- **网络连接问题**: 重试机制
- **权限问题**: 清晰的错误提示

## 日志系统

程序提供三个日志级别：
- **错误 (ERROR)**: 关键错误信息
- **警告 (WARNING)**: 警告信息
- **信息 (INFO)**: 详细操作信息

## 故障排除

### 常见问题

1. **服务器启动失败**
   - 检查可执行文件路径
   - 确认工作目录存在
   - 检查端口是否被占用

2. **配置文件读取失败**
   - 确认文件存在且可读
   - 检查文件格式是否正确
   - 验证路径分隔符（Windows使用\\，Linux使用/）

3. **权限问题**
   - Linux下确保有执行权限
   - Windows下以管理员身份运行

### 调试模式

使用 `-v` 选项启用详细输出：

```bash
./NFServerController -v start
```

## 贡献指南

欢迎提交问题报告和功能请求。在提交代码前，请确保：

1. 代码通过编译测试
2. 在Windows和Linux平台测试
3. 遵循现有代码风格
4. 添加必要的注释

## 许可证

本项目采用MIT许可证，详见LICENSE文件。

## 联系方式

如有问题或建议，请通过以下方式联系：
- 提交GitHub Issue
- 发送邮件至项目维护者

---

**注意**: 本程序需要与NFrame服务器框架配合使用，确保服务器可执行文件和配置文件路径正确。 