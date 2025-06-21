#pragma once

#include "evpp/platform_config.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"

#ifdef __cplusplus
#define GOOGLE_GLOG_DLL_DECL           // 使用静态glog库时，必须定义这个
#define GLOG_NO_ABBREVIATED_SEVERITIES // 没这个编译会出错,传说因为和Windows.h冲突

//#include <glog/logging.h>

class EvppLog
{
public:
	EvppLog()
	{
		m_pInstance = static_cast<EvppLog*>(this);
	}

	static EvppLog* GetSingletonPtr()
	{
		if (nullptr == m_pInstance)
		{
			m_pInstance = new EvppLog;
		}

		return m_pInstance;
	}

	static EvppLog& GetSingletonRef()
	{
		return *GetSingletonPtr();
	}

	template<typename T>
	EvppLog& operator<<(const T& str)
	{
		return *this;
	}
public:
	static EvppLog* m_pInstance;
};

#ifdef GOOGLE_STRIP_LOG

#if GOOGLE_STRIP_LOG == 0
#define EVPP_LOG_TRACE LOG(INFO)
#define EVPP_LOG_DEBUG LOG(INFO)
#define EVPP_LOG_INFO  LOG(INFO)
#define DEVPP_LOG_TRACE LOG(INFO) << __PRETTY_FUNCTION__ << " this=" << this << " "
#else
#define EVPP_LOG_TRACE if (false) LOG(INFO)
#define EVPP_LOG_DEBUG if (false) LOG(INFO)
#define EVPP_LOG_INFO  if (false) LOG(INFO)
#define DEVPP_LOG_TRACE if (false) LOG(INFO)
#endif

#if GOOGLE_STRIP_LOG <= 1
#define EVPP_LOG_WARN  LOG(WARNING)
#define DEVPP_LOG_WARN LOG(WARNING) << __PRETTY_FUNCTION__ << " this=" << this << " "
#else
#define EVPP_LOG_WARN  if (false) LOG(WARNING)
#define DEVPP_LOG_WARN if (false) LOG(WARNING)
#endif

#define EVPP_LOG_ERROR LOG(ERROR)
#define EVPP_LOG_FATAL LOG(FATAL)

#else
#define EVPP_LOG_TRACE EvppLog::GetSingletonRef()
#define EVPP_LOG_DEBUG EvppLog::GetSingletonRef()
#define EVPP_LOG_INFO  EvppLog::GetSingletonRef()
#define EVPP_LOG_WARN  EvppLog::GetSingletonRef()
#define EVPP_LOG_ERROR EvppLog::GetSingletonRef()
#define EVPP_LOG_FATAL EvppLog::GetSingletonRef()
#define DEVPP_LOG_TRACE EvppLog::GetSingletonRef()
#define DEVPP_LOG_WARN EvppLog::GetSingletonRef()
#define CHECK_NOTnullptr(val) EVPP_LOG_ERROR << "'" #val "' Must be non nullptr";
#endif
#endif // end of define __cplusplus

//#ifdef _DEBUG
//#ifdef assert
//#undef assert
//#endif
//#define assert(expr)  { if (!(expr)) { EVPP_LOG_FATAL << #expr ;} }
//#endif