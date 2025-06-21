// -------------------------------------------------------------------------
//    @FileName         :    NFCAsyMysqlModule.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Module           :    NFCAsyMysqlModule
//
// -------------------------------------------------------------------------

#include "NFCAsyDBModule.h"
#include "NFComm/NFPluginModule/NFCheck.h"
#include "NFComm/NFPluginModule/NFIPluginManager.h"
#include "NFComm/NFPluginModule/NFTask.h"
#include "NFComm/NFPluginModule/NFITaskComponent.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include "NFComm/NFPluginModule/NFINosqlModule.h"
#include "NFCNosqlDriverManager.h"

/**
 * @class NFDbTask
 * @brief NFDBTask类继承自NFTask，用于处理数据库相关任务。
 */
class NFDbTask : public NFTask
{
public:
    /**
     * @brief 构造函数，初始化NFDBTask对象。
     * @param strServerId 服务器ID，用于标识任务所属的服务器。
     * @param bUseCache 是否使用缓存，默认为false。
     */
    explicit NFDbTask(const std::string& strServerId, bool bUseCache = false) : m_strServerId(strServerId)
    {
        m_pMysqlDriver = nullptr; // 初始化MySQL驱动指针为空
        m_pNosqlDriver = nullptr; // 初始化NoSQL驱动指针为空
        m_taskName = GET_CLASS_NAME(NFDBTask); // 设置任务名称为类名
        m_bUseCache = bUseCache; // 设置是否使用缓存
    }

    /**
     * @brief 析构函数，释放NFDBTask对象。
     */
    ~NFDbTask() override = default;

    /**
     * @brief 检查任务是否需要执行检查操作。
     * @return 返回false，表示默认不执行检查操作。
     */
    virtual bool IsCheck()
    {
        return false;
    }

    /**
     * @brief 检查任务是否需要连接数据库。
     * @return 返回false，表示默认不执行连接操作。
     */
    virtual bool IsConnect()
    {
        return false;
    }

public:
    NFCMysqlDriver* m_pMysqlDriver; // MySQL驱动指针
    NFINosqlDriver* m_pNosqlDriver; // NoSQL驱动指针
    std::string m_strServerId; // 服务器ID
    bool m_bUseCache; // 是否使用缓存
};

/**
 * @class NFDbConnectTask
 * @brief 数据库连接任务类，继承自NFDBTask，用于处理数据库连接相关的任务。
 */
class NFDbConnectTask final : public NFDbTask
{
public:
    /**
     * @brief 构造函数，初始化数据库连接任务的基本信息。
     */
    NFDbConnectTask() : NFDbTask("")
    {
        m_taskName = GET_CLASS_NAME(NFDBConnectTask); // 设置任务名称为类名
        m_iReconnectTime = 0; // 初始化重连时间为0
        m_iReconnectCount = 0; // 初始化重连次数为0
        m_iPort = 0; // 初始化端口号为0
        m_iNosqlPort = 0; // 初始化NoSQL端口号为0
    }

    /**
     * @brief 析构函数，释放资源。
     */
    ~NFDbConnectTask() override = default;

    /**
     * @brief 判断是否已连接。
     * @return 返回true表示已连接，false表示未连接。
     */
    bool IsConnect() override
    {
        return true;
    }

    /**
     * @brief 异步线程处理函数，将在另一个线程里运行。
     * @return 返回true表示处理成功，false表示处理失败。
     */
    bool ThreadProcess() override
    {
        return true;
    }

    /**
     * @brief 主线程处理函数，将在线程处理完后，提交给主线程来处理。
     * @return 返回TPTaskState枚举值，表示任务的处理状态。
     */
    TPTaskState MainThreadProcess() override
    {
        return TPTASK_STATE_COMPLETED;
    }

public:
    std::string m_strServerId; // 服务器ID
    std::string m_strIp; // 数据库IP地址
    int m_iPort; // 数据库端口号
    std::string m_strDbName; // 数据库名称
    std::string m_strDbUser; // 数据库用户名
    std::string m_strDbPwd; // 数据库密码
    int m_iReconnectTime; // 重连时间间隔
    int m_iReconnectCount; // 重连次数

public:
    std::string m_strNosqlIp; // NoSQL数据库IP地址
    int m_iNosqlPort; // NoSQL数据库端口号
    std::string m_strNosqlPass; // NoSQL数据库密码
};


class NFDbCheckTask final : public NFDbTask
{
public:
    NFDbCheckTask() : NFDbTask("")
    {
        m_taskName = GET_CLASS_NAME(NFDBCheckTask);
    }

    ~NFDbCheckTask() override = default;

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

/**
 * @class NFDbSelectByCondTask
 * @brief 该类继承自NFDBTask，用于处理基于条件的数据库查询任务。
 *
 * 该任务支持使用缓存进行查询，并且可以在异步线程中执行查询操作，查询结果会在主线程中进行处理。
 */
class NFDbSelectByCondTask final : public NFDbTask
{
public:
    /**
     * @brief 构造函数，初始化查询任务的相关参数。
     * @param strServerId 服务器ID，用于标识任务所属的服务器。
     * @param stSelect 查询条件，包含查询的表名、条件等信息。
     * @param fCallback 查询完成后的回调函数，用于处理查询结果。
     * @param bUseCache 是否使用缓存进行查询。
     */
    NFDbSelectByCondTask(const std::string& strServerId, const NFrame::storesvr_sel& stSelect, const SelectByCondCb& fCallback, bool bUseCache) : NFDbTask(strServerId, bUseCache)
    {
        m_balanceId = 0;
        m_stSelect = stSelect;
        m_fCallback = fCallback;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFDBSelectByCondTask) + std::string("_") + stSelect.baseinfo().tbname();
    }

    /**
     * @brief 析构函数，清理资源。
     */
    ~NFDbSelectByCondTask() override = default;

    /**
     * @brief 异步线程处理函数，执行实际的数据库查询操作。
     * @return bool 返回true表示任务处理完成，false表示任务处理失败。
     *
     * 该函数在异步线程中执行，根据是否使用缓存，分别调用不同的查询逻辑。
     * 如果使用缓存，则先尝试从缓存中查询，缓存未命中时再从数据库中查询，并将结果保存到缓存中。
     * 如果不使用缓存，则直接从数据库中查询。
     */
    bool ThreadProcess() override
    {
        if (m_bUseCache)
        {
            if (m_pNosqlDriver && m_pMysqlDriver)
            {
                //针对private_keys，直接使用私有key,先绕过mysql,对cache优化处理
                if (m_stSelect.cond().private_keys_size() > 0 && m_stSelect.cond().where_conds_size() == 0 && m_stSelect.cond().where_additional_conds().size() == 0)
                {
                    std::string strPrivateKey;
                    std::unordered_set<std::string> stPrivateKeySet;
                    std::unordered_set<std::string> stFields;

                    int iRet = m_pNosqlDriver->SelectByCond(m_stSelect,  strPrivateKey, stFields, stPrivateKeySet);
                    CHECK_ERR_RE_VAL(0, iRet, false, "GetPrivateKey Failed!");

                    std::unordered_set<std::string> stLeftPrivateKeySet;
                    m_iRet = m_pNosqlDriver->SelectByCond(m_stSelect, strPrivateKey, stFields, stPrivateKeySet, stLeftPrivateKeySet, m_stSelectRes);
                    if (m_iRet != 0)
                    {
                        return true;
                    }

                    if (stLeftPrivateKeySet.empty())
                    {
                        return true;
                    }

                    std::map<std::string, std::string> stRecordsMap;
                    m_iRet = m_pMysqlDriver->SelectByCond(m_stSelect.baseinfo().package_name(), m_stSelect.baseinfo().tbname(), m_stSelect.baseinfo().clname(), strPrivateKey, stLeftPrivateKeySet, stRecordsMap);
                    if (m_iRet != 0)
                    {
                        return true;
                    }

                    m_iRet = m_pNosqlDriver->SaveObj(m_stSelect.baseinfo().package_name(), m_stSelect.baseinfo().tbname(), m_stSelect.baseinfo().clname(), strPrivateKey, stRecordsMap);
                    if (m_iRet != 0)
                    {
                        return true;
                    }

                    std::unordered_set<std::string> stNoPrivateKeySet;
                    m_iRet = m_pNosqlDriver->SelectByCond(m_stSelect, strPrivateKey, stFields, stLeftPrivateKeySet, stNoPrivateKeySet, m_stSelectRes);
                    if (m_iRet != 0)
                    {
                        return true;
                    }

                    if (!stNoPrivateKeySet.empty())
                    {
                        m_iRet = -1;
                        return true;
                    }
                }
                else
                {
                    std::string strPrivateKey;
                    std::unordered_set<std::string> stPrivateKeySet;
                    std::unordered_set<std::string> stFields;
                    m_iRet = m_pMysqlDriver->SelectByCond(m_stSelect, strPrivateKey, stFields, stPrivateKeySet);
                    if (m_iRet != 0)
                    {
                        return true;
                    }

                    std::unordered_set<std::string> stLeftPrivateKeySet;
                    m_iRet = m_pNosqlDriver->SelectByCond(m_stSelect, strPrivateKey, stFields, stPrivateKeySet, stLeftPrivateKeySet, m_stSelectRes);
                    if (m_iRet != 0)
                    {
                        return true;
                    }

                    if (stLeftPrivateKeySet.empty())
                    {
                        return true;
                    }

                    std::map<std::string, std::string> stRecordsMap;
                    m_iRet = m_pMysqlDriver->SelectByCond(m_stSelect.baseinfo().package_name(), m_stSelect.baseinfo().tbname(), m_stSelect.baseinfo().clname(), strPrivateKey, stLeftPrivateKeySet, stRecordsMap);
                    if (m_iRet != 0)
                    {
                        return true;
                    }

                    m_iRet = m_pNosqlDriver->SaveObj(m_stSelect.baseinfo().package_name(), m_stSelect.baseinfo().tbname(), m_stSelect.baseinfo().clname(), strPrivateKey, stRecordsMap);
                    if (m_iRet != 0)
                    {
                        return true;
                    }

                    std::unordered_set<std::string> stNoPrivateKeySet;
                    m_iRet = m_pNosqlDriver->SelectByCond(m_stSelect, strPrivateKey, stFields, stLeftPrivateKeySet, stNoPrivateKeySet, m_stSelectRes);
                    if (m_iRet != 0)
                    {
                        return true;
                    }

                    if (!stNoPrivateKeySet.empty())
                    {
                        m_iRet = -1;
                        return true;
                    }
                }
            }
            else
            {
                m_iRet = -1;
                return true;
            }
        }
        else
        {
            if (m_pMysqlDriver)
            {
                m_stSelectRes.Clear();
                m_iRet = m_pMysqlDriver->SelectByCond(m_stSelect, m_stSelectRes);
            }
            else
            {
                m_iRet = -1;
            }
        }

        return true;
    }

