// -------------------------------------------------------------------------
//    @FileName         :    NFOnlineServerPlugin.cpp
//    @Author           :    gaoyi
//    @Date             :    2024/12/18
//    @Email            :    445267987@qq.com
//    @Module           :    NFOnlineServerPlugin
//
// -------------------------------------------------------------------------

#include "NFOnlineServerPlugin.h"

#include "NFComm/NFPluginModule/NFIPluginManager.h"
#include "NFOnlineServerModule.h"


#ifdef NF_DYNAMIC_PLUGIN

NF_EXPORT void DllStartPlugin(NFIPluginManager* pm)
{
    CREATE_PLUGIN(pm, NFOnlineServerPlugin)
};

NF_EXPORT void DllStopPlugin(NFIPluginManager* pm)
{
    DESTROY_PLUGIN(pm, NFOnlineServerPlugin)
};

#endif


//////////////////////////////////////////////////////////////////////////

int NFOnlineServerPlugin::GetPluginVersion()
{
    return 0;
}

std::string NFOnlineServerPlugin::GetPluginName()
{
    return GET_CLASS_NAME(NFOnlineServerPlugin);
}

void NFOnlineServerPlugin::Install()
{
    NFGlobalSystem::Instance()->AddServerType(NF_ST_ONLINE_SERVER);
    REGISTER_MODULE(m_pObjPluginManager, NFIOnlineServerModule, NFOnlineServerModule);
}

void NFOnlineServerPlugin::Uninstall()
{
    UNREGISTER_MODULE(m_pObjPluginManager, NFIOnlineServerModule, NFOnlineServerModule);
}

bool NFOnlineServerPlugin::InitShmObjectRegister()
{
    return true;
}
