// -------------------------------------------------------------------------
//    @FileName         :    NFCShmOtherModule.cpp
//    @Author           :    gaoyi
//    @Date             :    23-9-8
//    @Email			:    445267987@qq.com
//    @Module           :    NFCShmOtherModule
//
// -------------------------------------------------------------------------

#include "NFCShmOtherModule.h"

#include <NFComm/NFCore/NFServerTime.h>

#include "NFShmTransMng.h"
#include "NFComm/NFPluginModule/NFIMemMngModule.h"

NFCShmOtherModule::NFCShmOtherModule(NFIPluginManager* p): NFIDynamicModule(p)
{
}

NFCShmOtherModule::~NFCShmOtherModule()
{
}

bool NFCShmOtherModule::Awake()
{
    Subscribe(NF_ST_NONE, NFrame::NF_EVENT_SERVER_APP_FINISH_INITED, NFrame::NF_EVENT_SERVER_TYPE, 0, __FUNCTION__);
    return true;
}

int NFCShmOtherModule::OnExecute(uint32_t serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const google::protobuf::Message* pMessage)
{
    if (srcType == NFrame::NF_EVENT_SERVER_TYPE)
    {
        if (eventId == NFrame::NF_EVENT_SERVER_APP_FINISH_INITED)
        {
            /*
            初始化完毕
            */
            FindModule<NFIMemMngModule>()->SetShmInitSuccessFlag();

            if (FindModule<NFIMemMngModule>()->GetSecOffSet() > 0)
            {
                NFServerTime::Instance()->SetSecOffSet(FindModule<NFIMemMngModule>()->GetSecOffSet());
            }
        }
    }

    return 0;
}

bool NFCShmOtherModule::CheckStopServer()
{
    if (!NFShmTransMng::Instance()->CheckStopServer())
    {
        return false;
    }
    return true;
}

bool NFCShmOtherModule::StopServer()
{
    if (!NFShmTransMng::Instance()->StopServer())
    {
        return false;
    }
    return true;
}


