// -------------------------------------------------------------------------
//    @FileName         :    NFTickByRunIndexOP.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFTickByRunIndexOP.cpp
//
// -------------------------------------------------------------------------

#include "NFTickByRunIndexOP.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"

int NFTickByRunIndexOP::TickNow(uint32_t dwCurRunIndex)
{
    int iRetCode = 0;

    if (IsNeedTick(dwCurRunIndex) == false)
    {
        return 0;
    }

    m_dwLastTickRunIndex=dwCurRunIndex;
    m_iTickedNum = 0;

    iRetCode = DoTick(dwCurRunIndex);
    if (iRetCode != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "DoTick Failed");
    }

    ResetPerTick(dwCurRunIndex);

    return 0;
}

int NFTickByRunIndexOP::TickAllWhenStop(uint32_t dwCurRunIndex)
{
    int iRetCode = 0;
    iRetCode = DoTick(dwCurRunIndex, true);
    if (iRetCode != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "DoTick Failed");
    }

    m_iTickedNum = 0;

    ResetPerTick(dwCurRunIndex);

    return 0;
}

int NFTickByRunIndexOP::DoChangeTickGap()
{
    return 0;
}