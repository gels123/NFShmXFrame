#pragma once

#include "NFServerComm/NFServerCommon/NFIDescStoreEx.h"
#include "NFServerComm/NFServerCommon/NFIDescTemplate.h"
#include "NFComm/NFShmCore/NFResDb.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmStl/NFShmHashMap.h"
#include "NFComm/NFShmStl/NFShmVector.h"
#include "NFLogicCommon/NFDescStoreTypeDefines.h"
#include "NFGameCommon/NFComTypeDefine.h"
#include "E_Skill_s.h"
#include "DescStore/SkillSkillDesc.h"
#include "NFLogicCommon/NFSkillDefine.h"

//��������
struct SkillCfg
{
    SkillCfg()
    {
    
    }
    
    const proto_ff_s::E_SkillSkill_s* GetCfg() const
    {
        return SkillSkillDesc::Instance()->GetDesc(m_skillId);
    }
    
    uint64_t m_skillId;
    VEC_INT32 vec_unlocktype;                    //��������
    VEC_INT32 vec_unlockvalue;                    //����ֵ
    
    int32_t rangeValue1 = 0;                    //��Χֵ1
    int32_t rangeValue2 = 0;                    //��Χֵ2
    int32_t rangeValue3 = 0;                    //��Χֵ3
    int32_t rangeValueEx1 = 0;                    //��չ��Χֵ1
    int32_t rangeValueEx2 = 0;                    //��չ��Χֵ2
    int32_t rangeValueEx3 = 0;                    //��չ��Χֵ3
    
    VEC_INT64 vecRelation;                        //��������
    int64_t preRelation = 0;                    //ǰ�ù�������ID
    VEC_UINT32 vecDamage;                        //�˺�ʱ���
    VEC_UINT32 vecDamageEx;                        //��չ�˺�ʱ���
};

class SkillDescEx : public NFShmObjGlobalTemplate<SkillDescEx, EOT_CONST_SKILL_DESC_EX_ID, NFIDescStoreEx>
{
public:
    using MapSkillCfg = std::unordered_map<uint64_t, SkillCfg>;
    using MapSkillUnlock = std::map<int32_t, std::unordered_set<int64_t> >;
public:
    SkillDescEx();
    
    virtual ~SkillDescEx();
    
    int CreateInit();
    
    int ResumeInit();

public:
    virtual int Load() override;
    
    virtual int CheckWhenAllDataLoaded() override;

public:
    bool Process();

public:
    const SkillCfg* GetCfg(uint64_t skillid);
    //�Ƿ�����Ч�ļ��ܽ�������
    bool ValidUnlockType(int32_t type);
    //�Ƿ��Ǳ�������
    bool IsPassive(uint64_t skillid);
    //�Ƿ��Ǳ�������
    bool IsPassive(const SkillCfg* pcfg);
    //�Ƿ�����Ч�ļ��ܷ�Χ����
    bool ValidRangeType(int32_t rangetype);
    //�Ƿ�������Ŀ��ķ�Χ����
    bool IsLockRangeType(int32_t rangetype);
    //�Ƿ�����ҪĿ���ļ���
    bool IsPosRangeType(int32_t rangetype);
    
    //�������ְҵ��ȡְҵ�����б�
    const SET_UINT64* GetProfSkillList(int8_t prof);
    
    //��ȡְҵ�ļ�����ID
    uint32_t GetProfSkillGroupId(int8_t prof);
    
    //�Ƿ���������
    bool IsGodSkill(uint64_t skillid);
    
    bool IsGodSkill(const SkillCfg* pcfg);
    //�Ƿ�����������
    bool IsGodGroup(uint32_t groupid);
    //�Ƿ��ǽ�ɫ����
    bool IsRoleSkill(uint64_t skillid);
    
    bool IsRoleSkill(const SkillCfg* pcfg);
    //�Ƿ��ǳ��＼��
    bool IsPetSkill(uint64_t skillid);
    
