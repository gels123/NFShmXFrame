// -------------------------------------------------------------------------
//    @FileName         :    NFCAsyMysqlModule.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Module           :    NFCAsyMysqlModule
//
// -------------------------------------------------------------------------

#include "NFCAsyMysqlModule.h"

#include "NFCMysqlDriverManager.h"
#include "NFComm/NFPluginModule/NFCheck.h"
#include "NFComm/NFPluginModule/NFIPluginManager.h"
#include "NFComm/NFPluginModule/NFTask.h"
#include "NFComm/NFPluginModule/NFITaskComponent.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"

/**
 * @class NFMysqlTask
 * @brief 继承自NFTask的MySQL任务类，用于处理与MySQL相关的任务。
 */
class NFMysqlTask : public NFTask
{
public:
    /**
     * @brief 构造函数，初始化NFMysqlTask对象。
     * @param strServerId 服务器ID，用于标识与MySQL服务器相关的任务。
     */
    explicit NFMysqlTask(const std::string& strServerId): m_strServerId(strServerId)
    {
        m_pMysqlDriver = nullptr; // 初始化MySQL驱动指针为空
        m_taskName = GET_CLASS_NAME(NFMysqlTask); // 设置任务名称为类名
    }

    /**
     * @brief 析构函数，清理NFMysqlTask对象。
     */
    ~NFMysqlTask() override
    {
    }

    /**
     * @brief 检查任务是否需要执行检查操作。
     * @return 返回false，表示默认不执行检查操作。
     */
    virtual bool IsCheck()
    {
        return false;
    }

    /**
     * @brief 检查任务是否需要连接MySQL服务器。
     * @return 返回false，表示默认不执行连接操作。
     */
    virtual bool IsConnect()
    {
        return false;
    }

public:
    NFCMysqlDriver* m_pMysqlDriver; // MySQL驱动指针，用于操作MySQL数据库
    std::string m_strServerId; // 服务器ID，标识与MySQL服务器相关的任务
};

/**
 * @class NFMysqlConnectTask
 * @brief 该类继承自NFMysqlTask，用于处理MySQL连接任务。
 */
class NFMysqlConnectTask final : public NFMysqlTask
{
public:
    /**
     * @brief 构造函数，初始化任务名称和连接相关参数。
     */
    NFMysqlConnectTask(): NFMysqlTask("")
    {
        m_taskName = GET_CLASS_NAME(NFMysqlConnectTask); // 设置任务名称为类名
        m_iReconnectTime = 0; // 初始化重连时间为0
        m_iReconnectCount = 0; // 初始化重连次数为0
        m_iPort = 0; // 初始化端口号为0
    }

    /**
     * @brief 析构函数，无特殊操作。
     */
    ~NFMysqlConnectTask() override
    {
    }

    /**
     * @brief 判断是否已连接。
     * @return 返回true表示已连接。
     */
    bool IsConnect() override
    {
        return true;
    }

    /**
     * @brief 异步线程处理函数，将在另一个线程里运行。
     * @return 返回true表示处理成功。
     */
    bool ThreadProcess() override
    {
        return true;
    }

    /**
     * @brief 主线程处理函数，将在线程处理完后，提交给主线程来处理。
     * @return 返回TPTaskState枚举值，表示任务状态。
     */
    TPTaskState MainThreadProcess() override
    {
        return TPTASK_STATE_COMPLETED; // 返回任务完成状态
    }

public:
    std::string m_strServerId; // 服务器ID
    std::string m_strIp; // 服务器IP地址
    int m_iPort; // 服务器端口号
    std::string m_strDbName; // 数据库名称
    std::string m_strDbUser; // 数据库用户名
    std::string m_strDbPwd; // 数据库密码
    int m_iReconnectTime; // 重连时间间隔
    int m_iReconnectCount; // 重连次数
};

/**
 * @class NFMysqlCheckTask
 * @brief 继承自NFMysqlTask的MySQL检查任务类，用于执行MySQL相关的检查任务。
 */
class NFMysqlCheckTask final : public NFMysqlTask
{
public:
    /**
     * @brief 构造函数，初始化任务名称为类名。
     */
    NFMysqlCheckTask(): NFMysqlTask("")
    {
        m_taskName = GET_CLASS_NAME(NFMysqlCheckTask);
    }

    /**
     * @brief 析构函数，无特殊操作。
     */
    ~NFMysqlCheckTask() override
    {
    }

    /**
     * @brief 判断当前任务是否为检查任务。
     * @return bool 返回true，表示该任务为检查任务。
     */
    bool IsCheck() override
    {
        return true;
    }

    /**
     * @brief 异步线程处理函数，将在另一个线程里运行。
     * @return bool 返回true，表示线程处理成功。
     */
    bool ThreadProcess() override
    {
        return true;
    }

    /**
     * @brief 主线程处理函数，将在线程处理完后，提交给主线程来处理。
     * @return TPTaskState 返回任务状态，TPTASK_STATE_COMPLETED表示任务已完成。
     */
    TPTaskState MainThreadProcess() override
    {
        return TPTASK_STATE_COMPLETED;
    }
};

/**
 * @class NFMysqlSelectByCondTask
 * @brief 继承自NFMysqlTask的类，用于处理基于条件的MySQL查询任务。
 *
 * 该类负责在异步线程中执行MySQL查询操作，并在主线程中处理查询结果。
 */
