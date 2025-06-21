
#pragma once

#include "NFINosqlDriver.h"

class NFINosqlModule
	: public NFIModule
{
public:
	NFINosqlModule(NFIPluginManager* p) : NFIModule(p)
	{

	}
public:
	virtual int AddNosqlServer(const std::string& strID, const std::string& ip) = 0;
	virtual int AddNosqlServer(const std::string& strID, const std::string& ip, const int nPort) = 0;
	virtual int AddNosqlServer(const std::string& strID, const std::string& ip, const int nPort, const std::string& strPass) = 0;
public:
    virtual int SelectObj(const std::string& strID, const NFrame::storesvr_selobj &select, NFrame::storesvr_selobj_res &select_res) = 0;
    virtual int SaveObj(const std::string& strID, const NFrame::storesvr_selobj &select, NFrame::storesvr_selobj_res &select_res) = 0;
    virtual int SaveObj(const std::string& strID, const NFrame::storesvr_insertobj &select, NFrame::storesvr_insertobj_res &select_res) = 0;
    virtual int SaveObj(const std::string& strID, const NFrame::storesvr_modobj &select, NFrame::storesvr_modobj_res &select_res) = 0;
    virtual int SaveObj(const std::string& strID, const NFrame::storesvr_updateobj &select, NFrame::storesvr_updateobj_res &select_res) = 0;

    virtual int DeleteObj(const std::string& strID, const NFrame::storesvr_delobj &select) = 0;
    virtual int DeleteObj(const std::string& strID, const NFrame::storesvr_insertobj &select) = 0;
    virtual int DeleteObj(const std::string& strID, const NFrame::storesvr_modobj &select) = 0;
public:
	virtual NFINosqlDriver* GetNosqlDriver(const std::string& strID) = 0;
};
