#pragma once

#include "NFServerComm/NFServerCommon/NFIDescStoreEx.h"
#include "NFServerComm/NFServerCommon/NFIDescTemplate.h"
#include "NFComm/NFShmCore/NFResDb.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmStl/NFShmHashMap.h"
#include "NFComm/NFShmStl/NFShmVector.h"
#include "NFLogicCommon/NFDescStoreTypeDefines.h"
#include "NFLogicCommon/NFFestivalDefine.h"

//���ջģ������
struct FestTplCfg
{
	FestTplCfg()
	{
		if (EN_OBJ_MODE_INIT == NFShmMgr::Instance()->GetCreateMode()) {
			CreateInit();
		}
		else {
			ResumeInit();
		}
	}
	
	int CreateInit()
	{
		timetype = 0;
		timeparam1 = 0;
		timeparam2 = 0;
		timeparam3 = 0;
		tempId = 0;
		return 0;
	}
	
	int ResumeInit()
	{
		return 0;
	}
	
	int32_t timetype;
	int64_t timeparam1;
	int64_t timeparam2;
	int64_t timeparam3;
	int64_t tempId;
};

class FestivalDescEx : public NFShmObjGlobalTemplate<FestivalDescEx, EOT_CONST_FESTIVAL_DESC_EX_ID, NFIDescStoreEx>
{
	using MapFestTplCfg = NFShmHashMap<int32_t, FestTplCfg, 10>;
public:
	FestivalDescEx();
	virtual ~FestivalDescEx();
	int CreateInit();
	int ResumeInit();
public:
	virtual int Load() override;
	virtual int CheckWhenAllDataLoaded() override;
public:
	//���ʱ���״̬(ֻ���ȫ���Ե�ʱ������)
	uint8_t ChkTimeState(const ChkFestParam& param, const FestTplCfg* tplcfg);
	//�Ƿ��ǵ���ķ���������
	bool IsDropServerType(int32_t serverType);
public:
	bool ProcessTemplate();
private:
	MapFestTplCfg m_festtpl;
};