class NFMysqlSelectByCondTask final : public NFMysqlTask
{
public:
    /**
     * @brief 构造函数，初始化任务对象。
     * @param strServerId 服务器ID，用于标识任务所属的服务器。
     * @param stSelect 查询条件，包含查询的具体信息。
     * @param fCallBack 查询完成后的回调函数，用于处理查询结果。
     */
    NFMysqlSelectByCondTask(const std::string& strServerId, const NFrame::storesvr_sel& stSelect, const SelectByCondCb& fCallBack) : NFMysqlTask(strServerId)
    {
        m_balanceId = 0;
        m_stSelect = stSelect;
        m_fCallback = fCallBack;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFMysqlSelectByCondTask);
    }

    /**
     * @brief 析构函数，清理资源。
     */
    ~NFMysqlSelectByCondTask() override
    {
    }

    /**
     * @brief 异步线程处理函数，执行MySQL查询操作。
     * @return 总是返回true，表示任务处理成功。
     */
    bool ThreadProcess() override
    {
        if (m_pMysqlDriver)
        {
            // 执行基于条件的查询操作，并将结果存储在mSelectRes中
            m_iRet = m_pMysqlDriver->SelectByCond(m_stSelect, m_stSelectRes);
        }
        else
        {
            m_iRet = -1; // 如果MySQL驱动未初始化，返回错误
        }
        return true;
    }

    /**
     * @brief 主线程处理函数，处理查询结果并调用回调函数。
     * @return 返回TPTaskState枚举值，表示任务状态。
     */
    TPTaskState MainThreadProcess() override
    {
        // 遍历查询结果，并调用回调函数处理每个结果
        for (int i = 0; i < m_stSelectRes.size(); i++)
        {
            m_fCallback(m_iRet, *m_stSelectRes.Mutable(i));
        }

        return TPTASK_STATE_COMPLETED; // 返回任务完成状态
    }

public:
    NFrame::storesvr_sel m_stSelect; // 查询条件
    google::protobuf::RepeatedPtrField<NFrame::storesvr_sel_res> m_stSelectRes; // 查询结果
    SelectByCondCb m_fCallback; // 查询完成后的回调函数
    int m_iRet; // 查询操作的返回值
};

/**
 * @class NFMysqlSelectObjTask
 * @brief 用于执行MySQL数据库对象选择任务的类，继承自NFMysqlTask。
 *
 * @param strServerId 服务器ID，用于标识任务所属的服务器。
 * @param stSelect 包含选择条件的对象，类型为NFrame::storesvr_selobj。
 * @param fCallBack 选择操作完成后的回调函数，类型为SelectObj_CB。
 */
class NFMysqlSelectObjTask final : public NFMysqlTask
{
public:
    NFMysqlSelectObjTask(const std::string& strServerId, const NFrame::storesvr_selobj& stSelect, const SelectObjCb& fCallBack) : NFMysqlTask(strServerId)
    {
        m_balanceId = stSelect.mod_key();
        m_stSelect = stSelect;
        m_fCallback = fCallBack;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFMysqlSelectObjTask);
    }

    ~NFMysqlSelectObjTask() override
    {
    }

    /**
     * @brief 异步线程处理函数，将在另一个线程里运行。
     * @return 返回true表示处理成功。
     */
    bool ThreadProcess() override
    {
        if (m_pMysqlDriver)
        {
            m_iRet = m_pMysqlDriver->SelectObj(m_stSelect, m_stSelectRes);
        }
        else
        {
            m_iRet = -1;
        }
        return true;
    }

    /**
     * @brief 主线程处理函数，将在线程处理完后，提交给主线程来处理。
     * @return 返回TPTaskState，表示任务状态。
     */
    TPTaskState MainThreadProcess() override
    {
        m_fCallback(m_iRet, m_stSelectRes);
        return TPTASK_STATE_COMPLETED;
    }

public:
    NFrame::storesvr_selobj m_stSelect; // 选择条件对象
    NFrame::storesvr_selobj_res m_stSelectRes; // 选择结果对象
    SelectObjCb m_fCallback; // 选择操作完成后的回调函数
    int m_iRet; // 操作返回值
};

/**
 * @class NFMysqlDeleteByCondTask
 * @brief 用于执行MySQL数据库条件删除任务的类，继承自NFMysqlTask。
 *
 * @param strServerId 服务器ID，用于标识任务所属的服务器。
 * @param stSelect 包含删除条件的对象，类型为NFrame::storesvr_del。
 * @param fCallBack 删除操作完成后的回调函数，类型为DeleteByCond_CB。
 */
class NFMysqlDeleteByCondTask final : public NFMysqlTask
{
public:
    NFMysqlDeleteByCondTask(const std::string& strServerId, const NFrame::storesvr_del& stSelect, const DeleteByCondCb& fCallBack) : NFMysqlTask(strServerId)
    {
        m_balanceId = 0;
        m_stSelect = stSelect;
        m_fCallback = fCallBack;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFMysqlDeleteByCondTask) + std::string("_") + stSelect.baseinfo().tbname();
    }

    ~NFMysqlDeleteByCondTask() override
    {
    }

    /**
     * @brief 异步线程处理函数，将在另一个线程里运行。
     * @return 返回true表示处理成功。
     */
    bool ThreadProcess() override
    {
        if (m_pMysqlDriver)
        {
            m_iRet = m_pMysqlDriver->DeleteByCond(m_stSelect, m_stSelectRes);
        }
        else
        {
            m_iRet = -1;
        }
        return true;
    }

    /**
     * @brief 主线程处理函数，将在线程处理完后，提交给主线程来处理。
     * @return 返回TPTaskState，表示任务状态。
     */
    TPTaskState MainThreadProcess() override
    {
        m_fCallback(m_iRet, m_stSelectRes);
        return TPTASK_STATE_COMPLETED;
    }

public:
    NFrame::storesvr_del m_stSelect; // 删除条件对象
    NFrame::storesvr_del_res m_stSelectRes; // 删除结果对象
    DeleteByCondCb m_fCallback; // 删除操作完成后的回调函数
    int m_iRet; // 操作返回值
};

