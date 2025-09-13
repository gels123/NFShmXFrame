CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
project(ShmNFrame)

message("CMAKE_VERSION: ${CMAKE_VERSION} ")
message("CMAKE_HOST_SYSTEM_NAME: ${CMAKE_HOST_SYSTEM_NAME} ")
message("CMAKE_HOST_SYSTEM: ${CMAKE_HOST_SYSTEM} ")

message("CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME} ")
message("CMAKE_SYSTEM: ${CMAKE_SYSTEM} ")

message("CMAKE_HOST_UNIX: ${CMAKE_HOST_UNIX} ")
message("CMAKE_HOST_WIN32: ${CMAKE_HOST_WIN32} ")

message("CMAKE_C_COMPILER: ${CMAKE_C_COMPILER} ")
message("CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER} ")

#---------------------------------------------------------------------------------------
# Compiler config
#---------------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
if (CMAKE_BUILD_TOOLS)
    set(CMAKE_CXX_STANDARD 17)
else()
    set(CMAKE_CXX_STANDARD 11)
endif()
message("CMAKE_BUILD_TOOLS: ${CMAKE_BUILD_TOOLS} ")

if (UNIX)
    # 初始化变量
    set(DISTRO_ID "unknown")
    set(DISTRO_VERSION_ID "")
    
    if (${CMAKE_VERSION} VERSION_LESS "3.22.0")
        # 对于较老的CMake版本，使用文件检测方法
        message(STATUS "使用文件检测方法判断Linux发行版...")
        
        # 方法1：检查 /etc/os-release（现代Linux标准）
        if(EXISTS "/etc/os-release")
            file(READ "/etc/os-release" OS_RELEASE_CONTENT)
            
            # 提取ID字段
            string(REGEX MATCH "ID=\"?([^\"\n]*)\"?" ID_MATCH "${OS_RELEASE_CONTENT}")
            if(ID_MATCH)
                string(REGEX REPLACE "ID=\"?([^\"]*)\"?" "\\1" DISTRO_ID "${ID_MATCH}")
                string(TOLOWER "${DISTRO_ID}" DISTRO_ID)
            endif()
            
            # 提取VERSION_ID字段
            string(REGEX MATCH "VERSION_ID=\"?([^\"\n]*)\"?" VERSION_MATCH "${OS_RELEASE_CONTENT}")
            if(VERSION_MATCH)
                string(REGEX REPLACE "VERSION_ID=\"?([^\"]*)\"?" "\\1" DISTRO_VERSION_ID "${VERSION_MATCH}")
            endif()
            
        # 方法2：检查 /etc/redhat-release（CentOS/RHEL）
        elseif(EXISTS "/etc/redhat-release")
            file(READ "/etc/redhat-release" REDHAT_RELEASE_CONTENT)
            if(REDHAT_RELEASE_CONTENT MATCHES "CentOS")
                set(DISTRO_ID "centos")
            elseif(REDHAT_RELEASE_CONTENT MATCHES "Red Hat")
                set(DISTRO_ID "rhel")
            endif()
            
            # 提取版本号
            string(REGEX MATCH "([0-9]+)\\." VERSION_MATCH "${REDHAT_RELEASE_CONTENT}")
            if(VERSION_MATCH)
                string(REGEX REPLACE "([0-9]+)\\." "\\1" DISTRO_VERSION_ID "${VERSION_MATCH}")
            endif()
            
        # 方法3：检查 /etc/lsb-release（Ubuntu）
        elseif(EXISTS "/etc/lsb-release")
            file(READ "/etc/lsb-release" LSB_RELEASE_CONTENT)
            if(LSB_RELEASE_CONTENT MATCHES "Ubuntu")
                set(DISTRO_ID "ubuntu")
            endif()
            
            # 提取版本号
            string(REGEX MATCH "DISTRIB_RELEASE=([^\n]*)" VERSION_MATCH "${LSB_RELEASE_CONTENT}")
            if(VERSION_MATCH)
                string(REGEX REPLACE "DISTRIB_RELEASE=([^\n]*)" "\\1" DISTRO_VERSION_ID "${VERSION_MATCH}")
            endif()
            
        # 方法4：检查 /etc/debian_version（Debian系）
        elseif(EXISTS "/etc/debian_version")
            set(DISTRO_ID "debian")
            file(READ "/etc/debian_version" DEBIAN_VERSION)
            string(STRIP "${DEBIAN_VERSION}" DISTRO_VERSION_ID)
        endif()
        
        # 如果仍然未检测到，设置默认值
        if(DISTRO_ID STREQUAL "unknown")
            set(DISTRO_ID "centos")
            set(DISTRO_VERSION_ID "7")
            message(WARNING "无法检测Linux发行版，使用默认值：CentOS 7")
        endif()
        
    else ()
        # 使用CMake 3.22+的内置功能
        message(STATUS "使用CMake内置功能检测Linux发行版...")
        
        cmake_host_system_information(RESULT PRETTY_NAME QUERY DISTRIB_PRETTY_NAME)
        message(STATUS "系统描述: ${PRETTY_NAME}")

        cmake_host_system_information(RESULT DISTRO QUERY DISTRIB_INFO)
        
        # 输出所有可用的发行版信息（调试用）
        foreach (VAR IN LISTS DISTRO)
            message(STATUS "${VAR}=`${${VAR}}`")
        endforeach ()
        
        # 尝试获取DISTRIB_ID
        if(DEFINED DISTRIB_ID)
            string(TOLOWER "${DISTRIB_ID}" DISTRO_ID)
        endif()
        
        # 尝试获取版本信息
        if(DEFINED DISTRIB_VERSION_ID)
            set(DISTRO_VERSION_ID "${DISTRIB_VERSION_ID}")
        elseif(DEFINED DISTRIB_RELEASE)
            set(DISTRO_VERSION_ID "${DISTRIB_RELEASE}")
        endif()

        # 如果CMake内置方法失败，回退到文件检测
        if(NOT DEFINED DISTRO_ID)
            message(STATUS "CMake内置检测失败，回退到文件检测方法...")

            if(EXISTS "/etc/os-release")
                file(READ "/etc/os-release" OS_RELEASE_CONTENT)
                string(REGEX MATCH "ID=\"?([^\"\n]*)\"?" ID_MATCH "${OS_RELEASE_CONTENT}")
                if(ID_MATCH)
                    string(REGEX REPLACE "ID=\"?([^\"]*)\"?" "\\1" DISTRO_ID "${ID_MATCH}")
                    string(TOLOWER "${DISTRO_ID}" DISTRO_ID)
                endif()
            endif()
        endif()
    endif ()
    
    # 标准化发行版名称
    if(DISTRO_ID MATCHES "rhel|redhat")
        set(DISTRO_ID "centos")  # 将RHEL也当作CentOS处理
    elseif(DISTRO_ID MATCHES "debian")
        # Debian系统可能需要特殊处理，但这里先按Ubuntu处理
        if(NOT DISTRO_ID STREQUAL "ubuntu")
            message(STATUS "检测到Debian系统，按Ubuntu配置处理")
        endif()
    endif()
    
    # 输出最终检测结果
    message(STATUS "=== Linux发行版检测结果 ===")
    message(STATUS "发行版ID: ${DISTRO_ID}")
    message(STATUS "版本号: ${DISTRO_VERSION_ID}")
    message(STATUS "==============================")
    
    #---------------------------------------------------------------------------------------
    # 依赖库检测和自动安装
    #---------------------------------------------------------------------------------------
    message(STATUS "=== 开始检测必要的依赖库 ===")
    
    # 添加用户配置选项
    option(AUTO_INSTALL_DEPS "Automatically install missing dependencies (requires sudo)" ON)
    option(SKIP_DEPENDENCY_CHECK "Skip dependency checking entirely" OFF)
    
    if(SKIP_DEPENDENCY_CHECK)
        message(STATUS "跳过依赖检测 (用户设置 SKIP_DEPENDENCY_CHECK=ON)")
        message(STATUS "=== 依赖库检测已跳过 ===")
    else()
        # 检查sudo权限
        set(HAS_SUDO FALSE)
        if(AUTO_INSTALL_DEPS)
            execute_process(
                COMMAND sudo -n true
                RESULT_VARIABLE SUDO_CHECK_RESULT
                OUTPUT_QUIET
                ERROR_QUIET
            )
            
            if(SUDO_CHECK_RESULT EQUAL 0)
                set(HAS_SUDO TRUE)
                message(STATUS "检测到sudo权限，可以进行自动安装")
            else()
                message(STATUS "未检测到sudo权限或sudo不可用")
                message(STATUS "将提供手动安装指导")
            endif()
        endif()
        
        # 检测krb5
        message(STATUS "检测 krb5 依赖...")
        find_path(KRB5_INCLUDE_DIR
            NAMES krb5.h
            PATHS 
                /usr/include
                /usr/local/include
                /usr/include/krb5
                /usr/local/include/krb5
                $ENV{HOME}/.local/include
                $ENV{HOME}/.local/include/krb5
            NO_DEFAULT_PATH
        )
        
        find_library(KRB5_LIBRARY
            NAMES krb5 libkrb5
            PATHS
                /usr/lib
                /usr/local/lib
                /usr/lib64
                /usr/local/lib64
                /usr/lib/x86_64-linux-gnu
                $ENV{HOME}/.local/lib
                $ENV{HOME}/.local/lib64
            NO_DEFAULT_PATH
        )
        
        if(NOT KRB5_INCLUDE_DIR OR NOT KRB5_LIBRARY)
            message(STATUS "krb5 未找到")
            
            if(AUTO_INSTALL_DEPS AND HAS_SUDO)
                message(STATUS "准备使用sudo自动安装krb5...")
                
                if (DISTRO_ID STREQUAL "centos" OR DISTRO_ID STREQUAL "rhel")
                    message(STATUS "使用 yum 安装 krb5 开发包...")
                    execute_process(
                        COMMAND sudo yum install -y krb5-devel
                        RESULT_VARIABLE KRB5_INSTALL_RESULT
                        OUTPUT_VARIABLE KRB5_INSTALL_OUTPUT
                        ERROR_VARIABLE KRB5_INSTALL_ERROR
                    )
                    
                    if(KRB5_INSTALL_RESULT EQUAL 0)
                        message(STATUS "krb5-devel 安装成功")
                    else()
                        message(WARNING "krb5-devel 安装失败: ${KRB5_INSTALL_ERROR}")
                    endif()
                    
                elseif (DISTRO_ID STREQUAL "ubuntu" OR DISTRO_ID STREQUAL "debian")
                    message(STATUS "使用 apt 安装 krb5 开发包...")
                    execute_process(
                        COMMAND sudo apt-get update
                        RESULT_VARIABLE APT_UPDATE_RESULT
                        OUTPUT_QUIET
                        ERROR_QUIET
                    )
                    
                    execute_process(
                        COMMAND sudo apt-get install -y libkrb5-dev
                        RESULT_VARIABLE KRB5_INSTALL_RESULT
                        OUTPUT_VARIABLE KRB5_INSTALL_OUTPUT
                        ERROR_VARIABLE KRB5_INSTALL_ERROR
                    )
                    
                    if(KRB5_INSTALL_RESULT EQUAL 0)
                        message(STATUS "libkrb5-dev 安装成功")
                    else()
                        message(WARNING "libkrb5-dev 安装失败: ${KRB5_INSTALL_ERROR}")
                    endif()
                endif()
                
                # 重新检测krb5
                find_path(KRB5_INCLUDE_DIR
                    NAMES krb5.h
                    PATHS 
                        /usr/include
                        /usr/local/include
                        /usr/include/krb5
                        /usr/local/include/krb5
                    NO_DEFAULT_PATH
                )
                
                find_library(KRB5_LIBRARY
                    NAMES krb5 libkrb5
                    PATHS
                        /usr/lib
                        /usr/local/lib
                        /usr/lib64
                        /usr/local/lib64
                        /usr/lib/x86_64-linux-gnu
                    NO_DEFAULT_PATH
                )
            else()
                message(STATUS "")
                message(STATUS "=== krb5 手动安装指导 ===")
                if (DISTRO_ID STREQUAL "centos" OR DISTRO_ID STREQUAL "rhel")
                    message(STATUS "请运行以下命令安装krb5开发包:")
                    message(STATUS "  sudo yum install -y krb5-devel")
                    message(STATUS "或者如果没有sudo权限，请联系系统管理员安装")
                elseif (DISTRO_ID STREQUAL "ubuntu" OR DISTRO_ID STREQUAL "debian")
                    message(STATUS "请运行以下命令安装krb5开发包:")
                    message(STATUS "  sudo apt-get update")
                    message(STATUS "  sudo apt-get install -y libkrb5-dev")
                    message(STATUS "或者如果没有sudo权限，请联系系统管理员安装")
                endif()
                message(STATUS "")
                message(STATUS "非root用户替代方案:")
                message(STATUS "1. 从源码编译安装到 ~/.local 目录")
                message(STATUS "2. 使用包管理器的用户模式 (如 conda, homebrew)")
                message(STATUS "3. 设置 CMAKE_PREFIX_PATH 指向已安装的krb5位置")
                message(STATUS "========================")
            endif()
        endif()
        
        # 检测tirpc
        message(STATUS "检测 tirpc 依赖...")
        find_path(TIRPC_INCLUDE_DIR
            NAMES rpc/rpc.h netinet/in.h
            PATHS 
                /usr/include/tirpc
                /usr/local/include/tirpc
                /usr/include
                /usr/local/include
                $ENV{HOME}/.local/include/tirpc
                $ENV{HOME}/.local/include
            NO_DEFAULT_PATH
        )
        
        find_library(TIRPC_LIBRARY
            NAMES tirpc libtirpc
            PATHS
                /usr/lib
                /usr/local/lib
                /usr/lib64
                /usr/local/lib64
                /usr/lib/x86_64-linux-gnu
                $ENV{HOME}/.local/lib
                $ENV{HOME}/.local/lib64
            NO_DEFAULT_PATH
        )
        
        if(NOT TIRPC_INCLUDE_DIR OR NOT TIRPC_LIBRARY)
            message(STATUS "tirpc 未找到")
            
            if(AUTO_INSTALL_DEPS AND HAS_SUDO)
                message(STATUS "准备使用sudo自动安装tirpc...")
                
                if (DISTRO_ID STREQUAL "centos" OR DISTRO_ID STREQUAL "rhel")
                    message(STATUS "使用 yum 安装 tirpc 开发包...")
                    execute_process(
                        COMMAND sudo yum install -y libtirpc-devel
                        RESULT_VARIABLE TIRPC_INSTALL_RESULT
                        OUTPUT_VARIABLE TIRPC_INSTALL_OUTPUT
                        ERROR_VARIABLE TIRPC_INSTALL_ERROR
                    )
                    
                    if(TIRPC_INSTALL_RESULT EQUAL 0)
                        message(STATUS "libtirpc-devel 安装成功")
                    else()
                        message(WARNING "libtirpc-devel 安装失败: ${TIRPC_INSTALL_ERROR}")
                    endif()
                    
                elseif (DISTRO_ID STREQUAL "ubuntu" OR DISTRO_ID STREQUAL "debian")
                    message(STATUS "使用 apt 安装 tirpc 开发包...")
                    execute_process(
                        COMMAND sudo apt-get install -y libtirpc-dev
                        RESULT_VARIABLE TIRPC_INSTALL_RESULT
                        OUTPUT_VARIABLE TIRPC_INSTALL_OUTPUT
                        ERROR_VARIABLE TIRPC_INSTALL_ERROR
                    )
                    
                    if(TIRPC_INSTALL_RESULT EQUAL 0)
                        message(STATUS "libtirpc-dev 安装成功")
                    else()
                        message(WARNING "libtirpc-dev 安装失败: ${TIRPC_INSTALL_ERROR}")
                    endif()
                endif()
                
                # 重新检测tirpc
                find_path(TIRPC_INCLUDE_DIR
                    NAMES rpc/rpc.h netinet/in.h
                    PATHS 
                        /usr/include/tirpc
                        /usr/local/include/tirpc
                        /usr/include
                        /usr/local/include
                    NO_DEFAULT_PATH
                )
                
                find_library(TIRPC_LIBRARY
                    NAMES tirpc libtirpc
                    PATHS
                        /usr/lib
                        /usr/local/lib
                        /usr/lib64
                        /usr/local/lib64
                        /usr/lib/x86_64-linux-gnu
                    NO_DEFAULT_PATH
                )
            else()
                message(STATUS "")
                message(STATUS "=== tirpc 手动安装指导 ===")
                if (DISTRO_ID STREQUAL "centos" OR DISTRO_ID STREQUAL "rhel")
                    message(STATUS "请运行以下命令安装tirpc开发包:")
                    message(STATUS "  sudo yum install -y libtirpc-devel")
                    message(STATUS "或者如果没有sudo权限，请联系系统管理员安装")
                elseif (DISTRO_ID STREQUAL "ubuntu" OR DISTRO_ID STREQUAL "debian")
                    message(STATUS "请运行以下命令安装tirpc开发包:")
                    message(STATUS "  sudo apt-get install -y libtirpc-dev")
                    message(STATUS "或者如果没有sudo权限，请联系系统管理员安装")
                endif()
                message(STATUS "")
                message(STATUS "非root用户替代方案:")
                message(STATUS "1. 从源码编译安装到 ~/.local 目录")
                message(STATUS "2. 使用包管理器的用户模式")
                message(STATUS "3. 设置 CMAKE_PREFIX_PATH 指向已安装的tirpc位置")
                message(STATUS "========================")
            endif()
        endif()
        
        # 最终检测结果
        if(KRB5_INCLUDE_DIR AND KRB5_LIBRARY)
            message(STATUS "krb5 已找到:")
            message(STATUS "  包含目录: ${KRB5_INCLUDE_DIR}")
            message(STATUS "  库文件: ${KRB5_LIBRARY}")
        else()
            message(WARNING "krb5 仍未找到，编译可能会失败")
            message(STATUS "可以使用 -DSKIP_DEPENDENCY_CHECK=ON 跳过检测")
        endif()
        
        if(TIRPC_INCLUDE_DIR AND TIRPC_LIBRARY)
            message(STATUS "tirpc 已找到:")
            message(STATUS "  包含目录: ${TIRPC_INCLUDE_DIR}")
            message(STATUS "  库文件: ${TIRPC_LIBRARY}")
        else()
            message(WARNING "tirpc 仍未找到，编译可能会失败")
            message(STATUS "可以使用 -DSKIP_DEPENDENCY_CHECK=ON 跳过检测")
        endif()
        
        message(STATUS "=== 依赖库检测完成 ===")
        message(STATUS "")
        message(STATUS "CMake配置选项:")
        message(STATUS "  -DAUTO_INSTALL_DEPS=OFF     禁用自动安装")
        message(STATUS "  -DSKIP_DEPENDENCY_CHECK=ON  跳过依赖检测")
        message(STATUS "  -DCMAKE_PREFIX_PATH=path    指定依赖库安装路径")
    endif()