    /**
     * @brief 主线程处理函数，处理查询结果并调用回调函数。
     * @return TPTaskState 返回任务状态，表示任务是否完成。
     *
     * 该函数在主线程中执行，遍历查询结果并调用回调函数处理每个结果。
     */
    TPTaskState MainThreadProcess() override
    {
        for (int i = 0; i < m_stSelectRes.size(); i++)
        {
            if (m_fCallback)
            {
                m_fCallback(m_iRet, *m_stSelectRes.Mutable(i));
            }
        }

        return TPTASK_STATE_COMPLETED;
    }

public:
    NFrame::storesvr_sel m_stSelect; ///< 查询条件
    google::protobuf::RepeatedPtrField<NFrame::storesvr_sel_res> m_stSelectRes; ///< 查询结果
    SelectByCondCb m_fCallback; ///< 查询完成后的回调函数
    int m_iRet; ///< 查询操作的返回状态
};

/**
 * @class NFDbSelectObjTask
 * @brief 该类继承自NFDBTask，用于处理数据库选择对象的异步任务。
 *
 * 该类负责在异步线程中执行数据库查询操作，并在主线程中处理查询结果。
 */
class NFDbSelectObjTask final : public NFDbTask
{
public:
    /**
     * @brief 构造函数，初始化任务对象。
     *
     * @param strServerId 服务器ID，用于标识任务所属的服务器。
     * @param stSelect 数据库查询对象，包含查询条件和相关信息。
     * @param fCb 查询完成后的回调函数，用于处理查询结果。
     * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用。
     */
    NFDbSelectObjTask(const std::string& strServerId, const NFrame::storesvr_selobj& stSelect, const SelectObjCb& fCb, bool bUseCache) : NFDbTask(strServerId, bUseCache)
    {
        m_balanceId = stSelect.mod_key(); // 设置任务的平衡ID
        m_stSelect = stSelect; // 保存查询对象
        m_fCallback = fCb; // 保存回调函数
        m_iRet = 0; // 初始化返回值为0
        m_taskName = GET_CLASS_NAME(NFDBSelectObjTask) + std::string("_") + stSelect.baseinfo().tbname() + "_" + NFCommon::tostr(stSelect.mod_key()); // 设置任务名称
    }

    /**
     * @brief 析构函数，清理资源。
     */
    ~NFDbSelectObjTask() override = default;

    /**
     * @brief 异步线程处理函数，将在另一个线程里运行。
     *
     * 该函数根据是否使用缓存，选择从缓存或数据库中查询数据，并处理查询结果。
     *
     * @return bool 返回true表示任务处理完成，false表示任务处理失败。
     */
    bool ThreadProcess() override
    {
        if (m_bUseCache)
        {
            // 如果使用缓存，首先尝试从缓存中查询数据
            if (m_pNosqlDriver)
            {
                m_iRet = m_pNosqlDriver->SelectObj(m_stSelect, m_stSelectRes);
                if (m_iRet < 0)
                {
                    return true; // <0 如果缓存查询失败，直接返回
                }
                else if (m_iRet == 0)
                {
                    return true; // =0 表示缓存中存在该数据
                }

                //>0 表示缓存中无该数据
                //继续从数据库中查询数据
                if (m_pMysqlDriver)
                {
                    m_iRet = m_pMysqlDriver->SelectObj(m_stSelect, m_stSelectRes);
                }
                else
                {
                    m_iRet = -1; // 如果数据库驱动不存在，返回错误
                    return true;
                }

                // 如果数据库查询成功，将结果保存到缓存中
                if (m_iRet == 0)
                {
                    m_stSelect.set_record(m_stSelectRes.record());
                    m_iRet = m_pNosqlDriver->SaveObj(m_stSelect, m_stSelectRes);
                }
            }
            else
            {
                m_iRet = -1; // 如果缓存驱动不存在，返回错误
                return true;
            }
        }
        else
        {
            // 如果不使用缓存，直接从数据库中查询数据
            if (m_pMysqlDriver)
            {
                m_iRet = m_pMysqlDriver->SelectObj(m_stSelect, m_stSelectRes);
            }
            else
            {
                m_iRet = -1; // 如果数据库驱动不存在，返回错误
            }
        }

        return true; // 任务处理完成
    }

    /**
     * @brief 主线程处理函数，将在线程处理完后，提交给主线程来处理。
     *
     * 该函数根据查询结果调用回调函数，并返回任务状态。
     *
     * @return TPTaskState 返回任务状态，TPTASK_STATE_COMPLETED表示任务完成。
     */
    TPTaskState MainThreadProcess() override
    {
        if (m_fCallback)
        {
            m_fCallback(m_iRet, m_stSelectRes); // 调用回调函数处理查询结果
        }
        return TPTASK_STATE_COMPLETED; // 返回任务完成状态
    }

public:
    NFrame::storesvr_selobj m_stSelect; // 数据库查询对象
    NFrame::storesvr_selobj_res m_stSelectRes; // 查询结果对象
    SelectObjCb m_fCallback; // 查询完成后的回调函数
    int m_iRet; // 查询结果返回值
};

/**
 * @class NFDbDeleteByCondTask
 * @brief 该类继承自NFDBTask，用于处理基于条件的数据库删除任务。
 *
 * 该类负责在异步线程中执行数据库删除操作，并在主线程中处理结果。
 */
