// -------------------------------------------------------------------------
//    @FileName         :    NFItemMgr.h
//    @Author           :    gaoyi
//    @Date             :    23-10-31
//    @Email			:    445267987@qq.com
//    @Module           :    NFItemMgr
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFLogicCommon/NFLogicShmTypeDefines.h"
#include "NFComm/NFShmCore/NFShmObjTemplate.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmCore/NFShmObj.h"
#include "NFLogicCommon/NFPackageDefine.h"
#include "NFGameCommon/NFComTypeDefine.h"
#include "NFItem.h"

class NFItemMgr
{
public:
    //��������Ʒ����
    static NFItem *MakeItemObj(uint64_t nItemId);
    //�ͷ���Ʒ����
    static bool FreeItemObj(NFItem *pItemObj);
public:
    //������Ʒ
    static bool CreateItem(const LIST_ITEM &lstItem, VEC_ITEM_PROTO_EX &vecProtoEx, SItemCond &itemCond, bool bMerge = true);
    //������Ʒ(�����õģ������ط���Ҫ��)
    static bool CreateItem(const VEC_ITEM_PROTO &vecProtoItems, VEC_PACKAGE_ITEM &vecItems);
    //������Ʒ(�����õģ������ط���Ҫ��)
    static bool CreateItem(const VEC_ITEM_PROTO_EX &vecProtoItemsEx, VEC_PACKAGE_ITEM &vecItems);
public:
    ///////////////////////////////// ��Դ�� ������Ʒ �ӿ� /////////////////////////////////////////
    //�Ƿ�����Դ�� ������Ʒ
    static bool IsVirItem(uint64_t itemId);
    //�Ƿ�����Դ�� ������Ʒ, ����ǣ��� attrId ������Ϊ ������Ʒ��Ӧ������ID
    static bool IsVirItem(uint64_t itemId, uint32_t &attrId);
    //���˳�������Ʒ������
    static void FilterVirItem(const LIST_ITEM &lstItem, LIST_ITEM &lstOutItem, MAP_UINT32_INT64 &mapAttr);
    //���˳�������Ʒ������
    static void FilterVirItem(const LIST_ITEM &lstItem, LIST_ITEM &lstOutItem);
    //���˳�������Ʒ������
    static void FilterVirItem(const VEC_ITEM_PROTO_EX &vecProtoItemsEx, VEC_ITEM_PROTO_EX &vecOutProtoItemsEx, MAP_UINT32_INT64 &mapAttr);
    //���˳�������Ʒ������
    static void FilterVirItem(const VEC_ITEM_PROTO_EX &vecProtoItemsEx, VEC_ITEM_PROTO_EX &vecOutProtoItemsEx);
    static bool IsDynExpItem(uint64_t itemId);
public:
    /////////////////////////////////////////////////////////////////////////////////////
    //������Ʒprotobuf��Ϣ
    static bool SetItemProtoInfo(NFItem *pItem, proto_ff::ItemProtoInfo *protoItem, int64_t nNum = -1);
    //��Ʒ�İ�״̬-���ݻ��;�� bindWay: EItemBindWay ö������
    static int8_t BindStateByWay(uint64_t nItemId, int8_t byBind, int8_t bindWay = EItemBindWay_UnBind);
    static int8_t BindStateByWay(const proto_ff_s::E_ItemItem_s*pItemCfg, int8_t byBind, int8_t bindWay = EItemBindWay_UnBind);
    static int8_t BindStateByWay(const proto_ff_s::E_EquipEquip_s*pEquipCfg, int8_t byBind, int8_t bindWay = EItemBindWay_UnBind);
    //��ȡ��Ʒ�ѵ���
    static bool ItemMaxPile(uint64_t itemId, int64_t& maxPile);
    static int64_t ItemMaxPile(const proto_ff_s::E_ItemItem_s *pItemCfg);
    static int64_t ItemMaxPile(const proto_ff_s::E_EquipEquip_s *pEquipCfg);
    //�Ƿ���Ч�İ�;��
    static bool ValidBindWay(int8_t bindWay);
    //�Ƿ���Ч�Ľ�ֹ����
    static bool ValidForbidType(int8_t forbidType);
    //�Ƿ���Ч����Ʒ���Գ�ʼ������
    static bool ValidInitAttrType(int8_t initAttrType);
    //��Ʒ�Ƿ��ܽ���
    static bool CanTrade(uint64_t itemId,int8_t byInBind);
    //��Ʒ�ܷ����
    static bool CanSell(uint64_t itemId,int8_t byInBind);
    //�Ƿ���������Ʒ
    static bool IsTaskItem(uint64_t itemId);
    //�ϲ���Ʒ
    static void MergeItem(const LIST_ITEM &lstItem, LIST_ITEM &lstOutItem);
    //������Ʒ��Ҫ�ĸ�������-ֻ�ǳ��Թ���������ɶѵ���������һ����ŵ�ֵ, ���ɶѵ�����Ʒռ�õĸ��������ǿ��Լ��������
    static uint32_t CalcGridNeed(const LIST_ITEM &lstItem, const VEC_ITEM_PROTO_EX &vecProtoEx,uint32_t &nNoPileNeed, uint32_t &nPileNeed);
    //�����Ʒ���ͣ������ͣ����ѵ���Ŀ
    static bool GetItemData(uint32_t itemId, uint8_t bindType, uint8_t& outItemType, uint8_t& outBindType, int64_t& maxPile);
public:
    ////////////////////////////////// ����ս��  /////////////////////////////////
    static uint64_t GetItemFight(NFItem *pItem, int32_t level);
public:
    //������Ʒ
    static bool CreateItem(const LIST_ITEM &lstItem, VEC_PACKAGE_ITEM &vecItems, SItemCond &itemCond, bool bMerge = true);
    //У����Ʒ
    static bool CheckItem(const VEC_ITEM_PROTO_EX &vecProtoItems);
};