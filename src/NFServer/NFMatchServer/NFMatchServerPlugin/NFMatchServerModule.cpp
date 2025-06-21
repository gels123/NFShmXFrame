// -------------------------------------------------------------------------
//    @FileName         :    NFMatchServerModule.cpp
//    @Author           :    gaoyi
//    @Date             :    2024/12/18
//    @Email            :    445267987@qq.com
//    @Module           :    NFMatchServerModule
//
// -------------------------------------------------------------------------

#include "NFMatchServerModule.h"

#include <NFServerComm/NFServerCommon/NFServerCommonDefine.h>
#include "NFComm/NFPluginModule/NFIPluginManager.h"
#include "NFComm/NFPluginModule/NFIMessageModule.h"
#include "NFServerComm/NFServerCommon/NFIServerMessageModule.h"
#include "NFComm/NFPluginModule/NFCheck.h"

NFMatchServerModule::NFMatchServerModule(NFIPluginManager* p): NFIMatchServerModule(p)
{
}

NFMatchServerModule::~NFMatchServerModule()
{
}

bool NFMatchServerModule::Awake()
{
    BindServer();
    return true;
}

bool NFMatchServerModule::Init()
{
    ConnectMasterServer();
    return true;
}

bool NFMatchServerModule::Execute()
{
    return true;
}

bool NFMatchServerModule::OnDynamicPlugin()
{
    FindModule<NFIMessageModule>()->CloseAllLink(NF_ST_MATCH_SERVER);
    return true;
}

int NFMatchServerModule::OnHandleServerMessage(uint64_t unLinkId, NFDataPackage& packet)
{
    int retCode = 0;
    switch (packet.nMsgId)
    {
        default:
            NFLogError(NF_LOG_DEFAULT, 0, "msg:({}) not handle", packet.ToString());
        break;
    }

    if (retCode != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "msg:({}) handle exist error", packet.ToString());
    }
    return 0;
}