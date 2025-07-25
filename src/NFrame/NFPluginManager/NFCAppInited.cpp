// -------------------------------------------------------------------------
//    @FileName         :    NFCAppInited.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFPluginManager
//
// -------------------------------------------------------------------------

#include "NFCAppInited.h"
#include "NFComm/NFPluginModule/NFCheck.h"
#include "NFComm/NFKernelMessage/FrameMsg.pb.h"
#include "NFComm/NFPluginModule/NFIEventModule.h"
#include "NFComm/NFPluginModule/NFIConfigModule.h"

/**
 * @brief 注册应用程序任务
 *
 * 该函数用于将指定的任务注册到应用程序的任务列表中。任务可以根据服务器类型、任务类型、任务描述和任务组进行分类。
 *
 * @param eServerType 服务器类型，用于指定任务所属的服务器
 * @param taskType 任务类型，用于标识任务的类型
 * @param desc 任务描述，提供任务的简要说明
 * @param taskGroup 任务组，用于将任务分组管理
 * @return int 返回0表示成功，返回-1表示失败
 */
int NFCAppInited::RegisterAppTask(NF_SERVER_TYPE eServerType, uint32_t taskType, const std::string& desc, uint32_t taskGroup)
{
    // 检查服务器类型和任务组是否有效
    CHECK_EXPR(static_cast<uint32_t>(eServerType) < static_cast<uint32_t>(m_serverTaskGroup.size()), -1, "serverType:{} taskType:{} desc:{} taskGroup:{}", (int)eServerType, taskType, desc, taskGroup);
    CHECK_EXPR(taskGroup < static_cast<uint32_t>(m_serverTaskGroup[eServerType].m_taskGroupList.size()), -1, "serverType:{} taskType:{} desc:{} taskGroup:{}", (int)eServerType, taskType, desc, taskGroup);

    // 创建任务对象并设置任务类型和描述
    NFCAppInitTask task;
    task.m_taskType = taskType;
    task.m_desc = desc;

    // 根据服务器类型将任务添加到相应的任务组中
    if (eServerType == NF_ST_NONE)
    {
        for (int i = NF_ST_NONE + 1; i < static_cast<int>(m_serverTaskGroup.size()); i++)
        {
            m_serverTaskGroup[i].m_taskGroupList[taskGroup].m_taskList.push_back(task);
        }
    }
    else
    {
        m_serverTaskGroup[eServerType].m_taskGroupList[taskGroup].m_taskList.push_back(task);
    }

    return 0;
}

/**
 * @brief 完成应用程序任务
 *
 * 该函数用于标记指定任务为已完成状态。如果任务未完成，则将其标记为完成，并记录日志。
 *
 * @param eServerType 服务器类型，用于指定任务所属的服务器
 * @param taskType 任务类型，用于标识任务的类型
 * @param taskGroup 任务组，用于指定任务所属的任务组
 * @return int 返回0表示成功，返回-1表示失败
 */
