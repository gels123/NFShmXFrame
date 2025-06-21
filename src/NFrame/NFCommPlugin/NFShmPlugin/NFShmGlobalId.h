// -------------------------------------------------------------------------
//    @FileName         :    NFShmGlobalId.h
//    @Author           :    Yi.Gao
//    @Date             :   2022-09-18
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------

#pragma once

#include <NFComm/NFShmStl/NFShmQueue.h>
#include <NFComm/NFShmStl/NFShmString.h>

#include "NFComm/NFObjCommon/NFObjPtr.h"
#include "NFComm/NFObjCommon/NFTypeDefines.h"

// 2的指数
#ifndef MAX_GLOBALID_NUM
#define MAX_GLOBALID_NUM 1048576
#endif

#define MAX_GLOBALID_NUM_MASK (MAX_GLOBALID_NUM-1)
#define MAX_GLOBALID_QUEUE_SIZE (MAX_GLOBALID_NUM+1)  //MAX_GLOBALID_NUM 的数值+1


#define BUFFSIZE 1024
#define GLOBALID_LOOP_BACK 2000

class NFObject;
class NFShmObjSeg;

struct NFShmIdIndex
{
	int m_iId;
	int m_iIndex;
	int m_iType;
	NFRawShmPtr<NFObject> m_pObjPtr;
};

class NFShmGlobalId final : public NFObject
{
public:
	NFShmGlobalId();
	~NFShmGlobalId() override;

	int CreateInit();
	int ResumeInit();

	int GetGlobalId(int iType, int iIndex, const NFObject* pObj);
	int ReleaseId(int iId);
	NFObject* GetObj(int iId);

public:
	static int RegisterClassToObjSeg(int bType, int iObjSize, int iObjCount, const std::string& className, bool useHash, bool singleton);
	static NFObject* ResumeObject(void* pBuffer);
	static NFObject* CreateObject();
	static void DestroyObject(NFObject* pObj);
	static std::string GetStaticClassName() { return "NFShmGlobalId"; }
	static int GetStaticClassType() { return EOT_GLOBAL_ID; }
	void* operator new(size_t nSize, void* pBuffer) throw();
public:
	int AddSecond(int iSecond);
	int SetSecOffSet(int iSecOffSet);
	int GetSecOffSet() const;
	int GetUseCount() const;

	std::string GetOperatingStatistic() const;
	void SetRunTimeFileId(int iId);
private:
	bool ResumeFileUpdateData();
	bool CalcRoundUpdateFile();
	bool WriteRound() const;

protected:
	int m_iThisRoundCountMax;
	int m_iUseCount;
	int m_iThisRoundCount;
	int m_iRoundMultiple;
	int m_iGlobalIdAppendNum;
	NFShmString<32> m_szFileName;
	NFShmQueue<int,MAX_GLOBALID_QUEUE_SIZE> m_stQueue;
	NFShmIdIndex m_stIdTable[MAX_GLOBALID_NUM];
	time_t m_tRoundBegin;
	int m_iSecOffSet;
};

