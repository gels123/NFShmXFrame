// -------------------------------------------------------------------------
//    @FileName         :    NFMemTimerMng.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFMemTimerMng.cpp
//
// -------------------------------------------------------------------------

#include "NFMemTimerMng.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include "NFComm/NFCore/NFTime.h"
#include "NFComm/NFCore/NFTimeUtility.h"
#include "NFComm/NFPluginModule/NFIMemMngModule.h"
#include "NFComm/NFPluginModule/NFCheck.h"

NFMemTimerSlot::NFMemTimerSlot()
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

NFMemTimerSlot::~NFMemTimerSlot()
{
}

int NFMemTimerSlot::CreateInit()
{
    m_headData.m_nextIndex = -1;
    m_headData.m_preIndex = -1;
    m_headData.m_curIndex = -1;
    m_headData.m_objId = -1;
    m_headData.m_slotId = -1;
    m_headData.m_isValid = false;
    m_index = -1;
    m_slotSeq = 0;
    m_curRunIndex = -1;
    m_count = 0;

    return 0;
}

int NFMemTimerSlot::ResumeInit()
{
    return 0;
}

int NFMemTimerSlot::AddTimer(NFMemTimer* timer, NFMemTimerIdData* idData, NFMemTimerIdData* allIdData)
{
    idData->m_objId = timer->GetObjId();
    idData->m_slotId = m_index;
    timer->SetSlotIndex(m_index);
    timer->SetListIndex(idData->m_curIndex);
    if (m_headData.m_preIndex == -1)
    {
        m_headData.m_preIndex = idData->m_curIndex;
        m_headData.m_nextIndex = idData->m_curIndex;
        idData->m_preIndex = -1;
        idData->m_nextIndex = -1;
    }
    else
    {
        if (m_headData.m_preIndex >= 0 && m_headData.m_preIndex < ALL_TIMER_COUNT)
        {
            allIdData[m_headData.m_preIndex].m_nextIndex = idData->m_curIndex;
            idData->m_nextIndex = -1;
            idData->m_preIndex = allIdData[m_headData.m_preIndex].m_curIndex;
            m_headData.m_preIndex = idData->m_curIndex;
        }
        else
        {
            NFLogError(NF_LOG_DEFAULT, 0, "head data index error :{} {}", m_index, timer->GetDetailStructMsg());
            return -1;
        }
    }

    ++m_count;
    return 0;
}

void NFMemTimerSlot::ClearRunStatus(uint32_t seq)
{
    m_curRunIndex = -1;
    m_slotSeq = seq;
}

bool NFMemTimerSlot::OnTick(NFMemTimerMng* pTimerManager, int64_t tick, list<NFMemTimer*>& timeoutList, uint32_t seq, NFMemTimerIdData* allIdData)
{
    if (seq == m_slotSeq)
    {
        // 表示已经遍历完了
        return true;
    }

    if (m_count <= 0)
    {
        m_count = 0;
        ClearRunStatus(seq);
        return true;
    }

    int curCount = 0;
    NFMemTimerIdData* tmpData = nullptr;
    int curTmpIndex = m_curRunIndex;
    int curTmpCount = m_count;

    if (m_curRunIndex == -1)
    {
        tmpData = &m_headData;
    }
    else
    {
        if (m_curRunIndex < 0 || m_curRunIndex >= ALL_TIMER_COUNT)
        {
            // 这个序号错误，就不再往下继续执行
            ClearRunStatus(seq);
            NFLogError(NF_LOG_DEFAULT, 0, "slot cur run index error:{} {}", m_curRunIndex, m_index);
            return true;
        }

        tmpData = &allIdData[m_curRunIndex];
    }

    // -1代表已经到了头节点
    while (tmpData->m_nextIndex != -1)
    {
        if (tmpData->m_nextIndex < 0 || tmpData->m_nextIndex >= ALL_TIMER_COUNT)
        {
            ClearRunStatus(seq);
            NFLogError(NF_LOG_DEFAULT, 0, "list next index error:{} {}", tmpData->m_curIndex, m_index);
            return true;
        }

        // 超出就下一帧来遍历
        if (curCount >= CUR_SLOT_TICK_MAX)
            break;

        tmpData = &allIdData[tmpData->m_nextIndex];
        if (!tmpData->m_isValid)
        {
            NFLogError(NF_LOG_DEFAULT, 0, "slot list index error : {} {} {} {}", m_index, curTmpCount,
                       curTmpIndex, tmpData->m_curIndex);
        }

        m_curRunIndex = tmpData->m_curIndex;
        NFMemTimer* pTimer = pTimerManager->GetTimer(tmpData->m_objId);
        if (pTimer && !pTimer->IsDelete())
        {
            if (pTimer->GetListIndex() != tmpData->m_curIndex)
            {
                NFLogError(NF_LOG_DEFAULT, 0, "time list index not equal to cur index: {} {}", tmpData->m_curIndex,
                           pTimer->GetDetailStructMsg());
            }

            if (pTimer->IsTimeOut(tick))
            {
                ++curCount;
                // 这个会自动返回一个tmpData
                tmpData = UnBindListTimer(pTimerManager, pTimer, tmpData, allIdData);
                //超时的定时器
                timeoutList.push_back(pTimer);
            }
        }
        else
        {
            if (!pTimer)
            {
                NFLogError(NF_LOG_DEFAULT, 0, "list index timer error : {} {} {}", m_index, tmpData->m_curIndex,
                           tmpData->m_objId);
            }

            ++curCount;
            tmpData = UnBindListTimer(pTimerManager, pTimer, tmpData, allIdData);
        }

        if (!tmpData)
        {
            ClearRunStatus(seq);
            NFLogError(NF_LOG_DEFAULT, 0, "UnBindListTimer error:{} {}", m_curRunIndex, m_index);
            return true;
        }
    }

    if (tmpData->m_nextIndex == -1)
    {
        ClearRunStatus(seq);
        return true;
    }

    return false;
}

