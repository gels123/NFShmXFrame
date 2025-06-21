// -------------------------------------------------------------------------
//    @FileName         :    NFTutorialShmObj.cpp
//    @Author           :    gaoyi
//    @Date             :    24-1-13
//    @Email			:    445267987@qq.com
//    @Module           :    NFTutorialShmObj
//
// -------------------------------------------------------------------------

#include "NFTutorialShmObj.h"

#include <NFComm/NFCore/NFCommon.h>

NFTutorialShmObj::NFTutorialShmObj()
{
	if (EN_OBJ_MODE_INIT == NFShmMgr::Instance()->GetCreateMode())
	{
		CreateInit();
	}
	else
	{
		ResumeInit();
	}
}

NFTutorialShmObj::~NFTutorialShmObj()
{
}

int NFTutorialShmObj::CreateInit()
{
	m_idCount = 0;
	//10???????��????
	m_timerId = SetTimer(10000, 0, 0, 0, 0, 0);
	m_test.push_back(NFTutorialTestData());
	NFLogError(NF_LOG_DEFAULT, 0, "CreateInit, m_idCount:{} m_timerId:{}", m_idCount, m_timerId);
	return 0;
}

int NFTutorialShmObj::ResumeInit()
{
	NFLogError(NF_LOG_DEFAULT, 0, "ResumeInit, m_idCount:{} m_timerId:{}", m_idCount, m_timerId);
	return 0;
}

int NFTutorialShmObj::OnTimer(int timeId, int callcount)
{
	if (timeId == m_timerId)
	{
		m_idCount++;
		NFLogError(NF_LOG_DEFAULT, 0, "OnTimer, m_idCount:{} m_timerId:{}", m_idCount, m_timerId);
	}
	return 0;
}
