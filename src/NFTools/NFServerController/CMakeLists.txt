CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(NFServerController)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 为所有目标设置静态运行时库（MTd/MT）
if(WIN32 AND MSVC)
    # 使用静态运行时库 /MT (Release) 和 /MTd (Debug)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
    
    # 确保CMake使用静态运行时库
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    
    message(STATUS "Using static runtime library: /MT (Release) /MTd (Debug)")
endif()

# 构建选项
option(BUILD_QT_GUI "Build Qt GUI version" ON)
#set(BUILD_QT_GUI OFF)

# 打印构建信息
message(STATUS "Building NFServerController CLI version")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "BUILD_QT_GUI: ${BUILD_QT_GUI}")
if (WIN32 AND BUILD_QT_GUI)
    message(STATUS "Qt5_DIR: ${Qt5_DIR}")
endif()

# 检查Qt是否可用（如果要求构建Qt版本）
if (WIN32 AND BUILD_QT_GUI)
    # 设置Qt5路径
    set(Qt5_DIR "C:/Qt/Qt5/5.12.10/msvc2017_64/lib/cmake/Qt5")
    
    # 检查Qt是否为静态编译版本
    if(EXISTS "C:/Qt/Qt5/5.12.10/msvc2017_64/plugins/platforms/qwindows.lib")
        set(QT_STATIC_BUILD ON)
        add_definitions(-DQT_STATIC_BUILD)
        message(STATUS "Detected Qt static build, using static linking")
    else()
        set(QT_STATIC_BUILD OFF)
        message(STATUS "Detected Qt dynamic build, using dynamic Qt with static runtime")
    endif()
    
    find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
    
    # 启用Qt的MOC
    set(CMAKE_AUTOMOC ON)
    
    # 添加Qt5头文件路径
    include_directories("C:/Qt/Qt5/5.12.10/msvc2017_64/include")
    include_directories("C:/Qt/Qt5/5.12.10/msvc2017_64/include/QtCore")
    include_directories("C:/Qt/Qt5/5.12.10/msvc2017_64/include/QtWidgets")
    include_directories("C:/Qt/Qt5/5.12.10/msvc2017_64/include/QtGui")
    
    # 静态运行时库已在全局设置中配置
    
    message(STATUS "Building Qt GUI version with static linking")
    message(STATUS "Qt5 version: ${Qt5_VERSION}")
    message(STATUS "Qt5 include path: C:/Qt/Qt5/5.12.10/msvc2017_64/include")
    message(STATUS "Using static runtime library: /MT (Release) /MTd (Debug)")
else()
    message(STATUS "Building Command Line version")
endif()

# 定义源文件
set(COMMON_SOURCES
    NFServerController.cpp
    NFServerController.h
)

# 命令行版本 (总是构建)
set(CLI_SOURCES
    ${COMMON_SOURCES}
    main.cpp
)

AUX_SOURCE_DIRECTORY(${CMAKE_NFSHM_SOURCE_DIR}/src/NFrame/NFComm/NFCore CLI_SOURCES)

# Qt版本源文件
if (WIN32 AND BUILD_QT_GUI)
    set(QT_SOURCES
        ${COMMON_SOURCES}
        NFServerControllerQt.cpp
        NFServerControllerQt.h
        main_qt.cpp
    )

    AUX_SOURCE_DIRECTORY(${CMAKE_NFSHM_SOURCE_DIR}/src/NFrame/NFComm/NFCore QT_SOURCES)
endif()

# 创建命令行版本可执行文件
ADD_EXECUTABLE(${PROJECT_NAME} ${CLI_SOURCES})

# 命令行版本的链接库设置
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    if(UNIX)
        TARGET_LINK_LIBRARIES(${PROJECT_NAME} resolv dl rt pthread)
    else(WIN32)
        # 使用静态运行时库，不再链接动态的msvcrt.lib
        TARGET_LINK_LIBRARIES(${PROJECT_NAME} libcmt.lib ws2_32.lib version.lib netapi32.lib Dbghelp.lib)
    endif()
    SET_TARGET_PROPERTIES(${PROJECT_NAME}
        PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Release"
    )
    if(UNIX)
        ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND mv ${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Release/${PROJECT_NAME} ${CMAKE_NFSHM_SOURCE_DIR}/tools/${PROJECT_NAME}
        )
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(UNIX)
        TARGET_LINK_LIBRARIES(${PROJECT_NAME} resolv dl rt pthread)
    else(WIN32)
        # 使用静态运行时库，不再链接动态的msvcrtd.lib
        TARGET_LINK_LIBRARIES(${PROJECT_NAME} libcmtd.lib ws2_32.lib version.lib netapi32.lib Dbghelp.lib)
    endif()
    SET_TARGET_PROPERTIES(${PROJECT_NAME}
        PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Debug"
    )
