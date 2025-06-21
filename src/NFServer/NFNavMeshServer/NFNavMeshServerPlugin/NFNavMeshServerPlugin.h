// -------------------------------------------------------------------------
//    @FileName         :    NFNavMeshServerPlugin.h
//    @Author           :    gaoyi
//    @Date             :    2024/12/18
//    @Email            :    445267987@qq.com
//    @Module           :    NFNavMeshServerPlugin
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"


#include "NFComm/NFPluginModule/NFIPlugin.h"
#include "NFComm/NFPluginModule/NFIPluginManager.h"
#include <string>

//////////////////////////////////////////////////////////////////////////
class NFNavMeshServerPlugin : public NFIPlugin
{
public:
    explicit NFNavMeshServerPlugin(NFIPluginManager* p): NFIPlugin(p)
    {
    }

    virtual int GetPluginVersion() override;

    virtual std::string GetPluginName() override;

    virtual void Install() override;

    virtual void Uninstall() override;

    virtual bool InitShmObjectRegister() override;
};