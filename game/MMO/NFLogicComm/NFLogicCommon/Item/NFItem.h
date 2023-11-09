// -------------------------------------------------------------------------
//    @FileName         :    NFItem.h
//    @Author           :    gaoyi
//    @Date             :    23-10-28
//    @Email			:    445267987@qq.com
//    @Module           :    NFItem
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFLogicCommon/NFLogicCommon.h"
#include "NFLogicCommon/NFLogicShmTypeDefines.h"
#include "NFComm/NFShmCore/NFISharedMemModule.h"
#include "NFComm/NFShmCore/NFShmObjTemplate.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmCore/NFShmObj.h"
#include "CommonClass_s.h"
#include "NFGameCommon/NFComTypeDefine.h"
#include "E_Equip_s.h"
#include "NFComm/NFShmStl/NFShmHashMap.h"
#include "NFComm/NFShmStl/NFShmHashSet.h"
#include "NFLogicCommon/NFPackageDefine.h"
#include "E_Item_s.h"
#include "E_Dragonsoul_s.h"
#include "E_Encyclopedia_s.h"

enum RandAttrType
{
    RandAttrType_rand = 0,    //���
    RandAttrType_min = 1,    //��Сֵ
    RandAttrType_max = 2    //���ֵ
};

//��������
struct stBlueAttr
{
    uint32_t id = 0;    //��������id
    int64_t value = 0;    //����ֵ
    int32_t lv_part = 0; //�ȼ��Σ�ÿ�����ӳɶ�������
};

//�Ǽ�����
struct stStarAttr
{
    uint32_t id = 0;
    int64_t value = 0;
};

using VEC_STAR_ATTR = std::vector<stStarAttr>;
using VEC_BLUE_ATTR = std::vector<stBlueAttr>;
using MAP_BLUE_ATTR = std::map<uint32_t, stBlueAttr>;

typedef NFShmHashMap<int32_t, int32_t, DEFINE_E_EQUIPATTRIBUTE_M_ATTRIBUTE_MAX_NUM> MiscAttrMap;

//EPackageType_DeityEquip = 5;	//����
struct DeityEquipExt
{
    DeityEquipExt()
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
        m_stronglv = 0;
        m_strongWearQuality = 0;
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    void Clear()
    {
        m_stronglv = 0;
        m_strongWearQuality = 0;
    }
    
    uint32_t m_stronglv; //ǿ���ȼ�
    uint32_t m_strongWearQuality; //ǿ���׼�
};

//EPackageType_BeastEquip = 7;	//����װ������
struct BeastEquipExt
{
    BeastEquipExt()
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
        m_stronglv = 0;
        m_strongExp = 0;
        m_strongWearQuality = 0;
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    void Clear()
    {
        m_stronglv = 0;
        m_strongExp = 0;
        m_strongWearQuality = 0;
    }
    
    uint32_t m_stronglv; //ǿ���ȼ�
    uint64_t m_strongExp; //ǿ���ȼ�����
    uint32_t m_strongWearQuality; //ǿ���׼�
    NFShmHashMap<uint32_t, uint32_t, DEFINE_E_ENCYCLOPEDIAEQUIPEXPVALUE_M_BEASTSTAR_MAX_NUM> m_blueAttr; //����װ����������
    NFShmHashMap<uint32_t, uint32_t, DEFINE_E_ENCYCLOPEDIAEQUIPEXPVALUE_M_GOLDSTAR_MAX_NUM> m_godAttr; //����װ���������� ����
};

//EPackageType_Longhun = 8;		//����
struct LongHunExt
{
    LongHunExt()
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
        m_stronglv = 0;
        m_strongExp = 0;
        m_strongWearQuality = 0;
        m_strongWearQualityExp = 0;
        m_awaken_lv = 0;
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    void Clear()
    {
        m_stronglv = 0;
        m_strongExp = 0;
        m_strongWearQuality = 0;
        m_strongWearQualityExp = 0;
        m_awaken_lv = 0;
    }
    
    uint32_t m_stronglv; //ǿ���ȼ�
    uint64_t m_strongExp; //ǿ���ȼ�����
    uint32_t m_strongWearQuality; //ǿ���׼�
    uint64_t m_strongWearQualityExp; //ǿ���׼�����
    uint32_t m_awaken_lv;   //���ѵȼ�
    NFShmHashSet<uint32_t, DEFINE_E_DRAGONSOULFLY_M_FLYATT_MAX_NUM> m_flyAttr; //�������������ID
};

