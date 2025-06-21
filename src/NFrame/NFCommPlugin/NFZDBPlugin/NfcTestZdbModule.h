// -------------------------------------------------------------------------
//    @FileName         :    NfcTestZdbModule.h
//    @Author           :    gaoyi
//    @Date             :    24-5-23
//    @Email			:    445267987@qq.com
//    @Module           :    NFCTestZdbModule
//
// -------------------------------------------------------------------------

#pragma once

#include <NFComm/NFPluginModule/NFIModule.h>
#include "NFComm/NFCore/NFPlatform.h"

class NFCTestZdbModule : public NFIModule
{
public:
    explicit NFCTestZdbModule(NFIPluginManager* p);

    virtual ~NFCTestZdbModule();

    virtual bool Awake();
};