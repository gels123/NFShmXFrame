// -------------------------------------------------------------------------
//    @FileName         :    NFGrowDescEx.h
//    @Author           :    gaoyi
//    @Date             :    23-11-4
//    @Email			:    445267987@qq.com
//    @Module           :    NFGrowDescEx
//
// -------------------------------------------------------------------------

#pragma once

#include "NFServerComm/NFServerCommon/NFIDescStoreEx.h"
#include "NFServerComm/NFServerCommon/NFIDescTemplate.h"
#include "NFComm/NFShmCore/NFResDb.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmStl/NFShmHashMap.h"
#include "NFComm/NFShmStl/NFShmVector.h"
#include "NFLogicCommon/NFDescStoreTypeDefines.h"
#include "NFComm/NFShmStl/NFShmHashSet.h"
#include "DescStore/FashionFashionDesc.h"
#include "DescStore/HeadportraitHeadDesc.h"
#include "DescStore/DecorateDecorateDesc.h"
#include "DescStore/FootprintFootprintDesc.h"
#include "DescStore/MeditationMeditationDesc.h"
#include "DescStore/HaloHaloDesc.h"
#include "ComDefine.pb.h"
#include "Com.pb.h"
#include "NFGameCommon/NFComTypeDefine.h"

#define MAX_GROW_ACTIVE_ATTR_NUM 10

//����������Ϣ
struct GrowInfoCfg
{
    GrowInfoCfg()
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
    
    int CreateInit()
    {
        id = 0;
        type = 0;
        activationItem =0;
        activationNum = 0;
        startItem =0;
        starBar = 0;
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    int64_t id;						//ID
    int32_t type;					//����
    NFShmHashSet<uint32_t, proto_ff::ERoleProf_MAX> profLimits;			//ְҵ����
    int64_t activationItem;		//�������ID
    int32_t activationNum;		//�����������
    int32_t startItem;			//���ǵ���id
    NFShmHashMap<int32_t, int32_t, 100> lvMap;			//�������ĵ�������
    int32_t starBar;			//ÿ�������������ӵİٷֱ�ֵ
    NFShmHashMap<int32_t, int32_t, MAX_GROW_ACTIVE_ATTR_NUM> attrsMap;		//���Լӳ� key->value attrid->attrvalue
    NFShmHashMap<int32_t, int32_t, MAX_GROW_ACTIVE_ATTR_NUM> activeAttrMap;	//����ӳɵ�����
    bool LimitProf(int32_t prof);
    int32_t GetStarLvNum(int32_t lv);
};

typedef NFShmHashMap<int64_t, GrowInfoCfg, 100> GrowCfgMap;
typedef NFShmHashMap<int32_t, GrowCfgMap, proto_ff::GrowType_MAX> GrowTypeMap;

class NFGrowDescEx : public NFShmObjGlobalTemplate<NFGrowDescEx, EOT_CONST_GROW_DESC_EX_ID, NFIDescStoreEx>
{
public:
    NFGrowDescEx();
    
    virtual ~NFGrowDescEx();
    
    int CreateInit();
    int ResumeInit();
public:
    virtual int Load() override;
    virtual int CheckWhenAllDataLoaded() override;
public:
    GrowInfoCfg* GetGrowCfg(int64_t id);
private:
    bool Process();
    bool ProcessLvMap(MAP_INT32_INT32& lvMap, const std::string& cfg);
private:
    bool ProcessFashion();			//����ʱװ
    bool ProcessDecorate();			//��������
    bool ProcessFootPrint();		//�����㼣
    bool ProcessMeditation();		//�������
    bool ProcessHalo();				//����⻷
    bool ProcessHead();				//����ͷ��
    
    bool addId(int64_t id, int32_t type); //��ID����
private:
    GrowTypeMap		m_typeGrowMap;
    NFShmHashMap<int64_t, int32_t, MAX_FASHION_FASHION_NUM+MAX_HEADPORTRAIT_HEAD_NUM+MAX_DECORATE_DECORATE_NUM+MAX_FOOTPRINT_FOOTPRINT_NUM+MAX_MEDITATION_MEDITATION_NUM+MAX_HALO_HALO_NUM> m_id2typeMap;	//�������ñ�ID�����ظ�
};