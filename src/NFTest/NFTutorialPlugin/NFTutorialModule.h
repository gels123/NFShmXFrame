// -------------------------------------------------------------------------
//    @FileName         :    NFTutorialModule.h
//    @Author           :    gaoyi
//    @Date             :    24-1-13
//    @Email			:    445267987@qq.com
//    @Module           :    NFTutorialModule
//
// -------------------------------------------------------------------------

#pragma once

#include <NFComm/NFPluginModule/NFIDynamicModule.h>

class NFTutorialModule : public NFIDynamicModule
{
public:
    NFTutorialModule(NFIPluginManager* p);
    
    virtual ~NFTutorialModule();

    virtual bool Awake();

    virtual bool Init();

    virtual bool Execute();

    virtual bool Shut();

    virtual bool Finalize();

    /*
     * ��̬�ȸ�dll/so֮��ģ����õĺ���
     * */
    virtual bool OnDynamicPlugin();

	/**
	 * \brief 
	 * \param nTimerID ��ʱ������
	 * \return 
	 */
	virtual int OnTimer(uint32_t nTimerID) override;
private:
	uint32_t m_idCount;
};