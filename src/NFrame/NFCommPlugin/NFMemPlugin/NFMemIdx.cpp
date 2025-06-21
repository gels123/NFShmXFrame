// -------------------------------------------------------------------------
//    @FileName         :    NFMemIdx.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------

#include "NFMemIdx.h"
#include "NFComm/NFObjCommon/NFObject.h"

NFMemIdx::NFMemIdx()
{
	Initialize();
}

NFMemIdx::~NFMemIdx()
{
	Initialize();
}

void NFMemIdx::Initialize()
{
	m_uEntity.m_pAttachedObj = nullptr;
    m_iIndex = -1;
}

void* NFMemIdx::GetObjBuf() const
{
	return m_uEntity.m_pBuf;
}

void NFMemIdx::SetObjBuf(void* pBuf)
{
	assert(pBuf);
	m_uEntity.m_pBuf = pBuf;
}

NFObject* NFMemIdx::GetAttachedObj()
{
	return m_uEntity.m_pAttachedObj;
}

const NFObject* NFMemIdx::GetAttachedObj() const
{
	return m_uEntity.m_pAttachedObj;
}

void NFMemIdx::SetAttachedObj(NFObject* pObj)
{
	assert(pObj);
	m_uEntity.m_pAttachedObj = pObj;
}

