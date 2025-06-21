// -------------------------------------------------------------------------
//    @FileName         :    NFIOnlineServerModule.h
//    @Author           :    gaoyi
//    @Date             :    2024/12/18
//    @Email            :    445267987@qq.com
//    @Module           :    NFIOnlineServerModule
//
// -------------------------------------------------------------------------

#pragma once
#include "NFWorkServerModule.h"

class NFIOnlineServerModule : public NFWorkServerModule
{
public:
    NFIOnlineServerModule(NFIPluginManager* p) : NFWorkServerModule(p, NF_ST_ONLINE_SERVER)
    {

    }

    virtual ~NFIOnlineServerModule()
    {

    }
};