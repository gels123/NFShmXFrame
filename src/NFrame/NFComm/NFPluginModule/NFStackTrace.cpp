// -------------------------------------------------------------------------
//    @FileName         :    NFStackTrace.cpp
//    @Author           :    gaoyi
//    @Date             :    24-8-23
//    @Email            :    445267987@qq.com
//    @Module           :    NFStackTrace
//
// -------------------------------------------------------------------------

#include "NFStackTrace.h"

#include <NFComm/NFCore/NFStringUtility.h>

#if NF_PLATFORM == NF_PLATFORM_WIN
#include "windows.h"
#include <psapi.h>
#include <iostream>
#include <sstream>
#include <cstddef>
#include <dbghelp.h>
#pragma comment( lib, "dbghelp" )
#pragma comment(lib, "iphlpapi.lib")
#pragma warning(disable:4100)

static HANDLE g_hProcess = 0;
#else
#include <cxxabi.h>

#endif


NFStackTrace::NFStackTrace()
{
#if NF_PLATFORM == NF_PLATFORM_WIN
    m_hProcess = GetCurrentProcess();
    g_hProcess = m_hProcess;
    DWORD dwOpts = SymGetOptions();
    dwOpts |= SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_DEBUG;
    SymSetOptions(dwOpts);
    ::SymInitialize(m_hProcess, 0, true);
#else
    m_iExecCnt = 0;
    m_tLastReportTime = 0;
#endif
}

NFStackTrace::~NFStackTrace()
{
#if NF_PLATFORM == NF_PLATFORM_WIN
    ::SymCleanup(m_hProcess);
#else
#endif
}

const char *NFStackTrace::TraceStack(bool bNeedExec/* = false*/, bool bFullParse/* = false*/)
{
#if NF_PLATFORM == NF_PLATFORM_WIN
    return TraceWindowsStack(bNeedExec, bFullParse);
#else
    return TraceLinuxStack(bNeedExec, bFullParse);
#endif
}

#if NF_PLATFORM == NF_PLATFORM_WIN
/////////////////////////////////////////////////////////////////////////////////////
enum BasicType
{
    btNoType = 0,
    btVoid = 1,
    btChar = 2,
    btWChar = 3,
    btInt = 6,
    btUInt = 7,
    btFloat = 8,
    btBCD = 9,
    btBool = 10,
    btLong = 13,
    btULong = 14,
    btCurrency = 25,
    btDate = 26,
    btVariant = 27,
    btComplex = 28,
    btBit = 29,
    btBSTR = 30,
    btHresult = 31
};

std::string NFStackTrace::addressToString(PVOID pVoid)
{
    std::ostringstream oss;
    STACKFRAME *pStackFrame = (STACKFRAME *) pVoid;
    DWORD dwAddress = pStackFrame->AddrPC.Offset;
    oss << "0x" << pVoid;
    struct tagSymInfo
    {
        IMAGEHLP_SYMBOL symInfo;
        char nameBuffer[4 * 256];
    } SymInfo = {{sizeof(IMAGEHLP_SYMBOL)}};
    IMAGEHLP_SYMBOL *pSym = &SymInfo.symInfo;
    pSym->MaxNameLength = sizeof(SymInfo) - offsetof(tagSymInfo, symInfo.Name);
    DWORD dwDisplacement;
#ifdef _MSC_VER
    if (SymGetSymFromAddr(m_hProcess, dwAddress, (PDWORD64) &dwDisplacement, pSym))
#else
    if (SymGetSymFromAddr(m_hProcess, dwAddress, &dwDisplacement, pSym))
#endif // _MSC_VER
    {
        oss << " " << pSym->Name;
        //if ( dwDisplacement != 0 )
        //	oss << "+0x" << std::hex << dwDisplacement << std::dec;
    }

    // Finally any file/line number
    IMAGEHLP_LINE lineInfo = {sizeof(IMAGEHLP_LINE)};
    if (SymGetLineFromAddr(m_hProcess, dwAddress, &dwDisplacement, &lineInfo))
    {
        char const *pDelim = strrchr(lineInfo.FileName, '\\');
        oss << " in " << (pDelim ? pDelim + 1 : lineInfo.FileName) << "(" << lineInfo.LineNumber << ")";
    }
    return oss.str();
}

