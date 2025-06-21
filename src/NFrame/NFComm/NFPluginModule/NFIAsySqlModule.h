// -------------------------------------------------------------------------
//    @FileName         :    NFIAsySqlModule.h
//    @Author           :    gaoyi
//    @Date             :    23-9-12
//    @Email			:    445267987@qq.com
//    @Module           :    NFIAsySqlModule
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFPluginModule/NFIAsyModule.h"

enum {
    NF_ASY_TASK_READ_GROUP = 0,
    NF_ASY_TASK_WRITE_GROUP = 1,
    NF_ASY_TASK_MAX_GROUP = 2,
};

class NFIAsySqlModule : public NFIAsyModule {
public:
    explicit NFIAsySqlModule(NFIPluginManager* pPluginManager) : NFIAsyModule(pPluginManager)
    {
    }

    ~NFIAsySqlModule() override = default;

    /**
     * @brief 根据条件从数据库中查询数据
     * @param strServerId 服务器ID，用于指定操作的目标服务器
     * @param stSelect 查询条件，包含查询的具体参数和条件
     * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
     * @param bCallback 查询完成后的回调函数，用于处理查询结果
     * @return 返回操作结果的状态码，通常为0表示成功，非0表示失败
     */
    virtual int SelectByCond(const std::string& strServerId, const NFrame::storesvr_sel& stSelect, bool bUseCache, const SelectByCondCb& bCallback) = 0;

    /**
     * @brief 查询单个对象
     * @param strServerId 服务器ID，用于指定操作的目标服务器
     * @param stSelect 查询条件，包含查询的具体参数和条件
     * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
     * @param bCallback 查询完成后的回调函数，用于处理查询结果
     * @return 返回操作结果的状态码，通常为0表示成功，非0表示失败
     */
    virtual int SelectObj(const std::string& strServerId, const NFrame::storesvr_selobj& stSelect, bool bUseCache, const SelectObjCb& bCallback) = 0;

    /**
     * @brief 根据条件删除数据库中的数据
     * @param strServerId 服务器ID，用于指定操作的目标服务器
     * @param stSelect 删除条件，包含删除的具体参数和条件
     * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
     * @param bCallback 删除完成后的回调函数，用于处理删除结果
     * @return 返回操作结果的状态码，通常为0表示成功，非0表示失败
     */
    virtual int DeleteByCond(const std::string& strServerId, const NFrame::storesvr_del& stSelect, bool bUseCache, const DeleteByCondCb& bCallback) = 0;

    /**
     * @brief 删除单个对象
     * @param strServerId 服务器ID，用于指定操作的目标服务器
     * @param stSelect 删除条件，包含删除的具体参数和条件
     * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
     * @param bCallback 删除完成后的回调函数，用于处理删除结果
     * @return 返回操作结果的状态码，通常为0表示成功，非0表示失败
     */
    virtual int DeleteObj(const std::string& strServerId, const NFrame::storesvr_delobj& stSelect, bool bUseCache, const DeleteObjCb& bCallback) = 0;

    /**
     * @brief 插入单个对象到数据库中
     * @param strServerId 服务器ID，用于指定操作的目标服务器
     * @param stSelect 插入条件，包含插入的具体参数和条件
     * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
     * @param bCallback 插入完成后的回调函数，用于处理插入结果
     * @return 返回操作结果的状态码，通常为0表示成功，非0表示失败
     */
    virtual int InsertObj(const std::string& strServerId, const NFrame::storesvr_insertobj& stSelect, bool bUseCache, const InsertObjCb& bCallback) = 0;

    /**
     * @brief 根据条件修改数据库中的数据
     * @param strServerId 服务器ID，用于指定操作的目标服务器
     * @param stSelect 修改条件，包含修改的具体参数和条件
     * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
     * @param bCallback 修改完成后的回调函数，用于处理修改结果
     * @return 返回操作结果的状态码，通常为0表示成功，非0表示失败
     */
    virtual int ModifyByCond(const std::string& strServerId, const NFrame::storesvr_mod& stSelect, bool bUseCache, const ModifyByCondCb& bCallback) = 0;

    /**
     * @brief 修改单个对象
     * @param strServerId 服务器ID，用于指定操作的目标服务器
     * @param stSelect 修改条件，包含修改的具体参数和条件
     * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
     * @param bCallback 修改完成后的回调函数，用于处理修改结果
     * @return 返回操作结果的状态码，通常为0表示成功，非0表示失败
     */
    virtual int ModifyObj(const std::string& strServerId, const NFrame::storesvr_modobj& stSelect, bool bUseCache, const ModifyObjCb& bCallback) = 0;

    /**
     * @brief 根据条件更新数据库中的数据
     * @param strServerId 服务器ID，用于指定操作的目标服务器
     * @param stSelect 更新条件，包含更新的具体参数和条件
     * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
     * @param bCallback 更新完成后的回调函数，用于处理更新结果
     * @return 返回操作结果的状态码，通常为0表示成功，非0表示失败
     */
    virtual int UpdateByCond(const std::string& strServerId, const NFrame::storesvr_update& stSelect, bool bUseCache, const UpdateByCondCb& bCallback) = 0;

    /**
     * @brief 更新单个对象
     * @param strServerId 服务器ID，用于指定操作的目标服务器
     * @param stSelect 更新条件，包含更新的具体参数和条件
     * @param bUseCache 是否使用缓存，true表示使用缓存，false表示不使用
     * @param bCallback 更新完成后的回调函数，用于处理更新结果
     * @return 返回操作结果的状态码，通常为0表示成功，非0表示失败
     */
    virtual int UpdateObj(const std::string& strServerId, const NFrame::storesvr_updateobj& stSelect, bool bUseCache, const UpdateObjCb& bCallback) = 0;

    /**
     * @brief 执行数据库操作
     * @param strServerId 服务器ID，用于指定操作的目标服务器
     * @param stSelect 执行条件，包含执行的具体参数和条件
     * @param bCallback 执行完成后的回调函数，用于处理执行结果
     * @return 返回操作结果的状态码，通常为0表示成功，非0表示失败
     */
    virtual int Execute(const std::string& strServerId, const NFrame::storesvr_execute& stSelect, const ExecuteCb& bCallback) = 0;

    /**
     * @brief 执行多个数据库操作
     * @param strServerId 服务器ID，用于指定操作的目标服务器
     * @param stSelect 执行条件，包含执行的具体参数和条件
     * @param bCallback 执行完成后的回调函数，用于处理执行结果
     * @return 返回操作结果的状态码，通常为0表示成功，非0表示失败
     */
    virtual int ExecuteMore(const std::string& strServerId, const NFrame::storesvr_execute_more& stSelect, const ExecuteMoreCb& bCallback) = 0;
};
