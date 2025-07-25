# NFServerController Qt版本

## 概述

NFServerController Qt版本是基于Qt框架开发的图形化游戏服务器管理工具，提供了直观的用户界面来管理NFrame游戏服务器集群。

## 功能特性

### 图形化界面
- 现代化的用户界面设计
- 服务器列表表格显示
- 实时状态更新
- 操作日志显示
- 状态栏信息

### 服务器管理
- **全局操作**: 启动/停止/重启/重载所有服务器
- **选中操作**: 对选中的服务器进行批量操作
- **模式操作**: 支持通配符模式匹配（如 `*.*.5.*` 表示所有类型5的服务器）
- **单服操作**: 双击服务器行查看详细信息

### 监控功能
- 自动刷新服务器状态（可配置间隔）
- 进程ID监控
- 服务器状态颜色编码
- 统计信息显示

### 日志管理
- 彩色日志输出
- 操作历史记录
- 日志保存功能
- 自动滚动到最新日志

## 系统要求

### Windows
- Windows 7 或更高版本
- Visual C++ Redistributable
- Qt5 运行时库（如果未静态编译）

### Linux
- 现代Linux发行版（Ubuntu 16.04+, CentOS 7+等）
- Qt5 运行时库
- 基本系统库（glibc, pthread等）

## 安装和构建

### 先决条件

#### Windows
1. **Visual Studio 2017或更高版本**
2. **CMake 3.10+**
3. **Qt5 开发环境**
   ```bash
   # 从官网下载并安装 Qt5
   # https://www.qt.io/download
   ```

#### Linux (Ubuntu/Debian)
```bash
# 安装开发工具
sudo apt-get update
sudo apt-get install build-essential cmake

# 安装Qt5开发环境
sudo apt-get install qtbase5-dev qttools5-dev
```

#### Linux (CentOS/RHEL)
```bash
# 安装开发工具
sudo yum groupinstall "Development Tools"
sudo yum install cmake

# 安装Qt5开发环境
sudo yum install qt5-qtbase-devel qt5-qttools-devel
```

### 构建步骤

#### 使用构建脚本（推荐）

**Windows:**
```cmd
# 双击运行或在命令行执行
build_qt.bat
```

**Linux:**
```bash
# 赋予执行权限并运行
chmod +x build_qt.sh
./build_qt.sh
```

#### 手动构建

```bash
# 创建构建目录
mkdir build_qt
cd build_qt

# 配置项目（启用Qt GUI）
cmake .. -DBUILD_QT_GUI=ON -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build . --config Release

# 或在Linux下使用make
make -j$(nproc)
```

#### 指定Qt安装路径（如果需要）
```bash
# Windows示例
cmake .. -DBUILD_QT_GUI=ON -DQt5_DIR="C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5"

# Linux示例
cmake .. -DBUILD_QT_GUI=ON -DQt5_DIR="/usr/lib/x86_64-linux-gnu/cmake/Qt5"
```

## 使用说明

### 启动应用程序

```bash
# Windows
NFServerControllerQt.exe

# Linux
./NFServerControllerQt

# 带参数启动
./NFServerControllerQt --config=servers.conf --maximized
```

### 命令行参数

| 参数 | 描述 | 示例 |
|------|------|------|
| `-c, --config` | 指定配置文件路径 | `--config=servers.conf` |
| `--style` | 指定UI样式 | `--style=fusion` |
| `--maximized` | 最大化窗口启动 | `--maximized` |
| `--minimized` | 最小化启动 | `--minimized` |
| `-h, --help` | 显示帮助信息 | `--help` |
| `-v, --version` | 显示版本信息 | `--version` |

### 界面布局

#### 主窗口区域
- **服务器列表**: 显示所有服务器的详细信息
- **控制面板**: 包含三个标签页的操作区域
- **操作日志**: 显示实时操作日志和状态信息

#### 工具栏
- **配置文件路径**: 显示和修改当前配置文件
- **自动刷新**: 开启/关闭自动刷新功能
- **刷新间隔**: 设置自动刷新的时间间隔

#### 状态栏
- **当前状态**: 显示当前操作状态
- **统计信息**: 服务器总数、运行数、停止数

### 操作指南

