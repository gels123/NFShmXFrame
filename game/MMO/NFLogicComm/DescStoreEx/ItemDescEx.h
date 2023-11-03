#pragma once

#include "NFServerComm/NFServerCommon/NFIDescStoreEx.h"
#include "NFServerComm/NFServerCommon/NFIDescTemplate.h"
#include "NFComm/NFShmCore/NFResDb.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmStl/NFShmHashMap.h"
#include "NFComm/NFShmStl/NFShmHashSet.h"
#include "NFComm/NFShmStl/NFShmVector.h"
#include "NFLogicCommon/NFDescStoreTypeDefines.h"
#include "NFLogicCommon/NFPackageDefine.h"
#include "NFGameCommon/NFComTypeDefine.h"
#include "Com.pb.h"
#include "DescStore/ItemItemDesc.h"
#include "DescStore/EquipEquipDesc.h"
#include "DescStore/AvatarChangeDesc.h"
#include "E_Condition_s.h"

#define MAX_NATURAL_BIND_ITEM_NUM (MAX_ITEM_ITEM_NUM/10)
#define MAX_MAP_LIMIT_ITEM_NUM (MAX_ITEM_ITEM_NUM/50)
#define MAX_MAP_LIMIT_ITEM_MAP_NUM 5
#define MAX_PROF_LIMIT_ITEM_NUM (MAX_EQUIP_EQUIP_NUM)
#define MAX_PROF_LIMIT_ITEM_MAP_NUM 10
#define MAX_ITEM_FUNC_PARAM_NUM 10
#define MAX_REPLACE_ITEM_NUM 100

#define MAX_DECOMPOSE_ITEM_KEY_NUM MAX_EQUIP_EQUIP_NUM*2
#define MAX_DECOMPOSE_ITEM_ONE_KEY_EQUIP_NUM 10

#define MAX_EQUIP_SMELT_EQUIP_NUM 100
#define MAX_EQUIP_SMELT_EQUIP_MAX_ITEM_NUM 2

#define MAX_DY_EXP_ITEM_NUM 100

//װ���ּ�   �Ǽ����׼���Ʒ�ʡ�ְҵ����λ  ��ϳ�һ��key��key��Ӧ���б�����������key������װ��ID
#define DE_COMPOSE_KEY(star,rank,qua,prof,pos) ((star << 32) | (rank << 24)| (qua << 16) | (prof << 8) | pos)

class ItemDescEx : public NFShmObjGlobalTemplate<ItemDescEx, EOT_CONST_ITEM_DESC_EX_ID, NFIDescStoreEx>
{
    enum
    {
        VIR_ITEM_ID_MAX_OFFSET = 1000,	//������ƷID���ƫ�ƣ�������ƷID���þ�������һЩ
    };
    