//EPackageType_shenji_aq = 11;	//���װ������
//EPackageType_shenji_lj = 12;	//���װ�����
struct ShengjiExt
{
    ShengjiExt()
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
        m_makeid = 0;
        m_state = 0;
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    void Clear()
    {
        m_makeid = 0;
        m_state = 0;
    }
    
    uint32_t m_makeid;//����ID�����װ����
    uint32_t m_state;//���װ����0��δװ�� 1ͳ�� 2װ��
};

//EPackageType_GodEvil = 13;		//��ħ����
struct GodEvilExt
{
    GodEvilExt()
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
        m_stronglv = 0;
        m_strongExp = 0;
        m_speclv = 0;
        m_savvy = 0;
        m_strongWearQualityExp = 0;
        m_make_time = 0;
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    void Clear()
    {
        m_stronglv = 0;
        m_strongExp = 0;
        m_speclv = 0;
        m_savvy = 0;
        m_strongWearQualityExp = 0;
        m_make_time = 0;
    }
    
    uint32_t m_stronglv; //ǿ���ȼ�
    uint64_t m_strongExp; //ǿ���ȼ�����
    int32_t m_speclv;   //�����ȼ� װ����specAttr��lv�ֶΡ�
    int32_t m_savvy;        //��Ʒ����(��ħװ��)
    uint64_t m_strongWearQualityExp; //ǿ���׼�����
    uint64_t m_make_time;            //װ������ʱ��(��ħװ��)
    NFCommonStr m_make_name;        //װ������������(��ħװ��
    NFShmHashSet<uint32_t, 10> m_goldStar; //���� ����ID�б�
    NFShmHashSet<uint32_t, 10> m_silverStar; //���� ����ID�б�(��ħװ��)
    NFShmHashMap<uint32_t, uint32_t, 10> m_skillMap; //��ħװ���������� skillid - level
};

//EPackageType_star = 14;		//�����ǳ�
struct StarExt
{
    StarExt()
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
        m_stronglv = 0;
        m_strongExp = 0;
        m_strongWearQuality = 0;
        m_strongWearQualityExp = 0;
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    void Clear()
    {
        m_stronglv = 0;
        m_strongExp = 0;
        m_strongWearQuality = 0;
        m_strongWearQualityExp = 0;
    }
    
    uint32_t m_stronglv; //ǿ���ȼ�
    uint64_t m_strongExp; //ǿ���ȼ�����
    uint32_t m_strongWearQuality; //ǿ���׼�
    uint64_t m_strongWearQualityExp; //ǿ���׼�����
};

//EPackageType_turn = 15;		//ת��װ��
struct TurnExt
{
    TurnExt()
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
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    void Clear()
    {
    }
};

//EPackageType_MountKun = 16;		//������װ��
struct MountKunExt
{
    MountKunExt()
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
        m_stronglv = 0;
        m_strongWearQuality = 0;
        m_awaken_lv = 0;
        m_awaken_exp = 0;
        m_awaken_star = 0;
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    void Clear()
    {
        m_stronglv = 0;
        m_strongWearQuality = 0;
        m_awaken_lv = 0;
        m_awaken_exp = 0;
        m_awaken_star = 0;
    }
    
    uint32_t m_stronglv; //ǿ���ȼ�
    uint32_t m_strongWearQuality; //ǿ���׼�
    uint32_t m_awaken_lv;   //ͻ�Ƶȼ�
    uint32_t m_awaken_exp;   //ͻ�ƾ���
    uint64_t m_awaken_star; //ͻ������
};

//EPackageType_YaoHun = 17;	//����װ��
struct YaoHunExt
{
    YaoHunExt()
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
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    void Clear()
    {
    }
};

class NFItem
{
public:
    NFItem()
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
    
    virtual ~NFItem()
    {
    
    }
    
