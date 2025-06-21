// -------------------------------------------------------------------------
//    @FileName         :    NFShmPlugin.cpp
//    @Author           :    gaoyi
//    @Date             :   2022-09-18
//    @Module           :    NFShmPlugin
//
// -------------------------------------------------------------------------

#include "NFShmPlugin.h"
#include "NFCShmMngModule.h"
#include "NFCShmOtherModule.h"

#include "NFShmGlobalId.h"
#include "NFShmTimer.h"
#include "NFShmTimerMng.h"
#include "NFShmTransMng.h"
#include "NFComm/NFObjCommon/NFTransBase.h"
#include "NFComm/NFPluginModule/NFIConfigModule.h"
#include "NFShmSubscribeInfo.h"
#include "NFShmEventMgr.h"

//
//
#ifdef NF_DYNAMIC_PLUGIN

NF_EXPORT void DllStartPlugin(NFIPluginManager* pm)
{
	CREATE_PLUGIN(pm, NFShmPlugin)
};

NF_EXPORT void DllStopPlugin(NFIPluginManager* pm)
{
	DESTROY_PLUGIN(pm, NFShmPlugin)
};

#endif

//////////////////////////////////////////////////////////////////////////

int NFShmPlugin::GetPluginVersion()
{
	return 0;
}

std::string NFShmPlugin::GetPluginName()
{
	return GET_CLASS_NAME(NFShmPlugin);
}

bool NFShmPlugin::IsDynamicLoad()
{
	return false;
}

void NFShmPlugin::Install()
{
    REGISTER_MODULE(m_pObjPluginManager, NFIMemMngModule, NFCShmMngModule);
    REGISTER_MODULE(m_pObjPluginManager, NFCShmOtherModule, NFCShmOtherModule);
}

void NFShmPlugin::Uninstall()
{
    UNREGISTER_MODULE(m_pObjPluginManager, NFIMemMngModule, NFCShmMngModule);
    UNREGISTER_MODULE(m_pObjPluginManager, NFCShmOtherModule, NFCShmOtherModule);
}

bool NFShmPlugin::InitShmObjectRegister()
{
    uint32_t maxOnlinePlayerNum = 100;
    if (!m_pObjPluginManager->IsLoadAllServer())
    {
        NFServerConfig* pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_NONE);
        NF_ASSERT(pConfig);

        maxOnlinePlayerNum = pConfig->MaxOnlinePlayerNum;
    }

    uint32_t maxShmtimer = ALL_TIMER_COUNT/10 + maxOnlinePlayerNum*10;
    if (maxShmtimer >= ALL_TIMER_COUNT)
    {
        maxShmtimer = ALL_TIMER_COUNT;
    }

    uint32_t maxShmEvent = NF_SHM_EVENT_KEY_MAX_NUM/10 + maxOnlinePlayerNum*10;
    if (maxShmEvent >= NF_SHM_EVENT_KEY_MAX_NUM)
    {
        maxShmEvent = NF_SHM_EVENT_KEY_MAX_NUM;
    }

    REGISTER_SHM_OBJ(NFObject, 0);
    REGISTER_SINGLETON_SHM_OBJ(NFShmGlobalId);
	REGISTER_SHM_OBJ(NFShmTimer, maxShmtimer);
    REGISTER_SINGLETON_SHM_OBJ(NFShmTimerMng);
    REGISTER_SHM_OBJ(NFShmSubscribeInfo, maxShmEvent);
    REGISTER_SINGLETON_SHM_OBJ(NFShmEventMgr);
    REGISTER_SINGLETON_SHM_OBJ(NFShmTransMng);
    REGISTER_SHM_OBJ(NFTransBase, 0);
	return true;
}
