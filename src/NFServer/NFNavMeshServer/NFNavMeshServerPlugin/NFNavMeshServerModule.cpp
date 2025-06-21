// -------------------------------------------------------------------------
//    @FileName         :    NFNavMeshServerModule.cpp
//    @Author           :    gaoyi
//    @Date             :    2024/12/18
//    @Email            :    445267987@qq.com
//    @Module           :    NFNavMeshServerModule
//
// -------------------------------------------------------------------------

#include "NFNavMeshServerModule.h"

#include <NFServerComm/NFServerCommon/NFServerCommonDefine.h>
#include "NFComm/NFPluginModule/NFIPluginManager.h"
#include "NFComm/NFPluginModule/NFIMessageModule.h"
#include "NFServerComm/NFServerCommon/NFIServerMessageModule.h"
#include "NFComm/NFPluginModule/NFCheck.h"

NFNavMeshServerModule::NFNavMeshServerModule(NFIPluginManager* p): NFINavMeshServerModule(p)
{
}

NFNavMeshServerModule::~NFNavMeshServerModule()
{
}

bool NFNavMeshServerModule::Awake()
{
    BindServer();
    return true;
}

bool NFNavMeshServerModule::Init()
{
    ConnectMasterServer();
    return true;
}

bool NFNavMeshServerModule::Execute()
{
    return true;
}

bool NFNavMeshServerModule::OnDynamicPlugin()
{
    FindModule<NFIMessageModule>()->CloseAllLink(NF_ST_NAVMESH_SERVER);
    return true;
}

int NFNavMeshServerModule::OnHandleServerMessage(uint64_t unLinkId, NFDataPackage& packet)
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