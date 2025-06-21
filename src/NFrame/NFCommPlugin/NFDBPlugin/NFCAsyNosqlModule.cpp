// -------------------------------------------------------------------------
//    @FileName         :    NFCAsyNoSqlModule.cpp
//    @Author           :    gaoyi
//    @Date             :    23-8-15
//    @Email			:    445267987@qq.com
//    @Module           :    NFCAsyNoSqlModule
//
// -------------------------------------------------------------------------

#include "NFCAsyNosqlModule.h"
#include "NFCNosqlDriverManager.h"
#include "NFComm/NFPluginModule/NFITaskComponent.h"
#include "NFComm/NFPluginModule/NFCheck.h"

class NFNosqlTask : public NFTask
{
public:
    explicit NFNosqlTask(const std::string& serverId) : m_serverId(serverId)
    {
        m_pNosqlDriver = nullptr;
        m_taskName = GET_CLASS_NAME(NFNosqlTask);
    }

    ~NFNosqlTask() override
    {
    }

    virtual bool IsCheck()
    {
        return false;
    }

    virtual bool IsConnect()
    {
        return false;
    }

public:
    NFINosqlDriver* m_pNosqlDriver;
    std::string m_serverId;
};

class NFNosqlConnectTask final : public NFNosqlTask
{
public:
    NFNosqlConnectTask() : NFNosqlTask("")
    {
        m_taskName = GET_CLASS_NAME(NFNosqlConnectTask);
        m_iNosqlPort = 0;
    }

    ~NFNosqlConnectTask() override
    {
    }

    bool IsConnect() override
    {
        return true;
    }

    /**
    **  异步线程处理函数，将在另一个线程里运行
    */
    bool ThreadProcess() override
    {
        return true;
    }

    /**
    ** 主线程处理函数，将在线程处理完后，提交给主线程来处理，根据返回函数是否继续处理
    返回值： thread::TPTask::TPTaskState， 请参看TPTaskState
    */
    TPTaskState MainThreadProcess() override
    {
        return TPTASK_STATE_COMPLETED;
    }

public:
    std::string m_strServerId;
    std::string m_strNosqlIp;
    int m_iNosqlPort;
    std::string m_strNosqlPass;
};

class NFNosqlCheckTask final : public NFNosqlTask
{
public:
    NFNosqlCheckTask() : NFNosqlTask("")
    {
        m_taskName = GET_CLASS_NAME(NFNosqlCheckTask);
    }

    ~NFNosqlCheckTask() override
    {
    }

    bool IsCheck() override
    {
        return true;
    }

    /**
    **  异步线程处理函数，将在另一个线程里运行
    */
    bool ThreadProcess() override
    {
        return true;
    }

    /**
    ** 主线程处理函数，将在线程处理完后，提交给主线程来处理，根据返回函数是否继续处理
    返回值： thread::TPTask::TPTaskState， 请参看TPTaskState
    */
    TPTaskState MainThreadProcess() override
    {
        return TPTASK_STATE_COMPLETED;
    }
};

class NFSelectObjNosqlTask final : public NFNosqlTask
{
public:
    NFSelectObjNosqlTask(const std::string& serverId, const NFrame::storesvr_selobj& select, const SelectObjCb& cb) : NFNosqlTask(serverId)
    {
        m_balanceId = select.mod_key();
        m_stSelect = select;
        m_fCallback = cb;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFSelectObjNosqlTask);
    }

    ~NFSelectObjNosqlTask() override
    {
    }

    /**
    **  异步线程处理函数，将在另一个线程里运行
    */
    bool ThreadProcess() override
    {
        if (m_pNosqlDriver)
        {
            m_iRet = m_pNosqlDriver->SelectObj(m_stSelect, m_stSelectRes);
        }
        else
        {
            m_iRet = -1;
        }

        return true;
    }

    /**
    ** 主线程处理函数，将在线程处理完后，提交给主线程来处理，根据返回函数是否继续处理
    返回值： thread::TPTask::TPTaskState， 请参看TPTaskState
    */
    TPTaskState MainThreadProcess() override
    {
        if (m_fCallback)
        {
            m_fCallback(m_iRet, m_stSelectRes);
        }
        return TPTASK_STATE_COMPLETED;
    }