    bool IsPetSkill(const SkillCfg* pcfg);
    //�Ƿ���ս�꼼��
    bool IsSpiritSkill(uint64_t skillid);
    
    bool IsSpiritSkill(const SkillCfg* pcfg);
    
    //ŭ��������ID
    uint32_t GetAngerSkillGroupId();
    //�Ƿ���ŭ������
    bool IsAngerSkill(uint64_t skillid);
    //�Ƿ���ŭ������
    bool IsAngerSkill(const SkillCfg* pcfg);
    //�Ƿ�����ͨ����������
    bool IsNorContiSkill(const SkillCfg* pcfg);
    
    //ŭ������ID
    uint64_t GetAngerSkillId() { return m_angerSkillId; }
    
    //��ȡ���＼����ID
    uint32_t GetPetSkillGroupId();
    
    //��ȡս�꼼����ID
    uint32_t GetSpiritSkillGroupId();
    
    //��ȡ��������ID
    uint32_t GetGodSkillGroupId();
    //�Ƿ�����Ҫ���㹫��CD�ļ���
    bool IsPubCdSkill(uint64_t skillid);
    
    bool IsPubCdSkill(const SkillCfg* pcfg);
    
    //��ȡ����cd
    int64_t GetPubCd(const SkillCfg* pcfg);
    //�Ƿ� �������ȹ�����Ŀ��
    bool IsPriorityTarget(uint8_t srckind, uint8_t dstkind);
    //�Ƿ�����Ч����Ҽ���λ��
    bool ValidRoleSkillPos(uint32_t pos) { return pos >= 0 && pos < MAX_ROLE_SKILL_POS; }
    //�Ƿ�����Ч��������λ��
    bool ValidGodSkillPos(uint32_t pos) { return pos >= 0 && pos < MAX_ROLE_SKILL_POS; }
    //�Ƿ�����Ч�ĳ��＼��λ��
    bool ValidPetSkillPos(uint32_t pos) { return pos >= 1 && pos < MAX_PET_SKILL_POS; }
    //�Ƿ�����Ч��ս�꼼��λ��
    bool ValidSpiritSkillPos(uint32_t pos) { return pos >= 1 && pos < MAX_SPIRIT_SKILL_POS; }
    //�Ƿ�����Ч��ŭ������λ��
    bool ValidAngerSkillPos(uint32_t pos) { return pos >= 1 && pos < MAX_ANGER_SKILL_POS; }
    
    //��ȡ����ս��
    int64_t GetSkillFight(uint64_t skillid, int32_t level);
    
    int64_t GetSkillFight(const SkillCfg* pcfg, int32_t level);
    //�Ƿ���Ҫ������ױ��ʵļ���
    bool IsAdvanceRateSkill(const SkillCfg* pcfg);
    //�Ƿ���ͨ�����׽����ļ���
    bool IsUnlockByAdv(uint64_t skillid);
    
    bool IsUnlockByAdv(const SkillCfg* pcfg);
    
    bool IsUnlockByAdv(int32_t adv, const SkillCfg* pcfg);
    
    //��ȡ���ܽ�������
    void GetSkillUnlockParam(const SkillCfg* pcfg, int32_t& type, int64_t& param1, int64_t& param2);
    //�Ƿ��Ƿ������꼼��
    bool IsMagWeaponSoulSkill(const SkillCfg* pcfg);
    //�Ƿ����������꼼��
    bool IsCapSoulSkill(const SkillCfg* pcfg);
    //�Ƿ���ս����������
    bool IsSoulSacrifice(uint64_t skillid);
    
    //��ȡս����������ID
    int64_t GetSacrificeSkillId();

private:
    MapSkillCfg m_cfg;                                    //��������
    int32_t m_maxPowerDiff = 0;                            //���ս����
    MAP_UINT32_SET_UINT64 m_mapGroup;                    //������-����ID�б�
    MAP_UINT64_UINT32 m_mapUpgradeGroup;                //����ID������������֮���ӳ��
    uint64_t m_angerSkillId = 0;                        //ŭ������ID
};