/**
 * @class NFMysqlDeleteObjTask
 * @brief 该类继承自NFMysqlTask，用于处理MySQL数据库中的删除对象任务。
 *
 * 该类负责在异步线程中执行删除操作，并在主线程中处理删除结果。
 */
class NFMysqlDeleteObjTask final : public NFMysqlTask
{
public:
    /**
     * @brief 构造函数，初始化删除对象任务。
     *
     * @param strServerId 服务器ID，用于标识任务所属的服务器。
     * @param stSelect 删除对象的相关信息，包含表名、条件等。
     * @param fCallBack 删除操作完成后的回调函数，用于处理删除结果。
     */
    NFMysqlDeleteObjTask(const std::string& strServerId, const NFrame::storesvr_delobj& stSelect, const DeleteObjCb& fCallBack) : NFMysqlTask(strServerId)
    {
        m_balanceId = stSelect.mod_key(); // 设置任务的平衡ID
        m_stSelect = stSelect; // 保存删除对象的相关信息
        m_fCallback = fCallBack; // 保存回调函数
        m_iRet = 0; // 初始化返回值为0
        m_taskName = GET_CLASS_NAME(NFMysqlDeleteObjTask) + std::string("_") + stSelect.baseinfo().tbname(); // 设置任务名称
    }

    /**
     * @brief 析构函数，清理资源。
     */
    ~NFMysqlDeleteObjTask() override
    {
    }

    /**
     * @brief 异步线程处理函数，负责执行删除操作。
     *
     * 该函数在另一个线程中运行，调用MySQL驱动执行删除操作，并记录操作结果。
     *
     * @return bool 始终返回true，表示任务处理完成。
     */
    bool ThreadProcess() override
    {
        if (m_pMysqlDriver)
        {
            m_iRet = m_pMysqlDriver->DeleteObj(m_stSelect, m_stSelectRes); // 执行删除操作并记录结果
        }
        else
        {
            m_iRet = -1; // 如果MySQL驱动不存在，返回错误
        }
        return true;
    }

    /**
     * @brief 主线程处理函数，负责处理删除操作的结果。
     *
     * 该函数在主线程中运行，调用回调函数处理删除结果，并返回任务完成状态。
     *
     * @return TPTaskState 返回任务完成状态，表示任务已处理完毕。
     */
    TPTaskState MainThreadProcess() override
    {
        m_fCallback(m_iRet, m_stSelectRes); // 调用回调函数处理删除结果
        return TPTASK_STATE_COMPLETED; // 返回任务完成状态
    }

public:
    NFrame::storesvr_delobj m_stSelect; // 删除对象的相关信息
    NFrame::storesvr_delobj_res m_stSelectRes; // 删除操作的结果
    DeleteObjCb m_fCallback; // 删除操作完成后的回调函数
    int m_iRet; // 删除操作的返回值
};

/**
 * @class NFMysqlInsertObjTask
 * @brief 该类用于处理MySQL插入对象任务，继承自NFMysqlTask。
 *
 * @param strServerId 服务器ID，用于标识任务所属的服务器。
 * @param stSelect 插入对象的相关信息，包含表名、插入数据等。
 * @param fCallBack 插入操作完成后的回调函数，用于处理插入结果。
 */
class NFMysqlInsertObjTask final : public NFMysqlTask
{
public:
    NFMysqlInsertObjTask(const std::string& strServerId, const NFrame::storesvr_insertobj& stSelect, const InsertObjCb& fCallBack) : NFMysqlTask(strServerId)
    {
        m_balanceId = stSelect.mod_key();
        m_stSelect = stSelect;
        m_fCallback = fCallBack;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFMysqlInsertObjTask) + std::string("_") + stSelect.baseinfo().tbname();
    }

    ~NFMysqlInsertObjTask() override
    {
    }

    /**
     * @brief 异步线程处理函数，将在另一个线程里运行。
     * @return 返回true表示线程处理成功。
     */
    bool ThreadProcess() override
    {
        if (m_pMysqlDriver)
        {
            m_iRet = m_pMysqlDriver->InsertObj(m_stSelect, m_stSelectRes);
        }
        else
        {
            m_iRet = -1;
        }
        return true;
    }

    /**
     * @brief 主线程处理函数，将在线程处理完后，提交给主线程来处理。
     * @return 返回TPTaskState，表示任务状态。
     */
    TPTaskState MainThreadProcess() override
    {
        m_fCallback(m_iRet, m_stSelectRes);
        return TPTASK_STATE_COMPLETED;
    }

public:
    NFrame::storesvr_insertobj m_stSelect; // 插入对象的相关信息
    NFrame::storesvr_insertobj_res m_stSelectRes; // 插入操作的结果
    InsertObjCb m_fCallback; // 插入操作完成后的回调函数
    int m_iRet; // 操作返回值
};

/**
 * @class NFMysqlModifyByCondTask
 * @brief 该类用于处理MySQL根据条件修改数据任务，继承自NFMysqlTask。
 *
 * @param strServerId 服务器ID，用于标识任务所属的服务器。
 * @param stSelect 修改条件及相关信息，包含表名、修改条件等。
 * @param fCallBack 修改操作完成后的回调函数，用于处理修改结果。
 */