int NFCAppInited::FinishAppTask(NF_SERVER_TYPE eServerType, uint32_t taskType, uint32_t taskGroup)
{
    // 检查服务器类型和任务组是否有效
    CHECK_EXPR(static_cast<uint32_t>(eServerType) < static_cast<uint32_t>(m_serverTaskGroup.size()), -1, "serverType:{} taskType:{} taskGroup:{}", eServerType, taskType, taskGroup);
    CHECK_EXPR(taskGroup < static_cast<uint32_t>(m_serverTaskGroup[eServerType].m_taskGroupList.size()), -1, "serverType:{} taskType:{} taskGroup:{}", eServerType, taskType, taskGroup);

    bool flag = false;
    // 根据服务器类型查找并标记任务为完成状态
    if (eServerType == NF_ST_NONE)
    {
        for (int j = 1; j < static_cast<int>(m_serverTaskGroup.size()); j++)
        {
            for (int i = 0; i < static_cast<int>(m_serverTaskGroup[j].m_taskGroupList[taskGroup].m_taskList.size()); i++)
            {
                NFCAppInitTask& task = m_serverTaskGroup[j].m_taskGroupList[taskGroup].m_taskList[i];
                if (task.m_taskType == taskType)
                {
                    flag = true;
                    if (task.m_finished == false)
                    {
                        task.m_finished = true;
                        NFLogInfo(NF_LOG_DEFAULT, 0, "Finish App Init Task, serverType:{}  desc:{}", j, task.m_desc);
                    }
                }
            }
        }
    }
    else
    {
        for (int i = 0; i < static_cast<int>(m_serverTaskGroup[eServerType].m_taskGroupList[taskGroup].m_taskList.size()); i++)
        {
            NFCAppInitTask& task = m_serverTaskGroup[eServerType].m_taskGroupList[taskGroup].m_taskList[i];
            if (task.m_taskType == taskType)
            {
                flag = true;
                if (task.m_finished == false)
                {
                    task.m_finished = true;
                    NFLogInfo(NF_LOG_DEFAULT, 0, "Finish App Init Task, serverType:{}  desc:{}", eServerType, task.m_desc);
                }
            }
        }
    }

    // 如果未找到任务，记录错误日志
    if (flag == false)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "Not Find the App Init Task, serverType:{} taskGroup:{} taskType:{} ", eServerType, taskGroup, taskType);
    }

    return 0;
}

/**
 * @brief 检查任务是否完成
 *
 * 该函数用于检查所有任务组中的任务是否已完成。如果所有任务都已完成，则触发相应的事件。
 *
 * @return int 返回0表示成功
 */