    int CreateInit()
    {
        m_nIndex = 0;              //����
        m_nItemID = 0;             //��ƷID
        m_nNum = 0;                //��Ʒ����
        m_byBind = 0;                //��״̬
        m_byType = 0;               //����
        m_nLevel = 0;              //�ȼ� Ԥ��(װ�������ȼ�����ҵȼ�)
        m_nExpiredTime = 0;        //����ʱ��,0,��������
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    virtual void Clear()
    {
        m_nIndex = 0;              //����
        m_nItemID = 0;             //��ƷID
        m_nNum = 0;                //��Ʒ����
        m_byBind = 0;                //��״̬
        m_byType = 0;               //����
        m_nLevel = 0;              //�ȼ� Ԥ��(װ�������ȼ�����ҵȼ�)
        m_nExpiredTime = 0;        //����ʱ��,0,��������
    }

public:
    uint16_t GetIndex() const { return m_nIndex; }
    
    uint64_t GetItemID() const { return m_nItemID; }
    
    uint64_t GetNum() const { return m_nNum; }
    
    int8_t GetBind() const { return m_byBind; }
    
    uint8_t GetType() const { return m_byType; }
    
    uint32_t GetLevel() const { return m_nLevel; }
    
    int64_t GetExpireTime() const { return m_nExpiredTime; }

public:
    bool IsExpire() const;
    
    void SetIndex(uint16_t nIndex) { m_nIndex = nIndex; }
    
    void SetType(uint8_t type) { m_byType = type; }
    
    void SetBind(int8_t byBind) { m_byBind = byBind; }
    
    void SetNum(uint64_t nNum) { m_nNum = nNum; }
    
    void SetLevel(uint32_t nLevel) { m_nLevel = nLevel; }
    
    void SetExpireTime(int64_t tick) { m_nExpiredTime = tick; }
    
    bool AddNum(int64_t nAddNum);
public:
    virtual bool Init(uint16_t nIndex, uint64_t nItemID, SItemCond &itemCond, uint64_t nNum = 1, int8_t byBind = (uint8_t) EBindState::EBindState_no);
    virtual void UnInit();
    virtual bool FromItemProto(const proto_ff::ItemProtoInfo &protoItem);
    virtual bool ToItemProto(proto_ff::ItemProtoInfo &protoItem);
    virtual bool SaveDB(proto_ff::ItemProtoInfo &protoItem);
    virtual void GetAllAttr(MAP_INT32_INT32 &attrs, int32_t level);
    virtual void CopyFrom(const NFItem &item);
    NFItem& operator=(const NFItem& item);
    
    virtual uint64_t GetBaseScore() { return 0; }
    
    virtual uint64_t GetItemFight(int32_t level);
public:
    const proto_ff_s::E_EquipEquip_s *GetEquipCfg() const;
    const proto_ff_s::E_ItemItem_s *GetItemCfg() const;
    const proto_ff_s::E_EquipAttribute_s *GetEquipAttributeCfg() const;
    bool IsProf(int32_t profId) const;                        //�Ƿ�����ĳ��ְҵ
protected:
    uint16_t m_nIndex;              //����
    uint64_t m_nItemID;             //��ƷID
    uint32_t m_nNum;                //��Ʒ����
    int8_t m_byBind;                //��״̬
    uint8_t m_byType;               //����
    uint32_t m_nLevel;              //�ȼ� Ԥ��(װ�������ȼ�����ҵȼ�)
    uint64_t m_nExpiredTime;        //����ʱ��,0,��������
    //����Ҳ�����֣����ڻ�����
};

class NFEquip : public NFItem
{
public:
    NFEquip();
    
    virtual ~NFEquip();
    
    int CreateInit();
    int ResumeInit();
    
    virtual void Clear()
    {
        NFItem::Clear();
        m_baseAttrPercent = 0;
        m_starAttrPercent = 0;
        m_godAttrPercent = 0;
    }

public:
    virtual MAP_INT32_INT32 GetBaseAttr();
    virtual VEC_STAR_ATTR GetStarAttr();
    virtual MAP_BLUE_ATTR GetBlueAttr();
    virtual MAP_INT32_INT32 GetGodAttr();
protected:
    bool genBaseAttr(SItemCond &itemCond);                //���ɻ�������
    bool genGodPinAttr(SItemCond &itemCond);            //������Ʒ����(������ �Ǽ����Ժ���������)
    bool genGodZunAttr(SItemCond &itemCond);            //������������
    void GenBaseScore();
public:
    virtual bool Init(uint16_t nIndex, uint64_t nItemID, SItemCond &itemCond, uint64_t nNum = 1, int8_t byBind = (uint8_t) EBindState::EBindState_no);
    virtual void UnInit();
    virtual bool FromItemProto(const proto_ff::ItemProtoInfo &protoItem);
    virtual bool ToItemProto(proto_ff::ItemProtoInfo &protoItem);
    virtual bool SaveDB(proto_ff::ItemProtoInfo &protoItem);
    virtual void GetAllAttr(MAP_INT32_INT32 &attrs, int32_t level);
    virtual void CopyFrom(const NFItem &item);
    virtual void CopyFrom(const NFEquip &equip);
    NFEquip& operator=(const NFItem& item);
    NFEquip& operator=(const NFEquip& item);
    
