// -------------------------------------------------------------------------
//    @FileName         :    NFNavMeshServerPlugin.cpp
//    @Author           :    gaoyi
//    @Date             :    2024/12/18
//    @Email            :    445267987@qq.com
//    @Module           :    NFNavMeshServerPlugin
//
// -------------------------------------------------------------------------

#include "NFNavMeshServerPlugin.h"

#include "NFComm/NFPluginModule/NFIPluginManager.h"
#include "NFNavMeshServerModule.h"


#ifdef NF_DYNAMIC_PLUGIN

NF_EXPORT void DllStartPlugin(NFIPluginManager* pm)
{
    CREATE_PLUGIN(pm, NFNavMeshServerPlugin)
};

NF_EXPORT void DllStopPlugin(NFIPluginManager* pm)
{
    DESTROY_PLUGIN(pm, NFNavMeshServerPlugin)
};

#endif


//////////////////////////////////////////////////////////////////////////

int NFNavMeshServerPlugin::GetPluginVersion()
{
    return 0;
}

std::string NFNavMeshServerPlugin::GetPluginName()
{
    return GET_CLASS_NAME(NFNavMeshServerPlugin);
}

void NFNavMeshServerPlugin::Install()
{
    NFGlobalSystem::Instance()->AddServerType(NF_ST_NAVMESH_SERVER);
    REGISTER_MODULE(m_pObjPluginManager, NFINavMeshServerModule, NFNavMeshServerModule);
}

void NFNavMeshServerPlugin::Uninstall()
{
    UNREGISTER_MODULE(m_pObjPluginManager, NFINavMeshServerModule, NFNavMeshServerModule);
}

bool NFNavMeshServerPlugin::InitShmObjectRegister()
{
    return true;
}
