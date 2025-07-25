// -------------------------------------------------------------------------
//    @FileName         :    NFObject.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------
#pragma once

#include "NFObjDefine.h"
#include "NFTypeDefines.h"
#include "NFComm/NFPluginModule/NFIMemMngModule.h"
#include "NFComm/NFPluginModule/NFEventObj.h"
#include "NFComm/NFObjCommon/NFObjectTemplate.h"

class NFRawObject;

class NFObject : public NFObjectTemplateNoParent<NFObject, EOT_OBJECT>
{
public:
    NFObject();

    ~NFObject() override;

    //非继承函数, 不要加virtual
    int CreateInit();

    //非继承函数, 不要加virtual
    int ResumeInit();

    template <typename T>
    T* FindModule() const
    {
        return NFGlobalSystem::Instance()->GetGlobalPluginManager()->FindModule<T>();
    }

    //must be virtual
    virtual int OnTimer(int timeId, int callCount);

#ifdef NF_DEBUG_MODE
    void CheckMemMagicNum() const;
#endif

    void SetInRecycle(bool bRet);
    bool IsInRecycle() const;

    int GetObjId() const;

    NFObjectHashKey GetHashKey() const;
    void SetHashKey(NFObjectHashKey id);

    int GetGlobalId() const;
    void SetGlobalId(int iId);

    int GetTypeIndexId() const;

    int GetMiscId() const;

public:
    virtual std::string GetClassName() const;
    virtual int GetItemCount() const;
    virtual int GetUsedCount() const;
    virtual int GetFreeCount() const;
public:
    virtual int OnExecute(uint32_t serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const google::protobuf::Message* pMessage);
    virtual int FireExecute(NF_SERVER_TYPE serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const google::protobuf::Message& message);
    virtual int FireBroadcast(NF_SERVER_TYPE serverType, NF_SERVER_TYPE recvServerType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const google::protobuf::Message& message, bool self = false);
    virtual int FireBroadcast(NF_SERVER_TYPE serverType, NF_SERVER_TYPE recvServerType, uint32_t busId, uint32_t eventId, uint32_t srcType, uint64_t srcId, const google::protobuf::Message& message, bool self = false);
    virtual int FireAllBroadcast(NF_SERVER_TYPE serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const google::protobuf::Message& message, bool self = false);
    virtual int FireAllBroadcast(NF_SERVER_TYPE serverType, uint32_t busId, uint32_t eventId, uint32_t srcType, uint64_t srcId, const google::protobuf::Message& message, bool self = false);

    virtual int Subscribe(NF_SERVER_TYPE serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const std::string& desc);
    virtual int UnSubscribe(NF_SERVER_TYPE serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId);
    virtual int UnSubscribeAll();
public:
    virtual int DeleteTimer(int timeObjId);
    virtual int DeleteAllTimer();
    virtual int DeleteAllTimer(NFRawObject* pRawShmObj);

    ////注册距离现在多少时间执行一次的定时器(hour  minutes  second  microSec为第一次执行距离现在的时分秒毫秒, 只执行一次)
    virtual int SetTimer(int hour, int minutes, int second, int microSec, NFRawObject* pRawShmObj = nullptr);

    //注册某一个时间点执行一次的定时器(hour  minutes  second为第一次执行的时间点时分秒, 只执行一次)
    virtual int SetCalender(int hour, int minutes, int second, NFRawObject* pRawShmObj = nullptr);

    //注册某一个时间点执行一次的定时器(timestamp为第一次执行的时间点的时间戳,单位是秒, 只执行一次)
    virtual int SetCalender(uint64_t timestamp, NFRawObject* pRawShmObj = nullptr);

    //注册循环执行定时器（hour  minutes  second  microSec为第一次执行距离现在的时分秒毫秒,  interval 为循环间隔时间，为毫秒）
    virtual int SetTimer(int interval, int callcount, int hour, int minutes, int second, int microSec, NFRawObject* pRawShmObj = nullptr);

    //注册循环执行定时器（hour  minutes  second  microSec为第一次执行距离现在的时分秒毫秒）
    virtual int SetDayTime(int callcount, int hour, int minutes, int second, int microSec, NFRawObject* pRawShmObj = nullptr);

    //注册某一个时间点日循环执行定时器（hour  minutes  second为一天中开始执行的时间点，    23：23：23     每天23点23分23秒执行）
    virtual int SetDayCalender(int callcount, int hour, int minutes, int second, NFRawObject* pRawShmObj = nullptr);

    //周循环（hour  minutes  second  microSec为第一次执行距离现在的时分秒毫秒）
    virtual int SetWeekTime(int callcount, int hour, int minutes, int second, int microSec, NFRawObject* pRawShmObj = nullptr);

    //注册某一个时间点周循环执行定时器（ weekDay  hour  minutes  second 为一周中某一天开始执行的时间点）
    virtual int SetWeekCalender(int callcount, int weekDay, int hour, int minutes, int second, NFRawObject* pRawShmObj = nullptr);

    //月循环（hour  minutes  second  microSec为第一次执行距离现在的时分秒毫秒,最好是同一天）
    virtual int SetMonthTime(int callcount, int hour, int minutes, int second, int microSec, NFRawObject* pRawShmObj = nullptr);

    //注册某一个时间点月循环执行定时器（ day  hour  minutes  second 为一月中某一天开始执行的时间点）
    virtual int SetMonthCalender(int callcount, int day, int hour, int minutes, int second, NFRawObject* pRawShmObj = nullptr);

public:

#if NF_DEBUG_MODE
    int m_iMagicCheckNum;
#endif
    int m_iGlobalId;
    int m_iObjId;
    int m_iObjSeq;
    NFObjectHashKey m_iHashKey;
    int m_iObjType;

    bool m_bIsInRecycle;
};

