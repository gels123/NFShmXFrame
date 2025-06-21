// -------------------------------------------------------------------------
//    @FileName         :    NFTimeUtil.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFTimeUtil.h
//
// -------------------------------------------------------------------------

#pragma once

#include "NFPlatform.h"

#include <stdlib.h>
#include <stdio.h>
#include "NFSocketLibFunction.h"


typedef struct timeval TTimeVal;

enum
{
    LOCAL_TIME_CORRECTION_HOUR = 8,
    LOCAL_TIME_CORRECTION = LOCAL_TIME_CORRECTION_HOUR * 3600, //本地时间时区偏移
    SECONDS_ADAY = 24 * 3600, //1天的秒数
    SECONDS_AWEEK = 24 * 3600 * 7, //1礼拜的秒数
    SECONDS_AHOUR = 3600,
    SECONDS_HALF_AHOUR = 1800,
    HOURS_AWEEK = 24 * 7,
    SECONDS_AMONTH = 24 * 3600 * 30,
    GAME_RESET_HOUR_EVERYDAY = 0, //游戏里按天重置的时间，比如疲劳，日常...
    GAME_RESET_MIN_EVERYDAY = 0,
    GAME_RESET_SEC_EVERYDAY = 0,
};

inline bool operator<(const TTimeVal &lhs, const TTimeVal &rhs)
{
    return ((lhs.tv_sec - rhs.tv_sec) * 1000000 + (lhs.tv_usec - rhs.tv_usec)) < 0;
}

inline bool operator<=(const TTimeVal &lhs, const TTimeVal &rhs)
{
    return !(rhs < lhs);
}

inline bool operator==(const TTimeVal &lhs, const TTimeVal &rhs)
{
    return (!(lhs < rhs) && !(rhs < lhs));
}

inline TTimeVal operator-(const TTimeVal &lhs, const TTimeVal &rhs)
{
    TTimeVal tvGap;
    tvGap.tv_sec = lhs.tv_sec - rhs.tv_sec;
    tvGap.tv_usec = lhs.tv_usec - rhs.tv_usec;

    if (tvGap.tv_usec < 0)
    {
        tvGap.tv_usec += 1000000;
        tvGap.tv_sec -= 1;
    }

    return tvGap;
}

class NFTimeUtil
{
public:
    /**
     * 将时间转换为日期时间字符串
     * @param mytime 指向time_t类型的指针，表示需要转换的时间
     * @param s 用于存储转换后日期时间字符串的字符数组
     * @param pio 指向整型变量的指针，用于输入输出转换过程中的信息
     * @param bOnlyDay 可选参数，若为true，则仅转换为日期格式，默认为false
     * @return 返回指向存储转换后日期时间字符串的字符数组的指针
     */
    static char *DateTimeToStr_R(time_t *mytime, char *s, int *pio, bool bOnlyDay = false);

    /**
     * 将时间转换为简化中文日期时间字符串
     * @param mytime 指向time_t类型的指针，表示需要转换的时间
     * @param s 用于存储转换后日期时间字符串的字符数组
     * @param pio 指向整型变量的指针，用于输入输出转换过程中的信息
     * @return 返回指向存储转换后简化中文日期时间字符串的字符数组的指针
     */
    static const char *DateTimeToStrSimCN_R(time_t *mytime, char *s, int *pio);

    /**
     * 将秒数转换为日期时间字符串
     * @param mytime 表示需要转换的秒数
     * @return 返回指向存储转换后日期时间字符串的字符数组的指针
     */
    static char *SecondToStr(time_t mytime);

    /**
     * 将时间转换为简化中文日期时间字符串
     * @param tmytime 表示需要转换的时间
     * @return 返回指向存储转换后简化中文日期时间字符串的字符数组的指针
     */
    static const char *DataTimeToStrSimCN(time_t tmytime);

    /**
     * 将微秒精度的时间转换为字符串
     * @param tvTime 表示需要转换的微秒精度时间
     * @param pszOut 可选参数，用于存储转换后时间字符串的字符数组，默认为NULL
     * @param iOutLen 可选参数，表示pszOut的长度，默认为128
     * @return 返回指向存储转换后时间字符串的字符数组的指针
     */
    static char *USecondTimeToStr(const TTimeVal &tvTime, char *pszOut = NULL, int iOutLen = 128);

    /**
     * 将时间转换为日期时间字符串
     * @param mytime 指向time_t类型的指针，表示需要转换的时间
     * @return 返回指向存储转换后日期时间字符串的字符数组的指针
     */
    static char *DateTimeToStr(time_t *mytime);

    /**
     * 将时间转换为日期时间字符串
     * @param mytime 表示需要转换的时间
     * @param bOnlyDay 可选参数，若为true，则仅转换为日期格式，默认为false
     * @return 返回指向存储转换后日期时间字符串的字符数组的指针
     */
    static char *DateTimeToStr(time_t mytime, bool bOnlyDay = false);

