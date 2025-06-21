// -------------------------------------------------------------------------
//    @FileName         :    NFCLogModule.h
//    @Author           :    Yi.Gao
//    @Date             :   2022-09-18
//    @Module           :    NFCLogModule
//    @Desc             :
// -------------------------------------------------------------------------
#pragma once

#include "NFComm/NFPluginModule/NFILogModule.h"
#include "NFComm/NFCore/NFPlatform.h"

#include "common/spdlog/spdlog.h"
#include "common/spdlog/fmt/fmt.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>


class NFCLogModule : public NFILogModule
{
public:
	NFCLogModule(NFIPluginManager* p);

	virtual ~NFCLogModule();

	virtual bool Shut() override;
	virtual bool OnReloadConfig() override;

	/**
	* @brief 初始化log系统
	*/
	virtual void InitLogSystem() override;
	/**
	* @brief 对外接口输出默认的LOG
	*
	* @param  log_level log等级
	* @param  function 
	* @param  line
	* @param  logId LOG选项ID，可以配置输出
	* @param  guid 一般是玩家ID，某些情况下，只想输出一个玩家的LOG
	* @param  log
	* @return void
	*/
	virtual void LogDefault(NF_LOG_LEVEL log_level, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, const std::string& log);

	virtual void LogDefault(NF_LOG_LEVEL log_level, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, uint32_t module, const std::string& log);

	virtual void LogBehaviour(NF_LOG_LEVEL log_level, uint32_t logId, const std::string& log);

	/**
	* @brief 是否输出对应等级的logId的log
	*
	* @param  log_level log等级
	* @param  logId LOG选项ID，可以配置输出
	* @param  guid 一般是玩家ID，某些情况下，只想输出一个玩家的LOG
	* @return bool
	*/
	virtual bool IsLogIdEnable(NF_LOG_LEVEL log_level, uint32_t logId);

	/**
	* @brief 对外接口输出默认的LOG
	*
	* @param  log_level log等级
	* @param  logId LOG选项ID，可以配置输出
	* @param  guid 一般是玩家ID，某些情况下，只想输出一个玩家的LOG
	* @param  log
	* @return bool
	*/
	virtual void LogDefault(NF_LOG_LEVEL log_level, uint32_t logId, uint64_t guid, const std::string& log);


	/**
	* @brief 设置log的配置
	*
	* @param  vecLogConfig
	* @return
	*/
	virtual void SetDefaultLogConfig();

	/**
	* @brief 创建别的LOG系统
	*
	* @param  logId log文件唯一的ID
	* @param  logName log文件名
	* @param  async 是否异步，多线程
	* @return bool
	*/
	std::shared_ptr<spdlog::logger> CreateLogger(uint32_t logId, const std::string& logName, bool async, uint32_t level = spdlog::level::level_enum::debug);

	std::shared_ptr<spdlog::logger> CreateLogger(const LogInfoConfig& logInfo);
protected:
	/*创建默认系统LOG系统*/
	void CreateDefaultLogger();
private:
	std::shared_ptr<spdlog::logger> m_defaultLogger;
	std::vector<std::shared_ptr<spdlog::logger>> m_loggerMap;
	std::vector<LogInfoConfig> m_logInfoConfig;
	NF_SHARE_PTR<spdlog::details::thread_pool> m_logThreadPool;
	std::string m_pid;
};


