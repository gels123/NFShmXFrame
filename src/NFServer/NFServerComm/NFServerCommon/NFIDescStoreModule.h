// -------------------------------------------------------------------------
//    @FileName         :    NFIDescStoreModule.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFIDescStoreModule.h
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFPluginModule/NFIModule.h"
#include "google/protobuf/message.h"
#include "NFComm/NFPluginModule/NFIAsyDBModule.h"
#include "NFComm/NFPluginModule/NFIDynamicModule.h"

class NFIDescStore;
class NFIDescStoreModule : public NFIDynamicModule
{
public:
    NFIDescStoreModule(NFIPluginManager* p):NFIDynamicModule(p)
    {

    }

    virtual ~NFIDescStoreModule()
    {

    }

	virtual void RegisterDescStore(const std::string& strClassName, int objType, const std::string& dbName) = 0;

    virtual void RegisterDescStore(const std::string& strClassName, int objType) = 0;

    virtual void RegisterDescStoreEx(const std::string& strClassName, int objType) = 0;

	virtual void UnRegisterDescStore(const std::string& strClassName) = 0;
	virtual void UnRegisterDescStoreEx(const std::string& strClassName) = 0;

	virtual NFIDescStore* FindDescStore(const std::string& strDescName) = 0;

	virtual NFIDescStore* FindDescStoreByFileName(const std::string& strDescName) = 0;

	virtual bool IsAllDescStoreLoaded() = 0;
	virtual bool IsAllDescStoreDBLoaded() = 0;

    /**
     * @brief 当有从DB数据数据库加载表格的需求时，请根据不同服务器，在使用服务器的bool AfterAllConnectFinish(NF_SERVER_TYPE serverType) override接口中，调用此接口。
     *			通用的很难搞定。因为服务器之间可能有依赖,会导致有依赖关系的服务器相互等待的情况发生
     * @return
     */
    virtual int LoadDBDescStore() = 0;

	template <typename T>
	T* FindDescStore()
	{
		NFIDescStore* pDescStore = FindDescStore(typeid(T).name());
		if (pDescStore)
		{
			if (!TIsDerived<T, NFIDescStore>::Result)
			{
				return nullptr;
			}
			//TODO OSX上dynamic_cast返回了NULL
#if NF_PLATFORM == NF_PLATFORM_APPLE
			T* pT = (T*)pDescStore;
#else
			T* pT = dynamic_cast<T*>(pDescStore);
#endif
			return pT;
		}
		return nullptr;
	}

    virtual int SaveDescStoreByFileName(const std::string& dbName, const std::string& strDescName, const google::protobuf::Message *pMessage) = 0;
    virtual int InsertDescStoreByFileName(const std::string& dbName, const std::string& strDescName, const google::protobuf::Message *pMessage) = 0;
    virtual int DeleteDescStoreByFileName(const std::string& dbName, const std::string& strDescName, const google::protobuf::Message *pMessage) = 0;

public:
    virtual int GetDescStoreByRpc(NF_SERVER_TYPE eType, const std::string& dbName, const std::string &table_name, google::protobuf::Message *pMessage) = 0;
};