// -------------------------------------------------------------------------
//    @FileName         :    NFCTestActorModule.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFTestPlugin
//
// -------------------------------------------------------------------------

#include "NFCTestModule.h"
#include "NFComm/NFPluginModule/NFIPluginManager.h"
#include "NFComm/NFPluginModule/NFIMysqlModule.h"

NFCTestModule::NFCTestModule(NFIPluginManager* p): NFIDynamicModule(p)
{
}

NFCTestModule::~NFCTestModule()
{
}



bool NFCTestModule::Init()
{
	SetTimer(1, 1000, 1);
	return true;
}


int NFCTestModule::OnTimer(uint32_t nTimerID)
{
    return 0;
}

bool NFCTestModule::Execute()
{
	return true;
}

bool NFCTestModule::BeforeShut()
{
	return true;
}

bool NFCTestModule::Shut()
{
	return true;
}