    virtual uint64_t GetBaseScore() { return 0; }
    
    virtual uint64_t GetItemFight(int32_t level);
protected:
    uint32_t m_baseAttrPercent;     //��������
    //��Ʒ���� = �Ǽ�����(����) + ��������(������)
    uint32_t m_starAttrPercent;     //�Ǽ�����
    uint32_t m_godAttrPercent;      //�������� (����װ����������)
};

class NFDeityEquip : public NFEquip
{
public:
    NFDeityEquip()
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
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    virtual void Clear()
    {
        NFItem::Clear();
        m_deityEquip.Clear();
    }
    
    virtual bool Init(uint16_t nIndex, uint64_t nItemID, SItemCond &itemCond, uint64_t nNum = 1, int8_t byBind = (uint8_t) EBindState::EBindState_no);
    virtual void UnInit();
    virtual bool FromItemProto(const proto_ff::ItemProtoInfo &protoItem);
    virtual bool ToItemProto(proto_ff::ItemProtoInfo &protoItem);
    virtual bool SaveDB(proto_ff::ItemProtoInfo &protoItem);
    virtual void GetAllAttr(MAP_INT32_INT32 &attrs, int32_t level);
    virtual void CopyFrom(const NFItem &item);
    virtual void CopyFrom(const NFEquip &equip);
    virtual void CopyFrom(const NFDeityEquip &equip);
    NFDeityEquip& operator=(const NFItem& item);
    NFDeityEquip& operator=(const NFEquip& item);
    NFDeityEquip& operator=(const NFDeityEquip& item);
    
    DeityEquipExt m_deityEquip;       //EPackageType_DeityEquip = 5;	//��������
};

class NFBeastEquip : public NFEquip
{
public:
    NFBeastEquip()
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
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    virtual void Clear()
    {
        NFItem::Clear();
        m_beastEquip.Clear();
    }
    
    virtual bool Init(uint16_t nIndex, uint64_t nItemID, SItemCond &itemCond, uint64_t nNum = 1, int8_t byBind = (uint8_t) EBindState::EBindState_no);
    virtual void UnInit();
    virtual bool FromItemProto(const proto_ff::ItemProtoInfo &protoItem);
    virtual bool ToItemProto(proto_ff::ItemProtoInfo &protoItem);
    virtual bool SaveDB(proto_ff::ItemProtoInfo &protoItem);
    virtual void GetAllAttr(MAP_INT32_INT32 &attrs, int32_t level);
    virtual void CopyFrom(const NFItem &item);
    virtual void CopyFrom(const NFEquip &equip);
    virtual void CopyFrom(const NFBeastEquip &equip);
    NFBeastEquip& operator=(const NFItem& item);
    NFBeastEquip& operator=(const NFEquip& item);
    NFBeastEquip& operator=(const NFBeastEquip& item);
    
    BeastEquipExt m_beastEquip;       //EPackageType_BeastEquip = 7;	//����װ������
};

class NFLongHunEquip : public NFEquip
{
public:
    NFLongHunEquip()
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
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    virtual void Clear()
    {
        NFItem::Clear();
        m_longHun.Clear();
    }
    
    virtual bool Init(uint16_t nIndex, uint64_t nItemID, SItemCond &itemCond, uint64_t nNum = 1, int8_t byBind = (uint8_t) EBindState::EBindState_no);
    virtual void UnInit();
    virtual bool FromItemProto(const proto_ff::ItemProtoInfo &protoItem);
    virtual bool ToItemProto(proto_ff::ItemProtoInfo &protoItem);
    virtual bool SaveDB(proto_ff::ItemProtoInfo &protoItem);
    virtual void GetAllAttr(MAP_INT32_INT32 &attrs, int32_t level);
    virtual void CopyFrom(const NFItem &item);
    virtual void CopyFrom(const NFEquip &equip);
    virtual void CopyFrom(const NFLongHunEquip &equip);
    NFLongHunEquip& operator=(const NFItem& item);
    NFLongHunEquip& operator=(const NFEquip& item);
    NFLongHunEquip& operator=(const NFLongHunEquip& item);
    
