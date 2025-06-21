// -------------------------------------------------------------------------
//    @FileName         :    NFCityServerPlugin.cpp
//    @Author           :    gaoyi
//    @Date             :    2024/12/18
//    @Email            :    445267987@qq.com
//    @Module           :    NFCityServerPlugin
//
// -------------------------------------------------------------------------

#include "NFCityServerPlugin.h"
#include "NFComm/NFPluginModule/NFIPluginManager.h"
#include "NFCityServerModule.h"


#ifdef NF_DYNAMIC_PLUGIN

NF_EXPORT void DllStartPlugin(NFIPluginManager* pm)
{
    CREATE_PLUGIN(pm, NFCityServerPlugin)
};

NF_EXPORT void DllStopPlugin(NFIPluginManager* pm)
{
    DESTROY_PLUGIN(pm, NFCityServerPlugin)
};

#endif


//////////////////////////////////////////////////////////////////////////

int NFCityServerPlugin::GetPluginVersion()
{
    return 0;
}

std::string NFCityServerPlugin::GetPluginName()
{
    return GET_CLASS_NAME(NFCityServerPlugin);
}

void NFCityServerPlugin::Install()
{
    NFGlobalSystem::Instance()->AddServerType(NF_ST_CITY_SERVER);
    REGISTER_MODULE(m_pObjPluginManager, NFICityServerModule, NFCityServerModule);
}

void NFCityServerPlugin::Uninstall()
{
    UNREGISTER_MODULE(m_pObjPluginManager, NFICityServerModule, NFCityServerModule);
}

bool NFCityServerPlugin::InitShmObjectRegister()
{
    return true;
}