// -------------------------------------------------------------------------
//    @FileName         :    NFIAsyNosqlModule.h
//    @Author           :    gaoyi
//    @Date             :    23-8-15
//    @Email			:    445267987@qq.com
//    @Module           :    NFIAsyNosqlModule
//
// -------------------------------------------------------------------------

#pragma once

#include "NFIAsyModule.h"

class NFIAsyNosqlModule
        : public NFIAsyModule
{
public:
    NFIAsyNosqlModule(NFIPluginManager *p) : NFIAsyModule(p)
    {

    }

    virtual ~NFIAsyNosqlModule()
    {

    }
public:
    /**
     * @brief 添加Mysql链接
     *
     * @return bool					成功或失败
     */
    virtual int AddDBServer(const std::string& nServerID, const std::string& noSqlIp, int nosqlPort, const std::string& noSqlPass) = 0;

    /**
     * @brief 通过select结构体， 从数据库获取数据，并把结果放到selelct_res
     *
     * @param  select 查询语句
     * @param  message 表结构体
     * @param  select_res 查询结果
     * @return int =0执行成功, != 0失败
     */
    virtual int SelectObj(const std::string& nServerID, const NFrame::storesvr_selobj &select,
                          const SelectObjCb& cb) = 0;

    /**
     * @brief 通过select结构体， 从数据库获取数据，并把结果放到selelct_res
     *
     * @param  select 查询语句
     * @param  select_res 查询结果
     * @return int =0执行成功, != 0失败
     */
    virtual int DeleteObj(const std::string& nServerID, const NFrame::storesvr_delobj &select,
                          const DeleteObjCb& cb) = 0;

    /**
     * @brief 通过select结构体， 从数据库获取数据，并把结果放到selelct_res
     *
     * @param  select 查询语句
     * @param  select_res 查询结果
     * @return int =0执行成功, != 0失败
     */
    virtual int InsertObj(const std::string& nServerID, const NFrame::storesvr_insertobj &select,
                          const InsertObjCb& cb) = 0;

    virtual int ModifyObj(const std::string& nServerID, const NFrame::storesvr_modobj &select,
                          const ModifyObjCb& cb) = 0;

    virtual int UpdateObj(const std::string& nServerID, const NFrame::storesvr_updateobj &select,
                          const UpdateObjCb& cb) = 0;
};