endif ()

if (WIN32)
    if (MSVC)
        add_compile_options(/wd4819)
        add_compile_options(/wd4828)
        add_compile_options(/wd4244)
        add_compile_options(/wd4267)
        add_compile_options(/wd4099)
        add_compile_options(/wd4005)
        add_compile_options(/wd4200)
        add_compile_options(/wd4065)
        add_compile_options(/wd4091)
        add_compile_options(/wd4251)
        add_compile_options(/wd4996)
        add_compile_options(/wd4275)
        add_compile_options(/wd4316)
        add_compile_options(/wd4244)
        add_compile_options(/wd4267)
        add_compile_options(/wd4244)
        add_compile_options(/wd4099)
        add_compile_options(/wd4018)
        add_compile_options(/wd4834)
        add_compile_options(/wd4065)
        add_compile_options(/wd4311)
        add_compile_options(/wd4309)
        add_compile_options(/wd4013)
        add_compile_options(/wd4024)
        add_compile_options(/wd4477)
        add_compile_options(/wd4133)
        add_compile_options(/wd4047)
        add_compile_options(/wd4099)
        add_compile_options(/wd4217)
        add_compile_options(/wd4286)
        add_compile_options(/wd4291)
        add_compile_options(/wd4099)
        add_compile_options(/wd4100)
    endif()
