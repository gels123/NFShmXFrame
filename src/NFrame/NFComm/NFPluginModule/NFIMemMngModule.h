// -------------------------------------------------------------------------
//    @FileName         :    NFIMemMngModule.h
//    @Author           :    Yi.Gao
//    @Date             :   2022-09-18
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------

#pragma once

#include <unordered_set>
#include "NFComm/NFPluginModule/NFIModule.h"
#include "NFComm/NFObjCommon/NFObjDefine.h"
#include "NFComm/NFObjCommon/NFObjectIterator.h"

class NFObject;
class NFRawObject;
class NFTransBase;

class NFIMemMngModule : public NFIModule
{
public:
    using iterator = NFObjectIterator<NFObject, NFObject&, NFObject*, NFIMemMngModule>;
    using const_iterator = NFObjectIterator<NFObject, const NFObject&, const NFObject*, NFIMemMngModule>;

    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator = std::reverse_iterator<iterator>;

public:
    explicit NFIMemMngModule(NFIPluginManager* p) : NFIModule(p)
    {
    }

    ~NFIMemMngModule() override
    {
    }

    /**
    * 共享内存模式
    */
    virtual EN_OBJ_MODE GetInitMode() = 0;

    /**
    * 共享内存模式
    */
    virtual void SetInitMode(EN_OBJ_MODE mode) = 0;

    /**
    * @brief  对象seq自增
    */
    virtual int IncreaseObjSeqNum() = 0;

    /**
    * @brief  获得类名
    */
    virtual std::string GetClassName(int bType) = 0;

    virtual int GetClassType(int bType) = 0;

    virtual void* AllocMemForObject(int iType) = 0;

    virtual void FreeMemForObject(int iType, void* pMem) = 0;

    virtual void RegisterClassToObjSeg(int bType, size_t nObjSize, int iItemCount, NFObject*(*pfResumeObj)(void*),
                                       NFObject*(*pCreatefn)(),
                                       void (*pDestroy)(NFObject*), int parentType,
                                       const std::string& pszClassName, bool useHash = false, bool singleton = false) = 0;

    /**
     * @brief 反注册对象类型到内存段的映射
     * @param bType 对象类型ID
     * @note 用于动态卸载对象类型，释放相关内存段资源
     * @warning 必须确保该类型的所有对象已经被正确销毁
     */
    virtual void UnRegisterClassToObjSeg(int bType) = 0;

    /**
    * @brief  设置功能内存初始化成功
    */
    virtual void SetShmInitSuccessFlag() = 0;

    virtual NFObject* CreateObjByHashKey(int iType, NFObjectHashKey hashKey) = 0;

    virtual NFObject* GetObjByHashKey(int iType, NFObjectHashKey hashKey) = 0;

    virtual const std::unordered_set<int>& GetChildrenType(int iType) = 0;

    virtual int GetItemCount(int iType) = 0;

    virtual int GetUsedCount(int iType) = 0;

    virtual int GetFreeCount(int iType) = 0;

    virtual int GetGlobalId(int iType, int iIndex, NFObject* pObj) = 0;

    virtual int GetObjId(int iType, NFObject* pObj) = 0;

    virtual NFObject* CreateObj(int iType) = 0;

    template <typename ShmObjType>
    ShmObjType* CreateObj()
    {
        return dynamic_cast<ShmObjType*>(CreateObj(ShmObjType::GetStaticClassType()));
    }

    template <typename ShmObjType>
    ShmObjType* CreateObjByHashKey(uint64_t hashKey)
    {
        return dynamic_cast<ShmObjType*>(CreateObjByHashKey(ShmObjType::GetStaticClassType(), hashKey));
    }

    template <typename ShmObjType>
    ShmObjType* GetObjByHashKey(uint64_t hashKey)
    {
        return dynamic_cast<ShmObjType*>(GetObjByHashKey(ShmObjType::GetStaticClassType(), hashKey));
    }

    virtual NFObject* GetHeadObj(int iType) = 0;

    template <typename ShmObjType>
    ShmObjType* GetHeadObj()
    {
        return dynamic_cast<ShmObjType*>(GetHeadObj(ShmObjType::GetStaticClassType()));
    }

