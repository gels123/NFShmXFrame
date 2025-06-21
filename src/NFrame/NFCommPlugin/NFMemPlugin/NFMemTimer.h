// -------------------------------------------------------------------------
//    @FileName         :    NFMemTimer.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFMemTimer.h
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFObjCommon/NFObject.h"
#include "NFComm/NFObjCommon/NFNodeList.h"
#include "NFComm/NFObjCommon/NFObjPtr.h"
#include "NFComm/NFObjCommon/NFRawObject.h"

#define NFSHM_INFINITY_CALL                0xffffffff    // 调用无限次

enum NFTimerRetType
{
    E_TIMER_TYPE_SUCCESS = 0, // 执行成功
    E_TIMER_HANDLER_NULL = 1, // 回调为空
    E_TIMER_NOT_TRIGGER = 2, // 没有触发
};

class NFMemTimer final : public NFObjectTemplate<NFMemTimer, EOT_TYPE_TIMER_OBJ, NFObject>, public NFListNodeObjWithGlobalId<NFMemTimer>
{
public:
    enum NFMemTimerType
    {
        LOOP_TIMER,
        ONCE_TIMER,
        MONTH_LOOP_TIMER,
    };

    NFMemTimer();

    ~NFMemTimer() override;

    int CreateInit();

    int ResumeInit();

    NFObject* GetTimerShmObj();

    int GetTimerShmObjId() const;

    void SetTimerShmObj(const NFObject* pObj);

    void SetTimerRawShmObj(const NFRawObject* pObj);

    NFRawObject* GetTimerRawShmObj();

    void PrintfDebug() const;

    NFTimerRetType HandleTimer(int timeId, int callCount);

    bool IsTimeOut(int64_t tick);

    NFTimerRetType OnTick(int64_t tick);

    bool IsDelete() const;

    void SetDelete();

    bool IsWaitDelete() const;

    void SetWaitDelete();

    NFMemTimerType GetType() const { return m_type; }

    int64_t GetBeginTime() const { return m_beginTime; }

    int64_t GetInterval() const { return m_interval; }

    int64_t GetNextRun() const { return m_nextRun; }

    int32_t GetCallCount() const { return m_callCount; }

    int GetSlotIndex() const { return m_slotIndex; }

    int GetListIndex() const { return m_listIndex; }

    std::string GetDetailStructMsg() const;

    void SetType(NFMemTimerType type) { m_type = type; }

    void SetCallCount(int32_t t) { m_callCount = t; }

    void SetInterval(int64_t interval) { m_interval = interval; }

    void SetNextRun() { m_nextRun += m_interval; }

    void SetNextRun(int64_t nextTime) { m_nextRun = nextTime; }

    void SetBeginTime(int64_t time) { m_beginTime = time; }

    void SetRound(int round) { m_round = round; }

    void SetSlotIndex(int index) { m_slotIndex = index; }

    void SetListIndex(int index) { m_listIndex = index; }

private:
    void DeleteFunc();

private:
    NFObjPtr<NFObject> m_shmObj;
    NFRawShmPtr<NFRawObject> m_rawShmObj;
    int m_shmObjId;
    NFMemTimerType m_type;

    int64_t m_beginTime; // 开始的时间
    int64_t m_nextRun; // 下一次执行的时间
    int64_t m_interval; // 间隔时间
    int32_t m_callCount;
    int32_t m_curCallCount;
    bool m_delFlag; // 是否删除
    int m_round;
    int m_slotIndex; // 槽id
    bool m_waitDel; // 等待删除标记
    int m_listIndex; // 绑定的list的序号，当为-1时，代表已经脱离绑定
};
