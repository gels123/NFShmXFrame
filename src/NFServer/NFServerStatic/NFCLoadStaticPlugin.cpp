// -------------------------------------------------------------------------
//    @FileName         :    NFCLoadStaticPlugin.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFPluginManager
//
// -------------------------------------------------------------------------
#include "NFPluginManager/NFCPluginManager.h"
#include "NFComm/NFCore/NFPlatform.h"

#include "NFComm/NFPluginModule/NFIEventModule.h"
#include "NFComm/NFPluginModule/NFITimerModule.h"
#include "NFComm/NFPluginModule/NFIConfigModule.h"
#include "NFComm/NFPluginModule/NFILogModule.h"
#include "NFComm/NFPluginModule/NFITaskModule.h"

#include "NFComm/NFPluginModule/NFEventObj.h"
#include "NFComm/NFPluginModule/NFTimerObj.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include "NFComm/NFPluginModule/NFIConfigModule.h"

#ifndef NF_DYNAMIC_PLUGIN

#include "NFCommPlugin/NFKernelPlugin/NFKernelPlugin.h"
#include "NFCommPlugin/NFNetPlugin/NFNetPlugin.h"
#include "NFCommPlugin/NFShmPlugin/NFShmPlugin.h"
#include "NFCommPlugin/NFMemPlugin/NFMemPlugin.h"
#include "NFCommPlugin/NFDBPlugin/NFDBPlugin.h"
#include "NFTest/NFTutorialPlugin/NFTutorialPlugin.h"

#include "NFCenterServer/NFCenterServerPlugin/NFCenterServerPlugin.h"
#include "NFGameServer/NFGameServerPlugin/NFGameServerPlugin.h"
#include "NFLogicServer/NFLogicServerPlugin/NFLogicServerPlugin.h"
#include "NFLoginServer/NFLoginServerPlugin/NFLoginServerPlugin.h"
#include "NFMasterServer/NFMasterServerPlugin/NFMasterServerPlugin.h"
#include "NFProxyAgentServer/NFProxyAgentServerPlugin.h"
#include "NFProxyServer/NFProxyServerPlugin/NFProxyServerPlugin.h"
#include "NFProxyServer/NFProxyClientPlugin/NFProxyClientPlugin.h"
#include "NFRouteAgentServer/NFRouteAgentServerPlugin/NFRouteAgentServerPlugin.h"
#include "NFRouteServer/NFRouteServerPlugin/NFRouteServerPlugin.h"
#include "NFSnsServer/NFSnsServerPlugin/NFSnsServerPlugin.h"
#include "NFStoreServer/NFStoreServerPlugin/NFStoreServerPlugin.h"
#include "NFWebServer/NFWebServerPlugin/NFWebServerPlugin.h"
#include "NFWorldServer/NFWorldServerPlugin/NFWorldServerPlugin.h"
#include "NFCityServer/NFCityServerPlugin/NFCityServerPlugin.h"
#include "NFMatchServer/NFMatchServerPlugin/NFMatchServerPlugin.h"
#include "NFNavMeshServer/NFNavMeshServerPlugin/NFNavMeshServerPlugin.h"
#include "NFOnlineServer/NFOnlineServerPlugin/NFOnlineServerPlugin.h"
#include "NFXPlugin/NFLuaScriptPlugin/NFLuaScriptPlugin.h"

#include "NFServerComm/NFServerCommonPlugin/NFServerCommonPlugin.h"
#include "NFServerComm/NFDescStorePlugin/NFDescStorePlugin.h"
#endif

bool NFCPluginManager::RegisterStaticPlugin()
{
#ifndef NF_DYNAMIC_PLUGIN
	////////////////////////////////////////////////////////////////
	REGISTER_STATIC_PLUGIN(this, NFKernelPlugin);
	REGISTER_STATIC_PLUGIN(this, NFNetPlugin);
	REGISTER_STATIC_PLUGIN(this, NFShmPlugin);
	REGISTER_STATIC_PLUGIN(this, NFMemPlugin);
	REGISTER_STATIC_PLUGIN(this, NFDBPlugin);
	REGISTER_STATIC_PLUGIN(this, NFTutorialPlugin);

	REGISTER_STATIC_PLUGIN(this, NFServerCommonPlugin);
	REGISTER_STATIC_PLUGIN(this, NFDescStorePlugin);
	REGISTER_STATIC_PLUGIN(this, NFCenterServerPlugin);
	REGISTER_STATIC_PLUGIN(this, NFGameServerPlugin);
	REGISTER_STATIC_PLUGIN(this, NFLogicServerPlugin);
	REGISTER_STATIC_PLUGIN(this, NFLoginServerPlugin);
	REGISTER_STATIC_PLUGIN(this, NFMasterServerPlugin);
	REGISTER_STATIC_PLUGIN(this, NFProxyAgentServerPlugin);
	REGISTER_STATIC_PLUGIN(this, NFProxyServerPlugin);
	REGISTER_STATIC_PLUGIN(this, NFProxyClientPlugin);
	REGISTER_STATIC_PLUGIN(this, NFRouteAgentServerPlugin);
	REGISTER_STATIC_PLUGIN(this, NFRouteServerPlugin);
	REGISTER_STATIC_PLUGIN(this, NFSnsServerPlugin);
	REGISTER_STATIC_PLUGIN(this, NFRouteServerPlugin);
	REGISTER_STATIC_PLUGIN(this, NFStoreServerPlugin);
	REGISTER_STATIC_PLUGIN(this, NFWebServerPlugin);
	REGISTER_STATIC_PLUGIN(this, NFWorldServerPlugin);
	REGISTER_STATIC_PLUGIN(this, NFCityServerPlugin);
	REGISTER_STATIC_PLUGIN(this, NFMatchServerPlugin);
	REGISTER_STATIC_PLUGIN(this, NFNavMeshServerPlugin);
	REGISTER_STATIC_PLUGIN(this, NFOnlineServerPlugin);
	REGISTER_STATIC_PLUGIN(this, NFLuaScriptPlugin);
#endif
	return true;
}
