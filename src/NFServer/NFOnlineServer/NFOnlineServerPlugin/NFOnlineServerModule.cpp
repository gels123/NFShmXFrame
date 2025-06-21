// -------------------------------------------------------------------------
//    @FileName         :    NFOnlineServerModule.cpp
//    @Author           :    gaoyi
//    @Date             :    2024/12/18
//    @Email            :    445267987@qq.com
//    @Module           :    NFOnlineServerModule
//
// -------------------------------------------------------------------------

#include "NFOnlineServerModule.h"

#include <NFServerComm/NFServerCommon/NFServerCommonDefine.h>
#include "NFComm/NFPluginModule/NFIPluginManager.h"
#include "NFComm/NFPluginModule/NFIMessageModule.h"
#include "NFServerComm/NFServerCommon/NFIServerMessageModule.h"
#include "NFComm/NFPluginModule/NFCheck.h"

NFOnlineServerModule::NFOnlineServerModule(NFIPluginManager* p): NFIOnlineServerModule(p)
{
}

NFOnlineServerModule::~NFOnlineServerModule()
{
}

bool NFOnlineServerModule::Awake()
{
    BindServer();
    return true;
}

bool NFOnlineServerModule::Init()
{
    ConnectMasterServer();
    return true;
}

bool NFOnlineServerModule::Execute()
{
    return true;
}

bool NFOnlineServerModule::OnDynamicPlugin()
{
    FindModule<NFIMessageModule>()->CloseAllLink(NF_ST_ONLINE_SERVER);
    return true;
}

int NFOnlineServerModule::OnHandleServerMessage(uint64_t unLinkId, NFDataPackage& packet)
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