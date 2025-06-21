// -------------------------------------------------------------------------
//    @FileName         :    NFServerTime.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFCore
//
// -------------------------------------------------------------------------

#pragma once

#include <string>

#include "NFPlatform.h"
#include "NFSingleton.hpp"

class _NFExport NFServerTime : public NFSingleton<NFServerTime>
{
public:
    NFServerTime();

    virtual ~NFServerTime();

public:
    bool Init(int fps);

    bool UnInit();

    bool Update(uint64_t tick);

    //gm set time
    int GmSetTime(int year, int month, int day, int hour, int min, int sec);
    int GmSetTime(uint64_t tTime);
public:
    uint64_t StartTick() { return m_startTick; }
    uint32_t CurElapse() { return (uint32_t) (m_curTick - m_startTick); }
    uint64_t Tick() { return m_curTick; }
    uint64_t LastTick() { return m_lastTick; }
    int DeltaTick() { return m_deltaTick; }
    uint64_t UnixSec() { return m_unixSec; }
    uint64_t UnixMSec() { return m_unixMSec; }
    uint64_t Frames() { return m_frames; }
    uint64_t Secs() { return m_secs; }
    int GetFPS() { return m_fps; }
    // 获得当日已过秒数(首日有16小时要去除)
    uint32_t CurDaySec() { return ((uint32_t) UnixSec() - 57600) % DayTotalSec(); };
    // 每日总秒数
    uint32_t DayTotalSec() { return m_daySec; };
    // 服务器当前所在的时区
    int32_t CurTimeZone() { return m_curzone; };

    bool CheckPerFrames(int perFrames) { return m_frames % perFrames == 0; }
    bool CheckPerSecs(int perSecs) { return m_secs % perSecs == 0 && m_perSecFirstFrame; }

    int GetSecOffSet() const { return m_iSecOffSet; }
    int SetSecOffSet(int iSecOffSet) { return m_iSecOffSet = iSecOffSet; }
protected:
    uint64_t m_startTick;
    uint64_t m_curTick; // 1 tick = 1 msec, cur update tick
    uint64_t m_lastTick; // 1 tick = 1 msec, last update tick
    int m_deltaTick; // 1 tick = 1 msec, ms_curTick - ms_lastTick
    uint64_t m_unixSec; // cur unix sec(cur frame is same)
    uint64_t m_unixMSec; // cur unix msec
    uint64_t m_frames; // frames count, from app start
    uint64_t m_secs; // seconds count, from app start
    bool m_perSecFirstFrame; // per sec the first frame is true
    int m_fps; // fps
    const uint32_t m_daySec = 86400; // day total sec
private:
    uint64_t m_lastUnixSec; // last unix sec
    int32_t m_curzone; // current time zone
private:
    int m_iSecOffSet;
};
