// -------------------------------------------------------------------------
//    @FileName         :    NFCLogModule.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Module           :    NFCLogModule
//    @Desc             :
// -------------------------------------------------------------------------

#include "NFComm/NFCore/NFPlatform.h"

#include "NFCLogModule.h"

#include "NFComm/NFPluginModule/NFIPluginManager.h"
#include "NFComm/NFPluginModule/NFIConfigModule.h"
#include "my_date_and_hour_file_sink.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include <iostream>
#include "common/spdlog/async_logger.h"
#include "common/spdlog/sinks/ansicolor_sink.h"
#include "NFComm/NFCore/NFCommon.h"
#include "NFComm/NFPluginModule/NFProtobufCommon.h"
#include "NFComm/NFPluginModule/NFIKernelModule.h"
#include "NFComm/NFCore/NFTime.h"


NFCLogModule::NFCLogModule(NFIPluginManager* p):NFILogModule(p)
{
	NFLogMgr::Instance()->Init(this);
#if NF_PLATFORM == NF_PLATFORM_WIN
	int proc_id = NFGetPID();
#else
    pid_t proc_id = NFGetPID();
#endif
    m_pid = NFCommon::tostr(proc_id);
}

/**
* @brief 初始化log系统
*/
void NFCLogModule::InitLogSystem()
{
	//只要是为了效率，浪费点内存
	m_loggerMap.resize(_NF_LOG_ID_ARRAYSIZE);
	m_logInfoConfig.resize(_NF_LOG_ID_ARRAYSIZE);
	m_logInfoConfig[NF_LOG_DEFAULT].mDisplay = true;
	m_logInfoConfig[NF_LOG_DEFAULT].mLogId = NF_LOG_DEFAULT;
	m_logInfoConfig[NF_LOG_DEFAULT].mLogName = "default";
	//Create spdlog
	try
	{
        m_logThreadPool = NF_SHARE_PTR<spdlog::details::thread_pool>(NF_NEW spdlog::details::thread_pool(1024, 1));

		CreateDefaultLogger();
	}
	catch (std::system_error& error)
	{
		std::cout << "Create logger failed, error = " << error.what() << std::endl;
		assert(false);
	}
}

NFCLogModule::~NFCLogModule()
{
	NFLogMgr::Instance()->UnInit();
	spdlog::drop_all();
}

bool NFCLogModule::Shut()
{
	return true;
}

bool NFCLogModule::OnReloadConfig()
{
	SetDefaultLogConfig();
	return true;
}

void NFCLogModule::LogDefault(NF_LOG_LEVEL log_level, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, const std::string& log)
{
	if (IsLogIdEnable(log_level, logId))
	{
		std::string str = fmt::format("[][] [{}:{}:{}] [{}] {}", loc.filename, loc.line, loc.funcname, guid, log);
		auto pLogger = m_defaultLogger;
		if (logId != NF_LOG_DEFAULT)
		{
			pLogger = m_loggerMap[logId];
			if (pLogger == NULL)
			{
				pLogger = m_defaultLogger;
			}
		}
		pLogger->log((spdlog::level::level_enum)log_level, str.c_str());
	}
}

void NFCLogModule::LogDefault(NF_LOG_LEVEL log_level, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, uint32_t module, const std::string& log)
{
	if (IsLogIdEnable(log_level, logId))
	{
		std::string str = fmt::format("[{}][] [{}:{}:{}] [{}] {}", module, loc.filename, loc.line, loc.funcname, guid, log);
		auto pLogger = m_defaultLogger;
		if (logId != NF_LOG_DEFAULT)
		{
			pLogger = m_loggerMap[logId];
			if (pLogger == NULL)
			{
				pLogger = m_defaultLogger;
			}
		}
		pLogger->log((spdlog::level::level_enum)log_level, str.c_str());
	}
}

void NFCLogModule::LogBehaviour(NF_LOG_LEVEL log_level, uint32_t logId, const std::string& log)
{
	if (IsLogIdEnable(log_level, logId))
	{
		auto pLogger = m_defaultLogger;
		if (logId != NF_LOG_DEFAULT)
		{
			pLogger = m_loggerMap[logId];
			if (pLogger == NULL)
			{
				pLogger = m_defaultLogger;
			}
		}
		pLogger->log((spdlog::level::level_enum)log_level, log.c_str());
	}
}

/**
* @brief 对外接口输出默认的LOG
*
* @param  log_level log等级
* @param  logId LOG选项ID，可以配置输出
* @param  guid 一般是玩家ID，某些情况下，只想输出一个玩家的LOG
* @param  log
* @return bool
*/
void NFCLogModule::LogDefault(NF_LOG_LEVEL log_level, uint32_t logId, uint64_t guid, const std::string& log)
{
	if (IsLogIdEnable(log_level, logId))
	{
		std::string str = fmt::format(" [{}] {}", guid, log);
		auto pLogger = m_defaultLogger;
		if (logId != NF_LOG_DEFAULT)
		{
			pLogger = m_loggerMap[logId];
			if (pLogger == NULL)
			{
				pLogger = m_defaultLogger;
			}
		}
		pLogger->log((spdlog::level::level_enum)log_level, str.c_str());
	}
}

