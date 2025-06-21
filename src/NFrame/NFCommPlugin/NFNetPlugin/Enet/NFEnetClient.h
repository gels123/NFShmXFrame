// -------------------------------------------------------------------------
//    @FileName         :    NFEnetClient.h
//    @Author           :    gaoyi
//    @Date             :    2025-03-13
//    @Email			:    445267987@qq.com
//    @Module           :    NFEnetClient
//
// -------------------------------------------------------------------------

#pragma once

#include "NFIEnetConnection.h"

class NFEnetClient final : public NFIEnetConnection
{
public:
    NFEnetClient(NFIPluginManager* p, NF_SERVER_TYPE serverType, const NFMessageFlag& flag): NFIEnetConnection(p, serverType, flag), m_pHost(nullptr)
    {
    }

    bool Init() override;

    bool Execute() override;

    bool Shut() override;

    bool Finalize() override;

private:
    ENetHost* m_pHost;
};