NFMemTimerIdData* NFMemTimerSlot::UnBindListTimer(NFMemTimerMng* pTimerManager, NFMemTimer* timer, const NFMemTimerIdData* tmpData, NFMemTimerIdData* allIdData)
{
    if (!tmpData || !allIdData)
        return nullptr;

    NFMemTimerIdData* nextData = nullptr;
    NFMemTimerIdData* prefData = nullptr;

    if (tmpData->m_nextIndex == -1)
    {
        nextData = &m_headData;
    }
    else
    {
        nextData = &allIdData[tmpData->m_nextIndex];
    }

    if (tmpData->m_preIndex == -1)
    {
        prefData = &m_headData;
    }
    else
    {
        prefData = &allIdData[tmpData->m_preIndex];
    }

    nextData->m_preIndex = prefData->m_curIndex;
    prefData->m_nextIndex = nextData->m_curIndex;
    --m_count;
    if (m_curRunIndex == tmpData->m_curIndex)
    {
        m_curRunIndex = prefData->m_curIndex;
    }

    if (timer)
        timer->SetListIndex(-1);

    if (pTimerManager)
    {
        pTimerManager->ReleaseTimerIdData(tmpData->m_curIndex);
    }

    return prefData;
}

bool NFMemTimerSlot::DeleteTimer(NFMemTimerMng* pTimerManager, NFMemTimer* timer, NFMemTimerIdData* allIdData)
{
    if (!allIdData || !timer || timer->IsDelete() || timer->IsWaitDelete())
    {
        return false;
    }

    if (timer->GetListIndex() < 0 || timer->GetListIndex() >= ALL_TIMER_COUNT)
    {
        return false;
    }

    NFMemTimerIdData* tmpData = &allIdData[timer->GetListIndex()];
    if (tmpData->m_objId != timer->GetObjId())
    {
        NFLogError(NF_LOG_DEFAULT, 0, "timer objid not equal id data : {} {}", tmpData->m_objId,
                   timer->GetDetailStructMsg());
        return false;
    }

    if (tmpData->m_slotId != m_index)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "list index not equal slot index : {} {} {}", tmpData->m_slotId, m_index,
                   timer->GetDetailStructMsg());
        return false;
    }

    //	LOGSVR_TRACE("slot delete timer : " << m_index << " " << m_count << timer->GetDetailStructMsg());
    UnBindListTimer(pTimerManager, timer, tmpData, allIdData);

    return true;
}

NFMemTimerMng::NFMemTimerMng()
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

NFMemTimerMng::~NFMemTimerMng()
{
}

int NFMemTimerMng::CreateInit()
{
    for (int i = 0; i < SLOT_COUNT; ++i)
    {
        m_slots[i].CreateInit();
        m_slots[i].SetIndex(i);
    }

    NFLogInfo(NF_LOG_DEFAULT, 0, " init timer manager : {}", NF_ADJUST_TIMENOW_MS());
    m_currSlot = 0;
    m_beforeTick = NF_ADJUST_TIMENOW_MS();
    m_iFreeIndex = 0;
    m_timerSeq = 1;
    for (int i = 0; i < ALL_TIMER_COUNT; ++i)
    {
        m_timerIdData[i].m_nextIndex = i + 1;
        m_timerIdData[i].m_preIndex = 0;
        m_timerIdData[i].m_curIndex = i;
        m_timerIdData[i].m_objId = -1;
        m_timerIdData[i].m_slotId = -1;
        m_timerIdData[i].m_isValid = false;
    }

    m_timerIdData[ALL_TIMER_COUNT].m_nextIndex = ALL_TIMER_COUNT;
    m_timerIdData[ALL_TIMER_COUNT].m_preIndex = ALL_TIMER_COUNT;
    m_timerIdData[ALL_TIMER_COUNT].m_curIndex = ALL_TIMER_COUNT;
    m_timerIdData[ALL_TIMER_COUNT].m_objId = -1;
    m_timerIdData[ALL_TIMER_COUNT].m_slotId = -1;
    m_timerIdData[ALL_TIMER_COUNT].m_isValid = false;

    return 0;
}

int NFMemTimerMng::ResumeInit()
{
    auto pTimer = dynamic_cast<NFMemTimer*>(FindModule<NFIMemMngModule>()->GetHeadObj(EOT_TYPE_TIMER_OBJ));
    while (pTimer)
    {
        if (pTimer->IsDelete() || pTimer->IsWaitDelete())
        {
            continue;
        }

        NFLogTrace(NF_LOG_DEFAULT, 0, " time set callback : {}", pTimer->GetDetailStructMsg());

        if (pTimer->GetListIndex() < 0)
        {
            NFLogTrace(NF_LOG_DEFAULT, 0, "timer not in list: {}", pTimer->GetDetailStructMsg());
            if (!AttachTimer(pTimer, m_beforeTick, false))
            {
                NFLogTrace(NF_LOG_DEFAULT, 0, "timer attach err");
            }
        }

        pTimer = dynamic_cast<NFMemTimer*>(FindModule<NFIMemMngModule>()->GetNextObj(EOT_TYPE_TIMER_OBJ, pTimer));
    }
    return 0;
}

NFMemTimer* NFMemTimerMng::GetTimer(int objectId)
{
    return NFMemTimer::GetObjByObjId(objectId);
}