    /**
     * 将整型时间转换为日期时间字符串
     * @param imytime 表示需要转换的整型时间
     * @return 返回指向存储转换后日期时间字符串的字符数组的指针
     */
    static char *DateTimeToStr(int imytime);

    // 将uint32_t类型的时间转换为字符串表示
    // 参数dwMytime: 代表时间的uint32_t类型变量
    // 返回值: 转换后的字符串指针
    static char *DateTimeToStrDw(uint32_t dwMytime);

    // 将time_t类型的时间转换为字符串表示
    // 参数mytime: 指向time_t类型的时间变量
    // 参数piIn: 用于输出的字符数组指针
    // 返回值: 转换后的字符串指针
    static char *DateTimeToStr(time_t *mytime, char *piIn);

    // 将当前时间转换为字符串表示
    // 参数tNow: 表示当前时间的time_t类型变量
    // 返回值: 转换后的字符串指针
    static char *CurTimeToStr(time_t tNow);

    // xxxx 20160325 修改时间由外部传入，因为时间可能不是time（NULL）取到的当前时间
    // 将当前时间转换为字符串表示，带缓冲区长度参数
    // 参数tNow: 表示当前时间的time_t类型变量
    // 参数pszDateTime: 用于输出的字符数组指针
    // 参数piInOutLen: 指向缓冲区长度的整型指针
    // 返回值: 转换后的字符串指针
    static char *CurTimeToStr_R(time_t tNow, char *pszDateTime, int *piInOutLen);


    /*
    //note: rhs 应该是间隔,而不是实际的时间,否则出错
    //因为返回值是32位。只能表示24天
    inline unsigned int ToMs(const TTimeVal &rhs)
    {
        unsigned int m_tmp =0;
        m_tmp = rhs.tv_sec * 1000;
        m_tmp = m_tmp + rhs.tv_usec / 1000;
        return m_tmp;
    }
    */
    static inline void AddMsWithPointer(TTimeVal *plhs, int iMs)
    {
        plhs->tv_sec += (iMs / 1000);
        plhs->tv_usec += ((iMs % 1000) * 1000);
        plhs->tv_sec += (plhs->tv_usec / 1000000);
        plhs->tv_usec = plhs->tv_usec % 1000000;
    }

    // Adds milliseconds to a TTimeVal structure
    static inline void AddMs(TTimeVal &lhs, int iMs)
    {
        AddMsWithPointer(&lhs, iMs);
    }

    // Adds microseconds to a TTimeVal structure, adjusting seconds as necessary
    static inline void AddUsWithPointer(TTimeVal *plhs, int iUs)
    {
        plhs->tv_usec += iUs;
        plhs->tv_sec += (plhs->tv_usec / 1000000);
        plhs->tv_usec = plhs->tv_usec % 1000000;
    }

    // Adds microseconds to a TTimeVal structure
    static inline void AddUs(TTimeVal &lhs, int iUs)
    {
        AddUsWithPointer(&lhs, iUs);
    }

    // Converts a string in the format "YYYY-MM-DD HH:MM:SS" to a time_t value
    static int StrToTime(const char *psztime, time_t *ptime);

    // Converts a string in the format "YYYY-MM-DD HH:MM:SS" to a time_t value without additional parameters
    static time_t StrToTimePure(const char *psztime);

    // Retrieves the system time in a specified buffer
    static int SysTime(char *pszBuff, int iBuff);

    // Converts __DATE__ and __TIME__ macros to a time_t value
    static time_t __DATE__TIME_toTime(const char *sz__DATE__, const char *sz__TIME__);

    // Retrieves the current time in microseconds
    static uint64_t GetCurrTimeUs();

    // Calculates the time difference in milliseconds between two timeval structures
    static inline unsigned int TimeMsPass(struct timeval *pstTv1, struct timeval *pstTv2)
    {
        int iSec;
        iSec = pstTv1->tv_sec - pstTv2->tv_sec;

        if (iSec < 0 || iSec > 100000)
        {
            iSec = 100000;
        }

        return iSec * 1000 + (pstTv1->tv_usec - pstTv2->tv_usec) / 1000;
    }

    // Calculates the time difference in microseconds between two timeval structures
    static inline int64_t TimeUsPass(struct timeval *pstTv1, struct timeval *pstTv2)
    {
        int iSec;
        iSec = pstTv1->tv_sec - pstTv2->tv_sec;

        if (iSec < 0 || iSec > 100000)
        {
            iSec = 100000;
        }

        return iSec * 1000000 + (pstTv1->tv_usec - pstTv2->tv_usec);
    }

    // Retrieves the current time of day in milliseconds
    static uint64_t GetTimeOfDayMS();