#### 1. 加载配置文件
- 点击工具栏的"浏览..."按钮选择配置文件
- 或者直接在路径编辑框中输入文件路径
- 点击"重新加载"按钮应用配置

#### 2. 服务器选择
- **全选**: 选择所有服务器
- **全不选**: 取消所有选择
- **选择运行中**: 仅选择正在运行的服务器
- **选择已停止**: 仅选择已停止的服务器

#### 3. 服务器操作

**全局操作**（影响所有服务器）:
- 启动所有服务器
- 停止所有服务器
- 重启所有服务器
- 重载所有配置

**选中操作**（影响选中的服务器）:
- 启动选中服务器
- 停止选中服务器
- 重启选中服务器
- 重载选中配置

**模式操作**（基于通配符模式）:
- 在模式输入框中输入模式，如：
  - `*.*.*.*` - 所有服务器
  - `*.*.5.*` - 所有类型5的服务器
  - `1.13.*.*` - 世界1区域13的所有服务器
- 点击相应的操作按钮

**共享内存清理**（清理共享内存段）:
- **清理所有服务器共享内存**: 停止所有服务器并基于 serverId 精确清理共享内存
- **清理选中服务器共享内存**: 停止选中的服务器并基于 serverId 精确清理共享内存
- **清理模式服务器共享内存**: 根据输入的模式清理匹配服务器的共享内存（基于 serverId）
- 通过 NFServerIDUtil 获取服务器专用的共享内存 key，实现精确定位和清理
- 使用纯 C++ 系统调用实现，不依赖外部脚本，安全高效
- 所有清理操作都会显示确认对话框，防止误操作
- 提供详细的日志输出，显示清理的共享内存段信息（ID、Key、大小、连接数）

#### 4. 监控功能
- 开启"自动刷新"复选框
- 选择合适的刷新间隔（1-60秒）
- 观察服务器状态变化

#### 5. 日志管理
- 查看实时操作日志
- 使用"清空日志"清除历史记录
- 使用"保存日志"将日志保存到文件

### 配置文件格式

配置文件使用管道符(|)分隔的格式：

```
ServerName|ServerID|ConfigPath|PluginPath|LuaScriptPath|LogPath|GameName|ExecutablePath|WorkingDir
MasterServer|1.13.1.1|../../Config|../../Plugin|../../LuaScript|../../logs|LieRen|../../Bin/NFServerStatic|../../
```

字段说明：
- **ServerName**: 服务器名称
- **ServerID**: 服务器ID（格式：世界.区域.类型.索引）
- **ConfigPath**: 配置文件路径
- **PluginPath**: 插件路径
- **LuaScriptPath**: Lua脚本路径
- **LogPath**: 日志路径
- **GameName**: 游戏名称
- **ExecutablePath**: 可执行文件路径
- **WorkingDir**: 工作目录

## 状态说明

| 状态 | 颜色 | 描述 |
|------|------|------|
| 运行中 | 绿色 | 服务器正常运行 |
| 已停止 | 灰色 | 服务器已停止 |
| 启动中 | 橙色 | 服务器正在启动 |
| 停止中 | 橙色 | 服务器正在停止 |
| 错误 | 红色 | 服务器出现错误 |

## 常见问题

### Q: 启动时提示找不到配置文件
**A**: 请确保配置文件路径正确，或者使用"浏览..."按钮重新选择配置文件。

### Q: 服务器状态显示不正确
**A**: 点击"刷新状态"按钮手动更新，或检查服务器进程是否正常运行。

### Q: 无法启动服务器
**A**: 检查：
1. 可执行文件路径是否正确
2. 工作目录是否存在
3. 是否有足够的权限
4. 端口是否被占用

### Q: Qt版本编译失败
**A**: 确保：
1. 已正确安装Qt5开发环境
2. Qt5路径已添加到环境变量
3. CMake版本不低于3.10
4. 编译器支持C++17

## 技术支持

如果遇到问题，请检查：
1. 操作日志中的错误信息
2. 系统事件日志
3. 服务器进程状态

## 版本历史

### v2.0 (当前版本)
- 全新的Qt图形化界面
- 支持批量操作和模式匹配
- 实时状态监控
- 详细的操作日志
- 自动刷新功能
- 设置保存和恢复

---

**注意**: 本工具仅用于开发和测试环境，生产环境使用请谨慎测试。 