public:
    NFrame::storesvr_selobj m_stSelect;
    NFrame::storesvr_selobj_res m_stSelectRes;
    SelectObjCb m_fCallback;
    int m_iRet;
};

class NFDeleteObjNosqlTask final : public NFNosqlTask
{
public:
    NFDeleteObjNosqlTask(const std::string& serverId, const NFrame::storesvr_delobj& select, const DeleteObjCb& cb) : NFNosqlTask(serverId)
    {
        m_balanceId = select.mod_key();
        m_stSelect = select;
        m_fCallback = cb;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFDeleteObjNosqlTask) + std::string("_") + select.baseinfo().tbname();
    }

    ~NFDeleteObjNosqlTask() override
    {
    }

    /**
    **  异步线程处理函数，将在另一个线程里运行
    */
    bool ThreadProcess() override
    {
        if (m_pNosqlDriver)
        {
            m_iRet = m_pNosqlDriver->DeleteObj(m_stSelect);
        }
        return true;
    }

    /**
    ** 主线程处理函数，将在线程处理完后，提交给主线程来处理，根据返回函数是否继续处理
    返回值： thread::TPTask::TPTaskState， 请参看TPTaskState
    */
    TPTaskState MainThreadProcess() override
    {
        if (m_fCallback)
        {
            m_fCallback(m_iRet, m_stSelectRes);
        }
        return TPTASK_STATE_COMPLETED;
    }

public:
    NFrame::storesvr_delobj m_stSelect;
    NFrame::storesvr_delobj_res m_stSelectRes;
    DeleteObjCb m_fCallback;
    int m_iRet;
};

class NFInsertObjNosqlTask final : public NFNosqlTask
{
public:
    NFInsertObjNosqlTask(const std::string& serverId, const NFrame::storesvr_insertobj& select, const InsertObjCb& cb) : NFNosqlTask(serverId)
    {
        m_balanceId = select.mod_key();
        m_stSelect = select;
        m_fCallback = cb;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFInsertObjNosqlTask) + std::string("_") + select.baseinfo().tbname();
    }

    ~NFInsertObjNosqlTask() override
    {
    }

    /**
    **  异步线程处理函数，将在另一个线程里运行
    */
    bool ThreadProcess() override
    {
        if (m_pNosqlDriver)
        {
            m_iRet = m_pNosqlDriver->SaveObj(m_stSelect, m_stSelectRes);
        }
        return true;
    }

    /**
    ** 主线程处理函数，将在线程处理完后，提交给主线程来处理，根据返回函数是否继续处理
    返回值： thread::TPTask::TPTaskState， 请参看TPTaskState
    */
    TPTaskState MainThreadProcess() override
    {
        if (m_fCallback)
        {
            m_fCallback(m_iRet, m_stSelectRes);
        }
        return TPTASK_STATE_COMPLETED;
    }

public:
    NFrame::storesvr_insertobj m_stSelect;
    NFrame::storesvr_insertobj_res m_stSelectRes;
    InsertObjCb m_fCallback;
    int m_iRet;
};

class NFModifyObjNosqlTask final : public NFNosqlTask
{
public:
    NFModifyObjNosqlTask(const std::string& serverId, const NFrame::storesvr_modobj& select, const ModifyObjCb& cb) : NFNosqlTask(serverId)
    {
        m_balanceId = select.mod_key();
        m_stSelect = select;
        m_fCallback = cb;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFModifyObjNosqlTask) + std::string("_") + select.baseinfo().tbname();
    }

    ~NFModifyObjNosqlTask() override
    {
    }

    /**
    **  异步线程处理函数，将在另一个线程里运行
    */
    bool ThreadProcess() override
    {
        if (m_pNosqlDriver)
        {
            m_iRet = m_pNosqlDriver->SaveObj(m_stSelect, m_stSelectRes);
        }
        return true;
    }

    /**
    ** 主线程处理函数，将在线程处理完后，提交给主线程来处理，根据返回函数是否继续处理
    返回值： thread::TPTask::TPTaskState， 请参看TPTaskState
    */
    TPTaskState MainThreadProcess() override
    {
        if (m_fCallback)
        {
            m_fCallback(m_iRet, m_stSelectRes);
        }
        return TPTASK_STATE_COMPLETED;
    }