endif ()

message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE} ")
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_BUILD_TYPE "Release")
    if (UNIX)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fPIC -g -O3 -Wall")
        add_definitions(
                -DNDEBUG
                -Wcomment
                -DNF_STATIC_PLUGIN
                -DEVPP_HTTP_CLIENT_SUPPORTS_SSL   #evpp http client ssl
                -DEVPP_HTTP_SERVER_SUPPORTS_SSL   #evpp http server ssl
                -DH_HAVE_CAMERON314_CONCURRENTQUEUE  #evpp need
                -DLUA_COMPAT_5_1
                -DLUA_USE_LINUX
                -fPIC
        )
    else (WIN32)
        set(CMAKE_CXX_FLAGS_RELEASE "/MT /bigobj")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi /O2 -DNDEBUG -DWIN32_LEAN_AND_MEAN -D_WINSOCK_DEPRECATED_NO_WARNINGS -D_LIB -D_WIN32 -D_CONSOLE -DNF_NONCLIENT_BUILD -DUNICODE -D_UNICODE -D_WINDOWS -DWIN32 -DNF_STATIC_PLUGIN -DLUA_COMPAT_5_1 -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_USRDLL -DDLL_EXPORTS -DMYSQLPP_MAKING_DLL -DHAVE_MYSQL_SSL_SET  -DBUILDING_LIBCURL -DCURL_STATICLIB -D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS -DNOMINMAX ")
    endif ()
elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_BUILD_TYPE "Debug")
    if (UNIX)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -Wall -g3 -ggdb -fPIC")
        add_definitions(
                -D_DEBUG
                -Wcomment
                -DNF_STATIC_PLUGIN
                -DEVPP_HTTP_CLIENT_SUPPORTS_SSL   #evpp http client ssl
                -DEVPP_HTTP_SERVER_SUPPORTS_SSL   #evpp http server ssl
                -DH_HAVE_CAMERON314_CONCURRENTQUEUE  #evpp need
                -DLUA_COMPAT_5_1
                -DLUA_USE_LINUX
                -fPIC
        )
    else (WIN32)
        if (MSVC)
            set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Ob0 /Od /RTC1 /DEBUG /MTd /bigobj")
        endif()
        #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCOMPILE_CHECK_STRING_FORMAT -DWIN32_LEAN_AND_MEAN -D_WINSOCK_DEPRECATED_NO_WARNINGS -D_LIB -D_WIN32 -D_CONSOLE -DNF_NONCLIENT_BUILD -DUNICODE -D_UNICODE -D_WINDOWS -DWIN32 -D_DEBUG -DNF_STATIC_PLUGIN -DLUA_COMPAT_5_1 -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_USRDLL -DDLL_EXPORTS -DMYSQLPP_MAKING_DLL -DHAVE_MYSQL_SSL_SET  -DBUILDING_LIBCURL -DCURL_STATICLIB -D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS -DNOMINMAX")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32_LEAN_AND_MEAN -D_WINSOCK_DEPRECATED_NO_WARNINGS -D_LIB -D_WIN32 -D_CONSOLE -DNF_NONCLIENT_BUILD -DUNICODE -D_UNICODE -D_WINDOWS -DWIN32 -D_DEBUG -DNF_STATIC_PLUGIN -DLUA_COMPAT_5_1 -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_USRDLL -DDLL_EXPORTS -DMYSQLPP_MAKING_DLL -DHAVE_MYSQL_SSL_SET  -DBUILDING_LIBCURL -DCURL_STATICLIB -D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS -DNOMINMAX")
    endif ()
