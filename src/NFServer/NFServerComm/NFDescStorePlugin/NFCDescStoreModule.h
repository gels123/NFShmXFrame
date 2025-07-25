// -------------------------------------------------------------------------
//    @FileName         :    NFCDescStoreModule.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFCDescStoreModule
//
// -------------------------------------------------------------------------

#pragma once

#include "NFServerComm/NFServerCommon/NFIDescStoreModule.h"
#include "NFServerComm/NFServerCommon/NFIDescStore.h"
#include "NFComm/NFPluginModule/NFEventObj.h"
#include "NFComm/NFPluginModule/NFEventObj.h"
#include "NFServerComm/NFServerCommon/NFIDescStoreEx.h"
#include <unordered_map>

class NFCDescStoreModule : public NFIDescStoreModule
{
public:
	NFCDescStoreModule(NFIPluginManager* p);
	~NFCDescStoreModule() override;

    virtual bool AfterInitShmMem() override;

	virtual bool Awake() override;

    virtual bool Execute() override;

	virtual bool OnReloadConfig() override;

	/*
	 * 停服之前，检查服务器是否满足停服条件
	 * */
	virtual bool CheckStopServer() override;

	/*
	 * 停服之前，做一些操作，满足停服条件
	 * */
	virtual bool StopServer() override;

	virtual bool AfterAllConnectFinish(NF_SERVER_TYPE serverType) override;
public:
	// 注册描述存储模块，支持数据库存储
	virtual void RegisterDescStore(const std::string& strClassName, int objType, const std::string& dbName) override;
    // 注册描述存储模块，使用默认数据库
    virtual void RegisterDescStore(const std::string& strClassName, int objType) override;
    // 注册扩展描述存储模块
    virtual void RegisterDescStoreEx(const std::string& strClassName, int objType) override;

	/**
	 * @brief 反注册描述存储模块
	 * @param strClassName 类名
	 * @note 用于动态卸载不再需要的描述存储模块，释放相关资源
	 */
	virtual void UnRegisterDescStore(const std::string& strClassName) override;

	/**
	 * @brief 反注册扩展描述存储模块
	 * @param strClassName 类名
	 * @note 用于动态卸载不再需要的扩展描述存储模块，释放相关资源
	 */
	virtual void UnRegisterDescStoreEx(const std::string& strClassName) override;

	virtual NFIDescStore* FindDescStore(const std::string& strDescName) override;

    virtual NFIDescStore* FindDescStoreByFileName(const std::string& strDescName) override;

	virtual int Initialize();

	virtual int LoadFileDescStore();
    /**
     * @brief 从数据库加载描述存储数据
     * @return 加载结果状态码
     * @note 当需要从数据库加载配置表时，应在各服务器的AfterAllConnectFinish回调中调用此接口
     */
    int LoadDBDescStore() override;

	virtual void AddDescStore(const std::string& strDescName, NFIDescStore* pDesc);

    virtual void AddDescStoreEx(const std::string& strDescName, NFIDescStoreEx* pDescEx);

	virtual void RemoveDescStore(const std::string& strDescName);

    virtual int InsertDescStoreByFileName(const std::string& dbName, const std::string& strDescName, const google::protobuf::Message *pMessage) override;
    virtual int DeleteDescStoreByFileName(const std::string& dbName, const std::string& strDescName, const google::protobuf::Message *pMessage) override;;
    virtual int SaveDescStoreByFileName(const std::string& dbName, const std::string& strDescName, const google::protobuf::Message *pMessage) override;;

	virtual void InitAllDescStore();
    virtual void InitAllDescStoreEx();

	virtual int InitDescStore(const std::string& descClass, NFIDescStore* pDescStore);
    virtual int InitDescStoreEx(const std::string& descClass, NFIDescStoreEx* pDescStore);
    virtual bool IsAllDescStoreLoaded() override;
	virtual bool IsAllDescStoreDBLoaded() override;

    virtual int LoadAllFileDescStore();
    virtual int LoadDB();
	virtual int Reload();


	virtual int LoadFileDescStore(NFIDescStore *pDescStore);
	virtual int LoadDBDescStore(NFIDescStore *pDescStore);

	virtual int ReLoadFileDescStore(NFIDescStore *pDescStore);

	virtual int ReLoadFileDescStore();
	virtual int ReLoadDBDescStore();
	bool HasDBDescStore();

	virtual int CheckWhenAllDataLoaded();

	virtual int LoadAllDescStoreEx();
	virtual int CheckExWhenAllDataLoaded();

	int GetFileContainMD5(const std::string& strFileName, std::string& fileMd5);

    NFResDb *CreateResDBFromRealDB();

    NFResDb *CreateResDBFromFiles(const std::string& dir);

    virtual void runAfterShmInit();
public:
    virtual int GetDescStoreByRpc(NF_SERVER_TYPE eType, const std::string& dbName, const std::string &table_name, google::protobuf::Message *pMessage) override;
private:
	std::unordered_map<std::string, NFIDescStore*> mDescStoreMap;
    std::unordered_map<std::string, NFIDescStore*> mDescStoreFileMap;
	std::unordered_map<std::string, int> mDescStoreRegister;
    std::vector<std::string> mDescStoreRegisterList;    //记录注册顺序，根据顺序来加载
	std::unordered_map<std::string, std::string> mDescStoreDBNameMap;
    NFResDb* m_pResFileDB;
    NFResDb* m_pResSqlDB;
    bool m_bFinishLoaded;
	bool m_bFinishReloaded;
	bool m_bLoadingDB;
private:
    std::unordered_map<std::string, NFIDescStoreEx*> mDescStoreExMap;
    std::unordered_map<std::string, int> mDescStoreExRegister;
    std::vector<std::string> mDescStoreExRegisterList;    //记录注册顺序，根据顺序来加载
};