int NFMemTimerMng::Delete(int objectId)
{
    NFMemTimer* timer = GetTimer(objectId);
    if (!timer)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "--delete timer error, not exist: {}", objectId);
        return -1;
    }

    if (timer->IsDelete())
    {
        NFLogError(NF_LOG_DEFAULT, 0, "--delete timer error, timer is delete: {}", timer->GetDetailStructMsg());
        return -1;
    }

    if (timer->IsWaitDelete())
    {
        NFLogWarning(NF_LOG_DEFAULT, 0, "--delete timer error, timer is IsWaitDelete: {}", timer->GetDetailStructMsg());
        return -1;
    }

    NFLogTrace(NF_LOG_DEFAULT, 0, "--delete timer : {}", timer->GetDetailStructMsg());

    int index = timer->GetSlotIndex();
    if (index >= 0 && index < SLOT_COUNT)
    {
        if (!m_slots[index].DeleteTimer(this, timer, m_timerIdData))
        {
            // 删除时找不到，只可能在OnTick的临时list中
            // 将它置成可删除状态
            timer->SetWaitDelete();
            NFLogTrace(NF_LOG_DEFAULT, 0, "delete timer in temp list:{}", timer->GetDetailStructMsg());
            return 0;
        }
    }
    else
    {
        timer->PrintfDebug();
        NFLogError(NF_LOG_DEFAULT, 0, "delete timer index error: {}", timer->GetDetailStructMsg());
        return -1;
    }

    if (!timer->IsDelete())
    {
        timer->SetDelete();
        FindModule<NFIMemMngModule>()->DestroyObj(timer);
    }
    else
    {
        timer->PrintfDebug();
        NFLogError(NF_LOG_DEFAULT, 0, "delete timer status error:{}", timer->GetDetailStructMsg());
        return -1;
    }

    return 0;
}

void NFMemTimerMng::OnTick(int64_t tick)
{
    int tickCount = 0;

    while (true)
    {
        if (m_beforeTick + SLOT_TICK_TIME > tick)
        {
            return;
        }

        list<NFMemTimer*> lRetsetTimer;

        // m_currSlot的里面，是时间在m_beforeTick ~ （m_beforeTick + SLOT_TICK_TIME）区间的timer
        bool isNext = m_slots[m_currSlot].OnTick(this, m_beforeTick + SLOT_TICK_TIME, lRetsetTimer, m_timerSeq,
                                                 m_timerIdData);

        list<NFMemTimer*>::iterator it;
        for (it = lRetsetTimer.begin(); it != lRetsetTimer.end(); ++it)
        {
            //(*it)->PrintfDebug();

            bool isDel = true;

            if (!(*it)->IsWaitDelete())
            {
                if (E_TIMER_HANDLER_NULL != (*it)->OnTick(m_beforeTick + SLOT_TICK_TIME))
                {
                    // 如果不是一次性的定时器，就又加入槽
                    if ((*it)->GetType() != NFMemTimer::ONCE_TIMER &&
                        ((*it)->GetCallCount() > 0 || (*it)->GetCallCount() == static_cast<int32_t>(NFSHM_INFINITY_CALL)))
                    {
                        // 一定要确保用了m_currSlot就必须是m_beforeTick作为时间，这样才能一致
                        // 由于attacktimer里面用的是m_currSLot作为起始点，那么时间传m_beforeTick
                        if (AttachTimer(*it, m_beforeTick, false))
                        {
                            isDel = false;
                        }
                    }
                }
            }

            if (isDel && !(*it)->IsDelete())
            {
                //				LOGSVR_TRACE("--delete timer tick : "<< tick << (*it)->GetDetailStructMsg());
                (*it)->SetDelete();
                FindModule<NFIMemMngModule>()->DestroyObj(*it);
            }
        }

        ++tickCount;
        if (isNext)
        {
            m_beforeTick += SLOT_TICK_TIME;
            ++m_timerSeq;
            if (++m_currSlot >= static_cast<uint32_t>(SLOT_COUNT))
                m_currSlot = 0;
        }

        if (tickCount >= 10000 || (tick - m_beforeTick) < 2 * SLOT_TICK_TIME)
            break;
    }
}

int NFMemTimerMng::AddTimer(NFMemTimer* timer, int64_t tick, bool isNewTimer)
{
    if (!timer)
        return -1;

    if (timer->GetBeginTime() <= 0 || timer->GetNextRun() <= 0 || timer->GetNextRun() - timer->GetBeginTime() <= 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "add timer error: {}", timer->GetDetailStructMsg());
        return -1;
    }

    if (!AttachTimer(timer, tick, isNewTimer))
    {
        NFLogError(NF_LOG_DEFAULT, 0, "attach timer error:{}", timer->GetDetailStructMsg());
        return -1;
    }

    return 0;
}

int NFMemTimerMng::AddTimer(NFMemTimer* timer, int slot)
{
    if (slot < 0 || SLOT_COUNT <= slot)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "slot index error : {} {}", slot, timer->GetDetailStructMsg());
        return -1;
    }

    NFMemTimerIdData* idData = GetFreeTimerIdData();
    if (!idData)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "get free timer id error : {} {}", slot, timer->GetDetailStructMsg());
        return -1;
    }

    if (m_slots[slot].AddTimer(timer, idData, m_timerIdData))
    {
        ReleaseTimerIdData(idData->m_curIndex);
        NFLogError(NF_LOG_DEFAULT, 0, "add slot timer error: {} {}", slot, timer->GetDetailStructMsg());
        return -1;
    }

    //timer->PrintfDebug();
    //	LOGSVR_TRACE("add timer success: " << slot << " " << m_slots[slot].GetCount() << timer->GetDetailStructMsg());

    return slot;
}

bool NFMemTimerMng::CheckFull() const
{
    if (m_iFreeIndex == ALL_TIMER_COUNT)
        return true;

    return false;
}