class NFDbDeleteByCondTask final : public NFDbTask
{
public:
    /**
     * @brief 构造函数，初始化删除任务的相关参数。
     * @param strServerId 服务器ID，用于标识任务所属的服务器。
     * @param stSelect 删除条件，包含要删除的数据表信息。
     * @param fCallback 删除操作完成后的回调函数。
     * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用。
     */
    NFDbDeleteByCondTask(const std::string& strServerId, const NFrame::storesvr_del& stSelect, const DeleteByCondCb& fCallback, bool bUseCache) : NFDbTask(strServerId, bUseCache)
    {
        m_balanceId = 0;
        m_stSelect = stSelect;
        m_fCallback = fCallback;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFDBDeleteByCondTask) + std::string("_") + stSelect.baseinfo().tbname();
    }

    /**
     * @brief 析构函数，清理资源。
     */
    ~NFDbDeleteByCondTask() override = default;

    /**
     * @brief 异步线程处理函数，负责执行数据库删除操作。
     * @return 返回true表示任务处理完成，false表示任务未完成。
     */
    bool ThreadProcess() override
    {
        // 如果使用缓存并且存在Nosql和Mysql驱动，则先删除Mysql中的数据，再删除Nosql中的数据
        if (m_bUseCache && m_pNosqlDriver && m_pMysqlDriver)
        {
            std::string strPrivateKey;
            std::unordered_set<std::string> stPrivateKeySet;
            m_iRet = m_pMysqlDriver->DeleteByCond(m_stSelect, strPrivateKey, stPrivateKeySet);
            if (m_iRet != 0)
            {
                return true;
            }

            m_iRet = m_pNosqlDriver->DeleteByCond(m_stSelect, strPrivateKey, stPrivateKeySet, m_stSelectRes);
            if (m_iRet != 0)
            {
                return true;
            }
            m_nextActorGroup = NF_ASY_TASK_WRITE_GROUP;
            return true;
        }

        // 如果当前执行的是写操作组，则直接删除Mysql中的数据
        if (m_runActorGroup == NF_ASY_TASK_WRITE_GROUP)
        {
            if (m_pMysqlDriver)
            {
                m_iRet = m_pMysqlDriver->DeleteByCond(m_stSelect, m_stSelectRes);
            }
            else
            {
                m_iRet = -1;
            }
        }

        return true;
    }

    /**
     * @brief 主线程处理函数，负责处理异步线程执行完成后的结果。
     * @return 返回TPTaskState，表示任务的状态，决定是否继续处理。
     */
    TPTaskState MainThreadProcess() override
    {
        // 如果使用缓存，则调用回调函数处理结果，并根据返回值决定是否继续处理
        if (m_bUseCache)
        {
            if (m_fCallback)
            {
                m_fCallback(m_iRet, m_stSelectRes);
            }

            if (m_iRet == 0)
            {
                m_bUseCache = false;
                m_fCallback = nullptr;
            }
            else
            {
                return TPTASK_STATE_COMPLETED;
            }
        }

        // 如果当前执行组与下一个执行组相同，则调用回调函数并完成任务
        if (m_runActorGroup == m_nextActorGroup)
        {
            if (m_fCallback)
            {
                m_fCallback(m_iRet, m_stSelectRes);
            }

            return TPTASK_STATE_COMPLETED;
        }
        return TPTASK_STATE_CONTINUE_CHILDTHREAD;
    }

public:
    NFrame::storesvr_del m_stSelect; ///< 删除条件，包含要删除的数据表信息
    NFrame::storesvr_del_res m_stSelectRes; ///< 删除操作的结果
    DeleteByCondCb m_fCallback; ///< 删除操作完成后的回调函数
    int m_iRet; ///< 删除操作的返回值
};

/**
 * @class NFDbDeleteObjTask
 * @brief 该类继承自NFDBTask，用于处理数据库删除对象的任务。
 *
 * 该类主要负责在异步线程中执行数据库删除操作，并在主线程中处理操作结果。
 */
class NFDbDeleteObjTask final : public NFDbTask
{
public:
    /**
     * @brief 构造函数，初始化删除对象任务的相关参数。
     *
     * @param strServerId 服务器ID，用于标识任务所属的服务器。
     * @param stSelect 删除对象的选择条件，包含表名、主键等信息。
     * @param fCallback 删除操作完成后的回调函数。
     * @param bUseCache 是否使用缓存进行删除操作。
     */
    NFDbDeleteObjTask(const std::string& strServerId, const NFrame::storesvr_delobj& stSelect, const DeleteObjCb& fCallback, bool bUseCache) : NFDbTask(strServerId, bUseCache)
    {
        m_balanceId = stSelect.mod_key(); // 设置任务的平衡ID为主键
        m_stSelect = stSelect; // 保存删除对象的选择条件
        m_fCallback = fCallback; // 保存回调函数
        m_iRet = 0; // 初始化返回值为0
        m_taskName = GET_CLASS_NAME(NFDBDeleteObjTask) + std::string("_") + stSelect.baseinfo().tbname() + "_" + NFCommon::tostr(stSelect.mod_key()); // 生成任务名称
    }

    /**
     * @brief 析构函数，清理资源。
     */
    ~NFDbDeleteObjTask() override = default;

    /**
     * @brief 异步线程处理函数，负责执行删除操作。
     *
     * 该函数在另一个线程中运行，根据是否使用缓存选择不同的删除方式。
     *
     * @return bool 返回true表示任务继续执行，false表示任务终止。
     */
    bool ThreadProcess() override
    {
        // 如果使用缓存且缓存驱动存在，则通过缓存删除对象
        if (m_bUseCache && m_pNosqlDriver)
        {
            m_iRet = m_pNosqlDriver->DeleteObj(m_stSelect);
            if (m_iRet != 0)
            {
                return true;
            }
            m_nextActorGroup = NF_ASY_TASK_WRITE_GROUP; // 设置下一个处理组为写组
            return true;
        }
        // 如果当前处理组为写组且MySQL驱动存在，则通过MySQL删除对象
        if (m_runActorGroup == NF_ASY_TASK_WRITE_GROUP)
        {
            if (m_pMysqlDriver)
            {
                m_iRet = m_pMysqlDriver->DeleteObj(m_stSelect, m_stSelectRes);
            }
            else
            {
                m_iRet = -1; // 如果MySQL驱动不存在，设置返回值为-1
            }
        }

        return true;
    }

    /**
     * @brief 主线程处理函数，负责处理删除操作的结果。
     *
     * 该函数在线程处理完后，提交给主线程来处理，根据返回函数是否继续处理。
     *
     * @return TPTaskState 返回任务状态，决定是否继续处理。
     */
    TPTaskState MainThreadProcess() override
    {
        // 如果使用缓存，则调用回调函数处理结果
        if (m_bUseCache)
        {
            if (m_fCallback)
            {
                m_fCallback(m_iRet, m_stSelectRes);
            }

            // 如果删除成功，则禁用缓存并清空回调函数
            if (m_iRet == 0)
            {
                m_bUseCache = false;
                m_fCallback = nullptr;
            }
            else
            {
                return TPTASK_STATE_COMPLETED; // 如果删除失败，返回任务完成状态
            }
        }

        // 如果当前处理组与下一个处理组相同，则调用回调函数并返回任务完成状态
        if (m_runActorGroup == m_nextActorGroup)
        {
            if (m_fCallback)
            {
                m_fCallback(m_iRet, m_stSelectRes);
            }

            return TPTASK_STATE_COMPLETED;
        }
        return TPTASK_STATE_CONTINUE_CHILDTHREAD; // 否则返回继续处理状态
    }

public:
    NFrame::storesvr_delobj m_stSelect; // 删除对象的选择条件
    NFrame::storesvr_delobj_res m_stSelectRes; // 删除操作的返回结果
    DeleteObjCb m_fCallback; // 删除操作完成后的回调函数
    int m_iRet; // 删除操作的返回值
};

/**
 * @class NFDbInsertObjTask
 * @brief 该类继承自NFDBTask，用于处理数据库插入对象的任务。
 *
 * 该类负责在异步线程中执行数据库插入操作，并在主线程中处理回调。
 */
