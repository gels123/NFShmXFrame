// -------------------------------------------------------------------------
//    @FileName         :    NFCheckServerPlugin.cpp
//    @Author           :    gaoyi
//    @Date             :    2024/12/18
//    @Email            :    445267987@qq.com
//    @Module           :    NFCheckServerPlugin
//
// -------------------------------------------------------------------------

#include "NFCheckServerPlugin.h"
#include "NFComm/NFPluginModule/NFIPluginManager.h"
#include "NFCheckServerModule.h"


#ifdef NF_DYNAMIC_PLUGIN

NF_EXPORT void DllStartPlugin(NFIPluginManager* pm)
{
    CREATE_PLUGIN(pm, NFCheckServerPlugin)
};

NF_EXPORT void DllStopPlugin(NFIPluginManager* pm)
{
    DESTROY_PLUGIN(pm, NFCheckServerPlugin)
};

#endif


//////////////////////////////////////////////////////////////////////////

int NFCheckServerPlugin::GetPluginVersion()
{
    return 0;
}

std::string NFCheckServerPlugin::GetPluginName()
{
    return GET_CLASS_NAME(NFCheckServerPlugin);
}

void NFCheckServerPlugin::Install()
{
    NFGlobalSystem::Instance()->AddServerType(NF_ST_CHECK_SERVER);
    REGISTER_MODULE(m_pObjPluginManager, NFICheckServerModule, NFCheckServerModule);
}

void NFCheckServerPlugin::Uninstall()
{
    UNREGISTER_MODULE(m_pObjPluginManager, NFICheckServerModule, NFCheckServerModule);
}

bool NFCheckServerPlugin::InitShmObjectRegister()
{
    return true;
}
