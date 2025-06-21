# NFShmXFrame架构服务器编译指南

## 概述

NFShmXFrame是一个高性能的共享内存框架服务器，支持多平台编译。本文档将详细介绍在不同操作系统上的编译流程。

### 支持的平台
- **CentOS 7+** （已预编译基础库）
- **Ubuntu 20.04+** （已预编译基础库）
- **Windows 10+** （已预编译基础库）
- **其他Linux发行版** （需手动编译依赖库）

### 编译方式
- **CMake方式**：推荐使用，支持自动依赖检测和安装
- **Makefile方式**：适用于开发调试，支持Excel自动解析生成代码

## 目录结构说明

```
NFShmXFrame/
├── thirdparty/
│   ├── pkg/                    # 第三方库源码包
│   ├── lib64_debug/            # Debug版本编译库
│   │   ├── ubuntu/             # Ubuntu专用库
│   │   └── (CentOS默认)
│   └── lib64_release/          # Release版本编译库
│       ├── ubuntu/             # Ubuntu专用库
│       └── (CentOS默认)
├── src/                        # 源代码目录
├── game/                       # 游戏逻辑代码
├── doc/                        # 文档目录
└── CMakeLists.txt             # CMake配置文件
```

## 一、Ubuntu 20.04+ 编译

### 1.1 系统依赖安装

```bash
# 更新包管理器
sudo apt update

# 安装基础编译工具
sudo apt install git build-essential cmake

# 安装项目必需依赖
sudo apt install libkrb5-dev        # Kerberos开发库
sudo apt install libtirpc-dev       # RPC开发库（Ubuntu 24需要）

# 配置动态链接库路径
sudo vim /etc/ld.so.conf
# 添加以下行：
# /usr/local/lib
sudo ldconfig
```

### 1.2 获取源码

```bash
git clone https://github.com/yigao/NFShmXFrame.git
cd NFShmXFrame
```

### 1.3 CMake编译（推荐）

#### Debug版本编译：
```bash
mkdir build_debug && cd build_debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

#### Release版本编译：
```bash
mkdir build_release && cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

#### 动态库版本编译：
```bash
# Debug动态库版本
mkdir build_dynamic_debug && cd build_dynamic_debug
cmake -DCMAKE_BUILD_TYPE=DynamicDebug ..
make -j$(nproc)

# Release动态库版本
mkdir build_dynamic_release && cd build_dynamic_release
cmake -DCMAKE_BUILD_TYPE=DynamicRelease ..
make -j$(nproc)
```

### 1.4 Makefile编译方式

如需使用Makefile进行开发调试：

```bash
# 进入项目的makefile目录，如：
cd game/TestGame/makefiles

# 执行make命令，自动解析Excel文件，生成proto、代码及bin文件
make
```

## 二、CentOS 7+ 编译

### 2.1 系统依赖安装

```bash
# 安装基础编译工具
sudo yum groupinstall "Development Tools"
sudo yum install git cmake3

# 安装项目必需依赖
sudo yum install krb5-devel.x86_64   # Kerberos开发库
sudo yum install libtirpc-devel      # RPC开发库
```

### 2.2 编译步骤

编译步骤与Ubuntu相同，使用cmake3替代cmake：

```bash
git clone https://github.com/yigao/NFShmXFrame.git
cd NFShmXFrame

# Debug版本
mkdir build_debug && cd build_debug
cmake3 -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

## 三、Windows 编译

### 3.1 环境准备

#### 安装Make工具
1. 解压 `thirdparty/pkg/win_make.zip` 到C盘
2. 将 `C:\win_make\chocolatey\bin` 添加到系统环境变量Path中

#### 安装Python 2.7
1. 使用 `thirdparty/pkg/python-2.7xxx.msi` 安装Python 2.7
2. 将 `C:\Python27` 添加到系统环境变量Path中

#### 安装Protobuf 3.6.0
1. 解压 `thirdparty/pkg/protobuf-3.6.0.zip`
2. 将 `tools/protoc.exe` 复制到 `protobuf-3.6.0/src/` 目录下

#### 安装Protobuf Python支持
```bash
# 打开Git Bash，进入protobuf python目录
cd protobuf-3.6.0/python

# 编译安装Python模块
python setup.py build
python setup.py test
python setup.py install
```

### 3.2 Visual Studio编译

推荐使用Visual Studio 2019或更高版本：

```bash
# 使用CMake生成VS解决方案
mkdir build
cd build
cmake -G "Visual Studio 16 2019" -A x64 ..

# 打开生成的.sln文件进行编译
```

### 3.3 CLion集成开发（可选）

如果使用CLion IDE：

1. 设置终端Shell路径：`C:\Program Files\Git\bin\bash.exe`
2. 在终端环境变量Path中添加：
   - `C:\win_make\chocolatey\bin`
   - `C:\Python27`
3. 在CLion中直接使用终端运行make命令

### 3.4 Makefile编译

```bash
# 在Git Bash中进入makefiles目录
cd game/TestGame/makefiles

