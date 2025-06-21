// -------------------------------------------------------------------------
//    @FileName         :    NFMemPlugin.h
//    @Author           :    gaoyi
//    @Date             :   2022-09-18
//    @Module           :    NFMemPlugin
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFPluginModule/NFIPlugin.h"
#include "NFComm/NFPluginModule/NFIPluginManager.h"

//////////////////////////////////////////////////////////////////////////
class NFMemPlugin final : public NFIPlugin
{
public:
	explicit NFMemPlugin(NFIPluginManager* p): NFIPlugin(p)
	{
	}

	~NFMemPlugin() override
	{
	}

	int GetPluginVersion() override;

	std::string GetPluginName() override;

	void Install() override;

	void Uninstall() override;

	bool IsDynamicLoad() override;

	bool InitShmObjectRegister() override;
};

