// -------------------------------------------------------------------------
//    @FileName         :    NFMemPlugin.cpp
//    @Author           :    gaoyi
//    @Date             :   2022-09-18
//    @Module           :    NFMemPlugin
//
// -------------------------------------------------------------------------

#include "NFMemPlugin.h"
#include "NFCMemMngModule.h"
#include "NFCMemOtherModule.h"

#include "NFMemGlobalId.h"
#include "NFMemTimer.h"
#include "NFMemTimerMng.h"
#include "NFMemTransMng.h"
#include "NFComm/NFObjCommon/NFTransBase.h"
#include "NFComm/NFPluginModule/NFIConfigModule.h"
#include "NFMemSubscribeInfo.h"
#include "NFMemEventMgr.h"

//
//
#ifdef NF_DYNAMIC_PLUGIN

NF_EXPORT void DllStartPlugin(NFIPluginManager* pm)
{
	CREATE_PLUGIN(pm, NFMemPlugin)
};

NF_EXPORT void DllStopPlugin(NFIPluginManager* pm)
{
	DESTROY_PLUGIN(pm, NFMemPlugin)
};

#endif

//////////////////////////////////////////////////////////////////////////

int NFMemPlugin::GetPluginVersion()
{
    return 0;
}

std::string NFMemPlugin::GetPluginName()
{
    return GET_CLASS_NAME(NFMemPlugin);
}

bool NFMemPlugin::IsDynamicLoad()
{
    return false;
}

void NFMemPlugin::Install()
{
    REGISTER_MODULE(m_pObjPluginManager, NFIMemMngModule, NFCMemMngModule);
    REGISTER_MODULE(m_pObjPluginManager, NFCMemOtherModule, NFCMemOtherModule);
}

void NFMemPlugin::Uninstall()
{
    UNREGISTER_MODULE(m_pObjPluginManager, NFIMemMngModule, NFCMemMngModule);
    UNREGISTER_MODULE(m_pObjPluginManager, NFCMemOtherModule, NFCMemOtherModule);
}

bool NFMemPlugin::InitShmObjectRegister()
{
    uint32_t maxOnlinePlayerNum = 100;
    if (!m_pObjPluginManager->IsLoadAllServer())
    {
        NFServerConfig* pConfig = FindModule<NFIConfigModule>()->GetAppConfig(NF_ST_NONE);
        NF_ASSERT(pConfig);

        maxOnlinePlayerNum = pConfig->MaxOnlinePlayerNum;
    }

    uint32_t maxShmtimer = ALL_TIMER_COUNT / 10 + maxOnlinePlayerNum * 10;
    if (maxShmtimer >= ALL_TIMER_COUNT)
    {
        maxShmtimer = ALL_TIMER_COUNT;
    }

    uint32_t maxShmEvent = NF_SHM_EVENT_KEY_MAX_NUM / 10 + maxOnlinePlayerNum * 10;
    if (maxShmEvent >= NF_SHM_EVENT_KEY_MAX_NUM)
    {
        maxShmEvent = NF_SHM_EVENT_KEY_MAX_NUM;
    }

    REGISTER_SHM_OBJ(NFObject, 0);
    REGISTER_SINGLETON_SHM_OBJ(NFMemGlobalId);
    REGISTER_SHM_OBJ(NFMemTimer, maxShmtimer);
    REGISTER_SINGLETON_SHM_OBJ(NFMemTimerMng);
    REGISTER_SHM_OBJ(NFMemSubscribeInfo, maxShmEvent);
    REGISTER_SINGLETON_SHM_OBJ(NFMemEventMgr);
    REGISTER_SINGLETON_SHM_OBJ(NFMemTransMng);
    REGISTER_SHM_OBJ(NFTransBase, 0);
    return true;
}
