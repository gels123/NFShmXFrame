# NFShmXFrame 服务器代码组织结构分析

## 目录概览

NFShmXFrame 是一个插件化的分布式游戏服务器框架，采用插件化架构设计。框架中的"X"代表脚本插件扩展能力，目前已实现单线程Lua脚本插件。框架提供了两种内存池插件供开发者选择：共享内存池插件和正常内存池插件，可以根据需求灵活切换。

### 项目根目录结构

```
NFShmXFrame/
├── src/                    # 核心源代码目录
│   ├── NFrame/            # 框架底层 - 核心框架层
│   ├── NFServer/          # 服务器架构层 - 各种服务器实现
│   ├── NFTools/           # 工具集
│   └── NFTest/            # 测试代码
├── game/                  # 游戏逻辑层 - 具体游戏业务实现
│   ├── Other/           # 其他项目
│   ├── TestGame/         # 测试游戏
│   └── NFGameCommon/     # 游戏公共模块
├── doc/                   # 文档目录
├── thirdparty/           # 第三方库
├── tools/                # 构建和管理工具
├── Build/                # 构建输出目录
└── Install/              # 安装目录
```

## 一、NFrame 框架底层（核心框架层）

NFrame是整个服务器架构的基础框架，提供了插件系统、内存池管理、网络通信、脚本扩展等核心功能。

### 1.1 框架核心组件

#### NFComm - 通信与工具模块
```
src/NFrame/NFComm/
├── NFCore/               # 核心工具类
│   ├── NFCommon.h/cpp   # 公共定义和工具函数
│   ├── NFTime.h/cpp     # 时间管理
│   ├── NFStringUtility.h/cpp  # 字符串工具
│   ├── NFFileUtility.h/cpp    # 文件操作工具
│   ├── NFHash.hpp       # 哈希算法
│   ├── NFRandom.hpp     # 随机数生成
│   └── NFSingleton.hpp  # 单例模式实现
├── NFPluginModule/      # 插件模块接口定义
├── NFShmStl/           # 共享内存STL容器
├── NFKernelMessage/    # 内核消息定义
├── NFObjCommon/        # 对象通用功能
└── Message/            # 消息协议定义
```

#### NFPluginManager - 插件管理器
```
src/NFrame/NFPluginManager/
```
- 负责插件的加载、卸载、初始化
- 管理插件间的依赖关系
- 提供插件热更新支持
- 管理内存池插件的切换

#### NFPluginLoader - 插件加载器
```
src/NFrame/NFPluginLoader/
```
- 程序入口点
- 负责启动参数解析
- 初始化插件管理器

#### NFCommPlugin - 通用插件
```
src/NFrame/NFCommPlugin/
```
- 提供基础的插件功能
- 包含核心模块实现

### 1.2 内存池插件系统

NFShmXFrame提供两种内存池插件，开发者可以根据需求和个人爱好进行选择：

#### 1.2.1 NFShmPlugin - 共享内存池插件
- **优点**：
  - 支持C++代码热更新
  - 服务器崩溃后数据不丢失
- **缺点**：
  - 数据必须放在共享内存中
  - 使用限制较多，需要特殊的共享内存STL容器
- **适用场景**：需要热更新和高可用性的生产环境

#### 1.2.2 NFMemPlugin - 正常内存池插件  
- **优点**：
  - 可以自由使用内存
  - 开发简单，无使用限制
  - 性能更优
- **缺点**：
  - 无法实现代码热更新
  - 服务器崩溃会丢失内存数据
- **适用场景**：开发测试环境或对热更新无要求的场景

### 1.3 脚本扩展系统（X插件）

NFShmXFrame中的"X"代表脚本插件扩展能力：

#### 1.3.1 Lua脚本插件
- **当前状态**：已实现单线程Lua插件
- **功能特性**：
  - 支持Lua脚本热更新
  - 业务逻辑可用Lua编写
  - C++底层框架 + Lua业务逻辑的混合开发模式
- **后续计划**：
  - 多线程Actor Lua系统（类似Skynet架构）
  - Python脚本插件
  - JavaScript脚本插件

### 1.4 框架特性

