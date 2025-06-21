// -------------------------------------------------------------------------
//    @FileName         :    NFMemTimerMng.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFMemTimerMng.h
//
// -------------------------------------------------------------------------

#pragma once

#include "NFMemTimer.h"
#include "NFComm/NFShmStl/NFShmHashMap.h"
#include "NFComm/NFObjCommon/NFNodeList.h"
#include <list>

class NFMemTimerMng;

#define SLOT_COUNT 600
#define SLOT_TICK_TIME 32
//#define ALL_TIMER_COUNT 300000
#define ALL_TIMER_COUNT 30000
#define CUR_SLOT_TICK_MAX 500

struct NFMemTimerIdData
{
    int m_preIndex;
    int m_nextIndex;
    int m_curIndex;
    int m_objId; // timer的objectid
    int m_slotId; // slot的m_index
    bool m_isValid; // 是否非法，做简单判断用
};

class NFMemTimerSlot
{
public:
    NFMemTimerSlot();
    ~NFMemTimerSlot();

    int CreateInit();

    int ResumeInit();

    void SetIndex(int i) { m_index = i; }

    int AddTimer(NFMemTimer* timer, NFMemTimerIdData* idData, NFMemTimerIdData* allIdData);

    bool OnTick(NFMemTimerMng* pTimerManager, int64_t tick, std::list<NFMemTimer*>& timeoutList, uint32_t seq, NFMemTimerIdData* allIdData);

    bool DeleteTimer(NFMemTimerMng* pTimerManager, NFMemTimer* timer, NFMemTimerIdData* allIdData);

    NFMemTimerIdData* UnBindListTimer(NFMemTimerMng* pTimerManager, NFMemTimer* timer, const NFMemTimerIdData* tmpData, NFMemTimerIdData* allIdData);

    void ClearRunStatus(uint32_t seq);

    int GetCount() const { return m_count; }

private:
    /**
     * @struct NFMemTimerIdData
     * @brief 内存定时器ID数据结构，用于管理定时器的链表头。
     */
    NFMemTimerIdData m_headData;

    /**
     * @var int m_index
     * @brief 当前索引，用于标识定时器在链表中的位置。
     */
    int m_index;

    /**
     * @var uint32_t m_slotSeq
     * @brief 每次tick的序列号，用于标识每次时间轮转的序号。
     */
    uint32_t m_slotSeq;

    /**
     * @var int m_curRunIndex
     * @brief 当前运行到的链表的序号，用于标识当前正在处理的链表位置。
     */
    int m_curRunIndex;

    /**
     * @var int m_count
     * @brief 定时器的个数，用于记录当前管理的定时器数量。
     */
    int m_count;
};

class NFMemTimerMng final : public NFObjectTemplate<NFMemTimerMng, EOT_TYPE_TIMER_MNG, NFObject>
{
public:
    NFMemTimerMng();

    ~NFMemTimerMng() override;

    int CreateInit();

    int ResumeInit();

    void OnTick(int64_t tick);

    // 删除此定时器
    int Delete(int objectId);

    NFMemTimer* GetTimer(int objectId);

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
    int AddShmObjTimer(const NFObject* pObj, NFMemTimer* pTimer);
    int ClearShmObjTimer(NFMemTimer* pTimer);
    int ClearAllTimer(const NFObject* pObj);
    int ClearAllTimer(const NFObject* pObj, const NFRawObject* pRawShmObj);

private:
    bool AttachTimer(NFMemTimer* timer, int64_t tick, bool isNewTimer);

    int AddTimer(NFMemTimer* timer, int64_t tick, bool isNewTimer = true);

    int AddTimer(NFMemTimer* timer, int slot);

    bool SetDistanceTime(NFMemTimer* stime, int hour, int minutes, int second, int microSec, int interval = 0, int callCount = 1);

    bool SetDayTime(NFMemTimer* stime, int hour, int minutes, int second, int interval = 0, int callCount = 1);

    bool SetDayTime(NFMemTimer* stime, uint64_t timestamp, int interval = 0, int callCount = 1);

    bool SetWeekTime(NFMemTimer* stime, int weekDay, int hour, int minutes, int second, int callCount = 1);

    bool SetMonthTime(NFMemTimer* stime, int day, int hour, int minutes, int second, int callCount = 1);

    NFMemTimerIdData* GetFreeTimerIdData();

    bool CheckFull() const;

private:
    NFMemTimerSlot m_slots[SLOT_COUNT];
    uint32_t m_currSlot;
    int64_t m_beforeTick; //上一次执行的tick数

    NFMemTimerIdData m_timerIdData[ALL_TIMER_COUNT + 1];
    int m_iFreeIndex;
    uint32_t m_timerSeq; // 每次tick的seq,只有当前m_currSlot已经遍历完了，才会++

    std::unordered_map<int, NFNodeObjList<NFMemTimer>> m_shmObjTimer;
};