int NFCAppInited::CheckTaskFinished()
{
    // 遍历所有任务组，检查任务是否完成
    for (int taskGroupType = APP_INIT_TASK_GROUP_NONE; taskGroupType < static_cast<int>(m_taskGroupFlag.size()); taskGroupType++)
    {
        if (!m_taskGroupFlag[taskGroupType])
        {
            m_taskGroupFlag[taskGroupType] = true;
            for (int serverType =  NF_ST_NONE + 1; serverType < static_cast<int>(m_serverTaskGroup.size()); serverType++)
            {
                if (m_serverTaskGroup[serverType].m_taskGroupList[taskGroupType].m_finishAllFlag == false)
                {
                    m_serverTaskGroup[serverType].m_taskGroupList[taskGroupType].m_finishAllFlag = true;
                    for (int task = 0; task < static_cast<int>(m_serverTaskGroup[serverType].m_taskGroupList[taskGroupType].m_taskList.size()); task++)
                    {
                        if (m_serverTaskGroup[serverType].m_taskGroupList[taskGroupType].m_taskList[task].m_finished == false)
                        {
                            m_taskGroupFlag[taskGroupType] = false;
                            m_serverTaskGroup[serverType].m_taskGroupList[taskGroupType].m_finishAllFlag = false;
                            break;
                        }
                    }

                    // 如果任务组中的所有任务都已完成，触发事件
                    if (m_serverTaskGroup[serverType].m_taskGroupList[taskGroupType].m_finishAllFlag)
                    {
                        NFrame::NFEventNoneData event;
                        FindModule<NFIEventModule>()->FireExecute(static_cast<NF_SERVER_TYPE>(serverType), NFrame::NF_EVENT_SERVER_TASK_GROUP_FINISH, NFrame::NF_EVENT_SERVER_TYPE, taskGroupType, event);
                        // 根据任务组类型执行相应的操作
                        if (taskGroupType == APP_INIT_TASK_GROUP_SERVER_CONNECT)
                        {
                            NFLogInfo(NF_LOG_DEFAULT, 0, "App Finish {} Server Connect Task ..............", NF_SERVER_TYPE_name(static_cast<NF_SERVER_TYPE>(serverType)));
                            m_pObjPluginManager->AfterAllConnectFinish(static_cast<NF_SERVER_TYPE>(serverType));
                            if (m_serverTaskGroup[serverType].m_taskGroupList[APP_INIT_TASK_GROUP_SERVER_LOAD_DESC_STORE].m_finishAllFlag)
                            {
                                m_pObjPluginManager->AfterAllConnectAndAllDescStore(static_cast<NF_SERVER_TYPE>(serverType));
                            }
                        }
                        else if (taskGroupType == APP_INIT_TASK_GROUP_SERVER_LOAD_DESC_STORE)
                        {
                            NFLogInfo(NF_LOG_DEFAULT, 0, "App Finish {} Server Load Store Task ..............", NF_SERVER_TYPE_name(static_cast<NF_SERVER_TYPE>(serverType)));
                            m_pObjPluginManager->AfterAllDescStoreLoaded(static_cast<NF_SERVER_TYPE>(serverType));
                            if (m_serverTaskGroup[serverType].m_taskGroupList[APP_INIT_TASK_GROUP_SERVER_CONNECT].m_finishAllFlag)
                            {
                                m_pObjPluginManager->AfterAllConnectAndAllDescStore(static_cast<NF_SERVER_TYPE>(serverType));
                            }
                        }
                        else if (taskGroupType == APP_INIT_TASK_GROUP_SERVER_LOAD_OBJ_FROM_DB)
                        {
                            NFLogInfo(NF_LOG_DEFAULT, 0, "App Finish {} Server Load Obj Task ..............", NF_SERVER_TYPE_name(static_cast<NF_SERVER_TYPE>(serverType)));
                            m_pObjPluginManager->AfterObjFromDBLoaded(static_cast<NF_SERVER_TYPE>(serverType));
                        }
                        else if (taskGroupType == APP_INIT_TASK_GROUP_SERVER_REGISTER)
                        {
                            NFLogInfo(NF_LOG_DEFAULT, 0, "App Finish {} Server Register Task ..............", NF_SERVER_TYPE_name(static_cast<NF_SERVER_TYPE>(serverType)));
                            m_pObjPluginManager->AfterServerRegisterFinish(static_cast<NF_SERVER_TYPE>(serverType));
                        }
                    }
                }
            }

            // 如果所有任务组中的任务都已完成，触发相应的事件
            if (m_taskGroupFlag[taskGroupType])
            {
                NFrame::NFEventNoneData event;
                FindModule<NFIEventModule>()->FireExecute(NF_ST_NONE, NFrame::NF_EVENT_SERVER_TASK_GROUP_FINISH, NFrame::NF_EVENT_SERVER_TYPE, taskGroupType, event);

                // 根据任务组类型执行相应的操作
                if (taskGroupType == APP_INIT_TASK_GROUP_SERVER_CONNECT)
                {
                    NFLogInfo(NF_LOG_DEFAULT, 0, "App Finish All Server Connect Task ..............");
                    m_pObjPluginManager->AfterAllConnectFinish();
                    if (m_taskGroupFlag[APP_INIT_TASK_GROUP_SERVER_LOAD_DESC_STORE])
                    {
                        m_pObjPluginManager->AfterAllConnectAndAllDescStore();
                    }
                }
                else if (taskGroupType == APP_INIT_TASK_GROUP_SERVER_LOAD_DESC_STORE)
                {
                    NFLogInfo(NF_LOG_DEFAULT, 0, "App Finish All Server Load Store Task ..............");
                    m_pObjPluginManager->AfterAllDescStoreLoaded();
                    if (m_taskGroupFlag[APP_INIT_TASK_GROUP_SERVER_CONNECT])
                    {
                        m_pObjPluginManager->AfterAllConnectAndAllDescStore();
                    }
                }
                else if (taskGroupType == APP_INIT_TASK_GROUP_SERVER_LOAD_OBJ_FROM_DB)
                {
                    NFLogInfo(NF_LOG_DEFAULT, 0, "App Finish All Server Load Obj Task ..............");
                    m_pObjPluginManager->AfterObjFromDBLoaded();
                }
                else if (taskGroupType == APP_INIT_TASK_GROUP_SERVER_REGISTER)
                {
                    NFLogInfo(NF_LOG_DEFAULT, 0, "App Finish All Server Register Task ..............");
                    m_pObjPluginManager->AfterServerRegisterFinish();
                }
            }
        }
    }

    for (int serverType = NF_ST_NONE + 1; serverType < static_cast<int>(m_serverTaskGroup.size()); serverType++)
    {
        if (!m_serverTaskGroup[serverType].m_isInited)
        {
            bool flag = true;
            for (int taskGroupType = APP_INIT_TASK_GROUP_NONE; taskGroupType < static_cast<int>(m_serverTaskGroup[serverType].m_taskGroupList.size()); taskGroupType++)
            {
                if (!m_serverTaskGroup[serverType].m_taskGroupList[taskGroupType].m_finishAllFlag)
                {
                    flag = false;
                    break;
                }
            }

            if (flag)
            {
                m_serverTaskGroup[serverType].m_isInited = true;
                NFrame::NFEventNoneData event;
                FindModule<NFIEventModule>()->FireExecute(static_cast<NF_SERVER_TYPE>(serverType), NFrame::NF_EVENT_SERVER_APP_FINISH_INITED, NFrame::NF_EVENT_SERVER_TYPE, 0, event);

                if (m_pObjPluginManager->IsLoadAllServer())
                {
                    NFLogInfo(NF_LOG_DEFAULT, 0, "Server:{} Finish All Task, App Inited Success..............", GetServerName(static_cast<NF_SERVER_TYPE>(serverType)));
                }
            }
        }
    }

    if (IsInitTasked())
    {
        m_pObjPluginManager->SetIsInited(true);
        NFLogInfo(NF_LOG_DEFAULT, 0, "App Finish All Task, App Inited Success..............");

        NFrame::NFEventNoneData event;
        FindModule<NFIEventModule>()->FireExecute(NF_ST_NONE, NFrame::NF_EVENT_SERVER_APP_FINISH_INITED, NFrame::NF_EVENT_SERVER_TYPE, 0, event);

        m_pObjPluginManager->AfterAppInitFinish();
    }

    return 0;
}