# 执行make命令
make
```

## 四、其他平台手动编译依赖库

对于未预编译基础库的平台，需要手动编译以下依赖库：

### 4.1 MySQL Client编译

```bash
cd thirdparty/mysql
mkdir build && cd build

# 安装低版本cmake（如果需要）
sudo apt install cmake

cmake ..
make
make install

# 卸载低版本cmake（如果后续需要cmake 3.26）
sudo apt remove cmake
```

### 4.2 CMake 3.26编译（可选）

```bash
cd thirdparty/pkg
tar -xvf cmake-3.26.3.tar.gz
cd cmake-3.26.3

./configure
make
sudo make install
```

### 4.3 Protobuf编译

```bash
cd thirdparty/pkg
tar -xvf protobuf-x.x.0.tar
cd protobuf-x.x.0

./configure
make
make install

# Python模块编译（可选）
cd python
python2.7 setup.py build
python2.7 setup.py test  
python2.7 setup.py install

# 验证安装
python2.7 -c "import google.protobuf.internal"
```

### 4.4 其他依赖库编译

#### libevent-2.1.8
```bash
cd thirdparty/pkg
tar -xvf libevent-release-2.1.8-stable.tar.gz
cd libevent-release-2.1.8-stable

./configure --with-pic
make
sudo make install
```

#### curl-7.60.0
```bash
cd thirdparty/pkg
tar -xvf curl-7.60.0.tar.bz2
cd curl-7.60.0

./configure --with-pic
make
sudo make install
```

#### OpenSSL-1.0.2n
```bash
cd thirdparty/pkg
tar -xvf openssl-1.0.2n.tar.gz
cd openssl-1.0.2n

./config
# 编辑Makefile，在CFLAG行添加-fPIC
make
sudo make install
```

#### zlib
```bash
cd thirdparty/zlib
./configure --with-pic
make
sudo make install
```

#### Theron
```bash
cd thirdparty/Theron
make
# 编译好的库在Theron/Lib目录下
```

## 五、CMake配置选项

### 5.1 构建类型

- `Release`: 优化版本，用于生产环境
- `Debug`: 调试版本，包含调试信息
- `DynamicRelease`: 动态库优化版本
- `DynamicDebug`: 动态库调试版本

### 5.2 CMake选项

```bash
# 禁用自动依赖安装
cmake -DAUTO_INSTALL_DEPS=OFF ..

# 跳过依赖检测
cmake -DSKIP_DEPENDENCY_CHECK=ON ..

# 指定依赖库路径
cmake -DCMAKE_PREFIX_PATH=/path/to/libs ..

# 编译工具版本
cmake -DCMAKE_BUILD_TOOLS=ON ..
```

### 5.3 平台特定配置

CMake会自动检测Linux发行版并使用相应的库路径：
- CentOS/RHEL: 使用 `thirdparty/lib64_*/` 默认路径
- Ubuntu/Debian: 使用 `thirdparty/lib64_*/ubuntu/` 路径

## 六、常见问题及解决方案

### 6.1 依赖库缺失

**问题**: `fatal error: krb5.h: No such file or directory`

**解决方案**:
```bash
# Ubuntu
sudo apt install libkrb5-dev

# CentOS  
sudo yum install krb5-devel
```

### 6.2 RPC头文件缺失

**问题**: `fatal error: rpc/rpc.h: No such file or directory`

**解决方案**:
```bash
# Ubuntu 24+
sudo apt install libtirpc-dev

# CentOS
sudo yum install libtirpc-devel
```

### 6.3 CMake版本过低

**解决方案**: 从源码编译CMake 3.26或使用包管理器更新

### 6.4 库路径问题

如果出现链接错误，检查 `/etc/ld.so.conf` 是否包含 `/usr/local/lib`，然后执行 `sudo ldconfig`

## 七、验证编译结果

### 7.1 检查生成文件

编译成功后，检查以下目录：
- `Build/Bin/`: 可执行文件
- `Build/Lib/`: 静态/动态库文件
- `Install/Bin/`: 安装后的可执行文件

### 7.2 运行测试

```bash
# 进入bin目录
cd Build/Bin/Debug  # 或Release

# 运行主程序
./NFPluginLoader

# 运行测试程序
./NFTestMain
```

## 八、开发建议

### 8.1 IDE推荐配置

- **CLion**: 支持CMake项目，调试功能强大
- **Visual Studio**: Windows平台首选
- **VSCode**: 轻量级，支持CMake插件

### 8.2 调试配置

使用DynamicDebug版本进行开发调试，支持：
- 内存泄漏检测 (`-DCHECK_MEM_LEAK`)
- 详细错误信息
- 运行时库动态加载

### 8.3 性能优化

Release版本编译选项：
- 使用`-O2`优化（避免`-O3`可能的不稳定性）
- 启用`-fPIC`支持位置无关代码
- 静态链接标准库`-static-libstdc++`

---

**注意**: 不同平台的预编译库已经准备就绪。如果您使用的是CentOS、Ubuntu或Windows，可以直接进行编译。其他平台需要按照本文档手动编译相关依赖库。 