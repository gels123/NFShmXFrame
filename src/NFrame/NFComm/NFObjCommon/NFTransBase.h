// -------------------------------------------------------------------------
//    @FileName         :    NFTransBase.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFTransBase.h
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFObjCommon/NFObject.h"
#include "NFComm/NFObjCommon/NFShmMgr.h"

enum
{
    ERR_TRANS_RUNED_TOO_MUCH = 100057,
    MAX_TRANS_RUNED_TIMES = 10000,
    TRANS_TIMEOUT_VALUE = 300,
#ifdef NF_DEBUG_MODE
    TRANS_ACTIVE_TIMEOUT = 200, //debug 模式下, 方便调试。。 trans每一步的最大生命周期200
#else
    TRANS_ACTIVE_TIMEOUT = 10, //trans每一步的最大生命周期10
#endif
    SAVE_TRANS_ACTIVE_TIMEOUT = 10, //跟TRANS_ACTIVE_TIMEOUT最小的一样
    MAX_WAIT_TRANS_FINISH_TIME = TRANS_ACTIVE_TIMEOUT + 1,
};

#define MAX_WAIT_TRANS_FINISH_TIME ()

class NFTransBase : public NFObjectTemplate<NFTransBase, EOT_TRANS_BASE, NFObject>
{
public:
    NFTransBase();

    ~NFTransBase() override;

    int CreateInit();
    int ResumeInit();

    int Init();

    void IncreaseRunTimes();

    void SetFinished(int iRetCode);

    bool IsFinished() const;

    int ProcessCSMsgReq(const google::protobuf::Message* pCsMsgReq);

    int ProcessDispSvrRes(uint32_t nMsgId, const NFDataPackage& packet, uint32_t reqTransId, uint32_t rspTransId);

    int ProcessDBMsgRes(const google::protobuf::Message* pSsMsgRes, uint32_t cmd, uint32_t tableId, uint32_t seq, int32_t errCode);

    int ProcessDBMsgRes(NFrame::Proto_FramePkg& svrPkg);

    bool IsCanRelease() const;

    virtual std::string DebugString() const;

    /**
     * @brief 定时检查函数，用于少数需要定时处理的trans（事务）场景。
     *
     * 该函数为虚函数，默认返回0，具体实现应由派生类根据需要进行重写。
     * 建议在具体的trans类中增加字段记录tick间隔，以便更好地控制定时检查的频率。
     *
     * @return int 返回值为0，表示默认的tick处理结果。派生类可以根据需要返回其他值。
     */
    virtual int Tick() { return 0; }
    int ProcessTick();

    uint16_t GetState() const { return m_wCurState; }
    void SetState(uint16_t wState) { m_wCurState = wState; }

    uint32_t GetStartTime() const { return m_dwStartTime; }
    uint32_t GetActiveTime() const { return m_dwActiveTime; }
    void SetActiveTime(uint32_t dwActiveTime) { m_dwActiveTime = dwActiveTime; }
    int GetActiveTimeOut() const { return m_iActiveTimeOut; }
    void SetActiveTimeOut(int timeOut) { m_iActiveTimeOut = timeOut; }
    uint32_t GetMaxRunTimes() const { return m_dwMaxRunTimes; }
    void SetMaxRunTimes(uint32_t dwMaxRunTimes) { m_dwMaxRunTimes = dwMaxRunTimes; }
    //trans自身是否超时了
    bool IsSelfTimeOut() const { return m_bIsSelfTransTimeout; }

    virtual int OnTimeOut();

    virtual bool IsTimeOut();

    virtual int OnTransFinished(int iRunLogicRetCode) { return 0; }
    virtual int HandleTransFinished(int iRunLogicRetCode) { return 0; }

protected:
    virtual int HandleCSMsgReq(const google::protobuf::Message* pCsMsgReq);

    virtual int HandleDispSvrRes(uint32_t nMsgId, const NFDataPackage& packet, uint32_t reqTransId, uint32_t rspTransId);

    virtual int HandleDBMsgRes(const google::protobuf::Message* pSsMsgRes, uint32_t cmd, uint32_t tableId, uint32_t seq, int32_t errCode);

protected:
    int CheckCanRunNow() const;

    int RunCommLogic();

protected:
    /**
     * @brief 该结构体用于管理事务（trans）的状态和相关信息。
     *
     * 该结构体包含多个成员变量，用于记录事务的启动时间、活跃时间、当前状态、运行次数、最大运行次数、运行逻辑返回值、RPC ID、事务是否超时以及超时设置等信息。
     *
     * 成员变量说明：
     * - m_bIsFinished: 标识事务是否已完成。
     * - m_dwStartTime: 记录事务的启动时间。
     * - m_dwActiveTime: 记录事务的活跃时间。
     * - m_wCurState: 记录事务的当前状态。
     * - m_wRunedTimes: 记录事务已经运行的次数。
     * - m_dwMaxRunTimes: 记录事务的最大允许运行次数。
     * - m_iRunLogicRetCode: 记录事务运行逻辑的返回值。
     * - m_rpcId: 记录事务的RPC ID。
     * - m_bIsSelfTransTimeout: 标识事务自身是否超时。由于外部可能会设置m_iRunLogicRetCode，因此使用该变量来确保超时判断的准确性。
     * - m_iActiveTimeOut: 设置事务的超时时间。
     */
    bool m_bIsFinished;
    uint32_t m_dwStartTime;
    uint32_t m_dwActiveTime;
    uint16_t m_wCurState;
    uint16_t m_wRunedTimes;
    uint32_t m_dwMaxRunTimes;
    int m_iRunLogicRetCode;
    int64_t m_rpcId;

    /**
     * @brief 标识事务自身是否超时。
     *
     * 由于外部可能会设置m_iRunLogicRetCode，因此使用该变量来确保超时判断的准确性。
     */
    bool m_bIsSelfTransTimeout;

    /**
     * @brief 设置事务的超时时间。
     */
    int m_iActiveTimeOut;
};

#define CHECK_ERR_AND_FIN_TRANS(iRetCode, pTrans, format, ...)\
    if( iRetCode != 0 )\
    {\
        pTrans->SetFinished( iRetCode );\
        std::stringstream ss;\
        ss << format;\
        std::string log_event = NF_FORMAT(ss.str().c_str(), ##__VA_ARGS__);\
        NFLogError(NF_LOG_DEFAULT, 0, "CHCK TRANS RetCode:{} failed:{}", iRetCode, log_event);\
        return iRetCode;\
    }

#define CHECK_EXPR_AND_FIN_TRANS(expr, iRetCode, pTrans, format, ...)\
    if( !(expr) )\
    {\
        pTrans->SetFinished( iRetCode );\
        std::stringstream ss;\
        ss << format;\
        std::string log_event = NF_FORMAT(ss.str().c_str(), ##__VA_ARGS__);\
        NFLogError(NF_LOG_DEFAULT, 0, "CHECK Trans:{} RetCode:{} failed:{}", #expr, iRetCode, log_event);\
        return iRetCode;\
    }