elseif (CMAKE_BUILD_TYPE STREQUAL "DynamicDebug")
    set(CMAKE_BUILD_TYPE "DynamicDebug")
    if (UNIX)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
        #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O0 -Wall -g3 -ggdb -fPIC -Wextra -Wno-unused-parameter -Wno-conversion-null -Wno-unused-but-set-variable -Wno-int-to-pointer-cast -Wno-array-bounds -static-libstdc++")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -O0 -Wall -g3 -ggdb -fPIC -static-libstdc++")
        add_definitions(
                -D_DEBUG
                -DCHECK_MEM_LEAK
                -Wcomment
                -DLUA_COMPAT_5_1
                -DLUA_USE_LINUX
                -DNF_DYNAMIC_PLUGIN
                -DEVPP_HTTP_CLIENT_SUPPORTS_SSL   #evpp http client ssl
                -DEVPP_HTTP_SERVER_SUPPORTS_SSL   #evpp http server ssl
                -DH_HAVE_CAMERON314_CONCURRENTQUEUE  #evpp need
                -fPIC
                -Wno-error
                -Wpointer-arith
                #-DCOMPILE_CHECK_STRING_FORMAT
        )


    else (WIN32)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32_LEAN_AND_MEAN -D_WINSOCK_DEPRECATED_NO_WARNINGS -D_LIB -D_WIN32 -D_CONSOLE -DNF_NONCLIENT_BUILD -DUNICODE -D_UNICODE -D_WINDOWS -DWIN32 -D_DEBUG -DNF_STATIC_PLUGIN -DLUA_COMPAT_5_1 -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_USRDLL -DDLL_EXPORTS -DMYSQLPP_MAKING_DLL -DHAVE_MYSQL_SSL_SET  -DBUILDING_LIBCURL -DCURL_STATICLIB ")
    endif ()
