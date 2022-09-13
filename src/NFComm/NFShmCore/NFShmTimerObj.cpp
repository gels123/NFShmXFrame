// -------------------------------------------------------------------------
//    @FileName         :    NFShmTimerObj.cpp
//    @Author           :    xxxxx
//    @Date             :   xxxx-xx-xx
//    @Email			:    xxxxxxxxx@xxx.xxx
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------

#include "NFShmTimerObj.h"
#include "NFShmMgr.h"

NFShmTimerObj::NFShmTimerObj()
{
	if (NFShmMgr::Instance()->GetCreateMode() == EN_OBJ_MODE_INIT)
	{
		CreateInit();
	}
	else
	{
		ResumeInit();
	}
}


NFShmTimerObj::~NFShmTimerObj()
{

}

int NFShmTimerObj::CreateInit()
{
#ifdef NF_DEBUG_MODE
    m_shmTimerCount = 0;
#endif
	return 0;
}

int NFShmTimerObj::DeleteTimer(int timeObjId)
{
	int iRet = NFShmMgr::Instance()->DeleteTimer(this, timeObjId);
    if (iRet == 0)
    {
#ifdef NF_DEBUG_MODE
        if (m_shmTimerCount > 0)
        {
            m_shmTimerCount--;
        }
        else
        {
            NFLogError(NF_LOG_SYSTEMLOG, 0, "error delete timer, m_shmTimerCount = 0");
        }
#endif
    }
    return iRet;
}

//ע��������ڶ���ʱ��ִ��һ�εĶ�ʱ��(hour  minutes  second  microSecΪ��һ��ִ�о������ڵ�ʱ�������, ִֻ��һ��)
int NFShmTimerObj::SetTimer(int hour, int minutes, int second, int microSec)
{
	int timerId = NFShmMgr::Instance()->SetTimer(this, hour, minutes, second, microSec);
    if (timerId >= 0)
    {
#ifdef NF_DEBUG_MODE
        m_shmTimerCount++;
        if (m_shmTimerCount >= 10)
        {
            NFLogWarning(NF_LOG_SYSTEMLOG, 0, "m_shmTimerCount:{}, the obj has too much timer", m_shmTimerCount);
        }
#endif
    }
    return timerId;
}

//ע��ĳһ��ʱ���ִ��һ�εĶ�ʱ��(hour  minutes  secondΪ��һ��ִ�е�ʱ���ʱ����, ִֻ��һ��)
int NFShmTimerObj::SetCalender(int hour, int minutes, int second)
{
    int timerId = NFShmMgr::Instance()->SetCalender(this, hour, minutes, second);
    if (timerId >= 0)
    {
#ifdef NF_DEBUG_MODE
        m_shmTimerCount++;
        if (m_shmTimerCount >= 10)
        {
            NFLogWarning(NF_LOG_SYSTEMLOG, 0, "m_shmTimerCount:{}, the obj has too much timer", m_shmTimerCount);
        }
#endif
    }
    return timerId;
}

//ע��ĳһ��ʱ���ִ��һ�εĶ�ʱ��(timestampΪ��һ��ִ�е�ʱ����ʱ���,��λ����, ִֻ��һ��)
int NFShmTimerObj::SetCalender(uint64_t timestamp)
{
    int timerId = NFShmMgr::Instance()->SetCalender(this, timestamp);
    if (timerId >= 0)
    {
#ifdef NF_DEBUG_MODE
        m_shmTimerCount++;
        if (m_shmTimerCount >= 10)
        {
            NFLogWarning(NF_LOG_SYSTEMLOG, 0, "m_shmTimerCount:{}, the obj has too much timer", m_shmTimerCount);
        }
#endif
    }
    return timerId;
}

//ע��ѭ��ִ�ж�ʱ����hour  minutes  second  microSecΪ��һ��ִ�о������ڵ�ʱ�������,  interval Ϊѭ�����ʱ�䣬Ϊ���룩
int NFShmTimerObj::SetTimer(int interval, int callcount, int hour, int minutes, int second, int microSec)
{
    int timerId = NFShmMgr::Instance()->SetTimer(this, interval, callcount, hour, minutes, second, microSec);
    if (timerId >= 0)
    {
#ifdef NF_DEBUG_MODE
        m_shmTimerCount++;
        if (m_shmTimerCount >= 10)
        {
            NFLogWarning(NF_LOG_SYSTEMLOG, 0, "m_shmTimerCount:{}, the obj has too much timer", m_shmTimerCount);
        }
#endif
    }
    return timerId;
}

