// -------------------------------------------------------------------------
//    @FileName         :    NFISaveDb.h
//    @Author           :    gaoyi
//    @Date             :    23-11-17
//    @Email			:    445267987@qq.com
//    @Module           :    NFISaveDb
//
// -------------------------------------------------------------------------

#pragma once

#include <NFComm/NFObjCommon/NFSeqOP.h>
#include <NFComm/NFObjCommon/NFTransBase.h>
#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFObjCommon/NFObjectTemplate.h"
#include "NFComm/NFObjCommon/NFObject.h"

class NFISaveDb : public NFSeqOP
{
public:
    NFISaveDb();

    ~NFISaveDb() override;

    int CreateInit();
    int ResumeInit();
public:
    uint64_t GetLastSaveDbTime() const { return m_lastSaveDbTime; }

    void SetLastSaveDbTime(uint64_t saveTime) { m_lastSaveDbTime = saveTime; }

    void SetCurSavingDbTime(uint64_t saveTime) { m_curSavingDbTime = saveTime; }

    uint64_t GetCurSaveingDbTime() const { return m_curSavingDbTime; }

    virtual bool IsNeedSave() const;

    virtual bool IsSavingDb() const;

    virtual int SendTransToDb(int iReason);

    virtual int SaveToDb(uint32_t maxTimes, int iReason, bool bForce = false);

    virtual bool CanSaveDb(uint32_t maxTimes, bool bForce = false);

    virtual int OnSaveDb(bool success, uint32_t seq);

    virtual uint32_t GetAllSeq() const;

    virtual void ClearAllSeq();
protected:
    uint64_t m_lastSaveDbTime;
    uint64_t m_curSavingDbTime;
    uint64_t m_lastAllSeq;
};