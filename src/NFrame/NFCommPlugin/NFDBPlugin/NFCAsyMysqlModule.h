// -------------------------------------------------------------------------
//    @FileName         :    NFCAsyMysqlModule.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Module           :    NFCAsyMysqlModule
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFPluginModule/NFEventDefine.h"
#include "NFComm/NFPluginModule/NFIAsyMysqlModule.h"

/**
 * @class NFCAsyMysqlModule
 * @brief NFCAsyMysqlModule 类是一个异步 MySQL 模块的实现类，继承自 NFIAsyMysqlModule 接口。
 *
 * 该类负责处理与 MySQL 数据库的异步交互，提供了异步执行 SQL 查询、更新等操作的功能。
 * 通过继承 NFIAsyMysqlModule 接口，该类实现了异步数据库操作的核心逻辑。
 *
 * @note 该类被标记为 final，表示不允许其他类继承自该类。
 */
class NFCAsyMysqlModule final : public NFIAsyMysqlModule
{
public:
    /**
     * @brief NFCAsyMysqlModule 构造函数
     * @param pPluginManager 插件管理器的指针，用于管理插件的生命周期和依赖关系
     */
    explicit NFCAsyMysqlModule(NFIPluginManager* pPluginManager);

    /**
     * @brief NFCAsyMysqlModule 析构函数
     * 用于释放资源并清理模块相关的内存
     */
    ~NFCAsyMysqlModule() override;

    /**
     * @brief Execute 函数
     * 执行模块的主要逻辑，通常在每个主循环中被调用
     * @return bool 返回执行结果，true 表示成功，false 表示失败
     */
    bool Execute() override;

    /**
     * @brief InitActorPool 函数
     * 初始化异步任务池，用于管理异步任务的执行
     * @param iMaxTaskGroup 最大任务组数，用于控制并发任务的数量
     * @param iMaxActorNum 最大任务数，默认为0，表示不限制任务数量
     * @return bool 返回初始化结果，true 表示成功，false 表示失败
     */
    bool InitActorPool(int iMaxTaskGroup, int iMaxActorNum = 0) override;

    /**
     * @brief 添加一个MySQL服务器配置
     *
     * 该函数用于向系统中添加一个MySQL服务器的配置信息，包括服务器ID、IP地址、端口号、数据库名称、
     * 数据库用户名、数据库密码等。还可以设置重连时间和重连次数。
     *
     * @param strServerId 服务器ID，用于唯一标识一个MySQL服务器
     * @param strIp MySQL服务器的IP地址
     * @param iPort MySQL服务器的端口号
     * @param strDbName 要连接的数据库名称
     * @param strDbUser 连接数据库的用户名
     * @param strDbPwd 连接数据库的密码
     * @param iReconnectTime 重连时间间隔，单位为秒，默认值为10秒
     * @param iReconnectCount 重连次数，默认值为-1，表示无限重连
     *
     * @return 返回一个整型值，表示操作结果。通常返回0表示成功，非0表示失败。
     */
    int AddMysqlServer(const std::string& strServerId, const std::string& strIp, int iPort, std::string strDbName,
                       std::string strDbUser, std::string strDbPwd, int iReconnectTime = 10,
                       int iReconnectCount = -1) override;

    /**
     * @brief 根据条件查询数据
     *
     * 该函数用于根据给定的条件从指定服务器中查询数据，并可以选择是否使用缓存。
     *
     * @param strServerId 服务器ID，标识要查询的服务器
     * @param stSelect 查询条件，包含查询的具体条件和参数
     * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
     * @param fCallback 查询完成后的回调函数，用于处理查询结果
     * @return int 返回操作结果，通常为错误码或状态码
     */
    int SelectByCond(const std::string& strServerId, const NFrame::storesvr_sel& stSelect, bool bUseCache, const SelectByCondCb& fCallback) override;

    /**
     * @brief 查询单个对象
     *
     * 该函数用于从指定服务器中查询单个对象，并可以选择是否使用缓存。
     *
     * @param strServerId 服务器ID，标识要查询的服务器
     * @param stSelect 查询条件，包含查询的具体条件和参数
     * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
     * @param fCallback 查询完成后的回调函数，用于处理查询结果
     * @return int 返回操作结果，通常为错误码或状态码
     */
    int SelectObj(const std::string& strServerId, const NFrame::storesvr_selobj& stSelect, bool bUseCache, const SelectObjCb& fCallback) override;

    /**
     * @brief 根据条件删除数据
     *
     * 该函数用于根据给定的条件从指定服务器中删除数据，并可以选择是否使用缓存。
     *
     * @param strServerId 服务器ID，标识要删除数据的服务器
     * @param stSelect 删除条件，包含删除的具体条件和参数
     * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
     * @param fCallback 删除完成后的回调函数，用于处理删除结果
     * @return int 返回操作结果，通常为错误码或状态码
     */
    int DeleteByCond(const std::string& strServerId, const NFrame::storesvr_del& stSelect, bool bUseCache, const DeleteByCondCb& fCallback) override;

    /**
     * @brief 删除单个对象
     *
     * 该函数用于从指定服务器中删除单个对象，并可以选择是否使用缓存。
     *
     * @param strServerId 服务器ID，标识要删除对象的服务器
     * @param stSelect 删除条件，包含删除的具体条件和参数
     * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
     * @param fCallback 删除完成后的回调函数，用于处理删除结果
     * @return int 返回操作结果，通常为错误码或状态码
     */
    int DeleteObj(const std::string& strServerId, const NFrame::storesvr_delobj& stSelect, bool bUseCache, const DeleteObjCb& fCallback) override;

