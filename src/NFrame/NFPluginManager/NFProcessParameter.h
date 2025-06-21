// -------------------------------------------------------------------------
//    @FileName         :    NFPrintfLogo.h
//    @Author           :    Yi.Gao
//    @Date             :   2022-09-18
//    @Module           :    NFPluginManager
//
// -------------------------------------------------------------------------

#pragma once

#include <iostream>

#include "NFPluginManager/NFCPluginManager.h"

#if NF_PLATFORM == NF_PLATFORM_LINUX
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/prctl.h>
#endif

void CloseXButton();
void ignore_pipe_new();

//转变成守护进程后，会新建一个进程
void InitDaemon();
void ProcessParameter(int argc, char* argv[]);
void ProcessParameter(NFIPluginManager* pPluginManager, const std::vector<std::string>& vecParam);