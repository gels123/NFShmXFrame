// -------------------------------------------------------------------------
//    @FileName         :    NFCPluginManager.h
//    @Author           :    LvSheng.Huang
//    @Date             :   2022-09-18
//    @Module           :    NFCPluginManager
//
// -------------------------------------------------------------------------

#include "NFCDynLib.h"
#include "NFComm/NFCore/NFFileUtility.h"
#include "NFComm/NFPluginModule/NFCheck.h"
#include "NFComm/NFCore/NFRandom.hpp"
#include "NFComm/NFCore/NFCommon.h"

/**
 * @brief 加载动态库
 *
 * 该函数用于加载指定的动态库文件。首先构造动态库的完整路径，然后尝试加载该库。
 * 如果加载成功，在非Windows平台且非调试模式下，会检查库文件是否存在，若不存在则记录错误并退出程序。
 * 最后，记录加载日志并返回加载结果。
 *
 * @return bool 返回动态库是否加载成功，true表示成功，false表示失败。
 */
bool NFCDynLib::Load()
{
    // 构造动态库的完整路径，默认路径为当前目录
    std::string strLibPath = "./";
    strLibPath += m_strName;

    // 尝试加载动态库
    m_inst = DYNLIB_LOAD(strLibPath.c_str());
    if (m_inst != nullptr)
    {
#if NF_PLATFORM != NF_PLATFORM_WIN
#ifndef NF_DEBUG_MODE
        // 在非Windows平台且非调试模式下，检查库文件是否存在
        if (!NFFileUtility::IsFileExist(strLibPath))
        {
            // 如果库文件不存在，记录错误日志并退出程序
            NFLogError(NF_LOG_DEFAULT, 0, "can't find the plugin file");
            NFSLEEP(1000);
            exit(0);
        }
#endif
#endif
    }

    // 记录加载动态库的日志
    NFLogWarning(NF_LOG_DEFAULT, 0, "LoadPlugin:{}", m_strName);

    // 返回动态库是否加载成功
    return m_inst != nullptr;
}

/**
 * @brief 卸载动态链接库
 *
 * 该函数用于卸载当前加载的动态链接库。根据不同的平台（Windows 或 非Windows），
 * 调用不同的卸载函数，并记录卸载结果日志。
 *
 * @return bool 始终返回 true，表示卸载操作已执行（无论成功与否）。
 */
bool NFCDynLib::UnLoad()
{
#if NF_PLATFORM == NF_PLATFORM_WIN
    // Windows 平台下卸载动态链接库
    DYNLIB_UNLOAD(m_inst);
    // 记录卸载日志
    NFLogWarning(NF_LOG_DEFAULT, 0, "UnLoadPlugin:{}", m_strName);
#else
    // 非Windows 平台下卸载动态链接库，并检查是否成功
    int succ = DYNLIB_UNLOAD(m_inst);
    if (succ)
    {
        // 卸载失败时记录错误日志
        NFLogWarning(NF_LOG_DEFAULT, 0, "UnLoadPlugin:{} failed, error={}", m_strName, dlerror());
    }
    else
    {
        // 卸载成功时记录日志
        NFLogWarning(NF_LOG_DEFAULT, 0, "UnLoadPlugin:{}", m_strName);
    }
#endif
    return true;
}


void* NFCDynLib::GetSymbol(const char* szProcName)
{
    return reinterpret_cast<DYNLIB_HANDLE>(DYNLIB_GETSYM(m_inst, szProcName));
}


