// -------------------------------------------------------------------------
//    @FileName         :    NFLogMgr.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------
#include "NFLogMgr.h"
#include "NFILogModule.h"
#include "common/spdlog/spdlog.h"
#include "common/spdlog/sinks/basic_file_sink.h"
#include "common/spdlog/sinks/ansicolor_sink.h"
#include "pb.h"

NFLogMgr::NFLogMgr()
{
	m_pLogModule = nullptr;

}

NFLogMgr::~NFLogMgr()
{
	spdlog::drop_all();
}

bool NFLogMgr::Init(NFILogModule* pSpdlogModule)
{
    if (m_pLogModule == NULL)
    {
        m_pLogModule = pSpdlogModule;
    	// 初始化nanopb日志回调
    	g_nanopb_frompb_log_handle = NanoFromPbLogHandle;
    }
	return true;
}

void NFLogMgr::UnInit()
{
	m_pLogModule = nullptr;
}


void NFLogMgr::CreateNoLog()
{
	if (!m_noLogger)
	{
		std::vector<spdlog::sink_ptr> sinks_vec;
		std::string log_name = "default.log";
		auto date_and_hour_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_name);
#if NF_PLATFORM == NF_PLATFORM_WIN
		auto color_sink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
		sinks_vec.push_back(color_sink);
#else
		auto color_sink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
		sinks_vec.push_back(color_sink);
#endif
		sinks_vec.push_back(date_and_hour_sink);
		m_noLogger = std::make_shared<spdlog::logger>("default", std::begin(sinks_vec), std::end(sinks_vec));
		m_noLogger->set_level(spdlog::level::level_enum::trace);

		m_noLogger->set_pattern("%^[%l | %Y-%m-%d %H:%M:%S.%e] | %v%$");

		m_noLogger->flush_on(spdlog::level::trace);

		spdlog::register_logger(m_noLogger);
	}
}

void NFLogMgr::NoLog(NF_LOG_LEVEL logLevel, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, const std::string& log)
{
	std::string str = fmt::format("[{}:{}:{}] | [{}:{}] | {}", loc.filename, loc.line, loc.funcname, "default", guid, log);
	m_noLogger->log((spdlog::level::level_enum)logLevel, str.c_str());
}

void NanoFromPbLogHandle(const char *format, ...)
{
	static char strLogContent[4096];
	va_list ap;
	va_start(ap, format);
	vsnprintf(strLogContent, sizeof(strLogContent), format, ap);
	va_end(ap);

	NFLogWarning(NF_LOG_DEFAULT, 0, "{}", strLogContent);
}
