// -------------------------------------------------------------------------
//    @FileName         :    NFIPlugin.h
//    @Author           :    LvSheng.Huang
//    @Date             :   2022-09-18
//    @Module           :    NFIPlugin
//
// -------------------------------------------------------------------------

#pragma once


#include <iostream>
#include <map>
#include <vector>
#include "NFComm/NFCore/NFPlatform.h"
#include "NFIModule.h"


//为什么要在这里加上pManager->InitSingleton()呢， 主要是为了在动态加载的情况下，在每个DLL中，都初始化一次单件系统
#define REGISTER_MODULE(pManager, classBaseName, className)  \
    assert((TIsDerived<classBaseName, NFIModule>::Result)); \
    assert((TIsDerived<className, classBaseName>::Result)); \
    NFIModule* pRegisterModule##className= new className(pManager); \
    pRegisterModule##className->m_strName = (#className);             \
    pManager->AddModule(#classBaseName, pRegisterModule##className );AddModule(#classBaseName, pRegisterModule##className );

#define UNREGISTER_MODULE(pManager, classBaseName, className) \
   NFIModule* pUnRegisterModule##className =  dynamic_cast<NFIModule*>(pManager->FindModule(typeid(classBaseName).name())); \
	pManager->RemoveModule( #classBaseName );RemoveModule(#classBaseName); delete pUnRegisterModule##className;

#define REGISTER_STATIC_PLUGIN(pManager, className)  pManager->RegisteredStaticPlugin(#className, [] (NFIPluginManager* pMan) ->NFIPlugin* { return NF_NEW className(pMan);});

#define CREATE_PLUGIN(pManager, className)  NFIPlugin* pCreatePlugin##className = new className(pManager); pManager->Registered( pCreatePlugin##className );

#define DESTROY_PLUGIN(pManager, className) pManager->UnRegistered( pManager->FindPlugin((#className)) );

class NFIPluginManager;

class NFIPlugin : public NFIModule
{
public:
	NFIPlugin(NFIPluginManager* p):NFIModule(p)
	{
	}

	virtual int GetPluginVersion() = 0;

	virtual std::string GetPluginName() = 0;

	virtual void Install() = 0;

	virtual void Uninstall() = 0;
public:
    bool AfterLoadAllPlugin() override;

    bool AfterInitShmMem() override;

	bool Awake() override;

	bool Init() override;

	bool CheckConfig() override;

	bool ReadyExecute() override;

	bool Execute() override;

	bool BeforeShut() override;

	bool Shut() override;

	bool Finalize() override;

	bool OnReloadConfig() override;

    bool AfterOnReloadConfig() override;
    /*
     * 热更退出app, 用于服务器需要热更app代码的情况，这时候会杀掉正在运行的的的app,重启新的服务器app
     * */
    bool HotfixServer() override;

    /*
     * 停服之前，检查服务器是否满足停服条件
     * */
    bool CheckStopServer() override;

    /*
     * 停服之前，做一些操作，满足停服条件
     * */
    bool StopServer() override;

    /*
     * 停服之前保存需要的数据
     * */
    bool OnServerKilling() override;

	bool OnDynamicPlugin() override;
public:
    virtual bool InitShmObjectRegister();


	virtual bool IsDynamicLoad();


	virtual void AddModule(const std::string& moduleName, NFIModule* pModule);

	virtual void RemoveModule(const std::string& moduleName);

    /**
    * @brief 服务器连接完成后
    * @return
    */
    bool AfterAllConnectFinish() override;

    /**
     * @brief 加载完服务器数据，包过excel, 以及从数据拉取的数据
     * @return
     */
    bool AfterAllDescStoreLoaded() override;

	bool AfterAllConnectAndAllDescStore() override;

    /**
     * @brief 从db加载全局数据, 这个加载一定在完成连接后，有可能依赖descstore数据，也可能不依赖
     * @return
     */
    bool AfterObjFromDBLoaded() override;

    /**
     * @brief 完成服务器之间的注册
     * @return
     */
    bool AfterServerRegisterFinish() override;

    /**
     * @brief  服务器完成初始化之后
     * @return
     */
    bool AfterAppInitFinish() override;

	/**
	 * @brief 服务器连接完成后调用。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool AfterAllConnectFinish(NF_SERVER_TYPE serverType) override;

	/**
	 * @brief 加载完服务器数据（如 Excel 和数据库数据）后调用。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool AfterAllDescStoreLoaded(NF_SERVER_TYPE serverType) override;

	/**
	 * @brief
	 * @return AfterAllConnectFinish 和 AfterAllDescStoreLoaded 都完成后
	 */
	bool AfterAllConnectAndAllDescStore(NF_SERVER_TYPE serverType) override;

	/**
	 * @brief 从数据库加载全局数据后调用。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool AfterObjFromDBLoaded(NF_SERVER_TYPE serverType) override;

	/**
	 * @brief 完成服务器之间的注册后调用。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool AfterServerRegisterFinish(NF_SERVER_TYPE serverType) override;

	/**
	 * @brief 服务器完成初始化后调用。
	 * @return 成功返回 true，失败返回 false。
	 */
	bool AfterAppInitFinish(NF_SERVER_TYPE serverType) override;
protected:
	std::map<std::string, NFIModule*> m_mapModule;
	std::vector<NFIModule*> m_vecModule;
};



