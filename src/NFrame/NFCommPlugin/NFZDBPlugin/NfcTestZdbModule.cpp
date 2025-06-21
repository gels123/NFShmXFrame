// -------------------------------------------------------------------------
//    @FileName         :    NfcTestZdbModule.cpp
//    @Author           :    gaoyi
//    @Date             :    24-5-23
//    @Email			:    445267987@qq.com
//    @Module           :    NFCTestZdbModule
//
// -------------------------------------------------------------------------

#include "NfcTestZdbModule.h"

#include "NFCZDBDriver.h"
#include "zdb.h"

NFCTestZdbModule::NFCTestZdbModule(NFIPluginManager* p):NFIModule(p)
{
}

NFCTestZdbModule::~NFCTestZdbModule()
{
}

bool NFCTestZdbModule::Awake()
{
	NFCZDBDriver dbDriver;
	dbDriver.Connect(NFrame::E_DBTYPE_MYSQL, "proto_ff", "E_DBTYPE_MYSQL", 3306, "gaoyi", "Mangguoyi@401");
	return true;
}
