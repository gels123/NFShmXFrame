// -------------------------------------------------------------------------
//    @FileName         :    NFCAppInited.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFPluginManager
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFPluginModule/NFServerDefine.h"
#include "NFComm/NFPluginModule/NFBaseObj.h"

/**
 * @class NFCAppInitTask
 * @brief 该类用于表示NFC应用程序初始化任务的状态和相关信息。
 *
 * 该类包含任务类型、任务完成状态以及任务描述等成员变量。
 */
class NFCAppInitTask
{
public:
    /**
     * @brief 默认构造函数，初始化任务类型为APP_INIT_NONE，任务完成状态为false。
     */
    NFCAppInitTask() : m_taskType(APP_INIT_NONE), m_finished(false)
    {
    }

public:
    uint32_t m_taskType; ///< 任务类型，用于标识不同的初始化任务。
    bool m_finished; ///< 任务完成状态，true表示任务已完成，false表示任务未完成。
    std::string m_desc; ///< 任务描述，用于记录任务的详细信息或说明。
};


/**
 * @class NFCAppInitTaskGroup
 * @brief 该类用于管理一组NFC应用初始化任务。
 *
 * 该类包含一个任务列表和一个标志位，用于指示所有任务是否已完成。
 */
class NFCAppInitTaskGroup
{
public:
    /**
     * @brief 构造函数，初始化任务组。
     *
     * 构造函数初始化 `m_finishAllFlag` 为 `false`，表示任务组尚未完成。
     */
    NFCAppInitTaskGroup()
    {
        m_finishAllFlag = false;
    }

    std::vector<NFCAppInitTask> m_taskList; ///< 存储NFC应用初始化任务的列表。
    bool m_finishAllFlag; ///< 标志位，表示所有任务是否已完成。
};

/**
 * @class NFCServerAppInitTaskGroup
 * @brief 该类用于管理NFC服务器应用程序初始化任务组。
 *
 * 该类包含一个任务组列表和一个初始化状态标志。构造函数负责初始化任务组列表和设置初始化状态。
 */
class NFCServerAppInitTaskGroup
{
public:
    /**
     * @brief 构造函数，初始化任务组列表和初始化状态。
     *
     * 构造函数将任务组列表的大小设置为APP_INIT_MAX_TASK_GROUP，并将初始化状态标志m_isInited设置为false。
     */
    NFCServerAppInitTaskGroup()
    {
        // 初始化任务组列表，大小为APP_INIT_MAX_TASK_GROUP
        m_taskGroupList.resize(APP_INIT_MAX_TASK_GROUP);
        // 设置初始化状态为false
        m_isInited = false;
    }

    std::vector<NFCAppInitTaskGroup> m_taskGroupList; // 任务组列表，存储NFCAppInitTaskGroup对象
    bool m_isInited; // 初始化状态标志，表示任务组是否已初始化
};

/**
 * @class NFCAppInited
 * @brief 该类用于管理应用程序的初始化任务，继承自NFObject。
 *
 * 该类负责管理不同服务器类型的初始化任务组，并提供任务注册、完成检查等功能。
 */
class NFCAppInited final : public NFBaseObj
{
public:
    /**
     * @brief 构造函数，初始化任务组和任务标志。
     * @param pPluginManager 插件管理器指针，用于传递给基类NFObject。
     *
     * 该构造函数初始化了任务组和任务标志，并设置初始时间。
     */
    explicit NFCAppInited(NFIPluginManager* pPluginManager) : NFBaseObj(pPluginManager)
    {
        // 初始化服务器任务组，并设置NF_ST_NONE类型的任务组为已初始化状态
        m_serverTaskGroup.resize(NF_ST_MAX);
        m_serverTaskGroup[NF_ST_NONE].m_isInited = true;

        // 将NF_ST_NONE类型的任务组中的所有任务标记为已完成
        for (int i = 0; i < static_cast<int>(m_serverTaskGroup[NF_ST_NONE].m_taskGroupList.size()); i++)
        {
            m_serverTaskGroup[NF_ST_NONE].m_taskGroupList[i].m_finishAllFlag = true;
        }

        // 初始化任务组标志，并设置为未完成状态
        m_taskGroupFlag.resize(APP_INIT_MAX_TASK_GROUP);
        for (int i = 0; i < static_cast<int>(m_taskGroupFlag.size()); i++)
        {
            m_taskGroupFlag[i] = false;
        }

        // 记录当前时间
        m_lastTime = NFGetSecondTime();
    }

