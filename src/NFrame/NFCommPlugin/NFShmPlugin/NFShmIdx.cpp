// -------------------------------------------------------------------------
//    @FileName         :    NFShmIdx.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------

#include "NFShmIdx.h"
#include "NFComm/NFObjCommon/NFObject.h"

NFShmIdx::NFShmIdx()
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

NFShmIdx::~NFShmIdx()
{
	Initialize();
}

int NFShmIdx::CreateInit()
{
	Initialize();
	return 0;
}

int NFShmIdx::ResumeInit()
{
	return 0;
}

void NFShmIdx::Initialize()
{
	m_uEntity.m_pAttachedObj = nullptr;
	m_iIndex = -1;
}

void* NFShmIdx::GetObjBuf() const
{
	return m_uEntity.m_pBuf;
}

void NFShmIdx::SetObjBuf(void* pBuf)
{
	assert(pBuf);
	m_uEntity.m_pBuf = pBuf;
}

NFObject* NFShmIdx::GetAttachedObj()
{
	return m_uEntity.m_pAttachedObj;
}

const NFObject* NFShmIdx::GetAttachedObj() const
{
	return m_uEntity.m_pAttachedObj;
}

void NFShmIdx::SetAttachedObj(NFObject* pObj)
{
	assert(pObj);
	m_uEntity.m_pAttachedObj = pObj;
}