//ע��ѭ��ִ�ж�ʱ����hour  minutes  second  microSecΪ��һ��ִ�о������ڵ�ʱ������룩
int NFShmTimerObj::SetDayTime(int callcount, int hour, int minutes, int second, int microSec)
{
    int timerId = NFShmMgr::Instance()->SetDayTime(this, callcount, hour, minutes, second, microSec);
    if (timerId >= 0)
    {
#ifdef NF_DEBUG_MODE
        m_shmTimerCount++;
        if (m_shmTimerCount >= 10)
        {
            NFLogWarning(NF_LOG_SYSTEMLOG, 0, "m_shmTimerCount:{}, the obj has too much timer", m_shmTimerCount);
        }
#endif
    }
    return timerId;
}

//ע��ĳһ��ʱ�����ѭ��ִ�ж�ʱ����hour  minutes  secondΪһ���п�ʼִ�е�ʱ��㣬    23��23��23     ÿ��23��23��23��ִ�У�
int NFShmTimerObj::SetDayCalender(int callcount, int hour, int minutes, int second)
{
    int timerId = NFShmMgr::Instance()->SetDayCalender(this, callcount, hour, minutes, second);
    if (timerId >= 0)
    {
#ifdef NF_DEBUG_MODE
        m_shmTimerCount++;
        if (m_shmTimerCount >= 10)
        {
            NFLogWarning(NF_LOG_SYSTEMLOG, 0, "m_shmTimerCount:{}, the obj has too much timer", m_shmTimerCount);
        }
#endif
    }
    return timerId;
}

//��ѭ����hour  minutes  second  microSecΪ��һ��ִ�о������ڵ�ʱ������룩
int NFShmTimerObj::SetWeekTime(int callcount, int hour, int minutes, int second, int microSec)
{
    int timerId = NFShmMgr::Instance()->SetWeekTime(this, callcount, hour, minutes, second, microSec);
    if (timerId >= 0)
    {
#ifdef NF_DEBUG_MODE
        m_shmTimerCount++;
        if (m_shmTimerCount >= 10)
        {
            NFLogWarning(NF_LOG_SYSTEMLOG, 0, "m_shmTimerCount:{}, the obj has too much timer", m_shmTimerCount);
        }
#endif
    }
    return timerId;
}

//ע��ĳһ��ʱ�����ѭ��ִ�ж�ʱ���� weekDay  hour  minutes  second Ϊһ����ĳһ�쿪ʼִ�е�ʱ��㣩
int NFShmTimerObj::SetWeekCalender(int callcount, int weekDay, int hour, int minutes, int second)
{
    int timerId = NFShmMgr::Instance()->SetWeekCalender(this, callcount, weekDay, hour, minutes, second);
    if (timerId >= 0)
    {
#ifdef NF_DEBUG_MODE
        m_shmTimerCount++;
        if (m_shmTimerCount >= 10)
        {
            NFLogWarning(NF_LOG_SYSTEMLOG, 0, "m_shmTimerCount:{}, the obj has too much timer", m_shmTimerCount);
        }
#endif
    }
    return timerId;
}

//��ѭ����hour  minutes  second  microSecΪ��һ��ִ�о������ڵ�ʱ�������,�����ͬһ�죩
int NFShmTimerObj::SetMonthTime(int callcount, int hour, int minutes, int second, int microSec)
{
    int timerId = NFShmMgr::Instance()->SetMonthTime(this, callcount, hour, minutes, second, microSec);
    if (timerId >= 0)
    {
#ifdef NF_DEBUG_MODE
        m_shmTimerCount++;
        if (m_shmTimerCount >= 10)
        {
            NFLogWarning(NF_LOG_SYSTEMLOG, 0, "m_shmTimerCount:{}, the obj has too much timer", m_shmTimerCount);
        }
#endif
    }
    return timerId;
}

//ע��ĳһ��ʱ�����ѭ��ִ�ж�ʱ���� day  hour  minutes  second Ϊһ����ĳһ�쿪ʼִ�е�ʱ��㣩
int NFShmTimerObj::SetMonthCalender(int callcount, int day, int hour, int minutes, int second)
{
    int timerId = NFShmMgr::Instance()->SetMonthCalender(this, callcount, day, hour, minutes, second);
    if (timerId >= 0)
    {
#ifdef NF_DEBUG_MODE
        m_shmTimerCount++;
        if (m_shmTimerCount >= 10)
        {
            NFLogWarning(NF_LOG_SYSTEMLOG, 0, "m_shmTimerCount:{}, the obj has too much timer", m_shmTimerCount);
        }
#endif
    }
    return timerId;
}