    /**
     * 将时间转换为字符串表示
     * @param pstCurr 指向时间结构体的指针，用于获取时间信息
     * @param pszString 用于存储时间字符串的字符数组指针，默认为NULL
     * @param iMaxLen 字符数组的最大长度，默认为64
     * @return 返回指向存储时间字符串的字符数组指针
     */
    static char *TimeToStr(const TTimeVal *pstCurr, char *pszString = NULL, int iMaxLen = 64);

    /**
     * 获取当前时间的字符串表示
     * @param pstCurr 指向时间结构体的指针，用于获取当前时间信息
     * @return 返回指向存储当前时间字符串的字符数组指针
     */
    static char *CurrTimeStr(const TTimeVal *pstCurr);

    /**
     * 生成一个表示时间的短整型，其中前7位表示年份（从2000年开始），中间4位表示月份，最后5位表示日期
     * @param tTime 表示时间的时间_t类型变量
     * @return 返回表示时间的短整型
     */
    static unsigned short MakeShortTime(time_t tTime);

    /**
     * 判断两个时间是否为游戏重置时间的同一天
     * @param tCur 第一个时间的时间_t类型变量
     * @param tBefore 第二个时间的时间_t类型变量
     * @return 如果两个时间是游戏重置时间的同一天，则返回true；否则返回false
     */
    static bool IsSameDayByGameResetTime(time_t tCur, time_t tBefore);

    /**
     * 获取给定时间的绝对周数
     * @param tTime 表示时间的时间_t类型变量
     * @return 返回给定时间的绝对周数
     */
    static uint32_t GetAbsWeek(time_t tTime);

    /**
     * 获取给定时间的绝对天数
     * @param tTime 表示时间的时间_t类型变量
     * @return 返回给定时间的绝对天数
     */
    static uint32_t GetAbsDay(time_t tTime);

    /**
     * 获取给定时间所在周的开始时间（以绝对时间表示）
     * @param tTime 表示时间的时间_t类型变量
     * @return 返回给定时间所在周的开始时间（以绝对时间表示）
     */
    static uint32_t GetThisWeekStartTime(time_t tTime);

    /**
     * 判断两个时间是否为同一个月份（考虑时区偏移）
     * @param tTimeA 第一个时间的时间_t类型变量
     * @param tTimeB 第二个时间的时间_t类型变量
     * @param iOffsetHour 时区偏移小时数
     * @return 如果两个时间是同一个月份，则返回true；否则返回false
     */
    static bool IsSameMonthWithOffsetHour(time_t tTimeA, time_t tTimeB, int iOffsetHour);

    /**
     * 判断两个时间是否为同一个月份
     * @param tTimeA 第一个时间的时间_t类型变量
     * @param tTimeB 第二个时间的时间_t类型变量
     * @return 如果两个时间是同一个月份，则返回true；否则返回false
     */
    static bool IsSameMonth(time_t tTimeA, time_t tTimeB);


    //根据周几和偏移时间值,拼接当前的绝对时间. bWeekDay 里 0 表示周一.
    //xxxx ,2013-03-21
    static uint32_t GetAbsTimeByWeekDayAndTime(time_t tNow, uint8_t bWeekDay, uint32_t dwWeekTime);

    // 获取自然年
    static uint16_t GetTimeYear(time_t tTime);

    // 获取星期几
    static uint16_t GetWeekDay(time_t tTime);

    // 获取当前星期几
    static uint16_t GetWeekDay();
    static uint16_t GetWeekDay127(time_t tTime);

    // 获取月份中的第几天
    static uint16_t GetMonthDay(time_t tTime);

    // 获取当前月份中的第几天
    static uint16_t GetMonthDay();

    // 获取月份
    static uint16_t GetMonth(time_t tTime);

    // 获取小时
    static uint16_t GetHour(time_t tTime);

    // 获取一天中的绝对秒数
    static uint32_t GetDayAbsSec(time_t tTime);

    // 将格式化的日期时间字符串转换为UTC时间
    // 格式：YYYY-MM-DD-HH-MM-SS
    static time_t time_str_to_utc(char *szInput);

    // 获取今天开始的时间，考虑到时区差异
    static time_t GetTodayStartTime(time_t tTimeNow, int iHour = 0);

    // 将日期时间字符串转换为本地时间
    // 新版本
    static int DataStrToLocalTimeNew(const char *pStr, time_t *pTime);

    // 将日期字符串转换为本地时间
    static int DateStrToLocalTime(const char *pStr, time_t *pTime);

    // 获取时间的秒部分
    static int GetTimeSec(const char *pStr, time_t *pTime);

    // 扩展版本，获取时间的秒部分
    static int GetTimeSecEx(const char *pStr, time_t *pTime);
};