    /**
     * @brief 析构函数，无特殊操作。
     */
    ~NFCAppInited() override = default;

    /**
     * @brief 执行初始化任务。
     * @return 返回执行结果，通常为成功或错误码。
     */
    int Execute();

    /**
     * @brief 注册应用程序任务。
     * @param eServerType 服务器类型。
     * @param taskType 任务类型。
     * @param desc 任务描述。
     * @param taskGroup 任务组，默认为APP_INIT_TASK_GROUP_SERVER_CONNECT。
     * @return 返回注册结果，通常为成功或错误码。
     */
    int RegisterAppTask(NF_SERVER_TYPE eServerType, uint32_t taskType, const std::string& desc, uint32_t taskGroup = APP_INIT_TASK_GROUP_SERVER_CONNECT);

    /**
     * @brief 标记应用程序任务为已完成。
     * @param eServerType 服务器类型。
     * @param taskType 任务类型。
     * @param taskGroup 任务组，默认为APP_INIT_TASK_GROUP_SERVER_CONNECT。
     * @return 返回标记结果，通常为成功或错误码。
     */
    int FinishAppTask(NF_SERVER_TYPE eServerType, uint32_t taskType, uint32_t taskGroup = APP_INIT_TASK_GROUP_SERVER_CONNECT);

    /**
     * @brief 检查所有任务是否已完成。
     * @return 返回检查结果，通常为成功或错误码。
     */
    int CheckTaskFinished();

    /**
     * @brief 判断初始化任务是否已完成。
     * @return 返回布尔值，表示初始化任务是否已完成。
     */
    bool IsInitTasked() const;

    /**
     * @brief 打印超时信息。
     */
    void PrintTimeout();

    /**
     * @brief 判断指定服务器类型是否已初始化。
     * @param eServerType 服务器类型。
     * @return 返回布尔值，表示指定服务器类型是否已初始化。
     */
    bool IsInited(NF_SERVER_TYPE eServerType) const;

    /**
     * @brief 判断指定服务器类型的任务组是否已完成。
     * @param eServerType 服务器类型。
     * @param taskGroup 任务组。
     * @return 返回布尔值，表示指定任务组是否已完成。
     */
    bool IsFinishAppTask(NF_SERVER_TYPE eServerType, uint32_t taskGroup) const;

    /**
     * @brief 判断指定服务器类型的任务组是否存在。
     * @param eServerType 服务器类型。
     * @param taskGroup 任务组。
     * @return 返回布尔值，表示指定任务组是否存在。
     */
    bool IsHasAppTask(NF_SERVER_TYPE eServerType, uint32_t taskGroup) const;

    /**
     * @brief 判断指定服务器类型的任务组中是否存在指定类型的任务。
     * @param eServerType 服务器类型。
     * @param taskGroup 任务组。
     * @param taskType 任务类型。
     * @return 返回布尔值，表示指定任务组中是否存在指定类型的任务。
     */
    bool IsHasAppTask(NF_SERVER_TYPE eServerType, uint32_t taskGroup, uint32_t taskType) const;

private:
    std::vector<NFCServerAppInitTaskGroup> m_serverTaskGroup; ///< 服务器任务组列表
    std::vector<bool> m_taskGroupFlag; ///< 任务组完成标志列表
    uint64_t m_lastTime; ///< 上次记录的时间
};