public:
    NFrame::storesvr_modobj m_stSelect;
    NFrame::storesvr_modobj_res m_stSelectRes;
    ModifyObjCb m_fCallback;
    int m_iRet;
};

class NFDbUpdateObjTask final : public NFNosqlTask
{
public:
    NFDbUpdateObjTask(const std::string& serverId, const NFrame::storesvr_updateobj& select, const UpdateObjCb& cb) : NFNosqlTask(serverId)
    {
        m_balanceId = select.mod_key();
        m_stSelect = select;
        m_fCallback = cb;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFDBUpdateObjTask) + std::string("_") + select.baseinfo().tbname();
    }

    ~NFDbUpdateObjTask() override
    {
    }

    /**
    **  异步线程处理函数，将在另一个线程里运行
    */
    bool ThreadProcess() override
    {
        if (m_pNosqlDriver)
        {
            m_iRet = m_pNosqlDriver->SaveObj(m_stSelect, m_strSelectRes);
        }
        return true;
    }

    /**
    ** 主线程处理函数，将在线程处理完后，提交给主线程来处理，根据返回函数是否继续处理
    返回值： thread::TPTask::TPTaskState， 请参看TPTaskState
    */
    TPTaskState MainThreadProcess() override
    {
        if (m_fCallback)
        {
            m_fCallback(m_iRet, m_strSelectRes);
        }
        return TPTASK_STATE_COMPLETED;
    }

public:
    NFrame::storesvr_updateobj m_stSelect;
    NFrame::storesvr_updateobj_res m_strSelectRes;
    UpdateObjCb m_fCallback;
    int m_iRet;
};

class NFNosqlTaskComponent final : public NFITaskComponent
{
public:
    NFNosqlTaskComponent()
    {
        m_pNoSqlDriverManager = NF_NEW NFCNosqlDriverManager();
    }

    ~NFNosqlTaskComponent() override
    {
        NF_SAFE_DELETE(m_pNoSqlDriverManager);
    }

    /**
     * @brief 处理任务启动的函数
     *
     * 该函数用于处理不同类型的任务启动逻辑，主要针对与NoSQL数据库相关的任务。
     * 根据任务类型的不同，执行相应的操作，如添加NoSQL服务器、检查NoSQL连接等。
     *
     * @param pTask 指向NFTask对象的指针，表示要处理的任务
     * @return void 无返回值
     */
    void ProcessTaskStart(NFTask* pTask) override
    {
        // 尝试将传入的任务转换为NFNosqlTask类型
        auto pMysqlTask = dynamic_cast<NFNosqlTask*>(pTask);
        if (pMysqlTask)
        {
            // 如果任务是连接任务
            if (pMysqlTask->IsConnect())
            {
                // 进一步将任务转换为NFNosqlConnectTask类型
                auto pConnectTask = dynamic_cast<NFNosqlConnectTask*>(pTask);
                if (pConnectTask == nullptr) return;

                // 尝试添加NoSQL服务器，如果失败则等待1秒后退出程序
                int iRet = m_pNoSqlDriverManager->AddNosqlServer(pConnectTask->m_strServerId, pConnectTask->m_strNosqlIp, pConnectTask->m_iNosqlPort,
                                                                 pConnectTask->m_strNosqlPass);
                if (iRet != 0)
                {
                    NFSLEEP(1000);
                    exit(0);
                }
            }
            // 如果任务是检查任务
            else if (pMysqlTask->IsCheck())
            {
                // 执行NoSQL连接的检查
                m_pNoSqlDriverManager->CheckNoSql();
            }
            // 其他任务类型
            else
            {
                // 先检查NoSQL连接，然后获取对应的NoSQL驱动
                m_pNoSqlDriverManager->CheckNoSql();
                pMysqlTask->m_pNosqlDriver = m_pNoSqlDriverManager->GetNosqlDriver(pMysqlTask->m_serverId);
                // 检查是否成功获取驱动，如果失败则记录错误信息
                CHECK_EXPR(pMysqlTask->m_pNosqlDriver, , "GetNosqlDriver:{} Failed", pMysqlTask->m_serverId);
            }
        }
    }

    void ProcessTask(NFTask* pTask) override
    {
        if (pTask)
        {
            pTask->ThreadProcess();
        }
    }