    virtual NFObject* GetNextObj(int iType, NFObject* pObj) = 0;

    template <typename ShmObjType>
    ShmObjType* GetNextObj(NFObject* pObj)
    {
        return dynamic_cast<ShmObjType*>(GetNextObj(ShmObjType::GetStaticClassType(), pObj));
    }

    virtual void DestroyObj(NFObject* pObj) = 0;

    virtual void ClearAllObj(int iType) = 0;

    virtual int DestroyObjAutoErase(int iType, int maxNum = INVALID_ID, const DESTROY_OBJECT_AUTO_ERASE_FUNCTION& func = nullptr) = 0;

    virtual NFObject* GetObjByObjId(int iType, int iIndex) = 0;

    virtual NFObject* GetObjByGlobalId(int iType, int iGlobalId, bool withChildrenType = false) = 0;

    template <typename ShmObjType>
    ShmObjType* GetObjByGlobalId(int iGlobalId, int iStrongType = 1)
    {
        return dynamic_cast<ShmObjType*>(GetObjByGlobalID(iGlobalId, ShmObjType::GetStaticClassType(), iStrongType));
    }

    virtual NFObject* GetObjByGlobalIdWithNoCheck(int iGlobalId) = 0;

    // 根据混合ID获得对象
    // iType不为-1表示校验对象类型
    virtual NFObject* GetObjByMiscId(int iMiscId, int iType = -1) = 0;

    virtual bool IsEnd(int iType, int iIndex) = 0;

    virtual void SetSecOffSet(int iOffset) = 0;

    virtual int GetSecOffSet() const = 0;

public:
    /**
     * @brief ShmObj类链表迭代器+1
     * @param iType
     * @param iPos
     * @return
     */
    virtual size_t IterIncr(int iType, size_t iPos) = 0;

    /**
     * @brief ShmObj类链表迭代器-1
     * @param iType
     * @param iPos
     * @return
     */
    virtual size_t IterDecr(int iType, size_t iPos) = 0;

    virtual iterator IterBegin(int iType) = 0;

    virtual iterator IterEnd(int iType) = 0;

    virtual const_iterator IterBegin(int iType) const = 0;

    virtual const_iterator IterEnd(int iType) const = 0;

    virtual reverse_iterator IterRBegin(int iType) { return reverse_iterator(IterEnd(iType)); }

    virtual reverse_iterator IterREnd(int iType) { return reverse_iterator(IterBegin(iType)); }

    virtual const_reverse_iterator IterRBegin(int iType) const { return const_reverse_iterator(IterEnd(iType)); }

    virtual const_reverse_iterator IterREnd(int iType) const { return const_reverse_iterator(IterBegin(iType)); }

    virtual iterator Erase(iterator iter) = 0;

    virtual bool IsValid(iterator iter) = 0;

    virtual NFObject* GetIterObj(int iType, size_t iPos) = 0;
    virtual const NFObject* GetIterObj(int iType, size_t iPos) const = 0;

public:
    virtual bool IsTypeValid(int iType) const = 0;

    virtual NFTransBase* CreateTrans(int iType) = 0;

    template <typename ShmObjType>
    ShmObjType* CreateTrans()
    {
        return dynamic_cast<ShmObjType*>(CreateTrans(ShmObjType::GetClassType(m_pObjPluginManager)));
    }

    virtual NFTransBase* GetTrans(uint64_t ullTransId) = 0;

    template <typename ShmObjType>
    ShmObjType* GetTrans(uint64_t ullTransId)
    {
        return dynamic_cast<ShmObjType*>(GetTrans(ullTransId));
    }

    /**
    * 共享内存创建对象模式
    */
    virtual EN_OBJ_MODE GetCreateMode() = 0;

    /**
    * 共享内存创建对象模式
    */
    virtual void SetCreateMode(EN_OBJ_MODE mode) = 0;

    virtual EN_OBJ_MODE GetRunMode() = 0;

    /**
     * @brief 删除指定定时器
     *
     * 该函数用于删除与指定对象关联的定时器。
     *
     * @param pObj 指向共享内存对象的指针，该对象与定时器相关联。
     * @param timeObjId 定时器的唯一标识符，用于指定要删除的定时器。
     * @return int 返回操作结果，通常为成功或失败的状态码。
     */
    virtual int DeleteTimer(NFObject* pObj, int timeObjId) = 0;