    typedef NFShmHashSet<uint64_t, MAX_NATURAL_BIND_ITEM_NUM> NaturalBindSet;
    typedef NFShmHashMap<uint64_t, NFShmHashSet<int64_t, MAX_MAP_LIMIT_ITEM_MAP_NUM>, MAX_MAP_LIMIT_ITEM_NUM> ItemMapLimitMap;
    typedef NFShmHashMap<uint64_t, NFShmHashSet<int32_t, MAX_PROF_LIMIT_ITEM_MAP_NUM>, MAX_PROF_LIMIT_ITEM_NUM> ItemProfLimitMap;
    typedef NFShmHashMap<uint64_t, NFShmVector<int64_t, MAX_ITEM_FUNC_PARAM_NUM>, DEFINE_SHEET_ITEMITEM_E_ITEMITEM_LIST_MAX_NUM> ItemFuncMap;
    typedef NFShmHashMap<uint64_t, NFShmPair<uint32_t, uint32_t>, MAX_REPLACE_ITEM_NUM> ItemReplaceMap;
    typedef NFShmHashMap<uint64_t, NFShmHashSet<uint64_t, MAX_DECOMPOSE_ITEM_ONE_KEY_EQUIP_NUM>, MAX_DECOMPOSE_ITEM_KEY_NUM> ItemDecomposeMap;
    typedef NFShmHashMap<uint64_t, NFShmHashMap<uint64_t, uint64_t, MAX_EQUIP_SMELT_EQUIP_MAX_ITEM_NUM>, MAX_EQUIP_SMELT_EQUIP_NUM> EqupSmeltMap;
    typedef NFShmHashMap<uint64_t, uint64_t, MAX_AVATAR_CHANGE_NUM> TsActiveMap;
    typedef NFShmHashMap<uint64_t, uint32_t, MAX_DY_EXP_ITEM_NUM> DyExpItemMap;
    typedef NFShmHashMap<uint64_t, NFShmHashSet<int64_t, 5>, 100> ItemUseConditionMap;
    typedef NFShmHashMap<uint64_t, uint64_t, 100> ItemAddMissionCheckMap;
    typedef NFShmHashMap<uint64_t, NFShmHashMap<uint64_t, uint64_t, 10>, 100> FixAttrMap;
public:
	ItemDescEx();
	virtual ~ItemDescEx();
	int CreateInit();
	int ResumeInit();
public:
	virtual int Load() override;
	virtual int CheckWhenAllDataLoaded() override;
public:
    const NFShmVector<int64_t, MAX_ITEM_FUNC_PARAM_NUM> *GetItemFuncParamCfg(uint64_t nItemId);		//��ȡ��Ʒ�������ò���
    int32_t GetItemOrGroupCd(uint64_t itemId_cdGroup);			//��ȡ��ƷCD
    bool IsNautralBind(uint64_t nItemId);						//�Ƿ���Ȼ����Ʒ
    bool IsMatchProf(uint64_t itemid, uint8_t prof);			//ְҵ�Ƿ�ƥ����Ʒ��ְҵ����
    bool IsNotAllowMap(uint64_t itemid, uint64_t mapid);		//�Ƿ��ǲ�����ʹ�õĵ�ͼ
    //���� �Ǽ����׼���Ʒ�ʡ�ְҵ����λ ���һ��װ��
    uint64_t RandDeComposeEquip(int64_t star, int64_t rank, int64_t qua, int64_t prof, int64_t pos);
    //������ƷID��ȡ��Ӧ������ID
    uint32_t AttrIdByItem(uint64_t itemId);
    //��������ID ��ȡ��Ӧ����ƷID
    uint64_t ItemIdByAttrId(uint32_t attrId);
    const NFShmHashMap<uint64_t, uint64_t, 10>* GetItemFixAttr(uint64_t nItemId);
    //��ȡװ�������Ĳ���
    const NFShmHashMap<uint64_t, uint64_t, MAX_EQUIP_SMELT_EQUIP_MAX_ITEM_NUM> *GetSmeltMaterialCfg(uint64_t equipid);
    //��ȡ��ҪУ��� ʹ����Ʒ�������
    const ItemAddMissionCheckMap *GetChkItemAddMission() { return &m_mapItemAddMissionCheck; }
    //У���Ƿ�������Ʒʹ������
    bool CheckItemUseCond(uint64_t itemid,proto_ff::StatisticDataProto &proto);
    int32_t GetEquipStoveExp(int32_t wearQuality, int32_t quality);
    bool replaceItem(uint64_t itemId, int32_t num, uint64_t& outItemId, int32_t& outNum);
    bool IsTianShenActiveNum(const proto_ff_s::E_ItemItem_s* itemCfg);
    int32_t GetTianShenId(uint64_t itemId);
    bool IsDynExpItem(uint64_t itemId);
    int32_t GetDynExpType(uint64_t itemId);
private:
    bool Process();
    bool ProcessTSActive();
    //�Ƿ�������Ʒʹ������
    bool CheckItemUseCond(const proto_ff_s::E_ConditionCondition_s *pCondCfg, proto_ff::StatisticDataProto &proto);
private:
    ItemFuncMap m_mapItemFunc;					//��Ʒ���ܲ�������
    ItemProfLimitMap m_mapItemProfLimit;			//��Ʒ��ְҵ����
    ItemMapLimitMap m_mapItemMapLimit;				//��Ʒ��ͼ����
    MAP_UINT64_INT32  m_mapItemOrCdGroup;				//��Ʒ��CD�� ��cd
    NaturalBindSet m_setNaturalBind;						//��Ȼ�󶨵���Ʒ�������зǰ󶨵�״̬
    
    uint64_t m_virMinId;							//������ƷID�б�����С����ƷID��1
    uint16_t m_virOffset[VIR_ITEM_ID_MAX_OFFSET+1]{};		//������ƷID���ƫ�ƣ� ��ƷID��Ӧ��ƫ�� = ��ƷID - m_virMinId ��
    uint16_t m_virAttr[proto_ff::A_COMMON_END+1]{};		//����ID��Ӧ��������ƷID��ƫ��
    
    FixAttrMap m_mapFixAttr;			//��Ʒ����������
    ItemDecomposeMap m_mapDecompose;				//�ֽ���Ҫ������
    
    EqupSmeltMap  m_mapEquipSmelt;		//װ����������
    
    ItemAddMissionCheckMap  m_mapItemAddMissionCheck;			//ʹ����Ʒ���������ҪУ�����Ʒ��Ӧ������ID
    
    ItemUseConditionMap m_mapItemUseCond;				//��Ʒʹ������
    ItemReplaceMap m_replaceItem; //ID -> replace itemid , num
    TsActiveMap m_ts_active_id;					//���񼤻���Ʒ��Ӧ����id
    DyExpItemMap m_dynexp_items;
};