class NFDbInsertObjTask final : public NFDbTask
{
public:
    /**
     * @brief 构造函数，初始化插入对象任务的相关参数。
     * @param strServerId 服务器ID，用于标识任务所属的服务器。
     * @param stSelect 插入对象的相关信息，包含表名、模块键值等。
     * @param fCallback 插入操作完成后的回调函数。
     * @param bUseCache 是否使用缓存，决定是否在插入操作后更新缓存。
     */
    NFDbInsertObjTask(const std::string& strServerId, const NFrame::storesvr_insertobj& stSelect, const InsertObjCb& fCallback, bool bUseCache) : NFDbTask(strServerId, bUseCache)
    {
        m_balanceId = stSelect.mod_key();
        m_stSelect = stSelect;
        m_fCallback = fCallback;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFDBInsertObjTask) + std::string("_") + stSelect.baseinfo().tbname() + "_" + NFCommon::tostr(stSelect.mod_key());
    }

    /**
     * @brief 析构函数，清理资源。
     */
    ~NFDbInsertObjTask() override = default;

    /**
     * @brief 异步线程处理函数，负责执行数据库插入操作。
     * @return 返回true表示任务处理完成，false表示任务处理失败。
     */
    bool ThreadProcess() override
    {
        // 如果数据库驱动存在，则执行插入操作
        if (m_pMysqlDriver)
        {
            m_iRet = m_pMysqlDriver->InsertObj(m_stSelect, m_stSelectRes);
        }
        else
        {
            m_iRet = -1;
        }

        // 如果使用缓存且缓存驱动存在，则更新缓存
        if (m_bUseCache && m_pNosqlDriver)
        {
            std::string strPrivateKey;
            std::string strPrivateKeyValue;
            std::string strDbKey;
            m_iRet = m_pNosqlDriver->GetObjKey(m_stSelect.baseinfo().package_name(), m_stSelect.baseinfo().tbname(), m_stSelect.baseinfo().clname(), m_stSelect.record(), strPrivateKey, strPrivateKeyValue, strDbKey);
            if (m_iRet != 0)
            {
                return true;
            }

            // 如果数据库驱动存在，则查询插入后的对象并更新缓存
            if (m_pMysqlDriver)
            {
                std::string strRecord;
                m_iRet = m_pMysqlDriver->SelectObj(m_stSelect.baseinfo().package_name(), m_stSelect.baseinfo().tbname(), m_stSelect.baseinfo().clname(), strPrivateKey, strPrivateKeyValue, strRecord);
                if (m_iRet != 0)
                {
                    return true;
                }

                m_stSelect.set_record(strRecord);
            }
            else
            {
                m_iRet = -1;
                return true;
            }

            m_iRet = m_pNosqlDriver->SaveObj(m_stSelect, m_stSelectRes);
        }

        return true;
    }

    /**
     * @brief 主线程处理函数，负责处理插入操作完成后的回调。
     * @return 返回TPTaskState，表示任务的状态。
     */
    TPTaskState MainThreadProcess() override
    {
        // 如果回调函数存在，则调用回调函数
        if (m_fCallback)
        {
            m_fCallback(m_iRet, m_stSelectRes);
        }
        return TPTASK_STATE_COMPLETED;
    }

public:
    NFrame::storesvr_insertobj m_stSelect; ///< 插入对象的相关信息
    NFrame::storesvr_insertobj_res m_stSelectRes; ///< 插入操作的结果
    InsertObjCb m_fCallback; ///< 插入操作完成后的回调函数
    int m_iRet; ///< 插入操作的返回结果
};

/**
 * @class NFDbModifyByCondTask
 * @brief 该类继承自NFDBTask，用于处理基于条件的数据库修改任务。
 *
 * @param serverId 服务器ID，用于标识任务所属的服务器。
 * @param select 包含修改条件的结构体，指定了要修改的数据库表、条件等信息。
 * @param cb 修改完成后的回调函数，用于处理修改结果。
 * @param useCache 是否使用缓存，true表示使用缓存，false表示不使用。
 */
class NFDbModifyByCondTask final : public NFDbTask
{
public:
    NFDbModifyByCondTask(const std::string& strServerId, const NFrame::storesvr_mod& stSelect, const ModifyByCondCb& fCallback, bool bUseCache) : NFDbTask(strServerId, bUseCache)
    {
        m_balanceId = 0;
        m_stSelect = stSelect;
        m_fCallback = fCallback;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFDBModifyByCondTask) + std::string("_") + stSelect.baseinfo().tbname();
    }

    ~NFDbModifyByCondTask() override = default;

    /**
     * @brief 异步线程处理函数，将在另一个线程里运行。
     * @return bool 返回true表示任务处理完成，false表示任务处理失败。
     */
    bool ThreadProcess() override
    {
        // 如果使用缓存并且存在Nosql和Mysql驱动，则先尝试在Mysql中修改数据，然后在Nosql中同步修改。
        if (m_bUseCache && m_pNosqlDriver && m_pMysqlDriver)
        {
            std::string strPrivateKey;
            std::unordered_set<std::string> stPrivateKeySet;
            m_iRet = m_pMysqlDriver->ModifyByCond(m_stSelect, strPrivateKey, stPrivateKeySet);
            if (m_iRet != 0)
            {
                return true;
            }

            std::unordered_set<std::string> stLeftPrivateKeySet;
            m_iRet = m_pNosqlDriver->ModifyByCond(m_stSelect, strPrivateKey, stPrivateKeySet, stLeftPrivateKeySet, m_stSelectRes);
            if (m_iRet != 0)
            {
                return true;
            }

            // 如果leftPrivateKeySet为空，表示所有数据都已同步，任务完成。
            if (stLeftPrivateKeySet.empty())
            {
                m_nextActorGroup = NF_ASY_TASK_WRITE_GROUP;
                return true;
            }

            // 如果leftPrivateKeySet不为空，则从Mysql中查询剩余数据并保存到Nosql中。
            std::map<std::string, std::string> stRecordsMap;
            m_iRet = m_pMysqlDriver->SelectByCond(m_stSelect.baseinfo().package_name(), m_stSelect.baseinfo().tbname(), m_stSelect.baseinfo().clname(), strPrivateKey, stLeftPrivateKeySet, stRecordsMap);
            if (m_iRet != 0)
            {
                return true;
            }

            m_iRet = m_pNosqlDriver->SaveObj(m_stSelect.baseinfo().package_name(), m_stSelect.baseinfo().tbname(), m_stSelect.baseinfo().clname(), strPrivateKey, stRecordsMap);
            if (m_iRet != 0)
            {
                return true;
            }

            // 再次尝试在Nosql中修改剩余数据。
            std::unordered_set<std::string> stOtherPrivateKeySet;
            m_iRet = m_pNosqlDriver->ModifyByCond(m_stSelect, strPrivateKey, stLeftPrivateKeySet, stOtherPrivateKeySet, m_stSelectRes);
            if (m_iRet != 0)
            {
                return true;
            }

            // 如果otherPrivateKeySet不为空，表示仍有数据未同步，任务失败。
            if (!stOtherPrivateKeySet.empty())
            {
                m_iRet = -1;
                return true;
            }

            m_nextActorGroup = NF_ASY_TASK_WRITE_GROUP;
            return true;
        }

        // 如果不使用缓存，则直接在Mysql中执行修改操作。
        if (m_runActorGroup == NF_ASY_TASK_WRITE_GROUP)
        {
            if (m_pMysqlDriver)
            {
                m_iRet = m_pMysqlDriver->ModifyByCond(m_stSelect, m_stSelectRes);
            }
            else
            {
                m_iRet = -1;
            }
        }

        return true;
    }

    /**
     * @brief 主线程处理函数，将在线程处理完后，提交给主线程来处理。
     * @return TPTaskState 返回任务状态，决定是否继续处理。
     */
    TPTaskState MainThreadProcess() override
    {
        // 如果使用缓存，则调用回调函数处理结果，并根据结果决定是否继续处理。
        if (m_bUseCache)
        {
            if (m_fCallback)
            {
                m_fCallback(m_iRet, m_stSelectRes);
            }

            if (m_iRet == 0)
            {
                m_bUseCache = false;
                m_fCallback = nullptr;
            }
            else
            {
                return TPTASK_STATE_COMPLETED;
            }
        }

        // 如果当前任务组与下一个任务组相同，则调用回调函数并完成任务。
        if (m_runActorGroup == m_nextActorGroup)
        {
            if (m_fCallback)
            {
                m_fCallback(m_iRet, m_stSelectRes);
            }

            return TPTASK_STATE_COMPLETED;
        }
        return TPTASK_STATE_CONTINUE_CHILDTHREAD;
    }

public:
    NFrame::storesvr_mod m_stSelect; // 修改条件结构体
    NFrame::storesvr_mod_res m_stSelectRes; // 修改结果结构体
    ModifyByCondCb m_fCallback; // 修改完成后的回调函数
    int m_iRet; // 任务执行结果
};

/**
 * @class NFDbModifyObjTask
 * @brief 该类继承自NFDBTask，用于处理数据库对象的修改任务。
 *
 * 该任务类负责在异步线程中处理数据库对象的修改操作，支持缓存和数据库的双重操作。
 */