    /**
     * @brief 删除所有与指定对象关联的定时器
     *
     * 该函数用于删除与指定共享内存对象关联的所有定时器。
     *
     * @param pObj 指向共享内存对象的指针，该对象与定时器相关联。
     * @return int 返回操作结果，通常为成功或失败的状态码。
     */
    virtual int DeleteAllTimer(NFObject* pObj) = 0;

    /**
     * @brief 删除所有与指定对象和原始共享内存对象关联的定时器
     *
     * 该函数用于删除与指定共享内存对象和原始共享内存对象关联的所有定时器。
     *
     * @param pObj 指向共享内存对象的指针，该对象与定时器相关联。
     * @param pRawShmObj 指向原始共享内存对象的指针，该对象与定时器相关联。
     * @return int 返回操作结果，通常为成功或失败的状态码。
     */
    virtual int DeleteAllTimer(NFObject* pObj, NFRawObject* pRawShmObj) = 0;

    /**
     * @brief 注册一个定时器，该定时器在距离当前时间指定的时分秒毫秒后执行一次。
     *
     * @param pObj 指向NFShmObj对象的指针，该对象将与定时器关联。
     * @param hour 距离当前时间的小时数。
     * @param minutes 距离当前时间的分钟数。
     * @param second 距离当前时间的秒数。
     * @param microSec 距离当前时间的毫秒数。
     * @param pRawShmObj 指向NFRawShmObj对象的指针，可选参数，默认为nullptr。
     * @return int 返回定时器注册的结果，通常为成功或失败的状态码。
     */
    virtual int SetTimer(NFObject* pObj, int hour, int minutes, int second, int microSec, NFRawObject* pRawShmObj = nullptr) = 0;

    /**
     * @brief 注册一个定时器，该定时器在指定的时分秒时间点执行一次。
     *
     * @param pObj 指向NFShmObj对象的指针，该对象将与定时器关联。
     * @param hour 执行时间点的小时数。
     * @param minutes 执行时间点的分钟数。
     * @param second 执行时间点的秒数。
     * @param pRawShmObj 指向NFRawShmObj对象的指针，可选参数，默认为nullptr。
     * @return int 返回定时器注册的结果，通常为成功或失败的状态码。
     */
    virtual int SetCalender(NFObject* pObj, int hour, int minutes, int second, NFRawObject* pRawShmObj = nullptr) = 0;

    /**
     * @brief 注册一个定时器，该定时器在指定的时间戳（秒为单位）执行一次。
     *
     * @param pObj 指向NFShmObj对象的指针，该对象将与定时器关联。
     * @param timestamp 执行时间点的时间戳，单位为秒。
     * @param pRawShmObj 指向NFRawShmObj对象的指针，可选参数，默认为nullptr。
     * @return int 返回定时器注册的结果，通常为成功或失败的状态码。
     */
    virtual int SetCalender(NFObject* pObj, uint64_t timestamp, NFRawObject* pRawShmObj = nullptr) = 0;

    /**
     * @brief 注册循环执行定时器
     *
     * 该函数用于注册一个循环执行的定时器，定时器的第一次执行时间由当前时间加上指定的时分秒毫秒确定，
     * 后续的执行间隔由指定的毫秒数决定。
     *
     * @param pObj 指向NFShmObj对象的指针，表示定时器关联的对象
     * @param interval 循环间隔时间，单位为毫秒
     * @param callCount 定时器调用的次数，如果为0则表示无限循环
     * @param hour 第一次执行距离当前时间的小时数
     * @param minutes 第一次执行距离当前时间的分钟数
     * @param second 第一次执行距离当前时间的秒数
     * @param microSec 第一次执行距离当前时间的毫秒数
     * @param pRawShmObj 指向NFRawShmObj对象的指针，默认为nullptr
     * @return int 返回定时器的ID或错误码
     */
    virtual int SetTimer(NFObject* pObj, int interval, int callCount, int hour, int minutes, int second, int microSec, NFRawObject* pRawShmObj = nullptr) = 0;

