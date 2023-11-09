#pragma once

#include "NFServerComm/NFServerCommon/NFIDescStoreEx.h"
#include "NFServerComm/NFServerCommon/NFIDescTemplate.h"
#include "NFComm/NFShmCore/NFResDb.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmStl/NFShmHashMap.h"
#include "NFComm/NFShmStl/NFShmVector.h"
#include "NFLogicCommon/NFDescStoreTypeDefines.h"
#include "E_Equip_s.h"
#include "DescStore/EquipStrongDesc.h"
#include "DescStore/EquipBeaststrongDesc.h"
#include "Com_s.h"
#include "NFLogicCommon/NFPackageDefine.h"
#include "DescStore/EquipStrongexpDesc.h"
#include "DescStore/EquipimproveUpqualityDesc.h"
#include "NFGameCommon/NFComTypeDefine.h"
#include "DescStore/EquipBreakDesc.h"

#define EQUIP_STRONG_ID(P, W) ((P)*10000+W)

//ǿ���ӳ�����
struct stEquipStrongAttrCfg
{
    stEquipStrongAttrCfg()
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
        attrId = 0;
        param0 = 0;
        param1 = 0;
        param2 = 0;
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    int32_t attrId;
    float param0;
    float param1;
    float param2;
};

struct stEquipStrongCfg
{
    stEquipStrongCfg()
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
        cfg_id = 0;
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    const proto_ff_s::E_EquipStrong_s *GetConfig()
    {
        return EquipStrongDesc::Instance()->GetDesc(cfg_id);
    }
    
    uint64_t cfg_id;
    NFShmVector<stEquipStrongAttrCfg, DEFINE_E_EQUIPSTRONG_M_TYPE_MAX_NUM> attr;
};

struct stEquipBeastStrongCfg
{
    stEquipBeastStrongCfg()
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
        cfg_id = 0;
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    const proto_ff_s::E_EquipBeaststrong_s* GetConfig()
    {
        return EquipBeaststrongDesc::Instance()->GetDesc(cfg_id);
    }
    
    uint64_t cfg_id;
    std::vector<stEquipStrongAttrCfg> attr;
};

//ǿ����������
struct stEquipLvCfg
{
    stEquipLvCfg()
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
        cfg_id = 0;
        costGold = 0;
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    const proto_ff_s::E_EquipStrongexp_s* GetConfig()
    {
        return EquipStrongexpDesc::Instance()->GetDesc(cfg_id);
    }
    
    uint64_t cfg_id;
    int32_t costGold;
    NFShmHashMap<int32_t, proto_ff_s::ComPair64_s, EEquipType_max> m_costItem;
};

//��ʯ����
struct stGemLvCfg
{
    stGemLvCfg()
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
        type = 0;
        gemLv = 0;
        lv1ItemID = 0;
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    int32_t type;
    int32_t gemLv;
    int64_t lv1ItemID;
};

//��Ʒ��Ϣ
struct stUpQualityInfo
{
    stUpQualityInfo()
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
        pos = 0;
        maxLv = 0;
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    int32_t pos;
    int32_t maxLv;
    NFShmHashMap<int32_t, int64_t, MAX_EQUIPIMPROVE_UPQUALITY_NUM> m_cfg; //star�Ǽ�->����
};
//��Ʒ��Ϣ����
using EquipUpQualityMap = NFShmHashMap<int32_t, stUpQualityInfo, 10>;

#define MAX_EQUIP_BREAK_POSITION 4

class EquipDescEx : public NFShmObjGlobalTemplate<EquipDescEx, EOT_CONST_EQUIP_DESC_EX_ID, NFIDescStoreEx>
{
public:
    EquipDescEx();
    virtual ~EquipDescEx();
    int CreateInit();
    int ResumeInit();
public:
    virtual int Load() override;
    virtual int CheckWhenAllDataLoaded() override;
public:
    int ProcessStrong();
    int ProcessStrongLv();
    int ProcessBreak();
public:
    stEquipStrongCfg* GetStrongCfg(int64_t id);
    stEquipLvCfg* GetStrongLvCfg(int32_t lv);
    stEquipBeastStrongCfg* GetBeastStrongCfg(int64_t id);
    const proto_ff_s::E_EquipBreak_s* GetBreakCfgInfo(int32_t pos, int32_t step);
private:
    int32_t maxStrongLv;
    NFShmHashMap<int64_t, stEquipStrongCfg, MAX_EQUIP_STRONG_NUM> m_strong;		//ǿ����Ϣ key
    NFShmHashMap<int64_t, stEquipBeastStrongCfg, MAX_EQUIP_BEASTSTRONG_NUM> m_strongBeast;		//ǿ����Ϣ key
    NFShmHashMap<int32_t, stEquipLvCfg, MAX_EQUIP_STRONGEXP_NUM> m_stronglv;			//ǿ���ȼ����� lv->
    NFShmHashMap<int32_t, NFShmHashMap<int32_t, int64_t, MAX_EQUIP_BREAK_NUM>, MAX_EQUIP_BREAK_POSITION> m_breakStrong; //break����
    std::map<uint64_t, MAP_INT32_INT32> m_specAttr;
};