elseif (CMAKE_BUILD_TYPE STREQUAL "DynamicRelease")
    set(CMAKE_BUILD_TYPE "DynamicRelease")
    if (UNIX)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
        #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fPIC -std=c++11 -O3 -g -Wall")
        #don't use -O3 用-O3来编译所有的软件包将产生更大体积更耗内存的二进制文件，
        #大大增加编译失败的机会或不可预知的程序行为（包括错误）。 这样做将得不偿失，记住过犹不及。 在gcc 4.x.中使用-O3是不推荐的
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -fPIC -static-libstdc++")
        add_definitions(
                -DNDEBUG
                -Wcomment
                -DLUA_COMPAT_5_1
                -DLUA_USE_LINUX
                -DNF_DYNAMIC_PLUGIN
                -DEVPP_HTTP_CLIENT_SUPPORTS_SSL   #evpp http client ssl
                -DEVPP_HTTP_SERVER_SUPPORTS_SSL   #evpp http server ssl
                -DH_HAVE_CAMERON314_CONCURRENTQUEUE  #evpp need
                -fPIC
                -Wno-error
                -Wpointer-arith
        )

    else (WIN32)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32_LEAN_AND_MEAN -D_WINSOCK_DEPRECATED_NO_WARNINGS -D_LIB -D_WIN32 -D_CONSOLE -DNF_NONCLIENT_BUILD -DUNICODE -D_UNICODE -D_WINDOWS -DWIN32 -D_DEBUG -DNF_STATIC_PLUGIN -DLUA_COMPAT_5_1 -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_USRDLL -DDLL_EXPORTS -DMYSQLPP_MAKING_DLL -DHAVE_MYSQL_SSL_SET  -DBUILDING_LIBCURL -DCURL_STATICLIB ")
    endif ()
endif ()

message("PROJECT_SOURCE_DIR = ${PROJECT_SOURCE_DIR}")
set(CMAKE_NFSHM_SOURCE_DIR ${PROJECT_SOURCE_DIR})
message("CMAKE_NFSHM_SOURCE_DIR = ${CMAKE_NFSHM_SOURCE_DIR}")
message("CMAKE_C_FLAGS = ${CMAKE_C_FLAGS}")
message("CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")

message("CONTRCT_VERSION_FILE = ${CONTRCT_VERSION_FILE}")
message("CONTRCT_VERSION_FILE2 = ${CONTRCT_VERSION_FILE2}")

if (UNIX)
    SET(INCLUDE_DIR "/usr/local/include"
            "/usr/include/tirpc"
            "/usr/local/include/tirpc"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/LuaBind/lua"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/common/"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/mysql/include"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/mysql/linux"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/Theron/Include"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/zdb"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/libevent/include"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/libevent/linux/include"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/zookeeper/include"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/common/hiredis/hiredis_linux"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/nanopb"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/OpenXLSX"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/enet/include"
            "${CMAKE_NFSHM_SOURCE_DIR}/src"
            "${CMAKE_NFSHM_SOURCE_DIR}/src/NFrame"
            "${CMAKE_NFSHM_SOURCE_DIR}/src/NFServer"
            "${CMAKE_NFSHM_SOURCE_DIR}/src/NFrame/NFComm/NFKernelMessage"
            "${CMAKE_NFSHM_SOURCE_DIR}/src/NFrame/NFComm/NFMessageDefine"
            "${CMAKE_NFSHM_SOURCE_DIR}/src/NFServer/NFServerComm/NFServerMessage")