void NFMemTimerMng::ReleaseTimerIdData(int index)
{
    if (index < 0 || index >= ALL_TIMER_COUNT)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "release timer id data error : {}", index);
        return;
    }

    NFMemTimerIdData* tmpBlock = &m_timerIdData[index];

    tmpBlock->m_nextIndex = m_iFreeIndex;
    tmpBlock->m_preIndex = 0;
    tmpBlock->m_objId = -1;
    tmpBlock->m_slotId = -1;
    tmpBlock->m_isValid = false;
    m_iFreeIndex = index;
}

NFMemTimerIdData* NFMemTimerMng::GetFreeTimerIdData()
{
    if (CheckFull())
    {
        NFLogError(NF_LOG_DEFAULT, 0, "timer id data is full");
        return nullptr;
    }

    int curIndex = m_iFreeIndex;
    if (m_timerIdData[m_iFreeIndex].m_nextIndex < 0 && m_timerIdData[m_iFreeIndex].m_nextIndex >= ALL_TIMER_COUNT)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "get id data but freeindex is invalid : {} {}", m_iFreeIndex,
                   m_timerIdData[m_iFreeIndex].m_nextIndex);
        return nullptr;
    }

    m_iFreeIndex = m_timerIdData[m_iFreeIndex].m_nextIndex;
    if (m_timerIdData[curIndex].m_isValid)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "get id data is invalid : {}", curIndex);
        return nullptr;
    }

    if (m_timerIdData[m_iFreeIndex].m_isValid)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "get id data, but free data invalid: {}", m_iFreeIndex);
    }

    m_timerIdData[curIndex].m_isValid = true;

    return &m_timerIdData[curIndex];
}

bool NFMemTimerMng::AttachTimer(NFMemTimer* timer, int64_t tick, bool isNewTimer)
{
    if (timer == nullptr || timer->IsDelete() || timer->IsWaitDelete())
    {
        return false;
    }

    int slotNum = 0;
    int ticks = 0;

    if (isNewTimer)
    {
        // 新定时器，加入的时候，有两种情况（tick表示的是服务器最新时间）：
        // 第一种：m_currSlot还没有运行，那么tick>=m_beforeTick
        // 第二种：m_currSlot已经运行，但是m_currSlot里面的定时器数量大于CUR_SLOT_TICK_MAX，那么tick >= (m_beforeTick+SLOT_TICK_TIME)
        // (timer->GetNextRun() - tick)>=SLOT_TICK_TIME,所以ticks>=1
        if (timer->GetNextRun() <= m_beforeTick)
        {
            NFLogError(NF_LOG_DEFAULT, 0, "next running time is less than cur ticked time : {}",
                       timer->GetDetailStructMsg());
            NFLogError(NF_LOG_DEFAULT, 0, "bext run: {}, m_beforeTick:{} ", timer->GetNextRun(), m_beforeTick);
            return false;
        }

        // 有一种可能性，造成注册时间，跟定时实际运行的时间不一致
        // 就是当服务器逻辑卡住了一会儿，或者m_currSlot里面的定时器数量大于CUR_SLOT_TICK_MAX，会造成后面注册的定时器起始时间
        // 跟定时器的当前时间有误差，因此修正
        int64_t distance = tick - m_beforeTick;
        if (distance < 0)
            distance = 0;

        ticks = (timer->GetNextRun() - tick + distance) / SLOT_TICK_TIME;
    }
    else
    {
        if (timer->GetType() == NFMemTimer::MONTH_LOOP_TIMER)
        {
            // 间隔月的天数不同，需要额外算间隔时间
            time_t t = timer->GetNextRun() / 1000;
            int nmonth = 0;
            int iYear = 0, iMonth = 0, iMonthDay = 0;

            NFTimeUtility::GetCurDate(static_cast<unsigned int>(t), iYear, iMonth, iMonthDay);
            int64_t interval = NFTimeUtility::GetNextMonthDay(iYear, iMonth, iMonthDay, nmonth);
            if (interval > 0)
            {
                timer->SetInterval(interval);
            }
            else
            {
                NFLogError(NF_LOG_DEFAULT, 0, "interval of timer is less than 0 :{}", timer->GetDetailStructMsg());
            }
        }

        timer->SetNextRun();
        if (timer->GetNextRun() - tick >= 0)
        {
            ticks = (timer->GetNextRun() - tick) / SLOT_TICK_TIME;
        }
        else
        {
            // 只有某一次定时器没有运行，当下一次论寻到他的时候，会发生这种情况
            NFLogError(NF_LOG_DEFAULT, 0, "next run time is less than tick : {} {} {}", tick,
                       m_slots[m_currSlot].GetCount(), timer->GetDetailStructMsg());
            ticks = 1;
        }
    }

    slotNum = (m_currSlot + ticks % SLOT_COUNT) % SLOT_COUNT;
    timer->SetRound(ticks / SLOT_COUNT + 1);
    slotNum = AddTimer(timer, slotNum);
    if (slotNum < 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "--add timer to slot but slot is less than 0 : {}",
                   timer->GetDetailStructMsg());
        return false;
    }

    return true;
}

bool NFMemTimerMng::SetDistanceTime(NFMemTimer* stime, int hour, int minutes, int second, int microSec, int interval, int callCount)
{
    int64_t start = static_cast<int64_t>(hour) * 3600 * 1000 + static_cast<int64_t>(minutes) * 60 * 1000 + static_cast<int64_t>(second) * 1000 + static_cast<int64_t>(microSec);
    if (start < SLOT_TICK_TIME)
        start = SLOT_TICK_TIME;

    int64_t nowTime = NF_ADJUST_TIMENOW_MS();
    stime->SetBeginTime(nowTime);
    stime->SetInterval(interval);
    stime->SetCallCount(callCount);
    stime->SetNextRun(nowTime + start);

    if (AddTimer(stime, nowTime))
    {
        return false;
    }

    return true;
}

