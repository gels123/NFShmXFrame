// -------------------------------------------------------------------------
//    @FileName         :    NFShmPlugin.h
//    @Author           :    gaoyi
//    @Date             :   2022-09-18
//    @Module           :    NFShmPlugin
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFPluginModule/NFIPlugin.h"
#include "NFComm/NFPluginModule/NFIPluginManager.h"

//////////////////////////////////////////////////////////////////////////
class NFShmPlugin final : public NFIPlugin
{
public:
	explicit NFShmPlugin(NFIPluginManager* p): NFIPlugin(p)
	{
	}

	~NFShmPlugin() override
	{
	}

	int GetPluginVersion() override;

	std::string GetPluginName() override;

	void Install() override;

	void Uninstall() override;

	bool IsDynamicLoad() override;

	bool InitShmObjectRegister() override;
};

