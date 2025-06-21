// -------------------------------------------------------------------------
//    @FileName         :    NFTickByRunIndexOP.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFTickByRunIndexOP.h
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFObjCommon/NFObject.h"
#include "NFComm/NFObjCommon/NFShmMgr.h"

class NFTickByRunIndexOP
{
public:
	NFTickByRunIndexOP()
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

	NFTickByRunIndexOP(uint32_t dwWaitTickIndex, int iNumPerTick)
	{
		Init(dwWaitTickIndex, iNumPerTick);
	}

	int CreateInit()
	{
		m_dwLastTickRunIndex = 0;
		m_iLastTickIndex     = 0;
		m_iTickedNum         = 0;
		m_dwWaitTickIndex    = 0;
		m_iNumPerTick        = 0;
		m_bIsTickFinished    = false;
		return 0;
	}

	int ResumeInit()
	{
		return 0;
	}

	void Init(uint32_t dwWaitTickIndex, int iNumPerTick)
	{
		m_dwLastTickRunIndex = 0;
		m_iLastTickIndex     = 0;
		m_iTickedNum         = 0;
		m_dwWaitTickIndex    = dwWaitTickIndex;
		m_iNumPerTick        = iNumPerTick;
		m_bIsTickFinished    = false;
		m_bNeedChangeGap = false;
		m_iDestTimePerRound = 0;
		m_iBaseTickMS = 10;
	}

	virtual ~NFTickByRunIndexOP()
	{
	}

	int TickNow(uint32_t dwCurRunIndex);

	int TickAllWhenStop(uint32_t dwCurRunIndex);
protected:
	virtual int DoTick(uint32_t dwCurRunIndex, bool bIsTickAll = false) = 0;

	virtual int DoChangeTickGap();

	// iTimeMS: tick一轮需要的时间
	// iBaseTickMS: gamesvr tick的时间, 一般10ms
	int SetChangeTickGap(int iTimeMS, int iBaseTickMS)
	{
		m_bNeedChangeGap = true;
		m_iDestTimePerRound = iTimeMS;
		m_iBaseTickMS = iBaseTickMS;
		return 0;
	}

	void ResetPerTick(uint32_t dwCurRunIndex)
	{
		if (m_bIsTickFinished)
		{
			m_iLastTickIndex     = 0;
			m_dwLastTickRunIndex = dwCurRunIndex;
			m_bIsTickFinished    = false;
		}

		m_iTickedNum = 0;
	}

	bool IsNeedTick(uint32_t dwCurRunIndex) const
	{
		return ((m_dwLastTickRunIndex + m_dwWaitTickIndex) <= dwCurRunIndex || (dwCurRunIndex < m_dwLastTickRunIndex));
	}
protected:
	uint32_t m_dwLastTickRunIndex; //外层tick上一次传入的索引值
	int m_iLastTickIndex; //内层tick已经执行的对象个数
	int m_iTickedNum; //本次tick执行的对象个数
	uint32_t m_dwWaitTickIndex; //外层调用多少次tick才真正执行一次内层的tick
	int m_iNumPerTick; ;//每次执行时需要操作的对象个数
	bool m_bIsTickFinished; //本次tick结束标志

	bool m_bNeedChangeGap; // 是否需要调整tick频率
	int m_iDestTimePerRound; // 调整到的执行一轮的目标时间
	int m_iBaseTickMS;     // server基础tick时间 一般为10ms
};
