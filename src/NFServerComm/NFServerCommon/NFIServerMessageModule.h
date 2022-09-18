// -------------------------------------------------------------------------
//    @FileName         :    NFIMsgModule.h
//    @Author           :    xxxxx
//    @Date             :   xxxx-xx-xx
//    @Email			:    xxxxxxxxx@xxx.xxx
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFPluginModule/NFError.h"
#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFPluginModule/NFIModule.h"
#include "NFComm/NFPluginModule/NFIMessageModule.h"

#include <map>
#include <unordered_map>
#include <list>
#include <string>
#include <map>
#include <functional>


/// @brief ������Ϣ��ͨѶ�ӿ���
class NFIServerMessageModule : public NFIDynamicModule
{
public:
    NFIServerMessageModule(NFIPluginManager* p) :NFIDynamicModule(p)
    {

    }

    virtual ~NFIServerMessageModule()
    {

    }
public:
    virtual int SendMsgToMasterServer(NF_SERVER_TYPES eSendTyp, uint32_t nMsgId, const google::protobuf::Message &xData, uint64_t nParam1 = 0, uint64_t nParam2 = 0) = 0;
};
