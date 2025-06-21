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

    virtual bool AfterLoadAllPlugin() override;

    virtual bool AfterInitShmMem() override;

	virtual bool Awake() override;

	virtual bool Init() override;

	virtual bool CheckConfig() override;

	virtual bool ReadyExecute() override;

	virtual bool Execute() override;

	virtual bool BeforeShut() override;

	virtual bool Shut() override;

	virtual bool Finalize() override;

	virtual bool OnReloadConfig() override;

    virtual bool AfterOnReloadConfig() override;
    /*
     * 热更退出app, 用于服务器需要热更app代码的情况，这时候会杀掉正在运行的的的app,重启新的服务器app
     * */
    virtual bool HotfixServer() override;

    /*
     * 停服之前，检查服务器是否满足停服条件
     * */
    virtual bool CheckStopServer() override;

    /*
     * 停服之前，做一些操作，满足停服条件
     * */
    virtual bool StopServer() override;

    /*
     * 停服之前保存需要的数据
     * */
    virtual bool OnServerKilling() override;

    virtual bool InitShmObjectRegister();

	virtual void Uninstall() = 0;

	virtual bool IsDynamicLoad();

	virtual bool OnDynamicPlugin() override;

	virtual void AddModule(const std::string& moduleName, NFIModule* pModule);

	virtual void RemoveModule(const std::string& moduleName);

    /**
    * @brief 服务器连接完成后
    * @return
    */
    virtual bool AfterAllConnectFinish() override;

    /**
     * @brief 加载完服务器数据，包过excel, 以及从数据拉取的数据
     * @return
     */
    virtual bool AfterAllDescStoreLoaded() override;

    /**
     * @brief 从db加载全局数据, 这个加载一定在完成连接后，有可能依赖descstore数据，也可能不依赖
     * @return
     */
    virtual bool AfterObjFromDBLoaded() override;

    /**
     * @brief 完成服务器之间的注册
     * @return
     */
    virtual bool AfterServerRegisterFinish() override;

    /**
     * @brief  服务器完成初始化之后
     * @return
     */
    virtual bool AfterAppInitFinish() override;
protected:
	std::map<std::string, NFIModule*> m_mapModule;
	std::vector<NFIModule*> m_vecModule;
};