else (WIN32)
    SET(INCLUDE_DIR "/usr/local/include"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/LuaBind/lua"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/common/"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/mysql/include"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/mysql/win"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/Theron/Include"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/zdb"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/libevent/include"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/libevent/win/include"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/zookeeper/include"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/common/hiredis/hiredis_win"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/nanopb"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/OpenXLSX"
            "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/enet/include"
            "${CMAKE_NFSHM_SOURCE_DIR}/src"
            "${CMAKE_NFSHM_SOURCE_DIR}/src/NFrame"
            "${CMAKE_NFSHM_SOURCE_DIR}/src/NFServer"
            "${CMAKE_NFSHM_SOURCE_DIR}/src/NFrame/NFComm/NFKernelMessage"
            "${CMAKE_NFSHM_SOURCE_DIR}/src/NFrame/NFComm/NFMessageDefine"
            "${CMAKE_NFSHM_SOURCE_DIR}/src/NFServer/NFServerComm/NFServerMessage")
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    if (UNIX)
        SET(LIB_DIR "/usr/lib"
                "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/lib64_release"
                "${CMAKE_NFSHM_SOURCE_DIR}/Build/Lib/Release")

        SET(LIB_LINK_DEP resolv dl rt pthread tirpc libprotobuf.a libCommon.a libLuaBind.a libNFMessageDefine.a libNFCore.a libNFPluginModule.a)

        SET(LIB_LINK_NF libNFKernelPlugin.a NFTestPlugin.a)
    else (WIN32)
        SET(LIB_DIR "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/lib64_release"
                "${CMAKE_NFSHM_SOURCE_DIR}/Build/Lib/Release"
                "${CMAKE_NFSHM_SOURCE_DIR}/Build/Lib/Release/Release")
    endif ()
elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (UNIX)
        SET(LIB_DIR "/usr/lib"
                "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/lib64_debug"
                "${CMAKE_NFSHM_SOURCE_DIR}/Build/Lib/Debug")

        SET(LIB_LINK_DEP resolv dl rt pthread tirpc libprotobuf.a libCommon.a libLuaBind.a libNFMessageDefine.a libNFCore.a libNFPluginModule.a)

        SET(LIB_LINK_NF libNFKernelPlugin.a NFTestPlugin.a)
    else (WIN32)
        SET(LIB_DIR "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/lib64_debug"
                "${CMAKE_NFSHM_SOURCE_DIR}/Build/Lib/Debug"
                "${CMAKE_NFSHM_SOURCE_DIR}/Build/Lib/Debug/Debug")
    endif ()
elseif (CMAKE_BUILD_TYPE STREQUAL "DynamicRelease")
    if (UNIX)
        message("DISTRO_ID: ${DISTRO_ID} ")
        message("DISTRO_VERSION_ID: ${DISTRO_VERSION_ID} ")

        if (DISTRO_ID STREQUAL "centos")
            message("DISTRO_ID: ${DISTRO_ID} ")
            message("DISTRO_VERSION_ID: ${DISTRO_VERSION_ID} ")
            SET(LIB_DIR "/usr/lib"
                    "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/lib64_release"
                    "${CMAKE_NFSHM_SOURCE_DIR}/Build/Lib/Release"
                    "${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Dynamic_Release"
            )
        elseif (DISTRO_ID STREQUAL "ubuntu")
            SET(LIB_DIR "/usr/lib"
                    "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/lib64_release/ubuntu"
                    "${CMAKE_NFSHM_SOURCE_DIR}/Build/Lib/Release"
                    "${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Dynamic_Release"
            )
        endif ()

        message(STATUS "=== DynamicRelease 库路径配置 ===")
        message(STATUS "目标发行版: ${DISTRO_ID}")
        message(STATUS "发行版版本: ${DISTRO_VERSION_ID}")

        if (DISTRO_ID STREQUAL "centos" OR DISTRO_ID STREQUAL "rhel")
            message(STATUS "配置CentOS/RHEL特定库路径")
            SET(LIB_DIR "/usr/lib"
                    "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/lib64_release"
                    "${CMAKE_NFSHM_SOURCE_DIR}/Build/Lib/Release"
                    "${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Dynamic_Release"
            )
        elseif (DISTRO_ID STREQUAL "ubuntu" OR DISTRO_ID STREQUAL "debian")
            message(STATUS "配置Ubuntu/Debian特定库路径")
            SET(LIB_DIR "/usr/lib"
                    "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/lib64_release/ubuntu"
                    "${CMAKE_NFSHM_SOURCE_DIR}/Build/Lib/Release"
                    "${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Dynamic_Release"
            )
        else()
            message(WARNING "未识别的发行版 '${DISTRO_ID}'，使用默认CentOS配置")
            SET(LIB_DIR "/usr/lib"
                    "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/lib64_release"
                    "${CMAKE_NFSHM_SOURCE_DIR}/Build/Lib/Release"
                    "${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Dynamic_Release"
            )
        endif ()

        SET(LIB_LINK_DEP resolv dl rt pthread tirpc)
        message(STATUS "库目录: ${LIB_DIR}")
    endif ()