class NFMysqlModifyByCondTask final : public NFMysqlTask
{
public:
    NFMysqlModifyByCondTask(const std::string& strServerId, const NFrame::storesvr_mod& stSelect, const ModifyByCondCb& fCallBack) : NFMysqlTask(strServerId)
    {
        m_balanceId = 0;
        m_stSelect = stSelect;
        m_fCallback = fCallBack;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFMysqlModifyByCondTask) + std::string("_") + stSelect.baseinfo().tbname();
    }

    ~NFMysqlModifyByCondTask() override
    {
    }

    /**
     * @brief 异步线程处理函数，将在另一个线程里运行。
     * @return 返回true表示线程处理成功。
     */
    bool ThreadProcess() override
    {
        if (m_pMysqlDriver)
        {
            m_iRet = m_pMysqlDriver->ModifyByCond(m_stSelect, m_stSelectRes);
        }
        else
        {
            m_iRet = -1;
        }
        return true;
    }

    /**
     * @brief 主线程处理函数，将在线程处理完后，提交给主线程来处理。
     * @return 返回TPTaskState，表示任务状态。
     */
    TPTaskState MainThreadProcess() override
    {
        m_fCallback(m_iRet, m_stSelectRes);
        return TPTASK_STATE_COMPLETED;
    }

public:
    NFrame::storesvr_mod m_stSelect; // 修改条件及相关信息
    NFrame::storesvr_mod_res m_stSelectRes; // 修改操作的结果
    ModifyByCondCb m_fCallback; // 修改操作完成后的回调函数
    int m_iRet; // 操作返回值
};

/**
 * @class NFMysqlModifyObjTask
 * @brief 该类用于处理MySQL修改对象任务，继承自NFMysqlTask。
 *
 * @param strServerId 服务器ID，用于标识任务所属的服务器。
 * @param stSelect 修改对象的相关信息，包含表名、修改数据等。
 * @param fCallBack 修改操作完成后的回调函数，用于处理修改结果。
 */
class NFMysqlModifyObjTask final : public NFMysqlTask
{
public:
    NFMysqlModifyObjTask(const std::string& strServerId, const NFrame::storesvr_modobj& stSelect, const ModifyObjCb& fCallBack) : NFMysqlTask(strServerId)
    {
        m_balanceId = stSelect.mod_key();
        m_stSelect = stSelect;
        m_fCallback = fCallBack;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFMysqlModifyObjTask) + std::string("_") + stSelect.baseinfo().tbname();
    }

    ~NFMysqlModifyObjTask() override
    {
    }

    /**
     * @brief 异步线程处理函数，将在另一个线程里运行。
     * @return 返回true表示线程处理成功。
     */
    bool ThreadProcess() override
    {
        if (m_pMysqlDriver)
        {
            m_iRet = m_pMysqlDriver->ModifyObj(m_stSelect, m_stSelectRes);
        }
        else
        {
            m_iRet = -1;
        }
        return true;
    }

    /**
     * @brief 主线程处理函数，将在线程处理完后，提交给主线程来处理。
     * @return 返回TPTaskState，表示任务状态。
     */
    TPTaskState MainThreadProcess() override
    {
        m_fCallback(m_iRet, m_stSelectRes);
        return TPTASK_STATE_COMPLETED;
    }

public:
    NFrame::storesvr_modobj m_stSelect; // 修改对象的相关信息
    NFrame::storesvr_modobj_res m_stSelectRes; // 修改操作的结果
    ModifyObjCb m_fCallback; // 修改操作完成后的回调函数
    int m_iRet; // 操作返回值
};

/**
 * @class NFMysqlUpdateByCondTask
 * @brief 继承自NFMysqlTask，用于处理基于条件的MySQL更新任务。
 *
 * @param strServerId 服务器ID，用于标识任务所属的服务器。
 * @param stSelect 包含更新条件和相关信息的结构体。
 * @param fCallBack 更新操作完成后的回调函数。
 */
class NFMysqlUpdateByCondTask final : public NFMysqlTask
{
public:
    NFMysqlUpdateByCondTask(const std::string& strServerId, const NFrame::storesvr_update& stSelect, const UpdateByCondCb& fCallBack) : NFMysqlTask(strServerId)
    {
        m_balanceId = 0;
        m_stSelect = stSelect;
        m_fCallback = fCallBack;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFMysqlUpdateByCondTask) + std::string("_") + stSelect.baseinfo().tbname();
    }

    ~NFMysqlUpdateByCondTask() override
    {
    }

    /**
     * @brief 异步线程处理函数，负责执行MySQL更新操作。
     * @return 返回true表示处理成功。
     */
    bool ThreadProcess() override
    {
        if (m_pMysqlDriver)
        {
            m_iRet = m_pMysqlDriver->UpdateByCond(m_stSelect, m_stSelectRes);
        }
        else
        {
            m_iRet = -1;
        }
        return true;
    }

    /**
     * @brief 主线程处理函数，负责在更新操作完成后调用回调函数。
     * @return 返回TPTASK_STATE_COMPLETED表示任务完成。
     */
    TPTaskState MainThreadProcess() override
    {
        m_fCallback(m_iRet, m_stSelectRes);
        return TPTASK_STATE_COMPLETED;
    }

public:
    NFrame::storesvr_update m_stSelect; // 更新条件和相关信息
    NFrame::storesvr_update_res m_stSelectRes; // 更新操作的结果
    UpdateByCondCb m_fCallback; // 更新操作完成后的回调函数
    int m_iRet; // 更新操作的返回值
};

/**
 * @class NFMysqlUpdateObjTask
 * @brief 继承自NFMysqlTask，用于处理基于对象的MySQL更新任务。
 *
 * @param strServerId 服务器ID，用于标识任务所属的服务器。
 * @param stSelect 包含更新对象和相关信息的结构体。
 * @param fCallBack 更新操作完成后的回调函数。
 */
