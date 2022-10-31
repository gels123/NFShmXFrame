// -------------------------------------------------------------------------
//    @FileName         :    NFStateBase.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-10-30
//    @Email			:    445267987@qq.com
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------
#pragma once

#include "NFComm/NFCore/NFPlatform.h"

/**
 * @brief ״̬����
 * @tparam T
 */
template<class T>
class NFStateBase
{
public:
    NFStateBase()
    {

    }

    virtual ~NFStateBase()
    {

    }

public:
    /**
     * ����״̬ʱ����
     * @param owner
     */
    void Enter(T owner)
    {

    }

    /**
     * �˳�״̬ʱ����
     * @param owner
     */
    void Exit(T owner)
    {

    }

    /**
     * �Ӹ�״̬ת����һ��״̬�Ĵ���
     * @param owner
     */
    void ToNextState(T owner)
    {

    }

    /**
     * ִ���¼�����
     * @param owner
     * @param cmd
     * @param gid
     * @param param
     */
    void Execute(T owner, uint32_t cmd)
    {

    }

    /**
     * ����
     * @param owner
     */
    void Reload(T owner)
    {

    }
};


