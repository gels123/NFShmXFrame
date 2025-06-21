// -------------------------------------------------------------------------
//    @FileName         :    NFEnetServer.h
//    @Author           :    gaoyi
//    @Date             :    2025-03-13
//    @Email			:    445267987@qq.com
//    @Module           :    NFEnetServer
//
// -------------------------------------------------------------------------

#pragma once

#include "NFIEnetConnection.h"

class NFEnetServer : public NFIEnetConnection
{
public:
    NFEnetServer(NFIPluginManager* p, NF_SERVER_TYPE serverType, const NFMessageFlag& flag);
    ~NFEnetServer() override;

public:
    bool Init() override;

    bool Execute() override;

    bool Shut() override;

    bool Finalize() override;

private:
    ENetHost* m_pHost;
};