const char *NFStackTrace::TraceWindowsStack(bool bNeedExec, bool bFullParse)
{
    m_szStackInfoString[0] = 0;
    char *szOut = m_szStackInfoString;
    int iOutLen = 0;
#ifndef _WIN64
    CONTEXT context = {CONTEXT_FULL};
    ::GetThreadContext( GetCurrentThread(), &context );
    _asm call $+5
    _asm pop eax
    _asm mov context.Eip, eax
    _asm mov eax, esp
    _asm mov context.Esp, eax
    _asm mov context.Ebp, ebp

PCONTEXT pContext = &context;
    iOutLen += _snprintf(szOut, sizeof(m_szStackInfoString), "%s", "TrackStack begin \n");
    STACKFRAME stackFrame = {0};
    stackFrame.AddrPC.Offset = pContext->Eip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = pContext->Ebp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = pContext->Esp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
    int i=0;
    while ( ::StackWalk(IMAGE_FILE_MACHINE_I386,m_hProcess,	GetCurrentThread(),
        &stackFrame,pContext,NULL,::SymFunctionTableAccess,::SymGetModuleBase,NULL))
    {
        i++;
        if(i > 3)
        {
            iOutLen += _snprintf(szOut+iOutLen, sizeof(m_szStackInfoString) - iOutLen, "%s", addressToString( (PVOID)&stackFrame).c_str());
            iOutLen += _snprintf(szOut+iOutLen, sizeof(m_szStackInfoString) - iOutLen, "%s", "\n");
        }
        if(i > MAX_BACK_TRACE_SIZE)
        {
            break;
        }
    }
    iOutLen += _snprintf(szOut+iOutLen, sizeof(m_szStackInfoString) - iOutLen, "%s", "TrackStack end \n");
#endif
    return m_szStackInfoString;
}
#else
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <signal.h>
#include <ucontext.h>
#include <dlfcn.h>
#include <execinfo.h>

#define MAX_BACK_TRACE_SIZE (11)//以前是11,减少到6,看看性能开销

int NFStackTrace::GetExecCnt()
{
    return m_iExecCnt;
}

time_t NFStackTrace::GetLastReportTime()
{
    return m_tLastReportTime;
}

void NFStackTrace::SetExecCnt(int iExecCnt)
{
    m_iExecCnt = iExecCnt;
}

void NFStackTrace::UpdateReportTime(time_t tCur)
{
    m_tLastReportTime = tCur;
}

const char* NFStackTrace::TraceLinuxStack(bool bNeedExec, bool bFullParse)
{
    if (false == bNeedExec)
    {
        return " ";
    }

    struct timeval stBegin;
    gettimeofday( &stBegin, NULL );

    m_iExecCnt++;

    m_szStackInfoString[0] = 0;
    char* pCurBuff = m_szStackInfoString;
    void *bt[MAX_BACK_TRACE_SIZE];
    int iOff = 0;
    iOff += snprintf(pCurBuff, sizeof(m_szStackInfoString), "%s", "TrackStack begin \n");
    size_t sz = backtrace(bt, MAX_BACK_TRACE_SIZE);
    char **strings = backtrace_symbols(bt, sz);
    if (NULL == strings)
    {
        return " ";
    }

    if (bFullParse)
    {
        TraceLinuxStackFullParse(strings, pCurBuff, iOff, sz);
    }
    else
    {
        TraceLinuxStackHalfParse(strings, pCurBuff, iOff, sz);
    }

    free(strings);

    iOff += snprintf(pCurBuff+iOff, sizeof(m_szStackInfoString)-iOff, "%s", "TrackStack end\n");

    struct timeval stEnd;
    gettimeofday( &stEnd, NULL );
    int64_t llCost = (int64_t)( stEnd.tv_sec - stBegin.tv_sec )*1000000 + stEnd.tv_usec - stBegin.tv_usec;

    iOff += snprintf(pCurBuff+iOff, sizeof(m_szStackInfoString)-iOff, "(bFullParse %d)TrackStack cost time %ld ms  %ld us",  (bFullParse ? 1 : 0), llCost/1000, llCost % 1000);

    return m_szStackInfoString;
}