bool NFMemTimerMng::SetDayTime(NFMemTimer* stime, uint64_t timestamp, int interval, int callCount)
{
    int64_t diffTime = 0;
    int64_t nowTime = NF_ADJUST_TIMENOW_MS();

    diffTime = timestamp * 1000 - nowTime;
    if (diffTime < 0)
    {
        diffTime += 24 * 3600 * 1000;
    }

    if (diffTime < SLOT_TICK_TIME)
        diffTime = SLOT_TICK_TIME;

    stime->SetBeginTime(nowTime);
    stime->SetInterval(interval);
    stime->SetCallCount(callCount);
    stime->SetNextRun(nowTime + diffTime);

    if (AddTimer(stime, nowTime))
    {
        return false;
    }

    return true;
}

bool NFMemTimerMng::SetDayTime(NFMemTimer* stime, int hour, int minutes, int second, int interval, int callCount)
{
    int64_t diftime = 0;
    time_t t = NF_ADJUST_TIMENOW();
    int nowWeek = 0, nowHour = 0, nowMinute = 0, nowSecond = 0;

    NFTimeUtility::GetCurTime(static_cast<unsigned int>(t), nowWeek, nowHour, nowMinute, nowSecond);

    diftime = (static_cast<int64_t>(hour) - static_cast<int64_t>(nowHour)) * 3600 * 1000 + (static_cast<int64_t>(minutes) - static_cast<int64_t>(nowMinute)) * 60 * 1000
        + (static_cast<int64_t>(second) - static_cast<int64_t>(nowSecond)) * 1000;
    if (diftime < 0)
    {
        diftime += 24 * 3600 * 1000;
    }

    if (diftime < SLOT_TICK_TIME)
        diftime = SLOT_TICK_TIME;

    int64_t nowTime = NF_ADJUST_TIMENOW_MS();
    stime->SetBeginTime(nowTime);
    stime->SetInterval(interval);
    stime->SetCallCount(callCount);
    stime->SetNextRun(nowTime + diftime);

    if (AddTimer(stime, nowTime))
    {
        return false;
    }

    return true;
}

bool NFMemTimerMng::SetWeekTime(NFMemTimer* stime, int weekDay, int hour, int minutes, int second, int callCount)
{
    int64_t diftime = 0;
    time_t t = NF_ADJUST_TIMENOW();
    int nowWeek = 0, nowHour = 0, nowMinute = 0, nowSecond = 0;

    NFTimeUtility::GetCurTime(static_cast<unsigned int>(t), nowWeek, nowHour, nowMinute, nowSecond);

    diftime = (static_cast<int64_t>(weekDay) - static_cast<int64_t>(nowWeek)) * 24 * 3600 * 1000 + (static_cast<int64_t>(hour) - static_cast<int64_t>(nowHour)) * 3600 * 1000 +
        (static_cast<int64_t>(minutes) - static_cast<int64_t>(nowMinute)) * 60 * 1000 + (static_cast<int64_t>(second) - static_cast<int64_t>(nowSecond)) * 1000;
    if (diftime < 0)
    {
        diftime += 7 * 24 * 3600 * 1000;
    }

    if (diftime < SLOT_TICK_TIME)
        diftime = SLOT_TICK_TIME;

    int64_t nowTime = NF_ADJUST_TIMENOW_MS();
    stime->SetBeginTime(nowTime);
    stime->SetInterval(7 * 24 * 3600 * 1000);
    stime->SetCallCount(callCount);
    stime->SetNextRun(nowTime + diftime);

    if (AddTimer(stime, nowTime))
    {
        return false;
    }

    return true;
}

bool NFMemTimerMng::SetMonthTime(NFMemTimer* stime, int day, int hour, int minutes, int second, int callCount)
{
    int64_t diffTime = 0;
    time_t t = NF_ADJUST_TIMENOW();
    int nmonth = 0;
    int nowWeek = 0, nowHour = 0, nowMinute = 0, nowSecond = 0;
    int iYear = 0, iMonth = 0, iMonthDay = 0;

    NFTimeUtility::GetCurDate(t, iYear, iMonth, iMonthDay);
    NFTimeUtility::GetCurTime(t, nowWeek, nowHour, nowMinute, nowSecond);

    if (day > NFTimeUtility::GetCurMonthDay(iYear, iMonth))
    {
        // 如果设定的日期超过了这个月的最大天数，就设定下次有这个月天数的月为执行时间
        diffTime += NFTimeUtility::GetNextMonthDay(iYear, iMonth, day, nmonth);
        diffTime += (static_cast<int64_t>(day) - static_cast<int64_t>(iMonthDay)) * 24 * 3600 * 1000 +
            (static_cast<int64_t>(hour) - static_cast<int64_t>(nowHour)) * 3600 * 1000 +
            (static_cast<int64_t>(minutes) - static_cast<int64_t>(nowMinute)) * 60 * 1000 +
            (static_cast<int64_t>(second) - static_cast<int64_t>(nowSecond)) * 1000;
    }
    else
    {
        diffTime = (static_cast<int64_t>(day) - static_cast<int64_t>(iMonthDay)) * 24 * 3600 * 1000 +
            (static_cast<int64_t>(hour) - static_cast<int64_t>(nowHour)) * 3600 * 1000 +
            (static_cast<int64_t>(minutes) - static_cast<int64_t>(nowMinute)) * 60 * 1000 +
            (static_cast<int64_t>(second) - static_cast<int64_t>(nowSecond)) * 1000;
        if (diffTime < 0)
        {
            diffTime += NFTimeUtility::GetNextMonthDay(iYear, iMonth, day, nmonth);
        }
    }

    if (diffTime < SLOT_TICK_TIME)
        diffTime = SLOT_TICK_TIME;

    int64_t nowTime = NF_ADJUST_TIMENOW_MS();
    stime->SetBeginTime(nowTime);
    stime->SetCallCount(callCount);
    stime->SetNextRun(nowTime + diffTime);

    if (AddTimer(stime, nowTime))
    {
        return false;
    }

    return true;
}

