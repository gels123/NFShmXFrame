// -------------------------------------------------------------------------
//    @FileName         :    NFCheckServerModule.cpp
//    @Author           :    gaoyi
//    @Date             :    2024/12/18
//    @Email            :    445267987@qq.com
//    @Module           :    NFCheckServerModule
//
// -------------------------------------------------------------------------

#include "NFCheckServerModule.h"

#include <NFServerComm/NFServerCommon/NFServerCommonDefine.h>
#include "NFComm/NFPluginModule/NFIPluginManager.h"
#include "NFComm/NFPluginModule/NFIMessageModule.h"
#include "NFServerComm/NFServerCommon/NFIServerMessageModule.h"
#include "NFComm/NFPluginModule/NFCheck.h"

NFCheckServerModule::NFCheckServerModule(NFIPluginManager* p): NFICheckServerModule(p)
{
}

NFCheckServerModule::~NFCheckServerModule()
{
}

bool NFCheckServerModule::Awake()
{
    BindServer();
    return true;
}

bool NFCheckServerModule::Init()
{
    ConnectMasterServer();
    return true;
}

bool NFCheckServerModule::Execute()
{
    return true;
}

bool NFCheckServerModule::OnDynamicPlugin()
{
    FindModule<NFIMessageModule>()->CloseAllLink(NF_ST_CHECK_SERVER);
    return true;
}

int NFCheckServerModule::OnHandleServerMessage(uint64_t unLinkId, NFDataPackage& packet)
{
    int retCode = 0;
    /*switch (packet.nMsgId)
    {
        default:
            NFLogError(NF_LOG_DEFAULT, 0, "msg:({}) not handle", packet.ToString());
        break;
    }*/

    if (retCode != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "msg:({}) handle exist error", packet.ToString());
    }
    return 0;
}
