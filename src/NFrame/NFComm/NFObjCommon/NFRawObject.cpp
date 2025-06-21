// -------------------------------------------------------------------------
//    @FileName         :    NFShmTimerObj.cpp
//    @Author           :    gaoyi
//    @Date             :    23-9-25
//    @Email			:    445267987@qq.com
//    @Module           :    NFShmTimerObj
//
// -------------------------------------------------------------------------

#include "NFRawObject.h"

NFRawObject::NFRawObject()
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

NFRawObject::~NFRawObject()
{
    NFRawObject::DeleteAllTimer();
}

int NFRawObject::CreateInit()
{
    m_pShmObj = nullptr;
    return 0;
}

int NFRawObject::ResumeInit()
{
    return 0;
}

int NFRawObject::InitShmObj(const NFObject* pShmObj)
{
    m_pShmObj = pShmObj;
    return 0;
}

NFObject* NFRawObject::GetShmObj()
{
    return m_pShmObj.GetPoint();
}

int NFRawObject::OnTimer(int timeId, int callCount)
{
    return 0;
}