    /**
     * @brief 插入对象到指定服务器
     *
     * 该函数用于将对象插入到指定的服务器中。可以选择是否使用缓存，并在操作完成后通过回调函数返回结果。
     *
     * @param strServerId 目标服务器的唯一标识符。
     * @param stSelect 包含插入对象的相关信息，如对象数据、插入条件等。
     * @param bUseCache 是否使用缓存进行插入操作。true表示使用缓存，false表示不使用。
     * @param fCallback 插入操作完成后的回调函数，用于处理插入结果。
     * @return int 返回操作结果的状态码，0表示成功，非0表示失败。
     */
    int InsertObj(const std::string& strServerId, const NFrame::storesvr_insertobj& stSelect, bool bUseCache, const InsertObjCb& fCallback) override;

    /**
     * @brief 根据条件修改对象
     *
     * 该函数用于根据指定条件修改服务器中的对象。可以选择是否使用缓存，并在操作完成后通过回调函数返回结果。
     *
     * @param strServerId 目标服务器的唯一标识符。
     * @param stSelect 包含修改条件及相关信息，如修改内容、条件表达式等。
     * @param bUseCache 是否使用缓存进行修改操作。true表示使用缓存，false表示不使用。
     * @param fCallback 修改操作完成后的回调函数，用于处理修改结果。
     * @return int 返回操作结果的状态码，0表示成功，非0表示失败。
     */
    int ModifyByCond(const std::string& strServerId, const NFrame::storesvr_mod& stSelect, bool bUseCache, const ModifyByCondCb& fCallback) override;

    /**
    * @brief 修改对象数据
    *
    * 该函数用于根据给定的服务器ID和选择条件，修改对象数据。可以选择是否使用缓存，并通过回调函数返回结果。
    *
    * @param strServerId 服务器ID，用于指定目标服务器。
    * @param stSelect 选择条件，包含用于查找和修改对象的相关信息。
    * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用缓存。
    * @param fCallback 回调函数，用于在操作完成后返回结果或错误信息。
    *
    * @return 返回操作结果，通常为错误码或状态码。具体返回值取决于实现。
    */
    int ModifyObj(const std::string& strServerId, const NFrame::storesvr_modobj& stSelect, bool bUseCache, const ModifyObjCb& fCallback) override;

    /**
     * @brief 根据条件更新数据
     *
     * 该函数用于根据指定的条件更新数据库中的数据。可以通过指定是否使用缓存来优化性能。
     *
     * @param strServerId 服务器ID，用于标识目标服务器
     * @param stSelect 更新条件，包含需要更新的数据和条件
     * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
     * @param fCallback 回调函数，用于处理更新操作完成后的结果
     * @return int 返回操作结果，通常为错误码或状态码
     */
    int UpdateByCond(const std::string& strServerId, const NFrame::storesvr_update& stSelect, bool bUseCache, const UpdateByCondCb& fCallback) override;

    /**
     * @brief 更新对象数据
     *
     * 该函数用于更新数据库中的对象数据。可以通过指定是否使用缓存来优化性能。
     *
     * @param strServerId 服务器ID，用于标识目标服务器
     * @param stSelect 更新条件，包含需要更新的对象数据
     * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
     * @param fCallback 回调函数，用于处理更新操作完成后的结果
     * @return int 返回操作结果，通常为错误码或状态码
     */
    int UpdateObj(const std::string& strServerId, const NFrame::storesvr_updateobj& stSelect, bool bUseCache, const UpdateObjCb& fCallback) override;

    /**
     * @brief 执行数据库操作
     *
     * 该函数用于执行指定的数据库操作，通常用于执行查询或更新操作。
     *
     * @param strServerId 服务器ID，用于标识目标服务器
     * @param stSelect 执行条件，包含需要执行的操作和数据
     * @param fCallback 回调函数，用于处理操作完成后的结果
     * @return int 返回操作结果，通常为错误码或状态码
     */
    int Execute(const std::string& strServerId, const NFrame::storesvr_execute& stSelect, const ExecuteCb& fCallback) override;

    /**
     * @brief 执行多个数据库操作
     *
     * 该函数用于执行多个指定的数据库操作，通常用于批量处理查询或更新操作。
     *
     * @param strServerId 服务器ID，用于标识目标服务器
     * @param stSelect 执行条件，包含需要执行的多个操作和数据
     * @param fCallback 回调函数，用于处理操作完成后的结果
     * @return int 返回操作结果，通常为错误码或状态码
     */
    int ExecuteMore(const std::string& strServerId, const NFrame::storesvr_execute_more& stSelect, const ExecuteMoreCb& fCallback) override;

private:
    /**
     * @brief 上次检查时间的存储变量。
     *
     * 该变量用于存储上次进行检查的时间戳，通常用于记录某个事件或操作的最后发生时间。
     * 时间戳通常以64位无符号整数表示，单位为毫秒或微秒。
     */
    uint64_t m_ullLastCheckTime;

    /**
     * @brief 组件初始化状态的标志变量。
     *
     * 该布尔变量用于标识某个组件是否已经完成初始化。
     * 如果为true，表示组件已经初始化完成；如果为false，表示组件尚未初始化或初始化未完成。
     */
    bool m_bInitComponent;
};
