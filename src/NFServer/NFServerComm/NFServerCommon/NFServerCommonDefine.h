// -------------------------------------------------------------------------
//    @FileName         :    NFServerDefine.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------
#pragma once

#include <stdint.h>
#include <functional>
#include <string>
#include <vector>

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFKernelMessage/FrameMsg.pb.h"
#include "NFComm/NFPluginModule/NFConfigDefine.h"
#include "NFComm/NFPluginModule/NFSystemInfo.h"

class NFServerCommon
{
public:
    NFServerCommon()
    {
    }

    virtual ~NFServerCommon()
    {
    };

    static void WriteServerInfo(NFrame::ServerInfoReport* pData, NFServerConfig* pConfig);
    static void WriteServerInfo(NFrame::ServerInfoReport* pData, const NFSystemInfo& systemInfo);
};
