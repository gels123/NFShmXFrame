//------------------------------------------------------------------------ -
//    @FileName         :    NFNetPlugin.h
//    @Author           :    LvSheng.Huang
//    @Date             :   2022-09-18
//    @Module           :    NFNetPlugin
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFPluginModule/NFIPlugin.h"
#include "NFComm/NFPluginModule/NFIPluginManager.h"

//////////////////////////////////////////////////////////////////////////
class NFNetPlugin : public NFIPlugin
{
public:
	explicit NFNetPlugin(NFIPluginManager* p):NFIPlugin(p)
	{

	}

	int GetPluginVersion() override;

	std::string GetPluginName() override;

	void Install() override;

	void Uninstall() override;

	bool IsDynamicLoad() override;
};
