#pragma once

#include "NFServerComm/NFServerCommon/NFIDescStoreEx.h"
#include "NFServerComm/NFServerCommon/NFIDescTemplate.h"
#include "NFComm/NFShmCore/NFResDb.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmStl/NFShmHashMap.h"
#include "NFComm/NFShmStl/NFShmVector.h"
#include "NFLogicCommon/NFDescStoreTypeDefines.h"
#include "NFLogicCommon/NFPackageDefine.h"
#include "NFGameCommon/NFComTypeDefine.h"
#include "Com.pb.h"
#include "E_Item_s.h"

class ItemDescEx : public NFShmObjGlobalTemplate<ItemDescEx, EOT_CONST_ITEM_DESC_EX_ID, NFIDescStoreEx>
{
    enum
    {
        VIR_ITEM_ID_MAX_OFFSET = 1000,	//������ƷID���ƫ�ƣ�������ƷID���þ�������һЩ
    };
public:
	ItemDescEx();
	virtual ~ItemDescEx();
	int CreateInit();
	int ResumeInit();
public:
	virtual int Load() override;
	virtual int CheckWhenAllDataLoaded() override;
public:
    const VEC_INT64 *GetItemFuncParamCfg(uint64_t nItemId);		//��ȡ��Ʒ�������ò���
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
    const MAP_UINT64_UINT64* GetItemFixAttr(uint64_t nItemId);
    //��ȡװ�������Ĳ���
    const MAP_UINT64_UINT64 *GetSmeltMaterialCfg(uint64_t equipid);
    //��ȡ��ҪУ��� ʹ����Ʒ�������
    const MAP_UINT64_INT64 *GetChkItemAddMission() { return &m_mapItemAddMissionCheck; }
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
    bool ProcessStoveExp();
    bool ProcessTSActive();
private:
    MAP_UINT64_VEC_INT64 m_mapItemFunc;					//��Ʒ���ܲ�������
    MAP_UINT64_SET_INT32 m_mapItemProfLimit;			//��Ʒ��ְҵ����
    MAP_UINT64_SET_INT64 m_mapItemMapLimit;				//��Ʒ��ͼ����
    MAP_UINT64_INT32  m_mapItemOrCdGroup;				//��Ʒ��CD�� ��cd
    SET_UINT64 m_setNaturalBind;						//��Ȼ�󶨵���Ʒ�������зǰ󶨵�״̬
    
    uint64_t m_virMinId = 0;							//������ƷID�б�����С����ƷID��1
    uint16_t m_virOffset[VIR_ITEM_ID_MAX_OFFSET+1];		//������ƷID���ƫ�ƣ� ��ƷID��Ӧ��ƫ�� = ��ƷID - m_virMinId ��
    uint16_t m_virAttr[proto_ff::A_COMMON_END+1];					//����ID��Ӧ��������ƷID��ƫ��
    
    MAP_UINT64_MAP_UINT64_UINT64 m_mapFixAttr;			//��Ʒ����������
    MAP_UINT64_SET_UINT64 m_mapDecompose;				//�ֽ���Ҫ������
    
    
    MAP_UINT64_MAP_UINT64_UINT64  m_mapEquipSmelt;		//װ����������
    MAP_UINT64_MAP_UINT64_UINT64  m_mapEquipStoveExp;   //��¯����
    
    MAP_UINT64_INT64  m_mapItemAddMissionCheck;			//ʹ����Ʒ���������ҪУ�����Ʒ��Ӧ������ID
    
    MAP_UINT64_SET_INT64 m_mapItemUseCond;				//��Ʒʹ������
    std::unordered_map<uint64_t, std::pair<uint64_t, int32_t>> m_replaceItem; //ID -> replace itemid , num
    MAP_UINT64_UINT32 m_ts_active_id;					//���񼤻���Ʒ��Ӧ����id
    MAP_UINT64_UINT32 m_dynexp_items;
};