class NFMysqlUpdateObjTask final : public NFMysqlTask
{
public:
    NFMysqlUpdateObjTask(const std::string& strServerId, const NFrame::storesvr_updateobj& stSelect, const UpdateObjCb& fCallBack): NFMysqlTask(strServerId)
    {
        m_balanceId = stSelect.mod_key();
        m_stSelect = stSelect;
        m_fCallback = fCallBack;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFMysqlUpdateObjTask) + std::string("_") + stSelect.baseinfo().tbname();
    }

    ~NFMysqlUpdateObjTask() override
    {
    }

    /**
     * @brief 异步线程处理函数，负责执行MySQL更新操作。
     * @return 返回true表示处理成功。
     */
    bool ThreadProcess() override
    {
        if (m_pMysqlDriver)
        {
            m_iRet = m_pMysqlDriver->UpdateObj(m_stSelect, m_stSelectRes);
        }
        else
        {
            m_iRet = -1;
        }
        return true;
    }

    /**
     * @brief 主线程处理函数，负责在更新操作完成后调用回调函数。
     * @return 返回TPTASK_STATE_COMPLETED表示任务完成。
     */
    TPTaskState MainThreadProcess() override
    {
        m_fCallback(m_iRet, m_stSelectRes);
        return TPTASK_STATE_COMPLETED;
    }

public:
    NFrame::storesvr_updateobj m_stSelect; // 更新对象和相关信息
    NFrame::storesvr_updateobj_res m_stSelectRes; // 更新操作的结果
    UpdateObjCb m_fCallback; // 更新操作完成后的回调函数
    int m_iRet; // 更新操作的返回值
};

/**
 * @class NFMysqlExecuteTask
 * @brief 继承自NFMysqlTask，用于执行MySQL命令任务。
 *
 * @param strServerId 服务器ID，用于标识任务所属的服务器。
 * @param stSelect 包含执行命令和相关信息的结构体。
 * @param fCallBack 执行操作完成后的回调函数。
 */
class NFMysqlExecuteTask final : public NFMysqlTask
{
public:
    NFMysqlExecuteTask(const std::string& strServerId, const NFrame::storesvr_execute& stSelect, const ExecuteCb& fCallBack): NFMysqlTask(strServerId)
    {
        m_balanceId = stSelect.mod_key();
        m_stSelect = stSelect;
        m_fCallback = fCallBack;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFMysqlExecuteTask) + std::string("_") + stSelect.baseinfo().tbname();
    }

    ~NFMysqlExecuteTask() override
    {
    }

    /**
     * @brief 异步线程处理函数，负责执行MySQL命令。
     * @return 返回true表示处理成功。
     */
    bool ThreadProcess() override
    {
        if (m_pMysqlDriver)
        {
            m_iRet = m_pMysqlDriver->Execute(m_stSelect, m_stSelectRes);
        }
        else
        {
            m_iRet = -1;
        }
        return true;
    }

    /**
     * @brief 主线程处理函数，负责在执行操作完成后调用回调函数。
     * @return 返回TPTASK_STATE_COMPLETED表示任务完成。
     */
    TPTaskState MainThreadProcess() override
    {
        m_fCallback(m_iRet, m_stSelectRes);
        return TPTASK_STATE_COMPLETED;
    }

public:
    NFrame::storesvr_execute m_stSelect; // 执行命令和相关信息
    NFrame::storesvr_execute_res m_stSelectRes; // 执行操作的结果
    ExecuteCb m_fCallback; // 执行操作完成后的回调函数
    int m_iRet; // 执行操作的返回值
};

/**
 * @class NFMysqlExecuteMoreTask
 * @brief 继承自NFMysqlTask的类，用于执行多个MySQL查询任务。
 */
class NFMysqlExecuteMoreTask final : public NFMysqlTask
{
public:
    /**
     * @brief 构造函数，初始化NFMysqlExecuteMoreTask对象。
     * @param strServerId 服务器ID，用于标识任务所属的服务器。
     * @param stSelect 包含多个查询请求的结构体。
     * @param fCallBack 回调函数，用于在任务完成后处理查询结果。
     */
    NFMysqlExecuteMoreTask(const std::string& strServerId, const NFrame::storesvr_execute_more& stSelect, const ExecuteMoreCb& fCallBack) : NFMysqlTask(strServerId)
    {
        m_balanceId = 0;
        m_stSelect = stSelect;
        m_fCallback = fCallBack;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFMysqlExecuteMoreTask);
    }

    /**
     * @brief 析构函数，清理资源。
     */
    ~NFMysqlExecuteMoreTask() override
    {
    }

    /**
     * @brief 异步线程处理函数，将在另一个线程里运行。
     * @return 返回true表示任务处理成功，false表示失败。
     */
    bool ThreadProcess() override
    {
        if (m_pMysqlDriver)
        {
            // 执行多个查询操作，并将结果存储在mSelectRes中
            m_iRet = m_pMysqlDriver->ExecuteMore(m_stSelect, m_stSelectRes);
        }
        else
        {
            m_iRet = -1;
        }
        return true;
    }

    /**
     * @brief 主线程处理函数，将在线程处理完后，提交给主线程来处理。
     * @return 返回TPTaskState，表示任务的状态。
     */
    TPTaskState MainThreadProcess() override
    {
        // 遍历查询结果，并调用回调函数处理每个结果
        for (int i = 0; i < m_stSelectRes.size(); i++)
        {
            m_fCallback(m_iRet, *m_stSelectRes.Mutable(i));
        }

        return TPTASK_STATE_COMPLETED;
    }

public:
    NFrame::storesvr_execute_more m_stSelect; // 存储多个查询请求的结构体
    google::protobuf::RepeatedPtrField<NFrame::storesvr_execute_more_res> m_stSelectRes; // 存储查询结果的容器
    ExecuteMoreCb m_fCallback; // 回调函数，用于处理查询结果
    int m_iRet; // 存储执行结果的状态码
};