elseif (CMAKE_BUILD_TYPE STREQUAL "DynamicRelease")
    if(UNIX)
        TARGET_LINK_LIBRARIES(${PROJECT_NAME} resolv dl rt pthread)
    else(WIN32)
        # 使用静态运行时库
        TARGET_LINK_LIBRARIES(${PROJECT_NAME} libcmt.lib ws2_32.lib version.lib netapi32.lib Dbghelp.lib)
    endif()
    message("CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
    
    SET_TARGET_PROPERTIES(${PROJECT_NAME}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Dynamic_Release"
    )
    
    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND mv ${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Dynamic_Release/${PROJECT_NAME} ${CMAKE_NFSHM_SOURCE_DIR}/tools/${PROJECT_NAME}
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "DynamicDebug")
    if(UNIX)
        TARGET_LINK_LIBRARIES(${PROJECT_NAME} resolv dl rt pthread)
    else(WIN32)
        # 使用静态运行时库
        TARGET_LINK_LIBRARIES(${PROJECT_NAME} libcmtd.lib ws2_32.lib version.lib netapi32.lib Dbghelp.lib)
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    SET_TARGET_PROPERTIES(${PROJECT_NAME} 
        PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Dynamic_Debug"
    )
endif()

# Qt GUI版本 (仅在启用选项时构建)
if (WIN32 AND BUILD_QT_GUI)
    # 创建Qt版本可执行文件
    add_executable(NFServerControllerQt ${QT_SOURCES})
    
    # 添加Qt库路径
    link_directories("C:/Qt/Qt5/5.12.10/msvc2017_64/lib")
    
    # 设置Qt静态链接属性
    set_target_properties(NFServerControllerQt PROPERTIES
        WIN32_EXECUTABLE TRUE
        AUTOMOC TRUE
    )
    
    # 链接Qt静态库和必要的Windows库
    target_link_libraries(NFServerControllerQt 
        Qt5::Core 
        Qt5::Widgets
    )
    
    # Windows库链接配置
    if(WIN32 AND MSVC)
        if(QT_STATIC_BUILD)
            # 静态Qt构建：链接所有必要的Windows库和Qt插件
            target_link_libraries(NFServerControllerQt
                # Qt静态库依赖的Windows库
                user32.lib
                gdi32.lib
                winspool.lib
                comdlg32.lib
                advapi32.lib
                shell32.lib
                ole32.lib
                oleaut32.lib
                uuid.lib
                odbc32.lib
                odbccp32.lib
                winmm.lib
                imm32.lib
                ws2_32.lib
                version.lib
                netapi32.lib
                Dbghelp.lib
                # Qt5平台插件需要的库
                dwmapi.lib
                uxtheme.lib
                # Qt5静态插件库
                "C:/Qt/Qt5/5.12.10/msvc2017_64/plugins/platforms/qwindows.lib"
                "C:/Qt/Qt5/5.12.10/msvc2017_64/plugins/styles/qwindowsvistastyle.lib"
            )
        else()
            # 动态Qt构建：只链接基本的Windows库，使用静态运行时库
            target_link_libraries(NFServerControllerQt
                user32.lib
                shell32.lib
                ws2_32.lib
                version.lib
                netapi32.lib
                Dbghelp.lib
            )
        endif()
    endif()
    
    # Qt版本的输出目录设置
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        if(UNIX)
            target_link_libraries(NFServerControllerQt resolv dl rt pthread)
        else(WIN32)
            # 使用静态运行时库，不再链接动态的msvcrt.lib
            target_link_libraries(NFServerControllerQt libcmt.lib)
        endif()
        SET_TARGET_PROPERTIES(NFServerControllerQt
            PROPERTIES 
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Release"
        )
        if(UNIX)
            ADD_CUSTOM_COMMAND(TARGET NFServerControllerQt POST_BUILD
                COMMAND mv ${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Release/NFServerControllerQt ${CMAKE_NFSHM_SOURCE_DIR}/tools/NFServerControllerQt
            )
        endif()
    elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(UNIX)
            target_link_libraries(NFServerControllerQt resolv dl rt pthread)
        else(WIN32)
            # 使用静态运行时库，不再链接动态的msvcrtd.lib
            target_link_libraries(NFServerControllerQt libcmtd.lib)
        endif()
        SET_TARGET_PROPERTIES(NFServerControllerQt
            PROPERTIES 
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Debug"
        )
    elseif (CMAKE_BUILD_TYPE STREQUAL "DynamicRelease")
        if(UNIX)
            target_link_libraries(NFServerControllerQt resolv dl rt pthread)
        else(WIN32)
            # 使用静态运行时库
            target_link_libraries(NFServerControllerQt libcmt.lib)
        endif()
        SET_TARGET_PROPERTIES(NFServerControllerQt
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Dynamic_Release"
        )
        ADD_CUSTOM_COMMAND(TARGET NFServerControllerQt POST_BUILD
            COMMAND mv ${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Dynamic_Release/NFServerControllerQt ${CMAKE_NFSHM_SOURCE_DIR}/tools/NFServerControllerQt
        )
    elseif(CMAKE_BUILD_TYPE STREQUAL "DynamicDebug")
        if(UNIX)
            target_link_libraries(NFServerControllerQt resolv dl rt pthread)
        else(WIN32)
            # 使用静态运行时库
            target_link_libraries(NFServerControllerQt libcmtd.lib)
        endif()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
        SET_TARGET_PROPERTIES(NFServerControllerQt
            PROPERTIES 
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Dynamic_Debug"
        )
    endif()
    
    message(STATUS "Building NFServerController Qt GUI version")
endif()






