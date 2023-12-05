#pragma once

#include "NFServerComm/NFServerCommon/NFIDescStoreEx.h"
#include "NFServerComm/NFServerCommon/NFIDescTemplate.h"
#include "NFComm/NFShmCore/NFResDb.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmStl/NFShmHashMap.h"
#include "NFComm/NFShmStl/NFShmVector.h"
#include "NFLogicCommon/NFDescStoreTypeDefines.h"
#include "NFGameCommon/NFComTypeDefine.h"

//��������
struct SkillCfg
{
	SkillCfg()
	{
	
	}
	
	VEC_INT32 vec_unlocktype;					//��������
	VEC_INT32 vec_unlockvalue;					//����ֵ
	
	int32_t rangeValue1 = 0;					//��Χֵ1
	int32_t rangeValue2 = 0;					//��Χֵ2
	int32_t rangeValue3 = 0;					//��Χֵ3
	int32_t rangeValueEx1 = 0;					//��չ��Χֵ1
	int32_t rangeValueEx2 = 0;					//��չ��Χֵ2
	int32_t rangeValueEx3 = 0;					//��չ��Χֵ3
	
	VEC_INT64 vecRelation;						//��������
	int64_t preRelation = 0;					//ǰ�ù�������ID
	VEC_UINT32 vecDamage;						//�˺�ʱ���
	VEC_UINT32 vecDamageEx;						//��չ�˺�ʱ���
};

class SkillDescEx : public NFShmObjGlobalTemplate<SkillDescEx, EOT_CONST_SKILL_DESC_EX_ID, NFIDescStoreEx>
{
public:
	SkillDescEx();
	virtual ~SkillDescEx();
	int CreateInit();
	int ResumeInit();
public:
	virtual int Load() override;
	virtual int CheckWhenAllDataLoaded() override;
};