/**
 * @class NFMysqlTaskComponent
 * @brief 该类继承自NFITaskComponent，用于处理与MySQL相关的任务。
 */
class NFMysqlTaskComponent final : public NFITaskComponent
{
public:
    /**
     * @brief 构造函数，初始化NFMysqlTaskComponent对象。
     * @param pAsyMysqlModule 指向NFCAsyMysqlModule的指针，用于异步MySQL操作。
     */
    explicit NFMysqlTaskComponent(NFCAsyMysqlModule* pAsyMysqlModule)
    {
        m_pAsyMysqlModule = pAsyMysqlModule;
        m_pMysqlDriverManager = NF_NEW NFCMysqlDriverManager();
    }

    /**
     * @brief 析构函数，释放NFMysqlTaskComponent对象占用的资源。
     */
    ~NFMysqlTaskComponent() override
    {
        NF_SAFE_DELETE(m_pMysqlDriverManager);
    }

    /**
     * @brief 处理任务开始时的逻辑。
     * @param pTask 指向NFTask的指针，表示要处理的任务。
     * @details 根据任务类型，执行不同的MySQL操作，如连接、检查等。
     */
    void ProcessTaskStart(NFTask* pTask) override
    {
        auto pMysqlTask = dynamic_cast<NFMysqlTask*>(pTask);
        if (pMysqlTask)
        {
            if (pMysqlTask->IsConnect())
            {
                auto pConnectTask = dynamic_cast<NFMysqlConnectTask*>(pTask);
                if (pConnectTask == nullptr) return;
                int iRet = m_pMysqlDriverManager->AddMysqlServer(pConnectTask->m_strServerId, pConnectTask->m_strIp, pConnectTask->m_iPort, pConnectTask->m_strDbName, pConnectTask->m_strDbUser,
                                                                 pConnectTask->m_strDbPwd, pConnectTask->m_iReconnectTime, pConnectTask->m_iReconnectCount);
                if (iRet != 0)
                {
                    NFSLEEP(1000);
                    exit(0);
                }
            }
            else if (pMysqlTask->IsCheck())
            {
                m_pMysqlDriverManager->CheckMysql();
            }
            else
            {
                m_pMysqlDriverManager->CheckMysql();
                pMysqlTask->m_pMysqlDriver = m_pMysqlDriverManager->GetMysqlDriver(pMysqlTask->m_strServerId);
                CHECK_EXPR(pMysqlTask->m_pMysqlDriver, , "GetMysqlDriver:{} Failed", pMysqlTask->m_strServerId);
            }
        }
    }

    /**
     * @brief 处理任务的逻辑。
     * @param pTask 指向NFTask的指针，表示要处理的任务。
     * @details 调用任务的线程处理函数。
     */
    void ProcessTask(NFTask* pTask) override
    {
        if (pTask)
        {
            pTask->ThreadProcess();
        }
    }

    /**
     * @brief 处理任务结束时的逻辑。
     * @param pTask 指向NFTask的指针，表示要处理的任务。
     * @details 将任务中的MySQL驱动指针置为空。
     */
    void ProcessTaskEnd(NFTask* pTask) override
    {
        auto pMysqlTask = dynamic_cast<NFMysqlTask*>(pTask);
        if (pMysqlTask)
        {
            pMysqlTask->m_pMysqlDriver = nullptr;
        }
    }

    /**
     * @brief 处理任务超时的逻辑。
     * @param strTaskName 任务名称。
     * @param ullUseTime 任务使用的总时间。
     * @details 记录任务超时的日志信息。
     */
    void HandleTaskTimeOut(const std::string& strTaskName, uint64_t ullUseTime) override
    {
        NFLogError(NF_LOG_DEFAULT, 0, "strTaskName:{} timeOut, userTime:{}", strTaskName, ullUseTime);
    }

public:
    NFCMysqlDriverManager* m_pMysqlDriverManager; ///< 管理MySQL驱动的对象。
    NFCAsyMysqlModule* m_pAsyMysqlModule; ///< 异步MySQL操作模块。
};

/**
 * @brief NFCAsyMysqlModule 构造函数
 * @param pPluginManager 插件管理器指针，用于初始化基类 NFIAsyMysqlModule
 */
NFCAsyMysqlModule::NFCAsyMysqlModule(NFIPluginManager* pPluginManager): NFIAsyMysqlModule(pPluginManager)
{
    m_ullLastCheckTime = NFGetTime(); // 初始化最后一次检查时间为当前时间
    m_bInitComponent = false; // 初始化组件标志为 false
}

/**
 * @brief NFCAsyMysqlModule 析构函数
 */
NFCAsyMysqlModule::~NFCAsyMysqlModule()
{
}

/**
 * @brief 初始化 Actor 池
 * @param iMaxTaskGroup 最大任务组数量
 * @param iMaxActorNum 每个任务组中最大 Actor 数量
 * @return 初始化成功返回 true，否则返回 false
 * @details 该函数用于初始化 Actor 池，并为每个 Actor 添加 MySQL 任务组件
 */
bool NFCAsyMysqlModule::InitActorPool(int iMaxTaskGroup, int iMaxActorNum)
{
    NFIAsyModule::InitActorPool(iMaxTaskGroup, iMaxActorNum); // 调用基类初始化 Actor 池
    if (!m_bInitComponent)
    {
        m_bInitComponent = true;
        // 遍历所有任务组和 Actor，为每个 Actor 添加 MySQL 任务组件
        for (size_t i = 0; i < m_stVecActorGroupPool.size(); i++)
        {
            for (size_t j = 0; j < m_stVecActorGroupPool[i].size(); j++)
            {
                auto pComponent = NF_NEW NFMysqlTaskComponent(this);
                AddActorComponent(i, m_stVecActorGroupPool[i][j], pComponent);
            }
        }
    }

    return true;
}

