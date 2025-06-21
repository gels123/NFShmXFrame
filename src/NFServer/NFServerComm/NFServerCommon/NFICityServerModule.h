// -------------------------------------------------------------------------
//    @FileName         :    NFICityServerModule.h
//    @Author           :    gaoyi
//    @Date             :    2024/12/18
//    @Email            :    445267987@qq.com
//    @Module           :    NFICityServerModule
//
// -------------------------------------------------------------------------

#pragma once

#include "NFWorkServerModule.h"

class NFICityServerModule : public NFWorkServerModule
{
public:
    NFICityServerModule(NFIPluginManager* p) : NFWorkServerModule(p, NF_ST_CITY_SERVER)
    {

    }

    virtual ~NFICityServerModule()
    {

    }
};