    LongHunExt m_longHun;             //EPackageType_Longhun = 8;		//����
};

//EPackageType_shenji_aq = 11;	//���װ������
//EPackageType_shenji_lj = 12;	//���װ�����
class NFShengjiEquip : public NFEquip
{
public:
    NFShengjiEquip()
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
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    virtual void Clear()
    {
        NFItem::Clear();
        m_shengji.Clear();
    }
    
    virtual bool Init(uint16_t nIndex, uint64_t nItemID, SItemCond &itemCond, uint64_t nNum = 1, int8_t byBind = (uint8_t) EBindState::EBindState_no);
    virtual void UnInit();
    virtual bool FromItemProto(const proto_ff::ItemProtoInfo &protoItem);
    virtual bool ToItemProto(proto_ff::ItemProtoInfo &protoItem);
    virtual bool SaveDB(proto_ff::ItemProtoInfo &protoItem);
    virtual void GetAllAttr(MAP_INT32_INT32 &attrs, int32_t level);
    virtual void CopyFrom(const NFItem &item);
    virtual void CopyFrom(const NFEquip &equip);
    virtual void CopyFrom(const NFShengjiEquip &equip);
    NFShengjiEquip& operator=(const NFItem& item);
    NFShengjiEquip& operator=(const NFEquip& item);
    NFShengjiEquip& operator=(const NFShengjiEquip& item);
    
    ShengjiExt m_shengji;
};

class NFGodEvilEquip : public NFEquip
{
public:
    NFGodEvilEquip()
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
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    virtual void Clear()
    {
        NFItem::Clear();
        m_godEvil.Clear();
    }
    
    virtual bool Init(uint16_t nIndex, uint64_t nItemID, SItemCond &itemCond, uint64_t nNum = 1, int8_t byBind = (uint8_t) EBindState::EBindState_no);
    virtual void UnInit();
    virtual bool FromItemProto(const proto_ff::ItemProtoInfo &protoItem);
    virtual bool ToItemProto(proto_ff::ItemProtoInfo &protoItem);
    virtual bool SaveDB(proto_ff::ItemProtoInfo &protoItem);
    virtual void GetAllAttr(MAP_INT32_INT32 &attrs, int32_t level);
    virtual void CopyFrom(const NFItem &item);
    virtual void CopyFrom(const NFEquip &equip);
    virtual void CopyFrom(const NFGodEvilEquip &equip);
    NFGodEvilEquip& operator=(const NFItem& item);
    NFGodEvilEquip& operator=(const NFEquip& item);
    NFGodEvilEquip& operator=(const NFGodEvilEquip& item);
    
    GodEvilExt m_godEvil;             //EPackageType_GodEvil = 13;		//��ħ����
};

class NFStarEquip : public NFEquip
{
public:
    NFStarEquip()
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
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    virtual void Clear()
    {
        NFItem::Clear();
        m_star.Clear();
    }
    
    virtual bool Init(uint16_t nIndex, uint64_t nItemID, SItemCond &itemCond, uint64_t nNum = 1, int8_t byBind = (uint8_t) EBindState::EBindState_no);
    virtual void UnInit();
    virtual bool FromItemProto(const proto_ff::ItemProtoInfo &protoItem);
    virtual bool ToItemProto(proto_ff::ItemProtoInfo &protoItem);
    virtual bool SaveDB(proto_ff::ItemProtoInfo &protoItem);
    virtual void GetAllAttr(MAP_INT32_INT32 &attrs, int32_t level);
    virtual void CopyFrom(const NFItem &item);
    virtual void CopyFrom(const NFEquip &equip);
    virtual void CopyFrom(const NFStarEquip &equip);
    NFStarEquip& operator=(const NFItem& item);
    NFStarEquip& operator=(const NFEquip& item);
    NFStarEquip& operator=(const NFStarEquip& item);
    
