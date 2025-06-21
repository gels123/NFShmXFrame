// -------------------------------------------------------------------------
//    @FileName         :    NFWebServerPlugin.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFWebServerPlugin.cpp
//
// -------------------------------------------------------------------------


#include "NFWebServerPlugin.h"
#include "NFComm/NFPluginModule/NFIPluginManager.h"
#include "NFCWebServerModule.h"

#ifdef NF_DYNAMIC_PLUGIN

NF_EXPORT void DllStartPlugin(NFIPluginManager* pm)
{
    CREATE_PLUGIN(pm, NFWebServerPlugin)

};

NF_EXPORT void DllStopPlugin(NFIPluginManager* pm)
{
    DESTROY_PLUGIN(pm, NFWebServerPlugin)
};

#endif


//////////////////////////////////////////////////////////////////////////

int NFWebServerPlugin::GetPluginVersion()
{
    return 0;
}

std::string NFWebServerPlugin::GetPluginName()
{
    return GET_CLASS_NAME(NFWebServerPlugin);
}

void NFWebServerPlugin::Install()
{
    NFGlobalSystem::Instance()->AddServerType(NF_ST_WEB_SERVER);
    REGISTER_MODULE(m_pObjPluginManager, NFIWebServerModule, NFCWebServerModule);
}

void NFWebServerPlugin::Uninstall()
{
    UNREGISTER_MODULE(m_pObjPluginManager, NFIWebServerModule, NFCWebServerModule);
}