int NFMemTimerMng::ClearShmObjTimer(NFMemTimer* pTimer)
{
    CHECK_NULL(0, pTimer);

    int globalId = pTimer->GetTimerShmObjId();
    auto iter = m_shmObjTimer.find(globalId);
    if (iter != m_shmObjTimer.end())
    {
        if (pTimer->GetListCheckId() == iter->second.GetLastCheckId())
        {
            iter->second.RemoveNode(pTimer);
        }
        if (iter->second.IsEmpty())
        {
            m_shmObjTimer.erase(globalId);
        }
    }
    return 0;
}

int NFMemTimerMng::ClearAllTimer(const NFObject* pObj)
{
    CHECK_NULL(0, pObj);

    auto iter = m_shmObjTimer.find(pObj->GetGlobalId());
    if (iter != m_shmObjTimer.end())
    {
        auto pNode = iter->second.GetHeadNodeObj();
        while (pNode)
        {
            auto pLastNode = pNode;
            pNode = iter->second.GetNextNodeObj(pNode);
            pLastNode->SetWaitDelete();
            iter->second.RemoveNode(pLastNode);
        }

        m_shmObjTimer.erase(pObj->GetGlobalId());
    }
    return 0;
}

int NFMemTimerMng::ClearAllTimer(const NFObject* pObj, const NFRawObject* pRawShmObj)
{
    CHECK_NULL(0, pObj);
    CHECK_NULL(0, pRawShmObj);

    auto iter = m_shmObjTimer.find(pObj->GetGlobalId());
    if (iter != m_shmObjTimer.end())
    {
        auto pNode = iter->second.GetHeadNodeObj();
        while (pNode)
        {
            auto pLastNode = pNode;
            pNode = iter->second.GetNextNodeObj(pNode);
            if (pLastNode->GetTimerRawShmObj() == pRawShmObj)
            {
                pLastNode->SetWaitDelete();
                iter->second.RemoveNode(pLastNode);
            }
        }
    }
    return 0;
}

int NFMemTimerMng::AddShmObjTimer(const NFObject* pObj, NFMemTimer* pTimer)
{
    auto iter = m_shmObjTimer.find(pObj->GetGlobalId());
    if (iter == m_shmObjTimer.end())
    {
        iter = m_shmObjTimer.emplace(pObj->GetGlobalId(), NFNodeObjList<NFMemTimer>()).first;
        if (iter == m_shmObjTimer.end())
        {
            NFLogError(NF_LOG_DEFAULT, 0, "m_shmObjTimer full, Insert Failed, Space Not Enough, obj type:{}", pObj->GetClassType());
            Delete(pTimer->GetObjId());

            return INVALID_ID;
        }
        iter->second.InitShmObj(this);
    }

    iter->second.AddNode(pTimer);
    return pTimer->GetObjId();
}

int NFMemTimerMng::SetTimer(const NFObject* pObj, int hour, int minutes, int second, int microSec, const NFRawObject* pRawShmObj/* = NULL*/)
{
    NFMemTimer* newTimer = NFMemTimer::CreateObj();
    if (!newTimer)
    {
        //S_STATIC_MANAGER->AddTimerMngStat(strErr.c_str(), EN_TIMER_STATISTIC_COUNT_CREATE);
        NFLogError(NF_LOG_DEFAULT, 0, "Create timer Obj Failed");
        return INVALID_ID;
    }

    newTimer->SetTimerShmObj(pObj);
    newTimer->SetTimerRawShmObj(pRawShmObj);
    newTimer->SetType(NFMemTimer::ONCE_TIMER);
    if (!SetDistanceTime(newTimer, hour, minutes, second, microSec, 0, 1))
    {
        //S_STATIC_MANAGER->AddTimerMngStat(newTimer->GetDetailMsg(), EN_TIMER_STATISTIC_COUNT_ADD);
        NFLogError(NF_LOG_DEFAULT, 0, "create time error :{}", newTimer->GetObjId());
        newTimer->SetDelete();
        FindModule<NFIMemMngModule>()->DestroyObj(newTimer);

        return INVALID_ID;
    }

    return AddShmObjTimer(pObj, newTimer);
}


int NFMemTimerMng::SetCalender(const NFObject* pObj, uint64_t timestamp, const NFRawObject* pRawShmObj/* = NULL*/)
{
    CHECK_EXPR(timestamp > static_cast<uint64_t>(NF_ADJUST_TIMENOW()), INVALID_ID, "Create timer timestamp err");

    NFMemTimer* newTimer = NFMemTimer::CreateObj();
    if (!newTimer)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "Create timer Obj Failed");
        return INVALID_ID;
    }

    newTimer->SetTimerShmObj(pObj);
    newTimer->SetTimerRawShmObj(pRawShmObj);
    newTimer->SetType(NFMemTimer::ONCE_TIMER);
    if (!SetDayTime(newTimer, timestamp, 0, 1))
    {
        //S_STATIC_MANAGER->AddTimerMngStat(newTimer->GetDetailMsg(), EN_TIMER_STATISTIC_COUNT_ADD);
        NFLogError(NF_LOG_DEFAULT, 0, "create time error :{}", newTimer->GetObjId());
        newTimer->SetDelete();
        FindModule<NFIMemMngModule>()->DestroyObj(newTimer);

        return INVALID_ID;
    }

    return AddShmObjTimer(pObj, newTimer);
}