bool NFCLogModule::IsLogIdEnable(NF_LOG_LEVEL log_level, uint32_t logId)
{
	if (logId < m_logInfoConfig.size() && m_logInfoConfig[logId].mDisplay)
	{
		return true;
	}
	return false;
}

/**
* @brief 设置log的配置
*
* @param  vecLogConfig
* @return
*/
void NFCLogModule::SetDefaultLogConfig()
{
	NFLogConfig* pLogConfig = FindModule<NFIConfigModule>()->GetLogConfig();
	for(size_t i = 0; i < pLogConfig->mLineConfigList.size(); i++)
	{
		LogInfoConfig& config = pLogConfig->mLineConfigList[i];
		if (config.mLogId < m_logInfoConfig.size())
		{
			m_logInfoConfig[config.mLogId] = config;
		}
	}

	for(int i = 0; i < (int)m_logInfoConfig.size(); i++)
	{
		auto pLogger = CreateLogger(m_logInfoConfig[i]);
		if (pLogger)
		{
			pLogger->set_level((spdlog::level::level_enum)m_logInfoConfig[i].mLevel);
		}
	}
}

void NFCLogModule::CreateDefaultLogger()
{
	m_defaultLogger = CreateLogger(m_logInfoConfig[NF_LOG_DEFAULT]);
}

std::shared_ptr<spdlog::logger> NFCLogModule::CreateLogger(const LogInfoConfig& logInfo)
{
	if (!logInfo.mDisplay)
	{
		return NULL;
	}

	return CreateLogger(logInfo.mLogId, logInfo.mLogName, true, logInfo.mLevel);
}

std::shared_ptr<spdlog::logger> NFCLogModule::CreateLogger(uint32_t logId, const std::string& logName, bool async, uint32_t level)
{
	CHECK_EXPR(logId < (uint32_t)m_loggerMap.size(), NULL, "logId:{}", logId);
	if (m_loggerMap[logId] != NULL)
	{
		return m_loggerMap[logId];
	}

	std::vector<spdlog::sink_ptr> sinks_vec;
	std::string log_name = NF_FORMAT("{}{}{}{}{}.log", m_pObjPluginManager->GetLogPath(), spdlog::details::os::folder_sep, m_pObjPluginManager->GetAppName() + "_" + m_pObjPluginManager->GetBusName(), spdlog::details::os::folder_sep, m_pObjPluginManager->GetAppName() + "_" + m_pObjPluginManager->GetBusName() + "_" + logName);
	auto date_and_hour_sink = std::make_shared<spdlog::sinks::my_date_and_hour_file_sink_mt>(log_name);

#if NF_PLATFORM == NF_PLATFORM_WIN
	auto color_sink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
	sinks_vec.push_back(color_sink);
#else
	if (m_pObjPluginManager->IsDaemon() == false)
	{
		auto color_sink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
		sinks_vec.push_back(color_sink);
	}
#endif

	sinks_vec.push_back(date_and_hour_sink);

	std::shared_ptr<spdlog::logger> pLogger;
#ifdef NF_DEBUG_MODE
	pLogger = std::make_shared<spdlog::logger>(m_pObjPluginManager->GetAppName() + "_" + m_pObjPluginManager->GetBusName()  + "_" + logName, std::begin(sinks_vec), std::end(sinks_vec));
#else
	if (async)
	{
		pLogger = std::make_shared<spdlog::async_logger>(m_pObjPluginManager->GetAppName() + "_" + m_pObjPluginManager->GetBusName() + "_" + logName, std::begin(sinks_vec), std::end(sinks_vec), m_logThreadPool);
	}
	else
	{
		pLogger = std::make_shared<spdlog::logger>(m_pObjPluginManager->GetAppName() + "_" + m_pObjPluginManager->GetBusName()  + "_" + logName, std::begin(sinks_vec), std::end(sinks_vec));
	}
#endif


	if (logId == NF_LOG_BEHAVIOUR)
	{
		std::string pattern = NF_FORMAT("%^%v%$", m_pObjPluginManager->GetBusName(), ThreadId());
		pLogger->set_pattern(pattern);
	}
	else
	{
		std::string pattern = NF_FORMAT("%^[%Y-%m-%d %H:%M:%S.%e] [%l] [{}][{}] %v%$", m_pObjPluginManager->GetBusName(), ThreadId());
		pLogger->set_pattern(pattern);
	}

	pLogger->set_level(spdlog::level::level_enum(level));
	pLogger->flush_on(spdlog::level::level_enum(level));

	spdlog::register_logger(pLogger);
	m_loggerMap[logId] = pLogger;
	return pLogger;
}