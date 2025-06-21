// -------------------------------------------------------------------------
//    @FileName         :    NFTime.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFCore
//
// -------------------------------------------------------------------------

#pragma once

#include <string>

#include "NFPlatform.h"

#include <time.h>
#include <stdint.h>
// Same to struct tm
struct _NFExport NFDate
{
	int sec;         /* seconds */
	int min;         /* minutes */
	int hour;        /* hours */
	int mday;        /* day of the month */
	int mon;         /* month */
	int year;        /* year */
	int wday;        /* day of the week */
	int yday;        /* day in the year */
	int isdst;       /* daylight saving time */
	long int __padding[4];
};

class _NFExport NFTime
{
protected:
	time_t sec_;
	uint64_t nsec_;
	uint64_t realSec_;
	NFTime() : sec_(0), nsec_(0), realSec_(0) {}
public:
	// create Time represents now
	static NFTime Now();

	//convert time
	static uint64_t GetSecTime(int year, int month, int day, int hour, int min, int sec);

	// create Time by specific date 年月日公历 非1900开始
	NFTime(int year, int month, int day, int hour, int min, int sec);

	// create Time by milliseconds from UnixMSec() or Tick()
	explicit NFTime(uint64_t ms)
	{
		sec_ = ms / 1000;
		nsec_ = (ms % 1000) * 1000000;
		realSec_ = 0;
	}

	// create Time by unix timestamp, nsec can be zero
	NFTime(time_t sec, uint64_t nsec)
	{
		sec_ = sec;
		nsec_ = nsec;
		realSec_ = 0;
	}

	// return number of milliseconds that have elapsed since
	// the system was started
	static uint64_t Tick();

	// return unix timestamp, from Jan 1 1970 UTC
	time_t UnixSec();

	time_t UnixSecOnMondayZero();

	// return unix timestamp, from Jan 1 1970 UTC
	uint64_t UnixMSec();

	uint64_t UnixNano();

	void LocalDate(struct NFDate *d);
	//static uint64_t LocalSec();
	//static uint64_t LocalMSec();
	bool IsZero();

	static void Sleep(uint32_t ms);

	// output formated time
	// fmt example:
	//  %Y-%m-%d[%H.%M.%S]
	//  %H:%M:%S
	bool LocalDateFormat(const char *fmt, char *timestr, size_t len);

	// %H:%M:%S
	bool LocalTime(char *timestr, size_t len)
	{
		return LocalDateFormat("%H:%M:%S", timestr, len);
	}

    // %H:%M:%S
    std::string LocalTime()
    {
        char str[32];
        if (LocalTime(str, sizeof(str)))
        {
            return std::string(str);
        }
        return std::string();
    }

	// %Y-%m-%d[%H.%M.%S]
	bool LocalDateTime(char *timestr, size_t len)
	{
		return LocalDateFormat("%Y-%m-%d[%H:%M:%S]", timestr, len);
	}

    // %Y-%m-%d[%H.%M.%S]
    std::string LocalDateTime()
    {
        char str[32];
        if (LocalDateTime(str, sizeof(str)))
        {
            return std::string(str);
        }
        return std::string();
    }

	// %Y-%m-%d %H:%M:%S
	bool GetFormatTime(char *timestr, size_t len)
	{
		return LocalDateFormat("%Y-%m-%d %H:%M:%S", timestr, len);
	}

    // %Y-%m-%d
    bool GetFormatDate(char *timestr, size_t len)
    {
        return LocalDateFormat("%Y-%m-%d", timestr, len);
    }

    // %Y-%m-%d
    std::string GetFormatDate()
    {
        char str[32];
        if (GetFormatDate(str, sizeof(str)))
        {
            return std::string(str);
        }
        return std::string();
    }

    // %Y-%m-%d %H:%M:%S
    std::string GetFormatTime()
    {
        char str[32];
        if (GetFormatTime(str, sizeof(str)))
        {
            return std::string(str);
        }
        return std::string();
    }

	uint64_t sec()
	{
		return sec_;
	}

	uint64_t nsec()
	{
		return nsec_;
	}

	static struct NFDate GetLocalDate();
    static uint64_t  GetLocalDateSec();
	static struct NFDate GetLocalDate(uint64_t unixSec);

	static int32_t GetLocalDayDifference(uint64_t left, uint64_t right);
	//获取每天更新的时间(秒数) nHour:时/* hours since midnight - [0,23] */
	static uint64_t GetDayUpdateTime(uint64_t timeSec, int32_t nHour);
	//获取每周更新的时间(秒数) nHour:时/* hours since midnight - [0,23] */
	static uint64_t GetWeekUpdateTime(uint64_t timeSec, int32_t nHour);

	//获取下周剩余的时间(秒数) nHour:时/* hours since midnight - [0,23] */
	static uint64_t GetNextWeekRemainingTime();

	//获取下月剩余的时间(秒数) nHour:时/* hours since midnight - [0,23] */
	static uint64_t GetNextMonthRemainingTime();

	static uint64_t GetZeroTime(uint64_t timeSec);

    static uint64_t GetMonthZeroTime(uint64_t timeSec);

    //获取当前服务器所在的时区,单位秒
    static int32_t GetCurTimeZone();
};

//ret time_t 系统时间
#define NF_TIMENOW() (NFServerTime::Instance()->UnixSec())

//ret time_t 系统时间，毫秒
#define NF_TIMENOW_MS() (NFServerTime::Instance()->UnixMSec())

//ret time_t 偏移时间
#define NF_ADJUST_TIMENOW() NFTime::Now().UnixSec()

//ret time_t 偏移时间，毫秒
#define NF_ADJUST_TIMENOW_MS() NFTime::Now().UnixMSec()