int
NFMemTimerMng::SetCalender(const NFObject* pObj, int hour, int minutes, int second, const NFRawObject* pRawShmObj/* = NULL*/)
{
    CHECK_EXPR(hour >= 0 && hour <= 23, INVALID_ID, "Create timer hour err");
    CHECK_EXPR(minutes >= 0 && minutes <= 59, INVALID_ID, "Create timer minute err");
    CHECK_EXPR(second >= 0 && second <= 59, INVALID_ID, "Create timer second err");

    NFMemTimer* newTimer = NFMemTimer::CreateObj();
    if (!newTimer)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "Create timer Obj Failed");
        return INVALID_ID;
    }

    newTimer->SetTimerShmObj(pObj);
    newTimer->SetTimerRawShmObj(pRawShmObj);
    newTimer->SetType(NFMemTimer::ONCE_TIMER);
    if (!SetDayTime(newTimer, hour, minutes, second, 0, 1))
    {
        //S_STATIC_MANAGER->AddTimerMngStat(newTimer->GetDetailMsg(), EN_TIMER_STATISTIC_COUNT_ADD);
        NFLogError(NF_LOG_DEFAULT, 0, "create time error :{}", newTimer->GetObjId());
        newTimer->SetDelete();
        FindModule<NFIMemMngModule>()->DestroyObj(newTimer);

        return INVALID_ID;
    }

    return AddShmObjTimer(pObj, newTimer);
}

int NFMemTimerMng::SetTimer(const NFObject* pObj, int interval, int callCount, int hour, int minutes, int second, int microSec, const NFRawObject* pRawShmObj/* = NULL*/)
{
    if (interval < SLOT_TICK_TIME)
    {
        interval = SLOT_TICK_TIME;
    }

    NFMemTimer* newTimer = NFMemTimer::CreateObj();
    if (!newTimer)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "Create timer Obj Failed");
        return INVALID_ID;
    }

    newTimer->SetTimerShmObj(pObj);
    newTimer->SetTimerRawShmObj(pRawShmObj);
    newTimer->SetType(NFMemTimer::LOOP_TIMER);
    if (callCount <= 0)
    {
        callCount = NFSHM_INFINITY_CALL; //无线次数
    }
    if (!SetDistanceTime(newTimer, hour, minutes, second, microSec, interval, callCount))
    {
        //S_STATIC_MANAGER->AddTimerMngStat(newTimer->GetDetailMsg(), EN_TIMER_STATISTIC_COUNT_ADD);
        NFLogError(NF_LOG_DEFAULT, 0, "create time error :{}", newTimer->GetObjId());
        newTimer->SetDelete();
        FindModule<NFIMemMngModule>()->DestroyObj(newTimer);

        return INVALID_ID;
    }

    return AddShmObjTimer(pObj, newTimer);
}

int NFMemTimerMng::SetDayTime(const NFObject* pObj, int callCount, int hour, int minutes, int second, int microSec, const NFRawObject* pRawShmObj/* = NULL*/)
{
    NFMemTimer* newTimer = NFMemTimer::CreateObj();
    if (!newTimer)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "Create timer Obj Failed");
        return INVALID_ID;
    }

    newTimer->SetTimerShmObj(pObj);
    newTimer->SetTimerRawShmObj(pRawShmObj);
    newTimer->SetType(NFMemTimer::LOOP_TIMER);
    if (callCount <= 0)
    {
        callCount = NFSHM_INFINITY_CALL; //无线次数
    }
    if (!SetDistanceTime(newTimer, hour, minutes, second, microSec, 24 * 3600 * 1000, callCount))
    {
        //S_STATIC_MANAGER->AddTimerMngStat(newTimer->GetDetailMsg(), EN_TIMER_STATISTIC_COUNT_ADD);
        NFLogError(NF_LOG_DEFAULT, 0, "create time error :{}", newTimer->GetObjId());
        newTimer->SetDelete();
        FindModule<NFIMemMngModule>()->DestroyObj(newTimer);

        return INVALID_ID;
    }

    return AddShmObjTimer(pObj, newTimer);
}

int NFMemTimerMng::SetDayCalender(const NFObject* pObj, int callCount, int hour, int minutes, int second, const NFRawObject* pRawShmObj/* = NULL*/)
{
    CHECK_EXPR(hour >= 0 && hour <= 23, INVALID_ID, "Create timer hour err");
    CHECK_EXPR(minutes >= 0 && minutes <= 59, INVALID_ID, "Create timer minute err");
    CHECK_EXPR(second >= 0 && second <= 59, INVALID_ID, "Create timer second err");

    NFMemTimer* newTimer = NFMemTimer::CreateObj();
    if (!newTimer)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "Create timer Obj Failed");
        return INVALID_ID;
    }

    newTimer->SetTimerShmObj(pObj);
    newTimer->SetTimerRawShmObj(pRawShmObj);
    newTimer->SetType(NFMemTimer::LOOP_TIMER);
    if (callCount <= 0)
    {
        callCount = NFSHM_INFINITY_CALL; //无线次数
    }
    if (!SetDayTime(newTimer, hour, minutes, second, 24 * 3600 * 1000, callCount))
    {
        //S_STATIC_MANAGER->AddTimerMngStat(newTimer->GetDetailMsg(), EN_TIMER_STATISTIC_COUNT_ADD);
        NFLogError(NF_LOG_DEFAULT, 0, "create time error :{}", newTimer->GetObjId());
        newTimer->SetDelete();
        FindModule<NFIMemMngModule>()->DestroyObj(newTimer);

        return INVALID_ID;
    }

    return AddShmObjTimer(pObj, newTimer);
}

