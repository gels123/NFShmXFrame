// -------------------------------------------------------------------------
//    @FileName         :    NFStackTrace.h
//    @Author           :    gaoyi
//    @Date             :    24-8-23
//    @Email            :    445267987@qq.com
//    @Module           :    NFStackTrace
//
// -------------------------------------------------------------------------

#pragma once

#include <NFComm/NFCore/NFSingleton.hpp>

#include "NFComm/NFCore/NFPlatform.h"

#define TRACE_STACK() (NFStackTrace::Instance()->TraceStack(true, true))

#ifdef NF_DEBUG_MODE
    #define DEBUG_TRACE_STACK() (NFStackTrace::Instance()->TraceStack(true, true))
#else
   //release版本直接不打印，因为此调用性能开销大
    #define DEBUG_TRACE_STACK() (SingleStackTrace::instance()->TraceStack(false, false))
#endif

#define GET_TRACE_STACK_EXEC_CNT() ((NFStackTrace::Instance()->GetExecCnt())
#define SET_TRACE_STACK_EXEC_CNT(iCnt) (NFStackTrace::Instance()->SetExecCnt(iCnt))
#define GET_TRACE_STACK_LAST_REPORT_TIME() ((NFStackTrace::Instance()->GetLastReportTime())
#define UPDATE_TRACE_STACK_REPORT_TIME(tCur) ((NFStackTrace::Instance()->UpdateReportTime(tCur))

class NFStackTrace : public NFSingleton<NFStackTrace>
{
public:
    //TraceStack一次执行耗时平均20ms左右，多的150ms左右,默认还是关闭,只在逻辑出错想打印堆栈的地方传true参数
    //bNeedExec是否执行tracestack，为false直接返回
    //bFullParse:是否完全解析。
    const char* TraceStack(bool bNeedExec = false, bool bFullParse = false);

public:
    NFStackTrace();
    ~NFStackTrace();
private:
    char m_szStackInfoString[10240];

#if NF_PLATFORM == NF_PLATFORM_WIN
    void* m_hProcess;
    std::string addressToString(void* address);
    const char* TraceWindowsStack(bool bNeedExec, bool bFullParse);
#else
public:
    int GetExecCnt();
    time_t GetLastReportTime();
    void SetExecCnt(int iExecCnt);
    void UpdateReportTime(time_t tCur);
private:
    const char* TraceLinuxStack(bool bNeedExec, bool bFullParse);
    char* ReplaceDumpSymname(char* dump_sym_str);

private:
    void TraceLinuxStackFullParse(char **strings, char* pCurBuff, int & iOff, size_t sz);
    void TraceLinuxStackHalfParse(char **strings, char* pCurBuff, int & iOff, size_t sz);
private:
    int m_iExecCnt;
    time_t m_tLastReportTime;
#endif
};