// -------------------------------------------------------------------------
//    @FileName         :    NFISaveDb.cpp
//    @Author           :    gaoyi
//    @Date             :    23-11-17
//    @Email			:    445267987@qq.com
//    @Module           :    NFISaveDb
//
// -------------------------------------------------------------------------

#include "NFISaveDb.h"

NFISaveDb::NFISaveDb()
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

NFISaveDb::~NFISaveDb()
{
}

int NFISaveDb::CreateInit()
{
    m_lastSaveDbTime = 0;
    m_curSavingDbTime = 0;
    m_lastAllSeq = 0;
    return 0;
}


int NFISaveDb::ResumeInit()
{
    return 0;
}


bool NFISaveDb::IsNeedSave() const
{
    if (IsUrgentNeedSave())
    {
        return true;
    }
    return true;
}

bool NFISaveDb::IsSavingDb() const
{
    return (m_curSavingDbTime != 0) && (static_cast<uint64_t>(NF_ADJUST_TIMENOW()) <= m_curSavingDbTime + SAVE_TRANS_ACTIVE_TIMEOUT + 5);
}

int NFISaveDb::SendTransToDb(int iReason)
{
    return 0;
}

bool NFISaveDb::CanSaveDb(uint32_t maxTimes, bool bForce)
{
    if (IsNeedSave() && !IsSavingDb())
    {
        if (bForce || NF_ADJUST_TIMENOW() - m_lastSaveDbTime >= maxTimes)
        {
            return true;
        }
    }
    return false;
}

int NFISaveDb::SaveToDb(uint32_t maxTimes, int iReason, bool bForce)
{
    if (CanSaveDb(maxTimes, bForce))
    {
        SendTransToDb(iReason);
    }
    return 0;
}

int NFISaveDb::OnSaveDb(bool success, uint32_t seq)
{
    m_lastAllSeq = seq;
    m_lastSaveDbTime = NF_ADJUST_TIMENOW();
    m_curSavingDbTime = 0;
    if (success && seq == GetAllSeq())
    {
        ClearAllSeq();
    }
    return 0;
}

uint32_t NFISaveDb::GetAllSeq() const
{
    return GetCurSeq();
}

void NFISaveDb::ClearAllSeq()
{
    ClearUrgent();
}