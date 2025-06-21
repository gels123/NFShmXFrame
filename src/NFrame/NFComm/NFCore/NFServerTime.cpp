// -------------------------------------------------------------------------
//    @FileName         :    NFServerTime.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFCore
//
// -------------------------------------------------------------------------

#include "NFServerTime.h"

#include <NFComm/NFPluginModule/NFLogMgr.h>

#include "NFTime.h"
#include "NFTimeUtility.h"

NFServerTime::NFServerTime()
{
    m_startTick = 0;
    m_curTick = 0;
    m_lastTick = 0;
    m_deltaTick = 0;
    m_unixSec = 0;
    m_frames = 0;
    m_secs = 0;
    m_perSecFirstFrame = 0;
    m_lastUnixSec = 0;
    m_fps = 0;
    m_iSecOffSet = 0;
}

NFServerTime::~NFServerTime()
{
}

bool NFServerTime::Init(int fps)
{
    m_fps = fps;
    m_curTick = NFGetTime();
    m_startTick = m_curTick;
    m_lastTick = m_curTick;
    m_deltaTick = 0;
    m_unixSec = NFGetTime()/1000;
    m_unixMSec =  NFGetTime();
    m_frames = 0;
    m_secs = 0;
    m_perSecFirstFrame = false;
    m_lastUnixSec = m_unixSec;
    m_curzone = (NFTime::GetCurTimeZone());
    return true;
}

bool NFServerTime::UnInit()
{
    return true;
}

bool NFServerTime::Update(uint64_t tick)
{
    m_lastTick = m_curTick;
    m_curTick = tick;
    m_deltaTick = (int) (m_curTick - m_lastTick);
    m_unixSec = tick / 1000;
    m_unixMSec = tick;

    ++m_frames;

    if (m_lastUnixSec - m_unixSec > 0)
    {
        m_secs += m_unixSec - m_lastUnixSec;
        m_lastUnixSec = m_unixSec;
        m_perSecFirstFrame = true;
    }
    else
    {
        m_perSecFirstFrame = false;
    }

    return true;
}

int NFServerTime::GmSetTime(int year, int month, int day, int hour, int min, int sec)
{
    struct tm tm;
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;

    return GmSetTime(mktime(&tm));
}

int NFServerTime::GmSetTime(uint64_t tTime)
{
    if (tTime < m_unixSec)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "cant not set server time before now.");
        return 1;
    }
    else
    {
        m_iSecOffSet = tTime - m_unixSec;
        NFLogWarning(NF_LOG_DEFAULT, 0, "cur server time {}", NFTime(m_unixSec, 0).GetFormatTime());
        NFLogWarning(NF_LOG_DEFAULT, 0, "set server time {}", NFTime::Now().GetFormatTime());
    }
    return 0;
}