int NFMemTimerMng::SetWeekTime(const NFObject* pObj, int callCount, int hour, int minutes, int second, int microSec, const NFRawObject* pRawShmObj/* = NULL*/)
{
    NFMemTimer* newTimer = NFMemTimer::CreateObj();
    if (!newTimer)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "Create timer Obj Failed");
        return INVALID_ID;
    }

    newTimer->SetTimerShmObj(pObj);
    newTimer->SetTimerRawShmObj(pRawShmObj);
    newTimer->SetType(NFMemTimer::LOOP_TIMER);
    if (callCount <= 0)
    {
        callCount = NFSHM_INFINITY_CALL; //无线次数
    }
    if (!SetDistanceTime(newTimer, hour, minutes, second, microSec, 7 * 24 * 3600 * 1000, callCount))
    {
        //S_STATIC_MANAGER->AddTimerMngStat(newTimer->GetDetailMsg(), EN_TIMER_STATISTIC_COUNT_ADD);
        NFLogError(NF_LOG_DEFAULT, 0, "create time error :{}", newTimer->GetObjId());
        newTimer->SetDelete();
        FindModule<NFIMemMngModule>()->DestroyObj(newTimer);

        return INVALID_ID;
    }

    return AddShmObjTimer(pObj, newTimer);
}

int NFMemTimerMng::SetWeekCalender(const NFObject* pObj, int callCount, int weekDay, int hour, int minutes, int second, const NFRawObject* pRawShmObj/* = NULL*/)
{
    CHECK_EXPR(weekDay >= 1 && weekDay <= 7, INVALID_ID, "Create timer week day err");
    CHECK_EXPR(hour >= 0 && hour <= 23, INVALID_ID, "Create timer hour err");
    CHECK_EXPR(minutes >= 0 && minutes <= 59, INVALID_ID, "Create timer minute err");
    CHECK_EXPR(second >= 0 && second <= 59, INVALID_ID, "Create timer second err");

    NFMemTimer* newTimer = NFMemTimer::CreateObj();
    if (!newTimer)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "Create timer Obj Failed");
        return INVALID_ID;
    }

    if (callCount <= 0)
    {
        callCount = NFSHM_INFINITY_CALL; //无线次数
    }
    newTimer->SetTimerShmObj(pObj);
    newTimer->SetTimerRawShmObj(pRawShmObj);
    newTimer->SetType(NFMemTimer::LOOP_TIMER);
    if (!SetWeekTime(newTimer, weekDay, hour, minutes, second, callCount))
    {
        //S_STATIC_MANAGER->AddTimerMngStat(newTimer->GetDetailMsg(), EN_TIMER_STATISTIC_COUNT_ADD);
        NFLogError(NF_LOG_DEFAULT, 0, "create time error :{}", newTimer->GetObjId());
        newTimer->SetDelete();
        FindModule<NFIMemMngModule>()->DestroyObj(newTimer);

        return INVALID_ID;
    }

    return AddShmObjTimer(pObj, newTimer);
}

int NFMemTimerMng::SetMonthTime(const NFObject* pObj, int callCount, int hour, int minutes, int second, int microSec, const NFRawObject* pRawShmObj/* = NULL*/)
{
    NFMemTimer* newTimer = NFMemTimer::CreateObj();
    if (!newTimer)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "Create timer Obj Failed");
        return INVALID_ID;
    }

    if (callCount <= 0)
    {
        callCount = NFSHM_INFINITY_CALL; //无线次数
    }
    newTimer->SetTimerShmObj(pObj);
    newTimer->SetTimerRawShmObj(pRawShmObj);
    newTimer->SetType(NFMemTimer::MONTH_LOOP_TIMER);
    if (!SetDistanceTime(newTimer, hour, minutes, second, microSec, 0, callCount))
    {
        //S_STATIC_MANAGER->AddTimerMngStat(newTimer->GetDetailMsg(), EN_TIMER_STATISTIC_COUNT_ADD);
        NFLogError(NF_LOG_DEFAULT, 0, "create time error :{}", newTimer->GetObjId());
        newTimer->SetDelete();
        FindModule<NFIMemMngModule>()->DestroyObj(newTimer);

        return INVALID_ID;
    }

    return AddShmObjTimer(pObj, newTimer);
}

int NFMemTimerMng::SetMonthCalender(const NFObject* pObj, int callCount, int day, int hour, int minutes, int second, const NFRawObject* pRawShmObj/* = NULL*/)
{
    CHECK_EXPR(day >= 1 && day <= 31, INVALID_ID, "Create timer month day err");
    CHECK_EXPR(hour >= 0 && hour <= 23, INVALID_ID, "Create timer hour err");
    CHECK_EXPR(minutes >= 0 && minutes <= 59, INVALID_ID, "Create timer minute err");
    CHECK_EXPR(second >= 0 && second <= 59, INVALID_ID, "Create timer second err");

    NFMemTimer* newTimer = NFMemTimer::CreateObj();
    if (!newTimer)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "Create timer Obj Failed");
        return INVALID_ID;
    }

    if (callCount <= 0)
    {
        callCount = NFSHM_INFINITY_CALL; //无线次数
    }
    newTimer->SetTimerShmObj(pObj);
    newTimer->SetTimerRawShmObj(pRawShmObj);
    newTimer->SetType(NFMemTimer::MONTH_LOOP_TIMER);
    if (!SetMonthTime(newTimer, day, hour, minutes, second, callCount))
    {
        //S_STATIC_MANAGER->AddTimerMngStat(newTimer->GetDetailMsg(), EN_TIMER_STATISTIC_COUNT_ADD);
        NFLogError(NF_LOG_DEFAULT, 0, "create time error :{}", newTimer->GetObjId());
        newTimer->SetDelete();
        FindModule<NFIMemMngModule>()->DestroyObj(newTimer);

        return INVALID_ID;
    }

    return AddShmObjTimer(pObj, newTimer);
}