elseif (CMAKE_BUILD_TYPE STREQUAL "DynamicDebug")
    if (UNIX)
        message("DISTRO_ID: ${DISTRO_ID} ")
        message("DISTRO_VERSION_ID: ${DISTRO_VERSION_ID} ")

        if (DISTRO_ID STREQUAL "centos")
            SET(LIB_DIR "/usr/lib"
                    "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/lib64_debug"
                    "${CMAKE_NFSHM_SOURCE_DIR}/Build/Lib/Debug"
                    "${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Dynamic_Debug"
            )
        elseif (DISTRO_ID STREQUAL "ubuntu")
            SET(LIB_DIR "/usr/lib"
                    "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/lib64_debug/ubuntu"
                    "${CMAKE_NFSHM_SOURCE_DIR}/Build/Lib/Debug"
                    "${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Dynamic_Debug"
            )
        endif ()

        message(STATUS "=== DynamicDebug 库路径配置 ===")
        message(STATUS "目标发行版: ${DISTRO_ID}")
        message(STATUS "发行版版本: ${DISTRO_VERSION_ID}")

        if (DISTRO_ID STREQUAL "centos" OR DISTRO_ID STREQUAL "rhel")
            message(STATUS "配置CentOS/RHEL特定库路径")
            SET(LIB_DIR "/usr/lib"
                    "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/lib64_debug"
                    "${CMAKE_NFSHM_SOURCE_DIR}/Build/Lib/Debug"
                    "${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Dynamic_Debug"
            )
        elseif (DISTRO_ID STREQUAL "ubuntu" OR DISTRO_ID STREQUAL "debian")
            message(STATUS "配置Ubuntu/Debian特定库路径")
            SET(LIB_DIR "/usr/lib"
                    "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/lib64_debug/ubuntu"
                    "${CMAKE_NFSHM_SOURCE_DIR}/Build/Lib/Debug"
                    "${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Dynamic_Debug"
            )
        else()
            message(WARNING "未识别的发行版 '${DISTRO_ID}'，使用默认CentOS配置")
            SET(LIB_DIR "/usr/lib"
                    "${CMAKE_NFSHM_SOURCE_DIR}/thirdparty/lib64_debug"
                    "${CMAKE_NFSHM_SOURCE_DIR}/Build/Lib/Debug"
                    "${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Dynamic_Debug"
            )
        endif ()

        message(STATUS "库目录: ${LIB_DIR}")
        SET(LIB_LINK_DEP resolv dl rt pthread tirpc)
    endif ()
endif ()

include_directories(${INCLUDE_DIR})

link_directories(${LIB_DIR})

if (CMAKE_BUILD_TOOLS)
    ADD_SUBDIRECTORY("src/NFTools")
else()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    ADD_SUBDIRECTORY("thirdparty")
    ADD_SUBDIRECTORY("src")
    ADD_SUBDIRECTORY("src/NFrame/NFPluginLoader")
    ADD_SUBDIRECTORY("src/NFrame/NFrameStatic")
    ADD_SUBDIRECTORY("src/NFServer/NFServerStatic")
    #ADD_SUBDIRECTORY("game")
elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
    ADD_SUBDIRECTORY("thirdparty")
    ADD_SUBDIRECTORY("src")
    ADD_SUBDIRECTORY("src/NFrame/NFPluginLoader")
    ADD_SUBDIRECTORY("src/NFrame/NFrameStatic")
    ADD_SUBDIRECTORY("src/NFServer/NFServerStatic")
    ADD_SUBDIRECTORY("src/NFTest/main")
    #ADD_SUBDIRECTORY("game")
elseif (CMAKE_BUILD_TYPE STREQUAL "DynamicRelease")
    ADD_SUBDIRECTORY("thirdparty")
    ADD_SUBDIRECTORY("src")
    ADD_SUBDIRECTORY("src/NFrame/NFPluginLoader")
    #ADD_SUBDIRECTORY("game")
    ADD_SUBDIRECTORY("src/NFTest/main")
elseif (CMAKE_BUILD_TYPE STREQUAL "DynamicDebug")
    ADD_SUBDIRECTORY("thirdparty")
    ADD_SUBDIRECTORY("src")
    ADD_SUBDIRECTORY("src/NFrame/NFPluginLoader")
    ADD_SUBDIRECTORY("src/NFTest/main")
    #ADD_SUBDIRECTORY("game")
endif ()

endif()
