// -------------------------------------------------------------------------
//    @FileName         :    NFICheckServerModule.h
//    @Author           :    gaoyi
//    @Date             :    2024/12/18
//    @Email            :    445267987@qq.com
//    @Module           :    NFICheckServerModule
//
// -------------------------------------------------------------------------

#pragma once
#include "NFWorkServerModule.h"

class NFICheckServerModule : public NFWorkServerModule
{
public:
    NFICheckServerModule(NFIPluginManager* p) : NFWorkServerModule(p, NF_ST_CHECK_SERVER)
    {

    }

    virtual ~NFICheckServerModule()
    {

    }
};