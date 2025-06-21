// -------------------------------------------------------------------------
//    @FileName         :    NFObject.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------

#include "NFDynamicHead.h"
#include "NFObject.h"
#include "NFShmMgr.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include "NFComm/NFPluginModule/NFIMemMngModule.h"
#include "NFComm/NFPluginModule/NFIEventModule.h"
#include "NFComm/NFPluginModule/NFIMessageModule.h"

NFObject::NFObject()
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


NFObject::~NFObject()
{
    if (m_iObjType > EOT_TYPE_EVENT_MGR)
    {
        NFObject::UnSubscribeAll();
        NFObject::DeleteAllTimer();
    }

#ifdef NF_DEBUG_MODE
    CheckMemMagicNum();

    //m_iMagicCheckNum = 0;
    if (m_iGlobalId != INVALID_ID)
    {
        //有globalid的对象删除没有使用CIDRuntimeClass::DestroyObj会发生这种问题，这是不允许的
        NFObject* pObj = FindModule<NFIMemMngModule>()->GetObjByGlobalIdWithNoCheck(m_iGlobalId);
        assert(pObj == NULL);
    }
#endif
    m_iGlobalId = INVALID_ID;
    m_iObjSeq = INVALID_ID;
    m_iObjId = INVALID_ID;
    m_iHashKey = INVALID_ID;
    m_iObjType = INVALID_ID;
}

int NFObject::CreateInit()
{
#if NF_DEBUG_MODE
    m_iMagicCheckNum = OBJECT_MAGIC_CHECK_NUMBER;
#endif

    m_iObjId = INVALID_ID;
    m_iGlobalId = INVALID_ID;
    m_iObjType = NFShmMgr::Instance()->m_iType;
    m_iObjId = FindModule<NFIMemMngModule>()->GetObjId(m_iObjType, this);
    NF_ASSERT(m_iObjId != INVALID_ID);

    int iId = FindModule<NFIMemMngModule>()->GetGlobalId(m_iObjType, m_iObjId, this);
    if (iId >= 0)
    {
        m_iGlobalId = iId;
    }

    m_iHashKey = INVALID_ID;
    m_iObjSeq = FindModule<NFIMemMngModule>()->IncreaseObjSeqNum();

    m_bIsInRecycle = false;

    return 0;
}

int NFObject::ResumeInit()
{
    return 0;
}

std::string NFObject::GetClassName() const
{
    return FindModule<NFIMemMngModule>()->GetClassName(GetClassType());
}

int NFObject::GetItemCount() const
{
    return FindModule<NFIMemMngModule>()->GetItemCount(GetClassType());
}
int NFObject::GetUsedCount() const
{
    return FindModule<NFIMemMngModule>()->GetUsedCount(GetClassType());
}
int NFObject::GetFreeCount() const
{
    return FindModule<NFIMemMngModule>()->GetFreeCount(GetClassType());
}

int NFObject::AfterOnReloadConfig()
{
    return 0;
}

int NFObject::OnTimer(int timeId, int callCount)
{
    NFLogError(NF_LOG_DEFAULT, 0, "shm obj type:{} no handle timer:{}", GetClassType(), timeId);
    return 0;
}

#ifdef NF_DEBUG_MODE
void NFObject::CheckMemMagicNum() const
{
    assert(m_iMagicCheckNum == OBJECT_MAGIC_CHECK_NUMBER);
}
#endif

void NFObject::SetInRecycle(bool bRet)
{
    m_bIsInRecycle = bRet;
}

bool NFObject::IsInRecycle() const
{
    return m_bIsInRecycle;
}

int NFObject::GetObjId() const
{
    return m_iObjId;
}

NFObjectHashKey NFObject::GetHashKey() const
{
    return m_iHashKey;
}

void NFObject::SetHashKey(NFObjectHashKey id)
{
    m_iHashKey = id;
}

int NFObject::GetGlobalId() const
{
    return m_iGlobalId;
}

void NFObject::SetGlobalId(int iId)
{
    m_iGlobalId = iId;
}

