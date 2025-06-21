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
#include "NFComm/NFPluginModule/NFIConfigModule.h"
#include "NFComm/NFPluginModule/NFEventObj.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"

#ifndef NF_DYNAMIC_PLUGIN

#include "NFCommPlugin/NFKernelPlugin/NFKernelPlugin.h"
#include "NFCommPlugin/NFNetPlugin/NFNetPlugin.h"
#include "NFCommPlugin/NFShmPlugin/NFShmPlugin.h"
#include "NFCommPlugin/NFMemPlugin/NFMemPlugin.h"
#include "NFCommPlugin/NFDBPlugin/NFDBPlugin.h"
#include "NFTest/NFTutorialPlugin/NFTutorialPlugin.h"
#include <NFTest/NFTestPlugin/NFTestPlugin.h>
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
	REGISTER_STATIC_PLUGIN(this, NFTestPlugin);

#endif
	return true;
}
