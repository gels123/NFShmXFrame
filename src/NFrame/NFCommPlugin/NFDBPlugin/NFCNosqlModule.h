#pragma once

#include "NFRedisDriver.h"
#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFCore/NFCommMapEx.hpp"
#include "NFComm/NFPluginModule/NFIPluginManager.h"
#include "NFComm/NFPluginModule/NFINosqlModule.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include "NFCNosqlDriverManager.h"

class NFCNosqlModule
	: public NFINosqlModule
{
public:
	NFCNosqlModule(NFIPluginManager* p);
	virtual ~NFCNosqlModule();
public:
	virtual bool Init();
	virtual bool Shut();
	virtual bool Execute();
	virtual bool AfterInit();

public:
	virtual int AddNosqlServer(const std::string& strID, const std::string& strIP);
	virtual int AddNosqlServer(const std::string& strID, const std::string& strIP, const int nPort);
	virtual int AddNosqlServer(const std::string& strID, const std::string& strIP, const int nPort, const std::string& strPass);
public:
    virtual int SelectObj(const std::string& strID, const NFrame::storesvr_selobj &select, NFrame::storesvr_selobj_res &select_res);
    virtual int SaveObj(const std::string& strID, const NFrame::storesvr_selobj &select,
                        NFrame::storesvr_selobj_res &select_res);
    virtual int SaveObj(const std::string& strID, const NFrame::storesvr_insertobj &select, NFrame::storesvr_insertobj_res &select_res);
    virtual int SaveObj(const std::string& strID, const NFrame::storesvr_modobj &select, NFrame::storesvr_modobj_res &select_res);
    virtual int SaveObj(const std::string& strID, const NFrame::storesvr_updateobj &select, NFrame::storesvr_updateobj_res &select_res);

    virtual int DeleteObj(const std::string& strID, const NFrame::storesvr_delobj &select);
    virtual int DeleteObj(const std::string& strID, const NFrame::storesvr_insertobj &select);
    virtual int DeleteObj(const std::string& strID, const NFrame::storesvr_modobj &select);
public:
    virtual bool Connect(const std::string& strIP, const int nPort, const std::string& strPass) { return false; };
    virtual bool Enable();
    virtual bool Busy();
    virtual bool KeepLive();
public:
	virtual NFINosqlDriver* GetNosqlDriver(const std::string& strID);
protected:
    NFCNosqlDriverManager* m_pNoSqlDriverManager;
};