int NFObject::GetTypeIndexId() const
{
    return ((GetClassType() << 23) | 0x80000000) | GetObjId();
}

int NFObject::GetMiscId() const
{
    if (m_iGlobalId >= 0)
    {
        return m_iGlobalId;
    }
    else
    {
        return GetTypeIndexId();
    }
}

int NFObject::OnExecute(uint32_t serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const google::protobuf::Message* pMessage)
{
    NFLogError(NF_LOG_DEFAULT, 0, "event not handle, shmobjType:{} serverType:{} nEventID:{} bySrcType:{} nSrcID:{}, message:{}", GetClassType(),
               serverType, eventId, srcType, srcId, pMessage->DebugString());
    return 0;
}

//发送执行事件
int NFObject::FireExecute(NF_SERVER_TYPE serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const google::protobuf::Message& message)
{
    int retCode = FindModule<NFIMemMngModule>()->FireExecute(serverType, eventId, srcType, srcId, message);
    if (retCode != 0)
    {
        return retCode;
    }

    FindModule<NFIEventModule>()->FireExecute(serverType, eventId, srcType, srcId, message);
    return retCode;
}

int NFObject::FireBroadcast(NF_SERVER_TYPE serverType, NF_SERVER_TYPE recvServerType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const google::protobuf::Message& message, bool self/* = false*/)
{
    FindModule<NFIMessageModule>()->BroadcastEventToServer(serverType, recvServerType, eventId, srcType, srcId, message);
    if (self)
    {
        FireExecute(serverType, eventId, srcType, srcId, message);
    }
    return 0;
}

int NFObject::FireBroadcast(NF_SERVER_TYPE serverType, NF_SERVER_TYPE recvServerType, uint32_t busId, uint32_t eventId, uint32_t srcType, uint64_t srcId, const google::protobuf::Message& message, bool self/* = false*/)
{
    FindModule<NFIMessageModule>()->BroadcastEventToServer(serverType, recvServerType, busId, eventId, srcType, srcId, message);
    if (self)
    {
        FireExecute(serverType, eventId, srcType, srcId, message);
    }
    return 0;
}

int NFObject::FireAllBroadcast(NF_SERVER_TYPE serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const google::protobuf::Message& message, bool self/* = false*/)
{
    FindModule<NFIMessageModule>()->BroadcastEventToAllServer(serverType, eventId, srcType, srcId, message);
    if (self)
    {
        FireExecute(serverType, eventId, srcType, srcId, message);
    }
    return 0;
}

int NFObject::FireAllBroadcast(NF_SERVER_TYPE serverType, uint32_t busId, uint32_t eventId, uint32_t srcType, uint64_t srcId, const google::protobuf::Message& message, bool self)
{
    FindModule<NFIMessageModule>()->BroadcastEventToAllServer(serverType, busId, eventId, srcType, srcId, message);
    if (self)
    {
        FireExecute(serverType, eventId, srcType, srcId, message);
    }
    return 0;
}

//订阅执行事件
int NFObject::Subscribe(NF_SERVER_TYPE serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const std::string& desc)
{
    return FindModule<NFIMemMngModule>()->Subscribe(this, serverType, eventId, srcType, srcId, desc);
}

//取消订阅执行事件
int NFObject::UnSubscribe(NF_SERVER_TYPE serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId)
{
    return FindModule<NFIMemMngModule>()->UnSubscribe(this, serverType, eventId, srcType, srcId);
}

//取消所有执行事件的订阅
int NFObject::UnSubscribeAll()
{
    return FindModule<NFIMemMngModule>()->UnSubscribeAll(this);
}

int NFObject::DeleteAllTimer()
{
    return FindModule<NFIMemMngModule>()->DeleteAllTimer(this);
}

int NFObject::DeleteAllTimer(NFRawObject* pRawShmObj)
{
    return FindModule<NFIMemMngModule>()->DeleteAllTimer(this, pRawShmObj);
}

