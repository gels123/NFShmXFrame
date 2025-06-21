// -------------------------------------------------------------------------
//    @FileName         :    NFTutorialModule.cpp
//    @Author           :    gaoyi
//    @Date             :    24-1-13
//    @Email			:    445267987@qq.com
//    @Module           :    NFTutorialModule
//
// -------------------------------------------------------------------------

#include "NFTutorialModule.h"

#include <FrameBehavior.pb.h>
#include <FrameTestComm.pb.h>

#define TUTORIAL_TIMER_ID 10

NFTutorialModule::NFTutorialModule(NFIPluginManager* p): NFIDynamicModule(p)
{
	m_idCount = 0;
}

NFTutorialModule::~NFTutorialModule()
{
}

bool NFTutorialModule::Awake()
{
	NFLogError(NF_LOG_DEFAULT, 0, "tutorial awake...........");
	/**
	 * ��һ��10��ִ��һ�εĶ�ʱ��
	 */
	SetTimer(TUTORIAL_TIMER_ID, 10000);
	return NFIDynamicModule::Awake();
}

bool NFTutorialModule::Init()
{
	NFLogError(NF_LOG_DEFAULT, 0, "tutorial init...........");

	return NFIDynamicModule::Init();
}

bool NFTutorialModule::Execute()
{
	return NFIDynamicModule::Execute();
}

bool NFTutorialModule::Shut()
{
	NFLogError(NF_LOG_DEFAULT, 0, "tutorial shut...........");
	return NFIDynamicModule::Shut();
}

bool NFTutorialModule::Finalize()
{
	NFLogError(NF_LOG_DEFAULT, 0, "tutorial finalize...........");
	return NFIDynamicModule::Finalize();
}

bool NFTutorialModule::OnDynamicPlugin()
{
	NFLogError(NF_LOG_DEFAULT, 0, "tutorial OnDynamicPlugin...........");
	return NFIDynamicModule::OnDynamicPlugin();
}

int NFTutorialModule::OnTimer(uint32_t nTimerID)
{
	LOG_STATISTIC("%d, %s", 123, "456");
	NFrameComm::BevLogTest bevLog;
	bevLog.set_tdteventtime(NF_ADJUST_TIMENOW());
	bevLog.set_szvgameip(m_pObjPluginManager->GetBusName());
	bevLog.set_iizoneareaid(m_pObjPluginManager->GetZoneID());
	bevLog.set_iitimeshift(0);
	NFrameComm::BevLogTestItem item;
	item.set_item_id(1);
	item.set_item_num(100);
	*bevLog.mutable_item() = item;
	item.set_item_id(2);
	item.set_item_num(200);
	*bevLog.add_item_list() = item;
	item.set_item_id(3);
	item.set_item_num(300);
	*bevLog.add_item_list() = item;
	item.set_item_id(4);
	item.set_item_num(400);
	*bevLog.add_item_list() = item;
	bevLog.add_id_list(10);
	bevLog.add_id_list(20);
	bevLog.add_id_list(30);
	LOG_BEHAVIOUR(bevLog);
	if (nTimerID == TUTORIAL_TIMER_ID)
	{
		m_idCount++;
		NFLogError(NF_LOG_DEFAULT, 0, "id count:{}.......", m_idCount);
		//NFLogError(NF_LOG_DEFAULT, 0, "xxxxxxxxxxxxxxxxxxxxx");
	}
	return 0;
}