/**
 * @brief 添加 MySQL 服务器
 * @param strServerId 服务器 ID
 * @param strIp 服务器 IP 地址
 * @param iPort 服务器端口
 * @param strDbName 数据库名称
 * @param strDbUser 数据库用户名
 * @param strDbPwd 数据库密码
 * @param iReconnectTime 重连时间间隔
 * @param iReconnectCount 重连次数
 * @return 成功返回 0，失败返回 -1
 * @details 该函数用于添加 MySQL 服务器，并为每个 Actor 添加 MySQL 连接任务
 */
int NFCAsyMysqlModule::AddMysqlServer(const std::string& strServerId, const std::string& strIp, int iPort, std::string strDbName,
                                      std::string strDbUser, std::string strDbPwd, int iReconnectTime,
                                      int iReconnectCount)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    InitActorPool(NF_TASK_MAX_GROUP_DEFAULT); // 初始化 Actor 池

    // 遍历所有任务组和 Actor，为每个 Actor 添加 MySQL 连接任务
    for (size_t i = 0; i < m_stVecActorGroupPool.size(); i++)
    {
        for (size_t j = 0; j < m_stVecActorGroupPool[i].size(); j++)
        {
            auto pTask = NF_NEW NFMysqlConnectTask();
            pTask->m_strServerId = strServerId;
            pTask->m_strIp = strIp;
            pTask->m_iPort = iPort;
            pTask->m_strDbName = strDbName;
            pTask->m_strDbUser = strDbUser;
            pTask->m_strDbPwd = strDbPwd;
            pTask->m_iReconnectTime = iReconnectTime;
            pTask->m_iReconnectCount = iReconnectCount;
            int iRet = FindModule<NFITaskModule>()->AddTask(i, m_stVecActorGroupPool[i][j], pTask);
            CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
        }
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

/**
 * @brief 根据条件查询数据
 * @param strServerId 服务器 ID
 * @param stSelect 查询条件
 * @param bUseCache 是否使用缓存
 * @param fCallback 查询完成后的回调函数
 * @return 成功返回 0，失败返回 -1
 * @details 该函数用于根据条件查询数据，并添加相应的任务
 */
int NFCAsyMysqlModule::SelectByCond(const std::string& strServerId, const NFrame::storesvr_sel& stSelect, bool bUseCache,
                                    const SelectByCondCb& fCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto pTask = NF_NEW NFMysqlSelectByCondTask(strServerId, stSelect, fCallback);
    int iRet = AddTask(NF_TASK_GROUP_DEFAULT, pTask);
    CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

/**
 * @brief 查询对象数据
 * @param strServerId 服务器 ID
 * @param stSelect 查询条件
 * @param bUseCache 是否使用缓存
 * @param fCallback 查询完成后的回调函数
 * @return 成功返回 0，失败返回 -1
 * @details 该函数用于查询对象数据，并添加相应的任务
 */
int NFCAsyMysqlModule::SelectObj(const std::string& strServerId, const NFrame::storesvr_selobj& stSelect, bool bUseCache,
                                 const SelectObjCb& fCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto pTask = NF_NEW NFMysqlSelectObjTask(strServerId, stSelect, fCallback);
    int iRet = AddTask(NF_TASK_GROUP_DEFAULT, pTask);
    CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

/**
 * @brief 根据条件删除数据
 * @param strServerId 服务器 ID
 * @param stSelect 删除条件
 * @param bUseCache 是否使用缓存
 * @param fCallback 删除完成后的回调函数
 * @return 成功返回 0，失败返回 -1
 * @details 该函数用于根据条件删除数据，并添加相应的任务
 */
int NFCAsyMysqlModule::DeleteByCond(const std::string& strServerId, const NFrame::storesvr_del& stSelect, bool bUseCache,
                                    const DeleteByCondCb& fCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto pTask = NF_NEW NFMysqlDeleteByCondTask(strServerId, stSelect, fCallback);
    int iRet = AddTask(NF_TASK_GROUP_DEFAULT, pTask);
    CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

/**
 * @brief 删除数据库中的对象
 *
 * @param strServerId 服务器ID，标识要操作的数据库服务器
 * @param stSelect 删除操作的条件和参数
 * @param bUseCache 是否使用缓存（未使用）
 * @param fCallback 删除操作完成后的回调函数
 * @return int 返回操作结果，0表示成功，-1表示失败
 */
int NFCAsyMysqlModule::DeleteObj(const std::string& strServerId, const NFrame::storesvr_delobj& stSelect, bool bUseCache,
                                 const DeleteObjCb& fCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto pTask = NF_NEW NFMysqlDeleteObjTask(strServerId, stSelect, fCallback);
    int iRet = AddTask(NF_TASK_GROUP_DEFAULT, pTask);
    CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

/**
 * @brief 向数据库中插入对象
 *
 * @param strServerId 服务器ID，标识要操作的数据库服务器
 * @param stSelect 插入操作的条件和参数
 * @param bUseCache 是否使用缓存（未使用）
 * @param fCallback 插入操作完成后的回调函数
 * @return int 返回操作结果，0表示成功，-1表示失败
 */
int NFCAsyMysqlModule::InsertObj(const std::string& strServerId, const NFrame::storesvr_insertobj& stSelect, bool bUseCache,
                                 const InsertObjCb& fCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto pTask = NF_NEW NFMysqlInsertObjTask(strServerId, stSelect, fCallback);
    int iRet = AddTask(NF_TASK_GROUP_DEFAULT, pTask);
    CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

/**
 * @brief 根据条件修改数据库中的对象
 *
 * @param strServerId 服务器ID，标识要操作的数据库服务器
 * @param stSelect 修改操作的条件和参数
 * @param bUseCache 是否使用缓存（未使用）
 * @param fCallback 修改操作完成后的回调函数
 * @return int 返回操作结果，0表示成功，-1表示失败
 */
int NFCAsyMysqlModule::ModifyByCond(const std::string& strServerId, const NFrame::storesvr_mod& stSelect, bool bUseCache,
                                    const ModifyByCondCb& fCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto pTask = NF_NEW NFMysqlModifyByCondTask(strServerId, stSelect, fCallback);
    int iRet = AddTask(NF_TASK_GROUP_DEFAULT, pTask);
    CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

/**
 * @brief 修改数据库中的对象
 *
 * @param strServerId 服务器ID，标识要操作的数据库服务器
 * @param stSelect 修改操作的条件和参数
 * @param bUseCache 是否使用缓存（未使用）
 * @param fCallback 修改操作完成后的回调函数
 * @return int 返回操作结果，0表示成功，-1表示失败
 */
int NFCAsyMysqlModule::ModifyObj(const std::string& strServerId, const NFrame::storesvr_modobj& stSelect, bool bUseCache,
                                 const ModifyObjCb& fCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto* pTask = NF_NEW NFMysqlModifyObjTask(strServerId, stSelect, fCallback);
    int iRet = AddTask(NF_TASK_GROUP_DEFAULT, pTask);
    CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

/**
 * @brief 根据条件更新数据库中的对象
 *
 * @param strServerId 服务器ID，标识要操作的数据库服务器
 * @param stSelect 更新操作的条件和参数
 * @param bUseCache 是否使用缓存（未使用）
 * @param fCallback 更新操作完成后的回调函数
 * @return int 返回操作结果，0表示成功，-1表示失败
 */
int NFCAsyMysqlModule::UpdateByCond(const std::string& strServerId, const NFrame::storesvr_update& stSelect, bool bUseCache,
                                    const UpdateByCondCb& fCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto* pTask = NF_NEW NFMysqlUpdateByCondTask(strServerId, stSelect, fCallback);
    int iRet = AddTask(NF_TASK_GROUP_DEFAULT, pTask);
    CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

/**
 * @brief 更新数据库中的对象
 *
 * @param strServerId 服务器ID，标识要操作的数据库服务器
 * @param stSelect 更新操作的条件和参数
 * @param bUseCache 是否使用缓存（未使用）
 * @param fCallback 更新操作完成后的回调函数
 * @return int 返回操作结果，0表示成功，-1表示失败
 */
int NFCAsyMysqlModule::UpdateObj(const std::string& strServerId, const NFrame::storesvr_updateobj& stSelect, bool bUseCache,
                                 const UpdateObjCb& fCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto* pTask = NF_NEW NFMysqlUpdateObjTask(strServerId, stSelect, fCallback);
    int iRet = AddTask(NF_TASK_GROUP_DEFAULT, pTask);
    CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

/**
 * @brief 执行数据库操作
 *
 * @param strServerId 服务器ID，标识要操作的数据库服务器
 * @param stSelect 执行操作的条件和参数
 * @param fCallback 执行操作完成后的回调函数
 * @return int 返回操作结果，0表示成功，-1表示失败
 */
int NFCAsyMysqlModule::Execute(const std::string& strServerId, const NFrame::storesvr_execute& stSelect,
                               const ExecuteCb& fCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto* pTask = NF_NEW NFMysqlExecuteTask(strServerId, stSelect, fCallback);
    int iRet = AddTask(NF_TASK_GROUP_DEFAULT, pTask);
    CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

/**
 * @brief 执行多个MySQL查询任务
 *
 * 该函数用于创建一个MySQL执行多个查询的任务，并将其添加到任务队列中。
 *
 * @param strServerId 服务器ID，标识要执行查询的MySQL服务器
 * @param stSelect 查询请求对象，包含要执行的查询信息
 * @param fCallback 回调函数，用于处理查询结果
 * @return int 返回0表示成功，返回-1表示添加任务失败
 */
int NFCAsyMysqlModule::ExecuteMore(const std::string& strServerId, const NFrame::storesvr_execute_more& stSelect,
                                   const ExecuteMoreCb& fCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    // 创建一个新的MySQL执行多个查询任务
    auto* pTask = NF_NEW NFMysqlExecuteMoreTask(strServerId, stSelect, fCallback);
    // 将任务添加到默认任务组中
    int iRet = AddTask(NF_TASK_GROUP_DEFAULT, pTask);
    // 检查任务是否添加成功
    CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

/**
 * @brief 执行MySQL检查任务
 *
 * 该函数用于定期执行MySQL检查任务，确保MySQL连接的健康状态。
 *
 * @return bool 返回true表示执行成功，返回false表示未初始化组件
 */
bool NFCAsyMysqlModule::Execute()
{
    // 如果组件未初始化，直接返回true
    if (!m_bInitComponent) return true;
    // 如果距离上次检查时间不足10秒，直接返回true
    if (NFGetTime() - m_ullLastCheckTime < 10000) return true;

    // 更新上次检查时间
    m_ullLastCheckTime = NFGetTime();

    // 遍历所有任务组，为每个任务组中的每个执行者添加MySQL检查任务
    for (int i = 0; i < static_cast<int>(m_stVecActorGroupPool.size()); i++)
    {
        for (int j = 0; j < static_cast<int>(m_stVecActorGroupPool[i].size()); j++)
        {
            auto pTask = NF_NEW NFMysqlCheckTask();
            FindModule<NFITaskModule>()->AddTask(i, m_stVecActorGroupPool[i][j], pTask);
        }
    }

    return true;
}