int NFObject::DeleteTimer(int timeObjId)
{
    return FindModule<NFIMemMngModule>()->DeleteTimer(this, timeObjId);
}

//注册距离现在多少时间执行一次的定时器(hour  minutes  second  microSec为第一次执行距离现在的时分秒毫秒, 只执行一次)
int NFObject::SetTimer(int hour, int minutes, int second, int microSec, NFRawObject* pRawShmObj)
{
    return FindModule<NFIMemMngModule>()->SetTimer(this, hour, minutes, second, microSec, pRawShmObj);
}

//注册某一个时间点执行一次的定时器(hour  minutes  second为第一次执行的时间点时分秒, 只执行一次)
int NFObject::SetCalender(int hour, int minutes, int second, NFRawObject* pRawShmObj)
{
    return FindModule<NFIMemMngModule>()->SetCalender(this, hour, minutes, second, pRawShmObj);
}

//注册某一个时间点执行一次的定时器(timestamp为第一次执行的时间点的时间戳,单位是秒, 只执行一次)
int NFObject::SetCalender(uint64_t timestamp, NFRawObject* pRawShmObj)
{
    return FindModule<NFIMemMngModule>()->SetCalender(this, timestamp, pRawShmObj);
}

//注册循环执行定时器（hour  minutes  second  microSec为第一次执行距离现在的时分秒毫秒,  interval 为循环间隔时间，为毫秒）
int NFObject::SetTimer(int interval, int callcount, int hour, int minutes, int second, int microSec, NFRawObject* pRawShmObj)
{
    return FindModule<NFIMemMngModule>()->SetTimer(this, interval, callcount, hour, minutes, second, microSec, pRawShmObj);
}

//注册循环执行定时器（hour  minutes  second  microSec为第一次执行距离现在的时分秒毫秒）
int NFObject::SetDayTime(int callcount, int hour, int minutes, int second, int microSec, NFRawObject* pRawShmObj)
{
    return FindModule<NFIMemMngModule>()->SetDayTime(this, callcount, hour, minutes, second, microSec, pRawShmObj);
}

//注册某一个时间点日循环执行定时器（hour  minutes  second为一天中开始执行的时间点，    23：23：23     每天23点23分23秒执行）
int NFObject::SetDayCalender(int callcount, int hour, int minutes, int second, NFRawObject* pRawShmObj)
{
    return FindModule<NFIMemMngModule>()->SetDayCalender(this, callcount, hour, minutes, second, pRawShmObj);
}

//周循环（hour  minutes  second  microSec为第一次执行距离现在的时分秒毫秒）
int NFObject::SetWeekTime(int callcount, int hour, int minutes, int second, int microSec, NFRawObject* pRawShmObj)
{
    return FindModule<NFIMemMngModule>()->SetWeekTime(this, callcount, hour, minutes, second, microSec, pRawShmObj);
}

//注册某一个时间点周循环执行定时器（ weekDay  hour  minutes  second 为一周中某一天开始执行的时间点）
int NFObject::SetWeekCalender(int callcount, int weekDay, int hour, int minutes, int second, NFRawObject* pRawShmObj)
{
    return FindModule<NFIMemMngModule>()->SetWeekCalender(this, callcount, weekDay, hour, minutes, second, pRawShmObj);
}

//月循环（hour  minutes  second  microSec为第一次执行距离现在的时分秒毫秒,最好是同一天）
int NFObject::SetMonthTime(int callcount, int hour, int minutes, int second, int microSec, NFRawObject* pRawShmObj)
{
    return FindModule<NFIMemMngModule>()->SetMonthTime(this, callcount, hour, minutes, second, microSec, pRawShmObj);
}

//注册某一个时间点月循环执行定时器（ day  hour  minutes  second 为一月中某一天开始执行的时间点）
int NFObject::SetMonthCalender(int callcount, int day, int hour, int minutes, int second, NFRawObject* pRawShmObj)
{
    return FindModule<NFIMemMngModule>()->SetMonthCalender(this, callcount, day, hour, minutes, second, pRawShmObj);
}
