// -------------------------------------------------------------------------
//    @FileName         :    NFSignalHandleMgr.h
//    @Author           :    Yi.Gao
//    @Date             :   2022-09-18
//    @Module           :    NFPluginManager
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFCore/NFSingleton.hpp"

#if NF_PLATFORM == NF_PLATFORM_WIN
#include <windows.h>
#include <thread>
#include <atomic>
#include <vector>
#include <functional>
#include <chrono>

// 前置声明
class NFIPluginManager;
#endif

#if NF_PLATFORM == NF_PLATFORM_LINUX
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/prctl.h>

#ifndef SIGUNUSED
#define SIGUNUSED	31
#endif

#ifndef SIGHUP
#define	SIGHUP		1	/* Hangup (POSIX).  */
#endif

#ifndef SIGINT
#define	SIGINT		2	/* Interrupt (ANSI).  */
#endif

#ifndef SIGQUIT
#define	SIGQUIT		3	/* Quit (POSIX).  */
#endif

#ifndef SIGILL
#define	SIGILL		4	/* Illegal instruction (ANSI).  */
#endif

#ifndef SIGTRAP
#define	SIGTRAP		5	/* Trace trap (POSIX).  */
#endif

#ifndef SIGABRT
#define	SIGABRT		6	/* Abort (ANSI).  */
#endif

#ifndef SIGIOT
#define	SIGIOT		6	/* IOT trap (4.2 BSD).  */
#endif

#ifndef SIGBUS
#define	SIGBUS		7	/* BUS error (4.2 BSD).  */
#endif

#ifndef SIGFPE
#define	SIGFPE		8	/* Floating-point exception (ANSI).  */
#endif


#ifndef SIGKILL
#define	SIGKILL		9	/* Kill, unblockable (POSIX).  */
#endif

#ifndef SIGUSR1
#define	SIGUSR1		10	/* User-defined signal 1 (POSIX).  */
#endif

#ifndef SIGSEGV
#define	SIGSEGV		11	/* Segmentation violation (ANSI).  */
#endif

#ifndef SIGUSR2
#define	SIGUSR2		12	/* User-defined signal 2 (POSIX).  */
#endif

#ifndef SIGPIPE
#define	SIGPIPE		13	/* Broken pipe (POSIX).  */
#endif

#ifndef SIGALRM
#define	SIGALRM		14	/* Alarm clock (POSIX).  */
#endif

#ifndef SIGTERM
#define	SIGTERM		15	/* Termination (ANSI).  */
#endif

#ifndef SIGSTKFLT
#define	SIGSTKFLT	16	/* Stack fault.  */
#endif

#ifndef SIGCLD
#define	SIGCLD		SIGCHLD	/* Same as SIGCHLD (System V).  */
#endif

#ifndef SIGCHLD
#define	SIGCHLD		17	/* Child status has changed (POSIX).  */
#endif

#ifndef SIGCONT
#define	SIGCONT		18	/* Continue (POSIX).  */
#endif

#ifndef SIGSTOP
#define	SIGSTOP		19	/* Stop, unblockable (POSIX).  */
#endif

#ifndef SIGTSTP
#define	SIGTSTP		20	/* Keyboard stop (POSIX).  */
#endif

#ifndef SIGTTIN
#define	SIGTTIN		21	/* Background read from tty (POSIX).  */
#endif

#ifndef SIGTTOU
#define	SIGTTOU		22	/* Background write to tty (POSIX).  */
#endif

#ifndef SIGURG
#define	SIGURG		23	/* Urgent condition on socket (4.2 BSD).  */
#endif

#ifndef SIGXCPU
#define	SIGXCPU		24	/* CPU limit exceeded (4.2 BSD).  */
#endif

#ifndef SIGXFSZ
#define	SIGXFSZ		25	/* File size limit exceeded (4.2 BSD).  */
#endif

#ifndef SIGVTALRM
#define	SIGVTALRM	26	/* Virtual alarm clock (4.2 BSD).  */
#endif

#ifndef SIGPROF
#define	SIGPROF		27	/* Profiling alarm clock (4.2 BSD).  */
#endif

#ifndef SIGWINCH
#define	SIGWINCH	28	/* Window size change (4.3 BSD, Sun).  */
#endif

#ifndef SIGPOLL
#define	SIGPOLL		SIGIO	/* Pollable event occurred (System V).  */
#endif

#ifndef SIGIO
#define	SIGIO		29	/* I/O now possible (4.2 BSD).  */
#endif

#ifndef SIGPWR
#define	SIGPWR		30	/* Power failure restart (System V).  */
#endif

#ifndef SIGSYS
#define SIGSYS		31	/* Bad system call.  */
#endif
#endif

// Linux信号处理函数（保持原有功能）
void InitSignal();
void HandleSignal(int signo);

#if NF_PLATFORM == NF_PLATFORM_WIN
// Windows事件处理管理器
class NFSignalHandlerMgr : public NFSingleton<NFSignalHandlerMgr>
{
public:
    // 初始化事件处理线程
    bool Initialize();

    // 停止事件处理线程
    void Shutdown();
public:
    NFSignalHandlerMgr() = default;
    ~NFSignalHandlerMgr() override = default;
    NFSignalHandlerMgr(const NFSignalHandlerMgr&) = delete;
    NFSignalHandlerMgr& operator=(const NFSignalHandlerMgr&) = delete;

    // 事件处理线程函数
    void EventHandlingThread();

    // 检查单个事件
    bool CheckEvent(const std::string& eventName, const std::function<void()>& callback);

    /**
	 * @brief 返回成功被杀的信号。
	 * @return 成功返回 0，失败返回非零值。
	 */
	int SendKillSuccess();

private:
    std::thread m_eventThread;
    std::atomic<bool> m_bRunning{false};
    DWORD m_processId = 0;
};
#endif