- **插件化架构**：所有功能以插件形式实现，支持动态加载/卸载
- **双内存池机制**：共享内存池和正常内存池可选择使用
- **脚本扩展能力**：支持多种脚本语言扩展（当前支持Lua）
- **灵活的热更新**：根据内存池选择决定是否支持热更新
- **跨平台支持**：Windows、Linux平台兼容

## 二、NFServer 服务器架构层

NFServer层实现了各种功能服务器，构建了完整的分布式游戏服务器架构。

### 2.1 核心服务器

#### 2.1.1 基础服务器
- **NFMasterServer**：主控服务器，负责服务器注册和发现
- **NFCenterServer**：中心服务器，处理跨服务器通信
- **NFRouteServer**：路由服务器，处理分布式消息路由
- **NFRouteAgentServer**：路由代理服务器，单机内路由

#### 2.1.2 网络层服务器
- **NFProxyServer**：代理服务器，客户端接入点
- **NFProxyAgentServer**：代理代理服务器，处理客户端连接

#### 2.1.3 业务逻辑服务器
- **NFLoginServer**：登录服务器，处理用户认证
- **NFLogicServer**：逻辑服务器，处理游戏核心逻辑
- **NFGameServer**：游戏服务器，处理游戏房间逻辑
- **NFWorldServer**：世界服务器，处理大世界逻辑

#### 2.1.4 数据与缓存服务器
- **NFStoreServer**：存储服务器，数据库操作
- **NFWebServer**：Web服务器，HTTP接口
- **NFOnlineServer**：在线状态服务器

#### 2.1.5 功能性服务器
- **NFSnsServer**：社交服务器，好友、公会等
- **NFMatchServer**：匹配服务器，游戏匹配
- **NFCityServer**：城市服务器，特定场景逻辑
- **NFCheckServer**：检查服务器，数据校验
- **NFNavMeshServer**：导航网格服务器，寻路服务

### 2.2 服务器公共模块

#### NFServerComm - 服务器通用组件
```
src/NFServer/NFServerComm/
├── NFServerCommon/      # 服务器公共代码
├── NFServerCommonPlugin/ # 服务器公共插件
├── NFDescStorePlugin/   # 配置存储插件
├── NFServerMessage/     # 服务器间消息定义
└── Message/            # 消息协议
```

#### NFXPlugin - 扩展插件
```
src/NFServer/NFXPlugin/
```
- 提供服务器扩展功能插件

### 2.3 服务器架构特点

- **分布式设计**：支持多服务器、多物理机部署
- **服务器ID机制**：每个服务器有唯一ID（如1.1.1.1），类似IP地址
- **统一通信接口**：TCP、共享内存Bus统一接口，配置切换
- **路由代理机制**：通过RouteAgent实现同机通信，跨机通过RouteServer
- **内存池无关性**：服务器层与底层内存池实现解耦

## 三、Game 游戏逻辑层

游戏逻辑层包含具体的游戏业务实现，每个游戏项目独立组织。可以选择使用C++或脚本语言进行开发。

### 3.1 项目结构

#### 3.1.1 Other 手游项目
```
game/Other/
├── source/              # 游戏源码
│   ├── Server/         # 服务器入口
│   ├── LoginServer/    # 登录服务器逻辑
│   ├── LogicServer/    # 逻辑服务器实现  
│   ├── GateServer/     # 网关服务器
│   ├── CenterServer/   # 中心服务器
│   ├── MasterServer/   # 主控服务器
│   ├── ZoneServer/     # 区域服务器
│   ├── LogServer/      # 日志服务器
│   ├── DB/            # 数据库相关
│   ├── Common/        # 游戏公共代码
│   └── Base/          # 基础组件
├── include/           # 头文件
├── Excel/            # Excel配置文件
├── makefiles/        # 构建文件
└── NFLogicComm/      # 逻辑通信组件
```

#### 3.1.2 NFGameCommon 游戏公共模块
```
game/NFGameCommon/
├── NFGameCommon/      # 游戏通用功能
├── Behavior/          # 行为系统
└── RecastNavigation/  # 导航系统
```

### 3.2 游戏层特点

- **业务分离**：游戏逻辑与底层框架完全分离
- **内存池兼容**：支持共享内存池和正常内存池两种模式
- **多语言支持**：C++和Lua混合开发，可根据需求选择
- **Excel配置系统**：支持Excel配置自动生成代码和数据库
- **协议生成**：基于Protobuf的协议自动生成
- **数据库ORM**：基于Protobuf反射的数据库操作，无需手写SQL