    StarExt m_star;                   //EPackageType_star = 14;		//�����ǳ�
};

class NFMountKunEquip : public NFEquip
{
public:
    NFMountKunEquip()
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
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    virtual void Clear()
    {
        NFItem::Clear();
        m_mountKun.Clear();
    }
    
    virtual bool Init(uint16_t nIndex, uint64_t nItemID, SItemCond &itemCond, uint64_t nNum = 1, int8_t byBind = (uint8_t) EBindState::EBindState_no);
    virtual void UnInit();
    virtual bool FromItemProto(const proto_ff::ItemProtoInfo &protoItem);
    virtual bool ToItemProto(proto_ff::ItemProtoInfo &protoItem);
    virtual bool SaveDB(proto_ff::ItemProtoInfo &protoItem);
    virtual void GetAllAttr(MAP_INT32_INT32 &attrs, int32_t level);
    virtual void CopyFrom(const NFItem &item);
    virtual void CopyFrom(const NFEquip &equip);
    virtual void CopyFrom(const NFMountKunEquip &equip);
    NFMountKunEquip& operator=(const NFItem& item);
    NFMountKunEquip& operator=(const NFEquip& item);
    NFMountKunEquip& operator=(const NFMountKunEquip& item);
    
    MountKunExt m_mountKun;           //EPackageType_MountKun = 16;		//������װ��
};

class NFTurnEquip : public NFEquip
{
public:
    NFTurnEquip()
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
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    virtual void Clear()
    {
        NFItem::Clear();
        m_turn.Clear();
    }
    
    virtual bool Init(uint16_t nIndex, uint64_t nItemID, SItemCond &itemCond, uint64_t nNum = 1, int8_t byBind = (uint8_t) EBindState::EBindState_no);
    virtual void UnInit();
    virtual bool FromItemProto(const proto_ff::ItemProtoInfo &protoItem);
    virtual bool ToItemProto(proto_ff::ItemProtoInfo &protoItem);
    virtual bool SaveDB(proto_ff::ItemProtoInfo &protoItem);
    virtual void GetAllAttr(MAP_INT32_INT32 &attrs, int32_t level);
    virtual void CopyFrom(const NFItem &item);
    virtual void CopyFrom(const NFEquip &equip);
    virtual void CopyFrom(const NFTurnEquip &equip);
    NFTurnEquip& operator=(const NFItem& item);
    NFTurnEquip& operator=(const NFEquip& item);
    NFTurnEquip& operator=(const NFTurnEquip& item);
    
    TurnExt m_turn;                   //EPackageType_turn = 15;		//ת��װ��
};

class NFYaoHunEquip : public NFEquip
{
public:
    NFYaoHunEquip()
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
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
    
    virtual void Clear()
    {
        NFItem::Clear();
        m_yaoHun.Clear();
    }
    
    virtual bool Init(uint16_t nIndex, uint64_t nItemID, SItemCond &itemCond, uint64_t nNum = 1, int8_t byBind = (uint8_t) EBindState::EBindState_no);
    virtual void UnInit();
    virtual bool FromItemProto(const proto_ff::ItemProtoInfo &protoItem);
    virtual bool ToItemProto(proto_ff::ItemProtoInfo &protoItem);
    virtual bool SaveDB(proto_ff::ItemProtoInfo &protoItem);
    virtual void GetAllAttr(MAP_INT32_INT32 &attrs, int32_t level);
    virtual void CopyFrom(const NFItem &item);
    virtual void CopyFrom(const NFEquip &equip);
    virtual void CopyFrom(const NFYaoHunEquip &equip);
    NFYaoHunEquip& operator=(const NFItem& item);
    NFYaoHunEquip& operator=(const NFEquip& item);
    NFYaoHunEquip& operator=(const NFYaoHunEquip& item);
    
    YaoHunExt m_yaoHun;               //EPackageType_YaoHun = 17;	//����װ��
};

//������Ʒ
typedef vector<NFItem *> VEC_PACKAGE_ITEM;

//proto��Ʒ
typedef vector<proto_ff::ItemProtoInfo *> VEC_ITEM_PROTO;
typedef vector<proto_ff::ItemProtoInfo> VEC_ITEM_PROTO_EX;
typedef map<uint16_t, proto_ff::ItemProtoInfo> MAP_ITEM_PROTO_EX;