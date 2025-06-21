// -------------------------------------------------------------------------
//    @FileName         :    NFShmTimer.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFShmTimer.cpp
//
// -------------------------------------------------------------------------

#include "NFShmTimer.h"
#include "NFComm/NFPluginModule/NFIMemMngModule.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include "NFShmTimerMng.h"

NFShmTimer::NFShmTimer()
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

NFShmTimer::~NFShmTimer()
{
    NFShmTimerMng::Instance()->ClearShmObjTimer(this);
    DeleteFunc();
}

void NFShmTimer::DeleteFunc()
{
    // 是在SubscriberSlot 创建的，必须在这销毁
    m_shmObj = nullptr;
    m_shmObjId = INVALID_ID;
    m_rawShmObj = nullptr;
}

int NFShmTimer::CreateInit()
{
    m_shmObj = nullptr;
    m_shmObjId = INVALID_ID;
    m_rawShmObj = nullptr;

    m_type = ONCE_TIMER;
    m_beginTime = 0;
    m_nextRun = 0;
    m_interval = 0;
    m_delFlag = false;
    m_round = 0;
    m_slotIndex = -1;
    m_waitDel = false;
    m_listIndex = -1;
    m_callCount = 0;
    m_curCallCount = 0;
    return 0;
}

int NFShmTimer::ResumeInit()
{
    return 0;
}

NFObject* NFShmTimer::GetTimerShmObj()
{
    return m_shmObj.GetPoint();
}

int NFShmTimer::GetTimerShmObjId() const
{
    return m_shmObjId;
}

void NFShmTimer::SetTimerShmObj(const NFObject* pObj)
{
    m_shmObj = pObj;
    m_shmObjId = pObj->GetGlobalId();
}

void NFShmTimer::SetTimerRawShmObj(const NFRawObject* pObj)
{
    m_rawShmObj = pObj;
}

NFRawObject* NFShmTimer::GetTimerRawShmObj()
{
    return m_rawShmObj;
}

void NFShmTimer::PrintfDebug() const
{
    LOG_DEBUG(0, "timer debug:{}", GetDetailStructMsg());
}

std::string NFShmTimer::GetDetailStructMsg() const
{
    std::ostringstream oss;

    oss << " type:" << m_type
        << " begintime:" << m_beginTime
        << " nextRun:" << m_nextRun
        << " interval:" << m_interval
        << " delFlag:" << m_delFlag
        << " round:" << m_round
        << " slotIndex:" << m_slotIndex
        << " waitDel:" << m_waitDel
        << " listIndex:" << m_listIndex
        << " objID:" << GetObjId()
        << " globalID:" << GetGlobalId();

#ifdef NF_DEBUG_MODE
    oss << " shmobj gloablid:" << m_shmObjId;
#endif

    return oss.str();
}

bool NFShmTimer::IsTimeOut(int64_t tick)
{
    --m_round;
    //	LOGSVR_TRACE("is time out: " << GetDetailStructMsg());
    if (tick - m_nextRun >= 0 || m_round <= 0)
    {
        return true;
    }

    return false;
}

NFTimerRetType NFShmTimer::OnTick(int64_t tick)
{
    if (tick - m_nextRun >= 0 || m_round <= 0)
    {
        if (m_shmObj)
        {
            //			LOGSVR_TRACE("time out: " << GetDetailStructMsg());
            if (m_callCount != static_cast<int32_t>(NFSHM_INFINITY_CALL) && m_callCount > 0)
            {
                m_callCount--;
            }
            m_curCallCount++;

            return HandleTimer(GetObjId(), m_curCallCount);
        }
        NFLogError(NF_LOG_DEFAULT, 0, "timer ontick error:{} ", GetDetailStructMsg());
        return E_TIMER_HANDLER_NULL;
    }

    return E_TIMER_NOT_TRIGGER;
}

NFTimerRetType NFShmTimer::HandleTimer(int timeId, int callCount)
{
    if (!m_shmObj)
    {
        return E_TIMER_HANDLER_NULL;
    }

#if NF_DEBUG_MODE
    if (m_shmObjId >= 0)
    {
        NF_ASSERT(m_shmObjId == m_shmObj->GetGlobalId());
        NFObject* pObjGetObjFromTypeIndex = FindModule<NFIMemMngModule>()->GetObjByGlobalIdWithNoCheck(m_shmObjId);
        NF_ASSERT(pObjGetObjFromTypeIndex == m_shmObj.GetPoint());
    }
#endif

    if (m_rawShmObj)
    {
        m_rawShmObj->OnTimer(timeId, callCount);
    }
    else
    {
        m_shmObj->OnTimer(timeId, callCount);
    }

    return E_TIMER_TYPE_SUCCESS;
}

bool NFShmTimer::IsDelete() const
{
    return m_delFlag;
}

void NFShmTimer::SetDelete()
{
    m_delFlag = true;
}

bool NFShmTimer::IsWaitDelete() const
{
    return m_waitDel;
}

void NFShmTimer::SetWaitDelete()
{
    m_waitDel = true;
}


