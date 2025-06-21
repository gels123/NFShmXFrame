// -------------------------------------------------------------------------
//    @FileName         :    NFINavMeshServerModule.h
//    @Author           :    gaoyi
//    @Date             :    2024/12/18
//    @Email            :    445267987@qq.com
//    @Module           :    NFINavMeshServerModule
//
// -------------------------------------------------------------------------

#pragma once
#include "NFWorkServerModule.h"

class NFINavMeshServerModule : public NFWorkServerModule
{
public:
    NFINavMeshServerModule(NFIPluginManager* p) : NFWorkServerModule(p, NF_ST_NAVMESH_SERVER)
    {

    }

    virtual ~NFINavMeshServerModule()
    {

    }
};