#pragma once

#include "NFServerComm/NFServerCommon/NFIDescStoreEx.h"
#include "NFServerComm/NFServerCommon/NFIDescTemplate.h"
#include "NFComm/NFShmCore/NFResDb.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmStl/NFShmHashMap.h"
#include "NFComm/NFShmStl/NFShmVector.h"
#include "NFLogicCommon/NFDescStoreTypeDefines.h"
#include "DescStore/FunctionunlockFunctionunlockDesc.h"
#include "NFGameCommon/NFComTypeDefine.h"

#define MAX_FUNCTIONUNLOCK_FUNCTIONUNLOCK_SPECIAL_TYPE_NUM 4

//���ܿ��Ž�������
enum FunctionUnlockType
{
    FUNCTION_UNLOCK_TYPE_LEVEL	  = 1,	// �ȼ�����
    FUNCTION_UNLOCK_TYPE_TAST	  = 2,	// ���񿪷�
    FUNCTION_UNLOCK_TYPE_TRANSFER = 3,	// תְ����
    FUNCTION_UNLOCK_TYPE_PAYRMB	  = 4,	// ��ʷ�ܳ�ֵ������
    FUNCTION_UNLOCK_TYPE_VIP_LEV = 5,	// VIP�ȼ�
    FUNCTION_UNLOCK_TYPE_SPECIAL = 6,	// ���⿪������(���ⲿϵͳ��������,����ʹ����Ʒ,������ÿ�ε�¼ʱ�������¼�鹦���Ƿ���)
    FUNCTION_UNLOCK_TYPE_DUP = 8,		// ��������
    FUNCTION_UNLOCK_TYPE_GOD_EVIL = 9,		// ��ħ����
    FUNCTION_UNLOCK_TYPE_BEST_EQ  = 10,	 // ���װ��
    FUNCTION_UNLOCK_TYPE_MARRY_TASK = 11, //����콱
    FUNCTION_UNLOCK_TYPE_RECHARGE = 12, //��ֵ����/�رչ���
    FUNCTION_UNLOCK_TYPE_TURN_TASK = 13, //�������ǰ������
    FUNCTION_UNLOCK_TYPE_SOUL_TASK = 14, //���ս��ǰ������
};


//���ܿ���ͬ������
enum FunctionSyncType
{
    FUNCTION_SYNC_TYPE_ALL	  = 1,	// ȫ��
    FUNCTION_SYNC_TYPE_ADD	  = 2,	// ����
};

class FunctionunlockDescEx : public NFShmObjGlobalTemplate<FunctionunlockDescEx, EOT_CONST_FUNCTIONUNLOCK_DESC_EX_ID, NFIDescStoreEx>
{
public:
	FunctionunlockDescEx();
	virtual ~FunctionunlockDescEx();
	int CreateInit();
	int ResumeInit();
public:
	virtual int Load() override;
	virtual int CheckWhenAllDataLoaded() override;
public:
    //���ݽ������ͺͽ���ֵ ��ȡ��Ӧ�Ĺ����б�
    const SET_UINT64* GetUnlockFunctionList(uint32_t nType, uint64_t nValue, bool externFlag = false);
protected:
    //��������� ��������  map<�������ͣ�map<����ֵ��vector<ϵͳID>>>
    MAP_UINT32_MAP_UINT64_VEC_UINT64 m_unlockFuncList;
    
    //��������� ��������  map<�������ͣ�map<����ֵ��set<����ID> > >
    ORDER_MAP_UINT32_ORDER_MAP_UINT64_SET_UINT64 m_mapUnlockCfg;
};