    /**
     * @brief 注册循环执行定时器
     *
     * 该函数用于注册一个循环执行的定时器，定时器的第一次执行时间由当前时间加上指定的时分秒毫秒确定，
     * 后续的执行间隔由系统默认值决定。
     *
     * @param pObj 指向NFShmObj对象的指针，表示定时器关联的对象
     * @param callCount 定时器调用的次数，如果为0则表示无限循环
     * @param hour 第一次执行距离当前时间的小时数
     * @param minutes 第一次执行距离当前时间的分钟数
     * @param second 第一次执行距离当前时间的秒数
     * @param microSec 第一次执行距离当前时间的毫秒数
     * @param pRawShmObj 指向NFRawShmObj对象的指针，默认为nullptr
     * @return int 返回定时器的ID或错误码
     */
    virtual int SetDayTime(NFObject* pObj, int callCount, int hour, int minutes, int second, int microSec, NFRawObject* pRawShmObj = nullptr) = 0;

    /**
     * @brief 注册某一个时间点日循环执行定时器
     *
     * 该函数用于注册一个每日在指定时间点执行的定时器，定时器将在每天的指定时分秒时刻执行。
     *
     * @param pObj 指向NFShmObj对象的指针，表示定时器关联的对象
     * @param callCount 定时器调用的次数，如果为0则表示无限循环
     * @param hour 每天开始执行的小时数
     * @param minutes 每天开始执行的分钟数
     * @param second 每天开始执行的秒数
     * @param pRawShmObj 指向NFRawShmObj对象的指针，默认为nullptr
     * @return int 返回定时器的ID或错误码
     */
    virtual int SetDayCalender(NFObject* pObj, int callCount, int hour, int minutes, int second, NFRawObject* pRawShmObj = nullptr) = 0;

    /**
     * @brief 设置周循环定时器
     *
     * 该函数用于设置一个周循环定时器，定时器将在指定的时间间隔后首次执行，并在之后的每周同一时间重复执行。
     *
     * @param pObj 指向NFShmObj对象的指针，用于管理定时器的共享内存对象。
     * @param callCount 定时器的调用次数，表示定时器将执行的总次数。
     * @param hour 首次执行距离当前时间的小时数。
     * @param minutes 首次执行距离当前时间的分钟数。
     * @param second 首次执行距离当前时间的秒数。
     * @param microSec 首次执行距离当前时间的微秒数。
     * @param pRawShmObj 指向NFRawShmObj对象的指针，用于管理原始共享内存对象，默认为nullptr。
     * @return int 返回操作结果，通常为成功或错误码。
     */
    virtual int SetWeekTime(NFObject* pObj, int callCount, int hour, int minutes, int second, int microSec, NFRawObject* pRawShmObj = nullptr) = 0;

    /**
     * @brief 注册周循环定时器
     *
     * 该函数用于注册一个周循环定时器，定时器将在每周的指定时间点开始执行。
     *
     * @param pObj 指向NFShmObj对象的指针，用于管理定时器的共享内存对象。
     * @param callCount 定时器的调用次数，表示定时器将执行的总次数。
     * @param weekDay 定时器开始执行的星期几，范围为0-6（0表示周日，1表示周一，依此类推）。
     * @param hour 定时器开始执行的小时数。
     * @param minutes 定时器开始执行的分钟数。
     * @param second 定时器开始执行的秒数。
     * @param pRawShmObj 指向NFRawShmObj对象的指针，用于管理原始共享内存对象，默认为nullptr。
     * @return int 返回操作结果，通常为成功或错误码。
     */
    virtual int SetWeekCalender(NFObject* pObj, int callCount, int weekDay, int hour, int minutes, int second, NFRawObject* pRawShmObj = nullptr) = 0;

