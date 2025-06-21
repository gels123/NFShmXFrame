// -------------------------------------------------------------------------
//    @FileName         :    NFCAsyMysqlModule.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Module           :    NFCAsyMysqlModule
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFPluginModule/NFIAsyDBModule.h"
#include "NFCMysqlDriverManager.h"
#include "NFCMysqlDriver.h"


/**
 * @brief 异步mysql
 */
class NFCAsyDbModule final : public NFIAsyDbModule
{
public:
	/**
	 * @brief NFCAsyDBModule 构造函数
	 * @param pPluginManager 插件管理器指针，用于管理模块的生命周期和依赖关系
	 */
	explicit NFCAsyDbModule(NFIPluginManager* pPluginManager);

	/**
	 * @brief NFCAsyDBModule 析构函数
	 * 负责释放模块占用的资源，清理内存等操作
	 */
	~NFCAsyDbModule() override;

	/**
	 * @brief 执行模块的主要逻辑
	 * @return 返回执行结果，true 表示成功，false 表示失败
	 * 该函数通常用于处理模块的核心业务逻辑，如数据库操作、网络通信等
	 */
	bool Execute() override;

	/**
	 * @brief 初始化Actor池
	 * @param iMaxTaskGroup 最大任务组数量，用于控制并发任务的数量
	 * @param iMaxActorNum 最大Actor数量，用于控制并发执行的任务数
	 * @return 返回初始化结果，true 表示成功，false 表示失败
	 * 该函数用于初始化Actor池，以便后续的任务调度和执行
	 */
	bool InitActorPool(int iMaxTaskGroup, int iMaxActorNum = 0) override;

	/**
	 * @brief 添加数据库服务器配置
	 * @param strServerId 服务器ID，用于唯一标识该数据库服务器
	 * @param strIp 数据库服务器的IP地址
	 * @param iPort 数据库服务器的端口号
	 * @param strDbName 数据库名称
	 * @param strDbUser 数据库用户名
	 * @param strDbPwd 数据库密码
	 * @param strNoSqlIp NoSQL数据库的IP地址
	 * @param iNosqlPort NoSQL数据库的端口号
	 * @param strNoSqlPass NoSQL数据库的密码
	 * @param iReconnectTime 重连时间间隔，单位为秒
	 * @param iReconnectCount 重连次数，-1 表示无限重连
	 * @return 返回添加结果，通常为服务器ID或其他标识符
	 * 该函数用于添加数据库服务器的配置信息，以便后续的数据库连接和操作
	 */
	int AddDbServer(const std::string& strServerId, const std::string& strIp, int iPort, const std::string& strDbName, const std::string& strDbUser, const std::string& strDbPwd,
	                const std::string& strNoSqlIp, int iNosqlPort, const std::string& strNoSqlPass,
	                int iReconnectTime = 10, int iReconnectCount = -1) override;

	/**
	 * @brief 根据条件从数据库中查询数据
	 * @param strServerId 服务器ID，用于标识目标服务器
	 * @param stSelect 查询条件对象，包含查询的具体条件
	 * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
	 * @param bCallback 查询完成后的回调函数，用于处理查询结果
	 * @return 返回操作结果，通常为错误码或状态码
	 */
	int SelectByCond(const std::string& strServerId, const NFrame::storesvr_sel& stSelect, bool bUseCache, const SelectByCondCb& bCallback) override;

	/**
	 * @brief 查询单个对象
	 * @param strServerId 服务器ID，用于标识目标服务器
	 * @param stSelect 查询条件对象，包含查询的具体条件
	 * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
	 * @param bCallback 查询完成后的回调函数，用于处理查询结果
	 * @return 返回操作结果，通常为错误码或状态码
	 */
	int SelectObj(const std::string& strServerId, const NFrame::storesvr_selobj& stSelect, bool bUseCache, const SelectObjCb& bCallback) override;

	/**
	 * @brief 根据条件删除数据
	 * @param strServerId 服务器ID，用于标识目标服务器
	 * @param stSelect 删除条件对象，包含删除的具体条件
	 * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
	 * @param bCallback 删除完成后的回调函数，用于处理删除结果
	 * @return 返回操作结果，通常为错误码或状态码
	 */
	int DeleteByCond(const std::string& strServerId, const NFrame::storesvr_del& stSelect, bool bUseCache, const DeleteByCondCb& bCallback) override;