/**
 * @brief 检查所有任务组是否已经初始化完成
 *
 * 该函数遍历 `m_taskGroupFlag` 容器，检查每个任务组的初始化标志。
 * 如果所有任务组的标志都为 `true`，则返回 `true`，表示所有任务组已经初始化完成；
 * 否则返回 `false`，表示至少有一个任务组未初始化完成。
 *
 * @return bool 返回 `true` 表示所有任务组已初始化，`false` 表示至少有一个任务组未初始化。
 */
bool NFCAppInited::IsInitTasked() const
{
    bool flag = true;

    // 遍历任务组标志容器，检查是否有未初始化的任务组
    for (int i = 1; i < static_cast<int>(m_taskGroupFlag.size()); i++)
    {
        if (m_taskGroupFlag[i] == false)
        {
            flag = false;
            break;
        }
    }

    return flag;
}

/**
 * @brief 检查指定服务器类型是否已经初始化
 * @param eServerType 服务器类型
 * @return 如果服务器类型有效且已初始化，返回true；否则返回false
 */
bool NFCAppInited::IsInited(NF_SERVER_TYPE eServerType) const
{
    // 检查服务器类型是否在有效范围内
    CHECK_EXPR(static_cast<uint32_t>(eServerType) < static_cast<uint32_t>(m_serverTaskGroup.size()), false, "");
    // 返回指定服务器类型的初始化状态
    return m_serverTaskGroup[eServerType].m_isInited;
}

/**
 * @brief 检查指定服务器类型的任务组是否已完成所有任务
 * @param eServerType 服务器类型
 * @param taskGroup 任务组ID
 * @return 如果服务器类型和任务组有效且所有任务已完成，返回true；否则返回false
 */
bool NFCAppInited::IsFinishAppTask(NF_SERVER_TYPE eServerType, uint32_t taskGroup) const
{
    // 检查服务器类型和任务组是否在有效范围内
    CHECK_EXPR(static_cast<uint32_t>(eServerType) < static_cast<uint32_t>(m_serverTaskGroup.size()), false, "serverType:{} taskGroup:{}", eServerType, taskGroup);
    CHECK_EXPR(taskGroup < static_cast<uint32_t>(m_serverTaskGroup[eServerType].m_taskGroupList.size()), false, "serverType:{} taskGroup:{}", eServerType, taskGroup);

    // 返回指定任务组的所有任务完成标志
    return m_serverTaskGroup[eServerType].m_taskGroupList[taskGroup].m_finishAllFlag;
}