//会把符号demangle，解析成可读性强的形式,但是性能太差，外网勿用
void NFStackTrace::TraceLinuxStackFullParse(char **strings, char* pCurBuff, int & iOff, size_t sz)
{
    for(size_t i = 3; i < sz; ++i)
    {
        const char * _xxs_str = ReplaceDumpSymname(strings[i]);
        if (_xxs_str && strlen(_xxs_str) > 0)
        {
            iOff += snprintf(pCurBuff+iOff, sizeof(m_szStackInfoString)-iOff, "%s \n", _xxs_str);
        }
        else if (_xxs_str)
        {
            iOff += snprintf(pCurBuff+iOff, sizeof(m_szStackInfoString)-iOff, "%s \n",strings[i]);
        }
        else
        {
            iOff += snprintf(pCurBuff+iOff, sizeof(m_szStackInfoString)-iOff, "\r\n");
        }
    }
}

//不执行demangle, 查看日志后，需要手动使用c++filt来解析成可读性强的格式，看看对比下性能如何
void NFStackTrace::TraceLinuxStackHalfParse(char **strings, char* pCurBuff, int & iOff, size_t sz)
{
    for(size_t i = 3; i < sz; ++i)
    {
        iOff += snprintf(pCurBuff+iOff, sizeof(m_szStackInfoString)-iOff, "%s \n",strings[i]);
    }
}

#define MAX_DEMANGLED_NAME_LEN	(2048)
char* NFStackTrace::ReplaceDumpSymname(char* dump_sym_str)
{
    static char mangled_str[MAX_DEMANGLED_NAME_LEN];
    static char demangled_str[MAX_DEMANGLED_NAME_LEN];
    static char bin_str[MAX_DEMANGLED_NAME_LEN];
    static char offset_str[32];
    static int	dump_stack_status=0;
    static char* dump_demangled=0;

    static char dump_demangled_str[MAX_DEMANGLED_NAME_LEN*2];

    if(sizeof(dump_demangled_str)<strlen(dump_sym_str)+1)
    {
        return NULL;
    }
    //fmt=%[^(](%s)
    std::vector<std::string> vecString;
    NFStringUtility::SplitString(dump_sym_str, "(", vecString);

    // /data/home/user00/projectx/svr/gamesvr/bin/gamesvr(_ZN5MAYEX10CServiceGM12ProcessGMReqEPNS_8CSessionEPKN2CS8CSMsgReqE+0x38b2)
    //自己解析,代替sscanf,sscanf被codeccc告警,可能会缓冲区溢出
    if (2 ==vecString.size())
    {
        //vecString[0]: bin_str
        //vecString[1]: func + offstring
        snprintf(bin_str, sizeof(bin_str), "%s", vecString[0].c_str());
        std::vector<std::string> vecString2;
        NFStringUtility::SplitString(vecString[1], "+", vecString2);
        if (2 == vecString2.size())
        {
            snprintf(mangled_str, sizeof(mangled_str), "%s", vecString2[0].c_str());
            snprintf(offset_str, sizeof(offset_str), "%s", vecString2[1].c_str());

            size_t _length = sizeof(demangled_str);
            dump_demangled = abi::__cxa_demangle(mangled_str,
                                demangled_str,
                                &_length,
                                &dump_stack_status);
            if( 0 == dump_stack_status && dump_demangled)
            {
                snprintf(dump_demangled_str,sizeof(dump_demangled_str),"%s(%s+%s)",bin_str,demangled_str,offset_str);
            }
            else
            {
                return NULL;
            }
        }
    }

    return dump_demangled_str;
}
#endif