	/**
	 * @brief 删除单个对象
	 * @param strServerId 服务器ID，用于标识目标服务器
	 * @param stSelect 删除条件对象，包含删除的具体条件
	 * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
	 * @param bCallback 删除完成后的回调函数，用于处理删除结果
	 * @return 返回操作结果，通常为错误码或状态码
	 */
	int DeleteObj(const std::string& strServerId, const NFrame::storesvr_delobj& stSelect, bool bUseCache, const DeleteObjCb& bCallback) override;

	/**
	 * @brief 插入单个对象
	 * @param strServerId 服务器ID，用于标识目标服务器
	 * @param stSelect 插入条件对象，包含插入的具体条件
	 * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
	 * @param fCallback 插入完成后的回调函数，用于处理插入结果
	 * @return 返回操作结果，通常为错误码或状态码
	 */
	int InsertObj(const std::string& strServerId, const NFrame::storesvr_insertobj& stSelect, bool bUseCache, const InsertObjCb& fCallback) override;

	/**
	 * @brief 根据条件修改数据
	 * @param strServerId 服务器ID，用于标识目标服务器
	 * @param stSelect 修改条件对象，包含修改的具体条件
	 * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
	 * @param fCallback 修改完成后的回调函数，用于处理修改结果
	 * @return 返回操作结果，通常为错误码或状态码
	 */
	int ModifyByCond(const std::string& strServerId, const NFrame::storesvr_mod& stSelect, bool bUseCache, const ModifyByCondCb& fCallback) override;

	/**
	 * @brief 修改单个对象
	 * @param strServerId 服务器ID，用于标识目标服务器
	 * @param stSelect 修改条件对象，包含修改的具体条件
	 * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
	 * @param bCallback 修改完成后的回调函数，用于处理修改结果
	 * @return 返回操作结果，通常为错误码或状态码
	 */
	int ModifyObj(const std::string& strServerId, const NFrame::storesvr_modobj& stSelect, bool bUseCache, const ModifyObjCb& bCallback) override;

	/**
	 * @brief 根据条件更新数据
	 * @param strServerId 服务器ID，用于标识目标服务器
	 * @param stSelect 更新条件对象，包含更新的具体条件
	 * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
	 * @param fCallback 更新完成后的回调函数，用于处理更新结果
	 * @return 返回操作结果，通常为错误码或状态码
	 */
	int UpdateByCond(const std::string& strServerId, const NFrame::storesvr_update& stSelect, bool bUseCache, const UpdateByCondCb& fCallback) override;

	/**
	 * @brief 更新单个对象
	 * @param strServerId 服务器ID，用于标识目标服务器
	 * @param stSelect 更新条件对象，包含更新的具体条件
	 * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
	 * @param fCallback 更新完成后的回调函数，用于处理更新结果
	 * @return 返回操作结果，通常为错误码或状态码
	 */
	int UpdateObj(const std::string& strServerId, const NFrame::storesvr_updateobj& stSelect, bool bUseCache, const UpdateObjCb& fCallback) override;

	/**
	 * @brief 执行数据库操作
	 * @param strServerId 服务器ID，用于标识目标服务器
	 * @param stSelect 执行条件对象，包含执行的具体条件
	 * @param fCallback 执行完成后的回调函数，用于处理执行结果
	 * @return 返回操作结果，通常为错误码或状态码
	 */
	int Execute(const std::string& strServerId, const NFrame::storesvr_execute& stSelect, const ExecuteCb& fCallback) override;

	/**
	 * @brief 执行多个数据库操作
	 * @param strServerId 服务器ID，用于标识目标服务器
	 * @param stSelect 执行条件对象，包含执行的具体条件
	 * @param fCallback 执行完成后的回调函数，用于处理执行结果
	 * @return 返回操作结果，通常为错误码或状态码
	 */
	int ExecuteMore(const std::string& strServerId, const NFrame::storesvr_execute_more& stSelect, const ExecuteMoreCb& fCallback) override;

private:
	/**
	 * @brief 最后一次检查时间的变量，用于记录某个操作或事件的上次检查时间。
	 *
	 * 该变量通常用于定时任务或周期性检查的场景，记录上一次执行检查的时间戳。
	 */
	uint64_t m_ullLastCheckTime;

	/**
	 * @brief 初始化组件状态的标志变量，用于标识组件是否已经完成初始化。
	 *
	 * 该变量通常用于控制组件的初始化流程，确保组件在首次使用时被正确初始化。
	 */
	bool m_bInitComponent;
};