class NFDbModifyObjTask final : public NFDbTask
{
public:
    /**
     * @brief 构造函数，初始化任务对象。
     *
     * @param strServerId 服务器ID，用于标识任务所属的服务器。
     * @param stSelect 包含修改对象的选择条件和其他相关信息。
     * @param fCallback 修改操作完成后的回调函数。
     * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用。
     */
    NFDbModifyObjTask(const std::string& strServerId, const NFrame::storesvr_modobj& stSelect, const ModifyObjCb& fCallback, bool bUseCache) : NFDbTask(strServerId, bUseCache)
    {
        m_balanceId = stSelect.mod_key();
        m_stSelect = stSelect;
        m_fCallback = fCallback;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFDBModifyObjTask) + std::string("_") + stSelect.baseinfo().tbname() + "_" + NFCommon::tostr(stSelect.mod_key());
    }

    /**
     * @brief 析构函数，清理资源。
     */
    ~NFDbModifyObjTask() override = default;

    /**
     * @brief 异步线程处理函数，负责在另一个线程中执行数据库对象的修改操作。
     *
     * 该函数根据是否使用缓存，分别处理缓存和数据库的修改操作。
     *
     * @return bool 返回true表示处理成功，false表示处理失败。
     */
    bool ThreadProcess() override
    {
        // 如果使用缓存且缓存驱动存在，优先处理缓存
        if (m_bUseCache && m_pNosqlDriver)
        {
            std::string strPrivateKey;
            std::string strPrivateKeyValue;
            std::string strDbKey;
            m_iRet = m_pNosqlDriver->GetObjKey(m_stSelect.baseinfo().package_name(), m_stSelect.baseinfo().tbname(), m_stSelect.baseinfo().clname(), m_stSelect.record(), strPrivateKey, strPrivateKeyValue, strDbKey);
            if (m_iRet != 0)
            {
                return true;
            }

            // 如果缓存中不存在该对象，尝试从数据库中获取并保存到缓存
            if (!m_pNosqlDriver->ExistObj(strDbKey))
            {
                if (m_pMysqlDriver)
                {
                    std::string strRecord;
                    m_iRet = m_pMysqlDriver->SelectObj(m_stSelect.baseinfo().package_name(), m_stSelect.baseinfo().tbname(), m_stSelect.baseinfo().clname(), strPrivateKey, strPrivateKeyValue, strRecord);
                    if (m_iRet != 0)
                    {
                        return true;
                    }

                    NFrame::storesvr_modobj stOldSelect = m_stSelect;
                    stOldSelect.set_record(strRecord);

                    m_iRet = m_pNosqlDriver->SaveObj(stOldSelect, m_stSelectRes);
                    if (m_iRet != 0)
                    {
                        return true;
                    }

                    m_iRet = m_pNosqlDriver->SaveObj(m_stSelect, m_stSelectRes);
                    if (m_iRet != 0)
                    {
                        return true;
                    }
                    m_nextActorGroup = NF_ASY_TASK_WRITE_GROUP;
                    return true;
                }
                m_iRet = -1;
                return true;
            }

            // 如果缓存中存在该对象，直接保存修改后的对象到缓存
            m_iRet = m_pNosqlDriver->SaveObj(m_stSelect, m_stSelectRes);
            if (m_iRet != 0)
            {
                return true;
            }
            m_nextActorGroup = NF_ASY_TASK_WRITE_GROUP;
            return true;
        }

        // 如果不使用缓存，直接处理数据库的修改操作
        if (m_runActorGroup == NF_ASY_TASK_WRITE_GROUP)
        {
            if (m_pMysqlDriver)
            {
                m_iRet = m_pMysqlDriver->ModifyObj(m_stSelect, m_stSelectRes);
            }
            else
            {
                m_iRet = -1;
            }
        }

        return true;
    }

    /**
     * @brief 主线程处理函数，负责在线程处理完成后，提交给主线程进行后续处理。
     *
     * 该函数根据任务的处理结果，决定是否继续处理或完成任务。
     *
     * @return TPTaskState 返回任务状态，表示任务是否完成或需要继续处理。
     */
    TPTaskState MainThreadProcess() override
    {
        // 如果使用缓存，调用回调函数并根据结果决定是否继续处理
        if (m_bUseCache)
        {
            if (m_fCallback)
            {
                m_fCallback(m_iRet, m_stSelectRes);
            }

            if (m_iRet == 0)
            {
                m_bUseCache = false;
                m_fCallback = nullptr;
            }
            else
            {
                return TPTASK_STATE_COMPLETED;
            }
        }

        // 如果当前执行组与下一个执行组相同，调用回调函数并完成任务
        if (m_runActorGroup == m_nextActorGroup)
        {
            if (m_fCallback)
            {
                m_fCallback(m_iRet, m_stSelectRes);
            }

            return TPTASK_STATE_COMPLETED;
        }
        return TPTASK_STATE_CONTINUE_CHILDTHREAD;
    }

public:
    NFrame::storesvr_modobj m_stSelect; ///< 包含修改对象的选择条件和其他相关信息。
    NFrame::storesvr_modobj_res m_stSelectRes; ///< 修改操作的结果。
    ModifyObjCb m_fCallback; ///< 修改操作完成后的回调函数。
    int m_iRet; ///< 操作结果，0表示成功，非0表示失败。
};

/**
 * @class NFDbUpdateByCondTask
 * @brief 该类继承自NFDBTask，用于处理基于条件的数据库更新任务。
 *
 * 该任务类支持异步线程处理和主线程处理，能够根据条件更新数据库记录，并支持缓存机制。
 */
class NFDbUpdateByCondTask final : public NFDbTask
{
public:
    /**
     * @brief 构造函数，初始化任务对象。
     * @param strServerId 服务器ID，用于标识任务所属的服务器。
     * @param stSelect 包含更新条件的结构体，用于指定更新操作的条件。
     * @param fCallback 更新操作完成后的回调函数，用于处理更新结果。
     * @param bUseCache 是否使用缓存机制，true表示使用缓存，false表示不使用。
     */
    NFDbUpdateByCondTask(const std::string& strServerId, const NFrame::storesvr_update& stSelect, const UpdateByCondCb& fCallback, bool bUseCache) : NFDbTask(
        strServerId, bUseCache)
    {
        m_balanceId = 0;
        m_stSelect = stSelect;
        m_fCallback = fCallback;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFDBUpdateByCondTask) + std::string("_") + stSelect.baseinfo().tbname();
    }

    /**
     * @brief 析构函数，清理资源。
     */
    ~NFDbUpdateByCondTask() override = default;

    /**
     * @brief 异步线程处理函数，将在另一个线程里运行。
     * @return 返回true表示任务处理成功，false表示失败。
     *
     * 该函数负责执行基于条件的数据库更新操作，支持缓存机制。如果使用缓存，会先更新缓存，再更新数据库。
     */
    bool ThreadProcess() override
    {
        if (m_bUseCache && m_pNosqlDriver && m_pMysqlDriver)
        {
            std::string strPrivateKey;
            std::unordered_set<std::string> stPrivateKeySet;
            m_iRet = m_pMysqlDriver->UpdateByCond(m_stSelect, strPrivateKey, stPrivateKeySet);
            if (m_iRet != 0)
            {
                return true;
            }

            std::unordered_set<std::string> stLeftPrivateKeySet;
            m_iRet = m_pNosqlDriver->UpdateByCond(m_stSelect, strPrivateKey, stPrivateKeySet, stLeftPrivateKeySet, m_stSelectRes);
            if (m_iRet != 0)
            {
                return true;
            }

            if (stLeftPrivateKeySet.empty())
            {
                m_nextActorGroup = NF_ASY_TASK_WRITE_GROUP;
                return true;
            }

            std::map<std::string, std::string> stRecordsMap;
            m_iRet = m_pMysqlDriver->SelectByCond(m_stSelect.baseinfo().package_name(), m_stSelect.baseinfo().tbname(), m_stSelect.baseinfo().clname(), strPrivateKey, stLeftPrivateKeySet, stRecordsMap);
            if (m_iRet != 0)
            {
                return true;
            }

            m_iRet = m_pNosqlDriver->SaveObj(m_stSelect.baseinfo().package_name(), m_stSelect.baseinfo().tbname(), m_stSelect.baseinfo().clname(), strPrivateKey, stRecordsMap);
            if (m_iRet != 0)
            {
                return true;
            }

            std::unordered_set<std::string> stOtherPrivateKeySet;
            m_iRet = m_pNosqlDriver->UpdateByCond(m_stSelect, strPrivateKey, stLeftPrivateKeySet, stOtherPrivateKeySet, m_stSelectRes);
            if (m_iRet != 0)
            {
                return true;
            }

            if (!stOtherPrivateKeySet.empty())
            {
                m_iRet = -1;
                return true;
            }

            m_nextActorGroup = NF_ASY_TASK_WRITE_GROUP;
            return true;
        }
        if (m_runActorGroup == NF_ASY_TASK_WRITE_GROUP)
        {
            if (m_pMysqlDriver)
            {
                m_iRet = m_pMysqlDriver->UpdateByCond(m_stSelect, m_stSelectRes);
            }
            else
            {
                m_iRet = -1;
            }
        }

        return true;
    }

    /**
     * @brief 主线程处理函数，将在线程处理完后，提交给主线程来处理。
     * @return 返回TPTaskState，表示任务的状态，决定是否继续处理。
     *
     * 该函数负责处理异步线程处理后的结果，并根据结果决定是否继续处理任务。
     */
    TPTaskState MainThreadProcess() override
    {
        if (m_bUseCache)
        {
            if (m_iRet == 0)
            {
                if (m_fCallback)
                {
                    m_fCallback(m_iRet, m_stSelectRes);
                }
                m_bUseCache = false;
                m_fCallback = nullptr;
                m_nextActorGroup = NF_ASY_TASK_WRITE_GROUP;
                return TPTASK_STATE_CONTINUE_CHILDTHREAD;
            }
            if (m_fCallback)
            {
                m_fCallback(m_iRet, m_stSelectRes);
            }
            return TPTASK_STATE_COMPLETED;
        }
        if (m_runActorGroup != NF_ASY_TASK_WRITE_GROUP)
        {
            m_nextActorGroup = NF_ASY_TASK_WRITE_GROUP;
            return TPTASK_STATE_CONTINUE_CHILDTHREAD;
        }
        if (m_fCallback)
        {
            m_fCallback(m_iRet, m_stSelectRes);
        }
        return TPTASK_STATE_COMPLETED;
    }

