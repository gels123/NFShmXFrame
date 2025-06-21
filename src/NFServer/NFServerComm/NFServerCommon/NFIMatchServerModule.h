// -------------------------------------------------------------------------
//    @FileName         :    NFIMatchServerModule.h
//    @Author           :    gaoyi
//    @Date             :    2024/12/18
//    @Email            :    445267987@qq.com
//    @Module           :    NFIMatchServerModule
//
// -------------------------------------------------------------------------

#pragma once
#include "NFWorkServerModule.h"

class NFIMatchServerModule : public NFWorkServerModule
{
public:
    NFIMatchServerModule(NFIPluginManager* p) : NFWorkServerModule(p, NF_ST_MATCH_SERVER)
    {

    }

    virtual ~NFIMatchServerModule()
    {

    }
};