// -------------------------------------------------------------------------
//    @FileName         :    NFCMemOtherModule.cpp
//    @Author           :    gaoyi
//    @Date             :    23-9-8
//    @Email			:    445267987@qq.com
//    @Module           :    NFCMemOtherModule
//
// -------------------------------------------------------------------------

#include "NFCMemOtherModule.h"

#include <NFComm/NFCore/NFServerTime.h>

#include "NFMemTransMng.h"
#include "NFComm/NFPluginModule/NFIMemMngModule.h"

NFCMemOtherModule::NFCMemOtherModule(NFIPluginManager* p): NFIDynamicModule(p)
{
}

NFCMemOtherModule::~NFCMemOtherModule()
{
}

bool NFCMemOtherModule::Awake()
{
    Subscribe(NF_ST_NONE, NFrame::NF_EVENT_SERVER_APP_FINISH_INITED, NFrame::NF_EVENT_SERVER_TYPE, 0, __FUNCTION__);
    return true;
}

int NFCMemOtherModule::OnExecute(uint32_t serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const google::protobuf::Message* pMessage)
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

bool NFCMemOtherModule::CheckStopServer()
{
    if (!NFMemTransMng::Instance()->CheckStopServer())
    {
        return false;
    }
    return true;
}

bool NFCMemOtherModule::StopServer()
{
    if (!NFMemTransMng::Instance()->StopServer())
    {
        return false;
    }
    return true;
}


