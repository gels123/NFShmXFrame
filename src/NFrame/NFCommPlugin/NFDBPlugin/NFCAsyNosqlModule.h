// -------------------------------------------------------------------------
//    @FileName         :    NFCAsyNoSqlModule.h
//    @Author           :    gaoyi
//    @Date             :    23-8-15
//    @Email            :    445267987@qq.com
//    @Module           :    NFCAsyNoSqlModule
//
// -------------------------------------------------------------------------

#pragma once

#include "NFCMysqlDriverManager.h"
#include "NFComm/NFPluginModule/NFIAsyNosqlModule.h"

class NFCAsyNosqlModule final : public NFIAsyNosqlModule
{
public:
    /**
     * @brief NFCAsyNosqlModule 构造函数
     * @param pPluginManager NFIPluginManager 指针，用于插件管理
     */
    explicit NFCAsyNosqlModule(NFIPluginManager* pPluginManager);

    /**
     * @brief NFCAsyNosqlModule 析构函数
     * 用于清理资源，释放内存
     */
    ~NFCAsyNosqlModule() override;

    /**
     * @brief 执行模块的主要逻辑
     * @return 返回执行结果，成功返回 true，失败返回 false
     */
    bool Execute() override;

    /**
     * @brief 初始化 Actor 池
     * @param iMaxTaskGroup 最大任务组数
     * @param iMaxActorNum 最大 Actor 数量，默认为 0
     * @return 返回初始化结果，成功返回 true，失败返回 false
     */
    bool InitActorPool(int iMaxTaskGroup, int iMaxActorNum = 0) override;

    /**
     * @brief 添加数据库服务器
     * @param strServerId 服务器 ID
     * @param strNoSqlIp NoSQL 数据库的 IP 地址
     * @param iNosqlPort NoSQL 数据库的端口号
     * @param strNoSqlPass NoSQL 数据库的密码
     * @return 返回操作结果，成功返回 0，失败返回其他值
     */
    int AddDBServer(const std::string& strServerId, const std::string& strNoSqlIp, int iNosqlPort, const std::string& strNoSqlPass) override;

    /**
     * @brief 查询对象
     * @param strServerId 服务器 ID
     * @param stSelect 查询条件
     * @param fCallback 查询完成后的回调函数
     * @return 返回操作结果，成功返回 0，失败返回其他值
     */
    int SelectObj(const std::string& strServerId, const NFrame::storesvr_selobj& stSelect, const SelectObjCb& fCallback) override;

    /**
     * @brief 删除对象
     * @param strServerId 服务器 ID
     * @param stSelect 删除条件
     * @param fCallback 删除完成后的回调函数
     * @return 返回操作结果，成功返回 0，失败返回其他值
     */
    int DeleteObj(const std::string& strServerId, const NFrame::storesvr_delobj& stSelect, const DeleteObjCb& fCallback) override;

    /**
     * @brief 插入对象
     * @param strServerId 服务器 ID
     * @param stSelect 插入条件
     * @param bCallback 插入完成后的回调函数
     * @return 返回操作结果，成功返回 0，失败返回其他值
     */
    int InsertObj(const std::string& strServerId, const NFrame::storesvr_insertobj& stSelect, const InsertObjCb& bCallback) override;

    /**
     * @brief 修改对象
     * @param strServerId 服务器 ID
     * @param stSelect 修改条件
     * @param fCallback 修改完成后的回调函数
     * @return 返回操作结果，成功返回 0，失败返回其他值
     */
    int ModifyObj(const std::string& strServerId, const NFrame::storesvr_modobj& stSelect, const ModifyObjCb& fCallback) override;

    /**
     * @brief 更新对象
     * @param strServerId 服务器 ID
     * @param stSelect 更新条件
     * @param fCallback 更新完成后的回调函数
     * @return 返回操作结果，成功返回 0，失败返回其他值
     */
    int UpdateObj(const std::string& strServerId, const NFrame::storesvr_updateobj& stSelect, const UpdateObjCb& fCallback) override;

private:
    uint64_t m_ullLastCheckTime;
    bool m_bInitComponent;
};
