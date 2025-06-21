// -------------------------------------------------------------------------
//    @FileName         :    NFZDBPlugin.cpp
//    @Author           :   gaoyi
//    @Date             :   2024-05-23
//    @Module           :    NFZDBPlugin
//
// -------------------------------------------------------------------------

#include "NFZDBPlugin.h"
//
//
#ifdef NF_DYNAMIC_PLUGIN

NF_EXPORT void DllStartPlugin(NFIPluginManager* pm)
{
	CREATE_PLUGIN(pm, NFZDBPlugin)
};

NF_EXPORT void DllStopPlugin(NFIPluginManager* pm)
{
	DESTROY_PLUGIN(pm, NFZDBPlugin)
};

#endif

//////////////////////////////////////////////////////////////////////////

int NFZDBPlugin::GetPluginVersion()
{
	return 0;
}

std::string NFZDBPlugin::GetPluginName()
{
	return GET_CLASS_NAME(NFZDBPlugin);
}

bool NFZDBPlugin::IsDynamicLoad()
{
	return false;
}

void NFZDBPlugin::Install()
{
	REGISTER_MODULE(m_pObjPluginManager, NFCTestZdbModule, NFCTestZdbModule);
}

void NFZDBPlugin::Uninstall()
{
	UNREGISTER_MODULE(m_pObjPluginManager, NFCTestZdbModule, NFCTestZdbModule);
}
