// -------------------------------------------------------------------------
//    @FileName         :    NFShmIdx.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------
#pragma once

#define MAXDSINFOITEM  2
#define IDXNULL			-1

class NFObject;

class NFShmIdx
{
public:
	NFShmIdx();
	~NFShmIdx();
	int CreateInit();
	int ResumeInit();
	void Initialize();
	NFObject* GetAttachedObj();
	const NFObject* GetAttachedObj() const;
	void SetAttachedObj(NFObject* pObj);
	void* GetObjBuf() const;
	void SetObjBuf(void* pBuf);
	int GetIndex() const { return m_iIndex; }
	void SetIndex(int iIndex) { m_iIndex = iIndex; }

	NFShmIdx& operator=(const NFShmIdx& idx)
	{
		if (this != &idx)
		{
			m_uEntity.m_pBuf = idx.m_uEntity.m_pBuf;
			m_iIndex = idx.m_iIndex;
		}

		return *this;
	}

private:
	union
	{
		NFObject* m_pAttachedObj;
		void* m_pBuf;
	} m_uEntity;

	int m_iIndex;
};