public:
    NFrame::storesvr_update m_stSelect; ///< 包含更新条件的结构体，用于指定更新操作的条件。
    NFrame::storesvr_update_res m_stSelectRes; ///< 更新操作的结果结构体，用于存储更新结果。
    UpdateByCondCb m_fCallback; ///< 更新操作完成后的回调函数，用于处理更新结果。
    int m_iRet; ///< 更新操作的返回码，表示操作的成功或失败。
};

/**
 * @class NFDbUpdateObjTask
 * @brief 该类继承自NFDBTask，用于处理数据库对象的更新任务。
 *
 * 该任务类负责在异步线程中处理数据库对象的更新操作，并在主线程中处理回调。
 */
class NFDbUpdateObjTask final : public NFDbTask
{
public:
    /**
     * @brief 构造函数，初始化任务对象。
     *
     * @param strServerId 服务器ID，用于标识任务所属的服务器。
     * @param stSelect 包含更新对象信息的结构体。
     * @param fCallback 更新操作完成后的回调函数。
     * @param bUseCache 是否使用缓存。
     */
    NFDbUpdateObjTask(const std::string& strServerId, const NFrame::storesvr_updateobj& stSelect, const UpdateObjCb& fCallback, bool bUseCache) : NFDbTask(strServerId, bUseCache)
    {
        m_balanceId = stSelect.mod_key();
        m_stSelect = stSelect;
        m_fCallback = fCallback;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFDBUpdateObjTask) + std::string("_") + stSelect.baseinfo().tbname();
    }

    /**
     * @brief 析构函数，清理资源。
     */
    ~NFDbUpdateObjTask() override = default;

    /**
     * @brief 异步线程处理函数，负责在另一个线程中执行数据库更新操作。
     *
     * 该函数根据是否使用缓存，分别处理缓存和数据库的更新操作。
     *
     * @return bool 返回true表示任务处理完成，false表示任务处理失败。
     */
    bool ThreadProcess() override
    {
        if (m_bUseCache && m_pNosqlDriver)
        {
            std::string strPrivateKey;
            std::string strPrivateKeyValue;
            std::string strDbKey;
            m_iRet = m_pNosqlDriver->GetObjKey(m_stSelect.baseinfo().package_name(), m_stSelect.baseinfo().tbname(), m_stSelect.baseinfo().clname(), m_stSelect.record(), strPrivateKey, strPrivateKeyValue, strDbKey);
            if (m_iRet != 0)
            {
                return true;
            }

            if (!m_pNosqlDriver->ExistObj(strDbKey))
            {
                if (m_pMysqlDriver)
                {
                    std::string strRecord;
                    m_iRet = m_pMysqlDriver->SelectObj(m_stSelect.baseinfo().package_name(), m_stSelect.baseinfo().tbname(), m_stSelect.baseinfo().clname(), strPrivateKey, strPrivateKeyValue, strRecord);
                    if (m_iRet == NFrame::ERR_CODE_STORESVR_ERRCODE_SELECT_EMPTY)
                    {
                        m_iRet = m_pMysqlDriver->UpdateObj(m_stSelect, m_strSelectRes);
                        if (m_iRet != 0)
                        {
                            return true;
                        }

                        m_iRet = m_pMysqlDriver->SelectObj(m_stSelect.baseinfo().package_name(), m_stSelect.baseinfo().tbname(), m_stSelect.baseinfo().clname(), strPrivateKey, strPrivateKeyValue, strRecord);
                        if (m_iRet != 0)
                        {
                            return true;
                        }

                        m_stSelect.set_record(strRecord);

                        m_iRet = m_pNosqlDriver->SaveObj(m_stSelect, m_strSelectRes);
                        return true;
                    }
                    if (m_iRet != 0)
                    {
                        return true;
                    }

                    NFrame::storesvr_updateobj strOldSelect = m_stSelect;
                    strOldSelect.set_record(strRecord);

                    m_iRet = m_pNosqlDriver->SaveObj(strOldSelect, m_strSelectRes);
                    if (m_iRet != 0)
                    {
                        return true;
                    }

                    m_iRet = m_pNosqlDriver->SaveObj(m_stSelect, m_strSelectRes);
                    if (m_iRet != 0)
                    {
                        return true;
                    }
                    m_nextActorGroup = NF_ASY_TASK_WRITE_GROUP;
                    return true;
                }
                m_iRet = -1;
                return true;
            }
            m_iRet = m_pNosqlDriver->SaveObj(m_stSelect, m_strSelectRes);
            if (m_iRet != 0)
            {
                return true;
            }
            m_nextActorGroup = NF_ASY_TASK_WRITE_GROUP;
            return true;
        }
        if (m_runActorGroup == NF_ASY_TASK_WRITE_GROUP)
        {
            if (m_pMysqlDriver)
            {
                m_iRet = m_pMysqlDriver->UpdateObj(m_stSelect, m_strSelectRes);
            }
            else
            {
                m_iRet = -1;
            }
        }
        else
        {
            m_nextActorGroup = NF_ASY_TASK_WRITE_GROUP;
        }

        return true;
    }

    /**
     * @brief 主线程处理函数，负责在线程处理完成后，提交给主线程处理回调。
     *
     * 该函数根据任务处理结果，决定是否继续处理或完成任务。
     *
     * @return TPTaskState 返回任务状态，决定是否继续处理或完成任务。
     */
    TPTaskState MainThreadProcess() override
    {
        if (m_bUseCache)
        {
            if (m_fCallback)
            {
                m_fCallback(m_iRet, m_strSelectRes);
            }

            if (m_iRet == 0)
            {
                m_bUseCache = false;
                m_fCallback = nullptr;
            }
            else
            {
                return TPTASK_STATE_COMPLETED;
            }
        }

        if (m_runActorGroup == m_nextActorGroup)
        {
            if (m_fCallback)
            {
                m_fCallback(m_iRet, m_strSelectRes);
            }

            return TPTASK_STATE_COMPLETED;
        }
        return TPTASK_STATE_CONTINUE_CHILDTHREAD;
    }

public:
    NFrame::storesvr_updateobj m_stSelect; ///< 更新对象的信息。
    NFrame::storesvr_updateobj_res m_strSelectRes; ///< 更新操作的结果。
    UpdateObjCb m_fCallback; ///< 更新操作完成后的回调函数。
    int m_iRet; ///< 操作返回码。
};

