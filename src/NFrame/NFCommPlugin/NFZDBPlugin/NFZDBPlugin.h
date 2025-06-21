// -------------------------------------------------------------------------
//    @FileName         :    NFZDBPlugin.h
//    @Author           :   gaoyi
//    @Date             :   2024-05-23
//    @Module           :    NFZDBPlugin
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFPluginModule/NFIPlugin.h"
#include "NFComm/NFPluginModule/NFIPluginManager.h"

#include "NfcTestZdbModule.h"

//////////////////////////////////////////////////////////////////////////
class NFZDBPlugin : public NFIPlugin
{
public:
	explicit NFZDBPlugin(NFIPluginManager* p):NFIPlugin(p)
	{

	}

	virtual ~NFZDBPlugin()
	{
	}

	virtual int GetPluginVersion() override;

	virtual std::string GetPluginName() override;

	virtual void Install() override;

	virtual void Uninstall() override;

	virtual bool IsDynamicLoad() override;
};

