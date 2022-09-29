// -------------------------------------------------------------------------
//    @FileName         :    NFIModule.h
//    @Author           :    LvSheng.Huang
//    @Date             :   2022-09-18
//    @Module           :    NFIModule
//
// -------------------------------------------------------------------------

#pragma once

#include <string>
#include "NFObject.h"

class NFIModule : public NFObject
{
public:
    NFIModule(NFIPluginManager *p) : NFObject(p)
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
     * �ȸ����������ú�ģ����õĺ���
     * */
    virtual bool OnReloadConfig()
    {
        return true;
    }

    /*
     * �ȸ�����ʱ��ģ���ȸ��ĵ��õĺ���
     * */
    virtual bool AfterOnReloadConfig()
    {
        return true;
    }

    /*
     * ��̬�ȸ�dll/so֮��ģ����õĺ���
     * */
    virtual bool OnDynamicPlugin()
    {
        return true;
    }

    /*
     * �ȸ��˳�app, ���ڷ�������Ҫ�ȸ�app������������ʱ���ɱ���������еĵĵ�app,�����µķ�����app
     * */
    virtual bool HotfixExitApp()
    {
        return true;
    }

    /*
     * ͣ��֮ǰ�����������Ƿ�����ͣ������
     * */
    virtual bool CheckStopServer()
    {
        return true;
    }

    /*
     * ͣ��֮ǰ����һЩ����������ͣ������
     * */
    virtual bool OnStopServer()
    {
        return true;
    }

    /*
     * ����ͣ������, ͣ��֮ǰ������Ҫ������
     * */
    virtual bool SaveDB()
    {
        return true;
    }

    std::string m_strName;
};