/**
 * @brief 检查指定服务器类型的任务组是否有任务
 * @param eServerType 服务器类型
 * @param taskGroup 任务组ID
 * @return 如果服务器类型和任务组有效且任务组中有任务，返回true；否则返回false
 */
bool NFCAppInited::IsHasAppTask(NF_SERVER_TYPE eServerType, uint32_t taskGroup) const
{
    // 检查服务器类型和任务组是否在有效范围内
    CHECK_EXPR(static_cast<uint32_t>(eServerType) < static_cast<uint32_t>(m_serverTaskGroup.size()), false, "serverType:{} taskGroup:{}", eServerType, taskGroup);
    CHECK_EXPR(taskGroup < static_cast<uint32_t>(m_serverTaskGroup[eServerType].m_taskGroupList.size()), false, "serverType:{} taskGroup:{}", eServerType, taskGroup);

    // 返回任务组中是否有任务
    return m_serverTaskGroup[eServerType].m_taskGroupList[taskGroup].m_taskList.size() > 0;
}

/**
 * @brief 检查指定服务器类型的任务组中是否有指定类型的任务
 * @param eServerType 服务器类型
 * @param taskGroup 任务组ID
 * @param taskType 任务类型
 * @return 如果服务器类型、任务组和任务类型有效且任务组中有指定类型的任务，返回true；否则返回false
 */
bool NFCAppInited::IsHasAppTask(NF_SERVER_TYPE eServerType, uint32_t taskGroup, uint32_t taskType) const
{
    // 检查服务器类型和任务组是否在有效范围内
    CHECK_EXPR(static_cast<uint32_t>(eServerType) < static_cast<uint32_t>(m_serverTaskGroup.size()), false, "serverType:{} taskGroup:{}", eServerType, taskGroup);
    CHECK_EXPR(taskGroup < static_cast<uint32_t>(m_serverTaskGroup[eServerType].m_taskGroupList.size()), false, "serverType:{} taskGroup:{}", eServerType, taskGroup);

    // 遍历任务组中的任务列表，查找指定类型的任务
    auto pTaskList = &m_serverTaskGroup[eServerType].m_taskGroupList[taskGroup].m_taskList;
    for (auto iter = pTaskList->begin(); iter != pTaskList->end(); ++iter)
    {
        if (iter->m_taskType == taskType)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief 打印超时未完成的任务信息
 */
void NFCAppInited::PrintTimeout()
{
    // 如果距离上次打印时间不足30秒，则直接返回
    if (NFGetSecondTime() - m_lastTime < 30)
    {
        return;
    }

    // 更新上次打印时间
    m_lastTime = NFGetSecondTime();

    // 遍历所有服务器类型、任务组和任务，打印未完成的任务信息
    for (int i = 0; i < static_cast<int>(m_serverTaskGroup.size()); i++)
    {
        if (!m_serverTaskGroup[i].m_isInited)
        {
            for (int j = 0; j < static_cast<int>(m_serverTaskGroup[i].m_taskGroupList.size()); j++)
            {
                if (!m_serverTaskGroup[i].m_taskGroupList[j].m_finishAllFlag)
                {
                    for (int x = 0; x < static_cast<int>(m_serverTaskGroup[i].m_taskGroupList[j].m_taskList.size()); x++)
                    {
                        if (m_serverTaskGroup[i].m_taskGroupList[j].m_taskList[x].m_finished == false)
                        {
                            NFLogError(NF_LOG_DEFAULT, 0, "App Init Task:{} not finish", m_serverTaskGroup[i].m_taskGroupList[j].m_taskList[x].m_desc);
                        }
                    }
                }
            }
        }
    }
}

/**
 * @brief 执行初始化任务检查
 * @return 始终返回0
 */
int NFCAppInited::Execute()
{
    // 如果未初始化任务，则检查任务完成状态并打印超时信息
    if (!IsInitTasked())
    {
        CheckTaskFinished();
        PrintTimeout();
    }
    return 0;
}