class NFDbExecuteTask final : public NFDbTask
{
public:
    NFDbExecuteTask(const std::string& strServerId, const NFrame::storesvr_execute& stSelect, const ExecuteCb& fCallback) : NFDbTask(strServerId)
    {
        m_balanceId = stSelect.mod_key();
        m_stSelect = stSelect;
        m_fCallback = fCallback;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFDBExecuteTask) + std::string("_") + stSelect.baseinfo().tbname();
    }

    ~NFDbExecuteTask() override = default;

    /**
    **  异步线程处理函数，将在另一个线程里运行
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
    NFrame::storesvr_execute m_stSelect;
    NFrame::storesvr_execute_res m_stSelectRes;
    ExecuteCb m_fCallback;
    int m_iRet;
};

class NFDbExecuteMoreTask final : public NFDbTask
{
public:
    NFDbExecuteMoreTask(const std::string& strServerId, const NFrame::storesvr_execute_more& stSelect, const ExecuteMoreCb& fCallback) : NFDbTask(strServerId)
    {
        m_balanceId = 0;
        m_stSelect = stSelect;
        m_fCallback = fCallback;
        m_iRet = 0;
        m_taskName = GET_CLASS_NAME(NFDBExecuteMoreTask);
    }

    ~NFDbExecuteMoreTask() override = default;

    /**
    **  异步线程处理函数，将在另一个线程里运行
    */
    bool ThreadProcess() override
    {
        if (m_pMysqlDriver)
        {
            m_iRet = m_pMysqlDriver->ExecuteMore(m_stSelect, m_stSelectRes);
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
        for (int i = 0; i < m_stSelectRes.size(); i++)
        {
            if (m_fCallback)
            {
                m_fCallback(m_iRet, *m_stSelectRes.Mutable(i));
            }
        }

        return TPTASK_STATE_COMPLETED;
    }

public:
    NFrame::storesvr_execute_more m_stSelect;
    google::protobuf::RepeatedPtrField<NFrame::storesvr_execute_more_res> m_stSelectRes;
    ExecuteMoreCb m_fCallback;
    int m_iRet;
};

/**
 * @class NFDbTaskComponent
 * @brief 该类继承自NFITaskComponent，用于管理与数据库相关的任务。
 *
 * 该类主要负责管理MySQL和NoSQL数据库的连接、任务处理以及资源释放。
 */
class NFDbTaskComponent final : public NFITaskComponent
{
public:
    /**
     * @brief 构造函数，初始化MySQL和NoSQL驱动管理器。
     */
    NFDbTaskComponent()
    {
        m_pMysqlDriverManager = NF_NEW NFCMysqlDriverManager();
        m_pNoSqlDriverManager = NF_NEW NFCNosqlDriverManager();
    }

    /**
     * @brief 析构函数，释放MySQL和NoSQL驱动管理器。
     */
    ~NFDbTaskComponent() override
    {
        NF_SAFE_DELETE(m_pMysqlDriverManager);
        NF_SAFE_DELETE(m_pNoSqlDriverManager);
    }

    /**
     * @brief 处理任务开始时的逻辑。
     * @param pTask 指向NFTask对象的指针，表示要处理的任务。
     *
     * 该函数根据任务类型执行不同的操作，包括连接数据库、检查数据库状态以及获取数据库驱动。
     */
    void ProcessTaskStart(NFTask* pTask) override
    {
        auto pMysqlTask = dynamic_cast<NFDbTask*>(pTask);
        if (pMysqlTask)
        {
            if (pMysqlTask->IsConnect())
            {
                auto pConnectTask = dynamic_cast<NFDbConnectTask*>(pTask);
                if (pConnectTask == nullptr) return;
                int iRet = m_pMysqlDriverManager->AddMysqlServer(pConnectTask->m_strServerId, pConnectTask->m_strIp, pConnectTask->m_iPort,
                                                                 pConnectTask->m_strDbName, pConnectTask->m_strDbUser,
                                                                 pConnectTask->m_strDbPwd, pConnectTask->m_iReconnectTime, pConnectTask->m_iReconnectCount);
                if (iRet != 0)
                {
                    NFSLEEP(1000);
                    exit(0);
                }

                iRet = m_pNoSqlDriverManager->AddNosqlServer(pConnectTask->m_strServerId, pConnectTask->m_strNosqlIp, pConnectTask->m_iNosqlPort,
                                                             pConnectTask->m_strNosqlPass);
                if (iRet != 0)
                {
                    NFSLEEP(1000);
                    exit(0);
                }
            }
            else if (pMysqlTask->IsCheck())
            {
                m_pMysqlDriverManager->CheckMysql();
                m_pNoSqlDriverManager->CheckNoSql();
            }
            else
            {
                m_pMysqlDriverManager->CheckMysql();
                m_pNoSqlDriverManager->CheckNoSql();
                pMysqlTask->m_pMysqlDriver = m_pMysqlDriverManager->GetMysqlDriver(pMysqlTask->m_strServerId);
                CHECK_EXPR(pMysqlTask->m_pMysqlDriver, , "GetMysqlDriver:{} Failed", pMysqlTask->m_strServerId);
                pMysqlTask->m_pNosqlDriver = m_pNoSqlDriverManager->GetNosqlDriver(pMysqlTask->m_strServerId);
                CHECK_EXPR(pMysqlTask->m_pNosqlDriver, , "GetNosqlDriver:{} Failed", pMysqlTask->m_strServerId);
            }
        }
    }

    /**
     * @brief 处理任务的逻辑。
     * @param pTask 指向NFTask对象的指针，表示要处理的任务。
     *
     * 该函数调用任务的线程处理函数。
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
     * @param pTask 指向NFTask对象的指针，表示要处理的任务。
     *
     * 该函数在任务结束时释放与任务相关的数据库驱动。
     */
    void ProcessTaskEnd(NFTask* pTask) override
    {
        const auto pMysqlTask = dynamic_cast<NFDbTask*>(pTask);
        if (pMysqlTask)
        {
            pMysqlTask->m_pMysqlDriver = nullptr;
            pMysqlTask->m_pNosqlDriver = nullptr;
        }
    }

    /**
     * @brief 处理任务超时的逻辑。
     * @param strTaskName 任务名称。
     * @param ullUseTime 任务使用的总时间。
     *
     * 该函数在任务超时时记录错误日志。
     */
    void HandleTaskTimeOut(const std::string& strTaskName, const uint64_t ullUseTime) override
    {
        NFLogError(NF_LOG_DEFAULT, 0, "strTaskName:{} timeOut, userTime:{}", strTaskName, ullUseTime);
    }

public:
    NFCMysqlDriverManager* m_pMysqlDriverManager; ///< MySQL驱动管理器
    NFCNosqlDriverManager* m_pNoSqlDriverManager; ///< NoSQL驱动管理器
};


NFCAsyDbModule::NFCAsyDbModule(NFIPluginManager* pPluginManager) : NFIAsyDbModule(pPluginManager)
{
    m_ullLastCheckTime = NFGetTime();
    m_bInitComponent = false;
}

NFCAsyDbModule::~NFCAsyDbModule() = default;

/**
 * @brief 初始化Actor池
 *
 * 该函数用于初始化Actor池，确保每个任务组中的每个Actor都有一个对应的任务组件。
 * 如果组件尚未初始化，则创建并添加任务组件到每个Actor中。
 *
 * @param iMaxTaskGroup 最大任务组数，用于初始化父类的Actor池
 * @param iMaxActorNum 每个任务组中的最大Actor数，用于初始化父类的Actor池
 * @return bool 始终返回true，表示初始化成功
 */
bool NFCAsyDbModule::InitActorPool(int iMaxTaskGroup, int iMaxActorNum)
{
    // 调用父类的InitActorPool方法，初始化任务组和Actor数量
    NFIAsyModule::InitActorPool(iMaxTaskGroup, iMaxActorNum);

    // 检查组件是否已经初始化，如果没有则进行初始化
    if (!m_bInitComponent)
    {
        m_bInitComponent = true;

        // 遍历所有任务组和每个任务组中的Actor
        for (size_t i = 0; i < m_stVecActorGroupPool.size(); i++)
        {
            for (size_t j = 0; j < m_stVecActorGroupPool[i].size(); j++)
            {
                // 为每个Actor创建一个新的任务组件
                const auto pComponent = NF_NEW NFDbTaskComponent();

                // 将任务组件添加到对应的Actor中
                AddActorComponent(i, m_stVecActorGroupPool[i][j], pComponent);
            }
        }
    }

    return true;
}