    void ProcessTaskEnd(NFTask* pTask) override
    {
        auto pMysqlTask = dynamic_cast<NFNosqlTask*>(pTask);
        if (pMysqlTask)
        {
            pMysqlTask->m_pNosqlDriver = nullptr;
        }
    }

    void HandleTaskTimeOut(const std::string& taskName, uint64_t useTime) override
    {
        NFLogError(NF_LOG_DEFAULT, 0, "taskName:{} timeOut, userTime:{}", taskName, useTime);
    }

public:
    NFCNosqlDriverManager* m_pNoSqlDriverManager;
};


NFCAsyNosqlModule::NFCAsyNosqlModule(NFIPluginManager* pPluginManager) : NFIAsyNosqlModule(pPluginManager)
{
    m_ullLastCheckTime = NFGetTime();
    m_bInitComponent = false;
}

NFCAsyNosqlModule::~NFCAsyNosqlModule()
{
}

bool NFCAsyNosqlModule::Execute()
{
    return NFIModule::Execute();
}

bool NFCAsyNosqlModule::InitActorPool(int iMaxTaskGroup, int iMaxActorNum)
{
    NFIAsyModule::InitActorPool(iMaxTaskGroup, iMaxActorNum);
    if (!m_bInitComponent)
    {
        m_bInitComponent = true;
        for (size_t i = 0; i < m_stVecActorGroupPool[NF_TASK_GROUP_DEFAULT].size(); i++)
        {
            auto pComponent = NF_NEW NFNosqlTaskComponent();
            AddActorComponent(NF_TASK_GROUP_DEFAULT, m_stVecActorGroupPool[NF_TASK_GROUP_DEFAULT][i], pComponent);
        }
    }

    return true;
}

int NFCAsyNosqlModule::AddDBServer(const std::string& strServerId, const string& strNoSqlIp, int iNosqlPort, const string& strNoSqlPass)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    InitActorPool(NF_TASK_MAX_GROUP_DEFAULT);

    for (size_t i = 0; i < m_stVecActorGroupPool[NF_TASK_GROUP_DEFAULT].size(); i++)
    {
        auto pTask = NF_NEW NFNosqlConnectTask();
        pTask->m_strServerId = strServerId;
        pTask->m_strNosqlIp = strNoSqlIp;
        pTask->m_iNosqlPort = iNosqlPort;
        pTask->m_strNosqlPass = strNoSqlPass;
        int iRet = FindModule<NFITaskModule>()->AddTask(m_stVecActorGroupPool[NF_TASK_GROUP_DEFAULT][i], pTask);
        CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCAsyNosqlModule::SelectObj(const string& strServerId, const NFrame::storesvr_selobj& stSelect, const SelectObjCb& fCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto pTask = NF_NEW NFSelectObjNosqlTask(strServerId, stSelect, fCallback);
    int iRet = AddTask(NF_TASK_GROUP_DEFAULT, pTask);
    CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCAsyNosqlModule::DeleteObj(const string& strServerId, const NFrame::storesvr_delobj& stSelect, const DeleteObjCb& fCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto pTask = NF_NEW NFDeleteObjNosqlTask(strServerId, stSelect, fCallback);
    int iRet = AddTask(NF_TASK_GROUP_DEFAULT, pTask);
    CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCAsyNosqlModule::InsertObj(const string& strServerId, const NFrame::storesvr_insertobj& stSelect, const InsertObjCb& bCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto pTask = NF_NEW NFInsertObjNosqlTask(strServerId, stSelect, bCallback);
    int iRet = AddTask(NF_TASK_GROUP_DEFAULT, pTask);
    CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCAsyNosqlModule::ModifyObj(const string& strServerId, const NFrame::storesvr_modobj& stSelect, const ModifyObjCb& fCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto pTask = NF_NEW NFModifyObjNosqlTask(strServerId, stSelect, fCallback);
    int iRet = AddTask(NF_TASK_GROUP_DEFAULT, pTask);
    CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCAsyNosqlModule::UpdateObj(const string& strServerId, const NFrame::storesvr_updateobj& stSelect, const UpdateObjCb& fCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto pTask = NF_NEW NFDbUpdateObjTask(strServerId, stSelect, fCallback);
    int iRet = AddTask(NF_TASK_GROUP_DEFAULT, pTask);
    CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}
