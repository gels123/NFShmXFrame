// -------------------------------------------------------------------------
//    @FileName         :    NFIModule.h
//    @Author           :    LvSheng.Huang
//    @Date             :   2022-09-18
//    @Module           :    NFIModule
//
// -------------------------------------------------------------------------

#pragma once

#include <string>
#include "NFBaseObj.h"

class NFIModule : public NFBaseObj
{
public:
    NFIModule(NFIPluginManager *p) : NFBaseObj(p)
    {

    }

    virtual ~NFIModule()
    {
    }

    virtual bool AfterLoadAllPlugin()
    {
        return true;
    }

    virtual bool AfterInitShmMem()
    {
        return true;
    }

    virtual bool Awake()
    {
        return true;
    }

    virtual bool Init()
    {
        return true;
    }

    virtual bool CheckConfig()
    {
        return true;
    }

    virtual bool ReadyExecute()
    {
        return true;
    }

    virtual bool Execute()
    {
        return true;
    }

    virtual bool BeforeShut()
    {
        return true;
    }

    virtual bool Shut()
    {
        return true;
    }

    virtual bool Finalize()
    {
        return true;
    }

    /*
     * 热更完所有配置后，模块调用的函数
     * */
    virtual bool OnReloadConfig()
    {
        return true;
    }

    /*
     * 热更配置时，模块热更的调用的函数
     * */
    virtual bool AfterOnReloadConfig()
    {
        return true;
    }

    /*
     * 动态热更dll/so之后，模块调用的函数
     * */
    virtual bool OnDynamicPlugin()
    {
        return true;
    }

    /*
     * 热更退出app, 用于服务器需要热更app代码的情况，这时候会杀掉正在运行的的的app,重启新的服务器app
     * */
    virtual bool HotfixServer()
    {
        return true;
    }

    /*
     * 停服之前，检查服务器是否满足停服条件, 如果不满足执行StopServer()
     * */
    virtual bool CheckStopServer()
    {
        return true;
    }

    /*
     * 停服之前，如果不满足停服条件， 做一些操作，满足停服条件
     * */
    virtual bool StopServer()
    {
        return true;
    }

    /*
     * 服务器被杀掉之前，执行这个函数
     * */
    virtual bool OnServerKilling()
    {
        return true;
    }

    /**
     * @brief 服务器连接完成后
     * @return
     */
    virtual bool AfterAllConnectFinish()
    {
        return true;
    }

    /**
     * @brief 加载完服务器数据，包过excel, 以及从数据拉取的数据
     * @return
     */
    virtual bool AfterAllDescStoreLoaded()
    {
        return true;
    }

    virtual bool AfterAllConnectAndAllDescStore()
    {
        return true;
    }

    /**
     * @brief 从db加载全局数据, 这个加载一定在完成连接后，有可能依赖descstore数据，也可能不依赖
     * @return
     */
    virtual bool AfterObjFromDBLoaded()
    {
        return true;
    }

    /**
     * @brief 完成服务器之间的注册
     * @return
     */
    virtual bool AfterServerRegisterFinish()
    {
        return true;
    }

    /**
     * @brief  服务器完成初始化之后
     * @return
     */
    virtual bool AfterAppInitFinish()
    {
        return true;
    }

    /**
     * @brief 服务器连接完成后调用。
     * @return 成功返回 true，失败返回 false。
     */
    virtual bool AfterAllConnectFinish(NF_SERVER_TYPE serverType)
    {
        return true;
    }

    /**
     * @brief 加载完服务器数据（如 Excel 和数据库数据）后调用。
     * @return 成功返回 true，失败返回 false。
     */
    virtual bool AfterAllDescStoreLoaded(NF_SERVER_TYPE serverType)
    {
        return true;
    }

    /**
     * @brief
     * @return AfterAllConnectFinish 和 AfterAllDescStoreLoaded 都完成后
     */
    virtual bool AfterAllConnectAndAllDescStore(NF_SERVER_TYPE serverType)
    {
        return true;
    }

    /**
     * @brief 从数据库加载全局数据后调用。
     * @return 成功返回 true，失败返回 false。
     */
    virtual bool AfterObjFromDBLoaded(NF_SERVER_TYPE serverType)
    {
        return true;
    }

    /**
     * @brief 完成服务器之间的注册后调用。
     * @return 成功返回 true，失败返回 false。
     */
    virtual bool AfterServerRegisterFinish(NF_SERVER_TYPE serverType)
    {
        return true;
    }

    /**
     * @brief 服务器完成初始化后调用。
     * @return 成功返回 true，失败返回 false。
     */
    virtual bool AfterAppInitFinish(NF_SERVER_TYPE serverType)
    {
        return true;
    }

    std::string m_strName;
};