int NFCAsyDbModule::AddDbServer(const std::string& strServerId, const std::string& strIp, int iPort, const std::string& strDbName,
                                const std::string& strDbUser, const std::string& strDbPwd, const std::string& strNoSqlIp, int iNosqlPort,
                                const std::string& strNoSqlPass, int iReconnectTime,
                                int iReconnectCount)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    InitActorPool(NF_ASY_TASK_MAX_GROUP);

    for (size_t i = 0; i < m_stVecActorGroupPool.size(); i++)
    {
        for (size_t j = 0; j < m_stVecActorGroupPool[i].size(); j++)
        {
            auto pTask = NF_NEW NFDbConnectTask();
            pTask->m_strServerId = strServerId;
            pTask->m_strIp = strIp;
            pTask->m_iPort = iPort;
            pTask->m_strDbName = strDbName;
            pTask->m_strDbUser = strDbUser;
            pTask->m_strDbPwd = strDbPwd;
            pTask->m_iReconnectTime = iReconnectTime;
            pTask->m_iReconnectCount = iReconnectCount;
            pTask->m_strNosqlIp = strNoSqlIp;
            pTask->m_iNosqlPort = iNosqlPort;
            pTask->m_strNosqlPass = strNoSqlPass;
            int iRet = FindModule<NFITaskModule>()->AddTask(i, m_stVecActorGroupPool[i][j], pTask);
            CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
        }
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCAsyDbModule::SelectByCond(const std::string& strServerId, const NFrame::storesvr_sel& stSelect, bool bUseCache,
                                 const SelectByCondCb& bCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto* pTask = NF_NEW NFDbSelectByCondTask(strServerId, stSelect, bCallback, bUseCache);
    int iRet = AddTask(NF_ASY_TASK_READ_GROUP, pTask);
    CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}


int NFCAsyDbModule::SelectObj(const std::string& strServerId, const NFrame::storesvr_selobj& stSelect, bool bUseCache,
                              const SelectObjCb& bCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto* pTask = NF_NEW NFDbSelectObjTask(strServerId, stSelect, bCallback, bUseCache);
    int iRet = AddTask(NF_ASY_TASK_READ_GROUP, pTask);
    CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}


int NFCAsyDbModule::DeleteByCond(const std::string& strServerId, const NFrame::storesvr_del& stSelect, bool bUseCache,
                                 const DeleteByCondCb& bCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    if (bUseCache)
    {
        auto* pTask = NF_NEW NFDbDeleteByCondTask(strServerId, stSelect, bCallback, bUseCache);
        int iRet = AddTask(NF_ASY_TASK_READ_GROUP, pTask);
        CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    }
    else
    {
        auto* pTask = NF_NEW NFDbDeleteByCondTask(strServerId, stSelect, bCallback, bUseCache);
        int iRet = AddTask(NF_ASY_TASK_WRITE_GROUP, pTask);
        CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}


int NFCAsyDbModule::DeleteObj(const std::string& strServerId, const NFrame::storesvr_delobj& stSelect, bool bUseCache,
                              const DeleteObjCb& bCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    if (bUseCache)
    {
        auto* pTask = NF_NEW NFDbDeleteObjTask(strServerId, stSelect, bCallback, bUseCache);
        int iRet = AddTask(NF_ASY_TASK_READ_GROUP, pTask);
        CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    }
    else
    {
        auto* pTask = NF_NEW NFDbDeleteObjTask(strServerId, stSelect, bCallback, bUseCache);
        int iRet = AddTask(NF_ASY_TASK_WRITE_GROUP, pTask);
        CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}


int NFCAsyDbModule::InsertObj(const std::string& strServerId, const NFrame::storesvr_insertobj& stSelect, bool bUseCache,
                              const InsertObjCb& fCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto* pTask = NF_NEW NFDbInsertObjTask(strServerId, stSelect, fCallback, bUseCache);
    int iRet = AddTask(NF_ASY_TASK_WRITE_GROUP, pTask);
    CHECK_EXPR(iRet == 0, -1, "AddTask Failed");

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCAsyDbModule::ModifyByCond(const std::string& strServerId, const NFrame::storesvr_mod& stSelect, bool bUseCache,
                                 const ModifyByCondCb& fCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    if (bUseCache)
    {
        auto* pTask = NF_NEW NFDbModifyByCondTask(strServerId, stSelect, fCallback, bUseCache);
        int iRet = AddTask(NF_ASY_TASK_READ_GROUP, pTask);
        CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    }
    else
    {
        auto* pTask = NF_NEW NFDbModifyByCondTask(strServerId, stSelect, fCallback, bUseCache);
        int iRet = AddTask(NF_ASY_TASK_WRITE_GROUP, pTask);
        CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCAsyDbModule::ModifyObj(const std::string& strServerId, const NFrame::storesvr_modobj& stSelect, bool bUseCache,
                              const ModifyObjCb& bCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    if (bUseCache)
    {
        auto* pTask = NF_NEW NFDbModifyObjTask(strServerId, stSelect, bCallback, bUseCache);
        int iRet = AddTask(NF_ASY_TASK_READ_GROUP, pTask);
        CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    }
    else
    {
        auto* pTask = NF_NEW NFDbModifyObjTask(strServerId, stSelect, bCallback, bUseCache);
        int iRet = AddTask(NF_ASY_TASK_WRITE_GROUP, pTask);
        CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCAsyDbModule::UpdateByCond(const std::string& strServerId, const NFrame::storesvr_update& stSelect, bool bUseCache,
                                 const UpdateByCondCb& fCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    if (bUseCache)
    {
        auto* pTask = NF_NEW NFDbUpdateByCondTask(strServerId, stSelect, fCallback, bUseCache);
        int iRet = AddTask(NF_ASY_TASK_READ_GROUP, pTask);
        CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    }
    else
    {
        auto* pTask = NF_NEW NFDbUpdateByCondTask(strServerId, stSelect, fCallback, bUseCache);
        int iRet = AddTask(NF_ASY_TASK_WRITE_GROUP, pTask);
        CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCAsyDbModule::UpdateObj(const std::string& strServerId, const NFrame::storesvr_updateobj& stSelect, bool bUseCache,
                              const UpdateObjCb& fCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    if (bUseCache)
    {
        auto* pTask = NF_NEW NFDbUpdateObjTask(strServerId, stSelect, fCallback, bUseCache);
        int iRet = AddTask(NF_ASY_TASK_READ_GROUP, pTask);
        CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    }
    else
    {
        auto* pTask = NF_NEW NFDbUpdateObjTask(strServerId, stSelect, fCallback, bUseCache);
        int iRet = AddTask(NF_ASY_TASK_WRITE_GROUP, pTask);
        CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCAsyDbModule::Execute(const std::string& strServerId, const NFrame::storesvr_execute& stSelect,
                            const ExecuteCb& fCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto* pTask = NF_NEW NFDbExecuteTask(strServerId, stSelect, fCallback);
    int iRet = AddTask(NF_ASY_TASK_WRITE_GROUP, pTask);
    CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

int NFCAsyDbModule::ExecuteMore(const std::string& strServerId, const NFrame::storesvr_execute_more& stSelect,
                                const ExecuteMoreCb& fCallback)
{
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- begin -- ");
    auto* pTask = NF_NEW NFDbExecuteMoreTask(strServerId, stSelect, fCallback);
    int iRet = AddTask(NF_ASY_TASK_WRITE_GROUP, pTask);
    CHECK_EXPR(iRet == 0, -1, "AddTask Failed");
    NFLogTrace(NF_LOG_DEFAULT, 0, "--- end -- ");
    return 0;
}

bool NFCAsyDbModule::Execute()
{
    if (!m_bInitComponent) return true;
    if (NFGetTime() - m_ullLastCheckTime < 10000) return true;

    m_ullLastCheckTime = NFGetTime();

    for (int i = 0; i < static_cast<int>(m_stVecActorGroupPool.size()); i++)
    {
        for (int j = 0; j < static_cast<int>(m_stVecActorGroupPool[i].size()); j++)
        {
            auto* pTask = NF_NEW NFDbCheckTask();
            FindModule<NFITaskModule>()->AddTask(i, m_stVecActorGroupPool[i][j], pTask);
        }
    }

    return true;
}