    /**
     * @brief 设置月循环定时器
     *
     * 该函数用于设置一个月循环定时器，定时器将在指定的时间间隔后首次执行，并在之后的每月同一时间重复执行。
     *
     * @param pObj 指向NFShmObj对象的指针，用于管理定时器的共享内存对象。
     * @param callCount 定时器的调用次数，表示定时器将执行的总次数。
     * @param hour 首次执行距离当前时间的小时数。
     * @param minutes 首次执行距离当前时间的分钟数。
     * @param second 首次执行距离当前时间的秒数。
     * @param microSec 首次执行距离当前时间的微秒数。
     * @param pRawShmObj 指向NFRawShmObj对象的指针，用于管理原始共享内存对象，默认为nullptr。
     * @return int 返回操作结果，通常为成功或错误码。
     */
    virtual int SetMonthTime(NFObject* pObj, int callCount, int hour, int minutes, int second, int microSec, NFRawObject* pRawShmObj = nullptr) = 0;

    /**
     * @brief 注册一个月循环执行的定时器
     *
     * 该函数用于在每月的某一天、某一时间点注册一个定时器，定时器会在每月的该时间点触发执行。
     *
     * @param pObj 指向共享内存对象的指针，用于存储定时器相关信息。
     * @param callCount 定时器触发的次数，若为0则表示无限次触发。
     * @param day 一个月中的某一天，范围为1-31。
     * @param hour 一天中的某一小时，范围为0-23。
     * @param minutes 一小时中的某一分钟，范围为0-59。
     * @param second 一分钟中的某一秒，范围为0-59。
     * @param pRawShmObj 指向原始共享内存对象的指针，可选参数，默认为nullptr。
     * @return int 返回操作结果，成功返回0，失败返回错误码。
     */
    virtual int SetMonthCalender(NFObject* pObj, int callCount, int day, int hour, int minutes, int second, NFRawObject* pRawShmObj = nullptr) = 0;

    /**
     * @brief 触发执行事件
     *
     * 该函数用于在指定服务器类型上触发一个事件，并传递相关的事件信息。
     *
     * @param serverType 服务器类型，表示事件触发的目标服务器。
     * @param eventId 事件ID，用于标识具体的事件。
     * @param srcType 事件源类型，表示事件的来源类型。
     * @param srcId 事件源ID，表示事件的具体来源。
     * @param message 事件消息，包含事件的具体信息，使用Google Protocol Buffers格式。
     * @return int 返回操作结果，成功返回0，失败返回错误码。
     */
    virtual int FireExecute(NF_SERVER_TYPE serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const google::protobuf::Message& message) = 0;

    /**
     * @brief 订阅事件
     *
     * 该函数用于在指定服务器类型上订阅一个事件，并指定事件的来源和描述信息。
     *
     * @param pObj 指向共享内存对象的指针，用于存储订阅事件的相关信息。
     * @param serverType 服务器类型，表示订阅事件的目标服务器。
     * @param eventId 事件ID，用于标识具体的事件。
     * @param srcType 事件源类型，表示事件的来源类型。
     * @param srcId 事件源ID，表示事件的具体来源。
     * @param desc 事件描述信息，用于描述事件的具体内容。
     * @return int 返回操作结果，成功返回0，失败返回错误码。
     */
    virtual int Subscribe(NFObject* pObj, NF_SERVER_TYPE serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const std::string& desc) = 0;

    /**
     * @brief 取消订阅指定的事件。
     *
     * 该函数用于取消订阅由 `pObj` 对象在 `serverType` 服务器类型下，针对特定事件 `eventId` 的订阅。
     * 订阅的源类型和源ID分别由 `srcType` 和 `srcId` 指定。
     *
     * @param pObj 指向共享内存对象的指针，表示订阅的主体。
     * @param serverType 服务器类型，表示事件订阅的服务器环境。
     * @param eventId 事件ID，表示要取消订阅的具体事件。
     * @param srcType 源类型，表示事件的来源类型。
     * @param srcId 源ID，表示事件的具体来源标识。
     * @return 返回操作结果，通常为成功或失败的状态码。
     */
    virtual int UnSubscribe(NFObject* pObj, NF_SERVER_TYPE serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId) = 0;

    /**
     * @brief 取消所有订阅。
     *
     * 该函数用于取消由 `pObj` 对象发起的所有事件订阅。
     *
     * @param pObj 指向共享内存对象的指针，表示订阅的主体。
     * @return 返回操作结果，通常为成功或失败的状态码。
     */
    virtual int UnSubscribeAll(NFObject* pObj) = 0;
};
