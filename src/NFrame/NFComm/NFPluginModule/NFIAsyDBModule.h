// -------------------------------------------------------------------------
//    @FileName         :    NFIAsyMysqlModule.h
//    @Author           :    LvSheng.Huang
//    @Date             :   2022-09-18
//    @Module           :    NFIAsyMysqlModule
//
//
// -------------------------------------------------------------------------

#pragma once

#include "NFIAsySqlModule.h"

class NFIAsyDbModule
	: public NFIAsySqlModule
{
public:
	explicit NFIAsyDbModule(NFIPluginManager* p) : NFIAsySqlModule(p)
	{

	}

	~NFIAsyDbModule() override = default;

	/**
	 * @brief 添加数据库服务器配置
	 * @param nServerID 服务器ID，用于唯一标识该数据库服务器
	 * @param strIP 数据库服务器的IP地址
	 * @param nPort 数据库服务器的端口号
	 * @param strDBName 数据库名称
	 * @param strDBUser 数据库用户名
	 * @param strDBPwd 数据库密码
	 * @param noSqlIp NoSQL数据库的IP地址
	 * @param nosqlPort NoSQL数据库的端口号
	 * @param noSqlPass NoSQL数据库的密码
	 * @param nReconnectTime 重连时间间隔，单位为秒
	 * @param nReconnectCount 重连次数，-1 表示无限重连
	 * @return 返回添加结果，通常为服务器ID或其他标识符
	 * 该函数用于添加数据库服务器的配置信息，以便后续的数据库连接和操作
	 */
	virtual int AddDbServer(const std::string& nServerID, const std::string& strIP, int nPort, const std::string& strDBName, const std::string& strDBUser, const std::string& strDBPwd,
	                        const std::string& noSqlIp, int nosqlPort, const std::string& noSqlPass,
	                        int nReconnectTime = 10, int nReconnectCount = -1) = 0;
};
