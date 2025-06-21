// -------------------------------------------------------------------------
//    @FileName         :    NFShmTimerMng.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFShmTimerMng.h
//
// -------------------------------------------------------------------------

#pragma once

#include "NFShmTimer.h"
#include "NFComm/NFShmStl/NFShmHashMap.h"
#include "NFComm/NFObjCommon/NFNodeList.h"
#include <list>

#define SLOT_COUNT 600
#define SLOT_TICK_TIME 32
//#define ALL_TIMER_COUNT 300000
#define ALL_TIMER_COUNT 30000
#define CUR_SLOT_TICK_MAX 500

struct NFTimerIdData
{
    int m_preIndex;
    int m_nextIndex;
    int m_curIndex;
    int m_objId; // timer的objectid
    int m_slotId; // slot的m_index
    bool m_isValid; // 是否非法，做简单判断用
};

class NFShmTimerMng;

class NFShmTimerSlot
{
public:
    NFShmTimerSlot();
    ~NFShmTimerSlot();

    int CreateInit();

    int ResumeInit();

    void SetIndex(int i) { m_index = i; }

    int AddTimer(NFShmTimer* timer, NFTimerIdData* idData, NFTimerIdData* allIdData);

    bool OnTick(NFShmTimerMng* pTimerManager, int64_t tick, std::list<NFShmTimer*>& timeoutList, uint32_t seq, NFTimerIdData* allIdData);

    bool DeleteTimer(NFShmTimerMng* pTimerManager, NFShmTimer* timer, NFTimerIdData* allIdData);

    NFTimerIdData* UnBindListTimer(NFShmTimerMng* pTimerManager, NFShmTimer* timer, const NFTimerIdData* tmpData, NFTimerIdData* allIdData);

    void ClearRunStatus(uint32_t seq);

    int GetCount() const { return m_count; }

private:
    //	map<uint32_t, Timer* > m_mapTimer;
    //	TIMER_SLOT_LIST m_mapTimer;
    NFTimerIdData m_headData;
    int m_index;
    uint32_t m_slotSeq; // 每次tick的seq
    int m_curRunIndex; // 当前运行到的链表的序号
    int m_count; // 定时器的个数
};

class NFShmTimerMng final : public NFObjectTemplate<NFShmTimerMng, EOT_TYPE_TIMER_MNG, NFObject>
{
public:
    NFShmTimerMng();

    ~NFShmTimerMng() override;

    int CreateInit();

    int ResumeInit();

    void OnTick(int64_t tick);

    // 删除此定时器
    int Delete(int objectId);

    NFShmTimer* GetTimer(int objectId);

    void ReleaseTimerIdData(int index);

    //注册距离现在多少时间执行一次的定时器(hour  minutes  second  microSec为第一次执行距离现在的时分秒毫秒, 只执行一次)
    int SetTimer(const NFObject* pObj, int hour, int minutes, int second, int microSec, const NFRawObject* pRawShmObj = nullptr);

    //注册某一个时间点执行一次的定时器(hour  minutes  second为第一次执行的时间点时分秒, 只执行一次)
    int SetCalender(const NFObject* pObj, int hour, int minutes, int second, const NFRawObject* pRawShmObj = nullptr);

    //注册某一个时间点执行一次的定时器(timestamp为第一次执行的时间点的时间戳,单位是秒, 只执行一次)
    int SetCalender(const NFObject* pObj, uint64_t timestamp, const NFRawObject* pRawShmObj = nullptr);

    //注册循环执行定时器（hour  minutes  second  microSec为第一次执行距离现在的时分秒毫秒,  interval 为循环间隔时间，为毫秒）
    int SetTimer(const NFObject* pObj, int interval, int callCount, int hour, int minutes, int second, int microSec, const NFRawObject* pRawShmObj = nullptr);

    //注册循环执行定时器（hour  minutes  second  microSec为第一次执行距离现在的时分秒毫秒）
    int SetDayTime(const NFObject* pObj, int callCount, int hour, int minutes, int second, int microSec, const NFRawObject* pRawShmObj = nullptr);

    //注册某一个时间点日循环执行定时器（hour  minutes  second为一天中开始执行的时间点，    23：23：23     每天23点23分23秒执行）
    int SetDayCalender(const NFObject* pObj, int callCount, int hour, int minutes, int second, const NFRawObject* pRawShmObj = nullptr);

    //周循环（hour  minutes  second  microSec为第一次执行距离现在的时分秒毫秒）
    int SetWeekTime(const NFObject* pObj, int callCount, int hour, int minutes, int second, int microSec, const NFRawObject* pRawShmObj = nullptr);

    //注册某一个时间点周循环执行定时器（ weekDay  hour  minutes  second 为一周中某一天开始执行的时间点）
    int SetWeekCalender(const NFObject* pObj, int callCount, int weekDay, int hour, int minutes, int second, const NFRawObject* pRawShmObj = nullptr);

    //月循环（hour  minutes  second  microSec为第一次执行距离现在的时分秒毫秒,最好是同一天）
    int SetMonthTime(const NFObject* pObj, int callCount, int hour, int minutes, int second, int microSec, const NFRawObject* pRawShmObj = nullptr);

    //注册某一个时间点月循环执行定时器（ day  hour  minutes  second 为一月中某一天开始执行的时间点）
    int SetMonthCalender(const NFObject* pObj, int callCount, int day, int hour, int minutes, int second, const NFRawObject* pRawShmObj = nullptr);

public:
    int AddShmObjTimer(const NFObject* pObj, NFShmTimer* pTimer);
    int ClearShmObjTimer(NFShmTimer* pTimer);
    int ClearAllTimer(const NFObject* pObj);
    int ClearAllTimer(const NFObject* pObj, const NFRawObject* pRawShmObj);

private:
    bool AttachTimer(NFShmTimer* timer, int64_t tick, bool isNewTimer);

    int AddTimer(NFShmTimer* timer, int64_t tick, bool isNewTimer = true);

    int AddTimer(NFShmTimer* timer, int slot);

    bool SetDistanceTime(NFShmTimer* stime, int hour, int minutes, int second, int microSec, int interval = 0, int callCount = 1);

    bool SetDayTime(NFShmTimer* stime, int hour, int minutes, int second, int interval = 0, int callCount = 1);

    bool SetDayTime(NFShmTimer* stime, uint64_t timestamp, int interval = 0, int callCount = 1);

    bool SetWeekTime(NFShmTimer* stime, int weekDay, int hour, int minutes, int second, int callCount = 1);

    bool SetMonthTime(NFShmTimer* stime, int day, int hour, int minutes, int second, int callCount = 1);

    // 获取空闲的链表结构
    NFTimerIdData* GetFreeTimerIdData();

    bool CheckFull() const;

private:
    NFShmTimerSlot m_slots[SLOT_COUNT];
    uint32_t m_currSlot;
    int64_t m_beforeTick; //上一次执行的tick数

    NFTimerIdData m_timerIdData[ALL_TIMER_COUNT + 1];
    int m_iFreeIndex;
    uint32_t m_timerSeq; // 每次tick的seq,只有当前m_currSlot已经遍历完了，才会++

    NFShmHashMap<int, NFNodeObjList<NFShmTimer>, ALL_TIMER_COUNT> m_shmObjTimer;
};