## 四、工具与辅助系统

### 4.1 NFTools 工具集
```
src/NFTools/
├── NFServerController/  # 服务器控制工具
└── NFExcelProcess/     # Excel处理工具
```

### 4.2 第三方库支持
- **网络**：libevent、evpp
- **数据库**：MySQL、mysqlpp、hiredis(Redis)
- **序列化**：Google Protobuf
- **脚本**：Lua、LuaIntf
- **其他**：spdlog、rapidjson、OpenSSL等

## 五、架构设计原则

### 5.1 分层架构
```
游戏逻辑层 (Game)
    ↓
服务器架构层 (NFServer)  
    ↓
框架基础层 (NFrame)
```

### 5.2 核心设计思想

1. **插件化**：所有功能模块化，支持热插拔
2. **内存池插件化**：共享内存池和正常内存池可选择使用
3. **脚本扩展性**：支持多种脚本语言扩展（X插件系统）
4. **分布式**：支持单进程调试和多进程分布式部署
5. **灵活热更新**：根据内存池选择决定热更新能力
6. **配置驱动**：Excel配置自动生成代码和数据库
7. **协议自动化**：基于Protobuf的协议和数据库操作自动化

### 5.3 技术特色

- **双内存池机制**：NFShmPlugin（共享内存池）和NFMemPlugin（正常内存池）可互换
- **脚本插件系统**：X插件提供多语言脚本支持
- **共享内存STL**：完整实现可用于共享内存的STL容器（用于NFShmPlugin）
- **服务器ID路由**：类似IP的服务器标识和路由机制
- **统一通信接口**：TCP和共享内存Bus接口统一
- **Protobuf反射**：自动化数据库操作，无需手写SQL
- **Excel工具链**：配置到代码到数据库的自动化流程

## 六、部署架构

### 6.1 单机架构
- 所有服务器运行在单进程内
- 便于开发调试
- 通过配置参数控制
- 可选择内存池类型

### 6.2 分布式架构
- 多服务器多物理机部署
- RouteAgent处理同机通信
- RouteServer处理跨机通信
- Master服务器作为注册中心
- 各服务器可独立选择内存池类型

## 七、开发流程

### 7.1 选择开发模式
1. **内存池选择**：
   - 生产环境选择NFShmPlugin（支持热更新）
   - 开发环境可选择NFMemPlugin（开发简单）
2. **语言选择**：
   - C++开发：完整功能，性能最优
   - Lua脚本开发：快速开发，支持热更新

### 7.2 添加新功能
1. 定义Protobuf消息结构
2. 生成相关代码（Excel、数据库、协议）
3. 实现业务逻辑Module（C++或Lua）
4. 编写Plugin集成到框架
5. 配置服务器加载

### 7.3 热更新流程
#### 使用NFShmPlugin（共享内存池）：
1. 修改共享内存类（不改变结构大小）
2. 重新编译生成.so文件
3. 使用热更新命令加载新代码
4. 业务逻辑无需重启继续运行

#### 使用Lua脚本：
1. 修改Lua脚本文件
2. 发送热更新命令
3. 脚本逻辑立即生效

## 八、总结

NFShmXFrame通过三层架构设计和插件化机制，实现了高度灵活的服务器框架：

- **NFrame层**提供稳定的框架基础，包含插件系统、双内存池机制、网络通信、脚本扩展等核心功能
- **NFServer层**实现各种服务器类型，构建完整的分布式游戏服务器架构
- **Game层**专注于具体游戏业务逻辑实现，支持C++和脚本混合开发

### 框架核心优势：

1. **灵活的内存管理**：NFShmPlugin和NFMemPlugin两种内存池可根据需求选择
2. **可选的热更新能力**：基于内存池选择和脚本系统的灵活热更新机制  
3. **多语言支持**：X插件系统提供脚本语言扩展能力
4. **生产就绪**：从开发调试到生产部署的完整解决方案

这种设计使得框架既能满足开发阶段的简单易用需求，又能提供生产环境所需的高可用性和热更新能力，特别适合大型MMO游戏和分布式系统开发。 