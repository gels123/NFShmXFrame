// -------------------------------------------------------------------------
//    @FileName         :    NFPackageBag.h
//    @Author           :    gaoyi
//    @Date             :    23-11-3
//    @Email			:    445267987@qq.com
//    @Module           :    NFPackageBag
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFLogicCommon/NFLogicShmTypeDefines.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmCore/NFRawShmObj.h"
#include "NFComm/NFShmCore/NFShmObjTemplate.h"
#include "NFLogicCommon/NFPackageDefine.h"
#include "NFComm/NFShmStl/NFShmVector.h"
#include "NFGameCommon/NFComTypeDefine.h"
#include "NFLogicCommon/Item/NFItem.h"
#include "NFLogicCommon/Item/NFItemMgr.h"
#include "Package.pb.h"
#include "NFLogicCommon/NFLogDefine.h"
#include "DescStoreEx/ItemDescEx.h"
#include "Facade/NFDeityPart.h"

class NFPlayer;

class NFPackageBag : public NFRawShmObj
{
public:
    //��չ��Ʒ�ṹ(��ʱ�����õ��м�ṹ)
    struct SItemEx : public SItem
    {
        int64_t stackNum;    //�ѵ�����
        uint8_t byType;        //���� EItemType ö��
        SItemEx() : stackNum(0), byType(0) {}
    };
    
    typedef std::list<SItemEx> LIST_ITEM_EX;
// index - SItemEx
    typedef std::map<uint16_t, SItemEx> MAP_INDEX_ITEM_EX;
// label - MAP_INDEX_ITEM_EX
    typedef std::map<uint8_t, MAP_INDEX_ITEM_EX> MAP_LABEL_MAP_INDEX_ITEM_EX;

// index - SItem
    typedef std::map<uint16_t, SItem> MAP_INDEX_ITEM;

//label - VEC_ITEM_PROTO_EX
    typedef std::unordered_map<uint8_t, VEC_ITEM_PROTO_EX> MAP_LABEL_VEC_ITEM_PROTO_EX;
//index - ItemProtoInfo
    typedef std::map<uint16_t, proto_ff::ItemProtoInfo> MAP_INDEX_ITEM_PROTO_EX;
// label - LIST_ITEM_EX(��ǩ-��Ʒ�б�)
    typedef std::unordered_map<uint8_t, LIST_ITEM_EX> MAP_LABEL_LIST_ITEM_EX;
public:
    NFPackageBag();
    
    virtual ~NFPackageBag();
    
    int CreateInit();
    int ResumeInit();
public:
    virtual int Init(NFShmObj *pShmObj, NFPlayer *pMaster);
    
    virtual int LoadFromDB(const proto_ff::RoleDBUnitBagData* pUnitBagData);
    
    virtual int32_t GetPackageType() { return m_nPackageType; }
    
    virtual uint32_t GetOpenGrid() { return m_nOpenGrid; }
    
    virtual bool ValidIndex(uint16_t nIndex) { return nIndex < m_nOpenGrid; }
    
    virtual void SetPackageSaveFlag(bool bSave = true);
    
    void SendPackageInfoToClient();
    
    void Save(proto_ff::RoleDBUnitBagData& bagData);
public:
    //ͨ��������ȡ��Ʒ
    virtual bool IsEmptyGridByIndex(uint16_t nIndex) = 0;
    virtual NFItem *GetItemByIndex(uint16_t nIndex) = 0;
    virtual uint16_t SetItemByIndex(uint16_t nIndex, const NFItem &item) = 0;
    virtual uint16_t SetItemByIndex(uint16_t nIndex, const NFItem *pItem) = 0;
    //��ȡָ����Ʒ����
    virtual int64_t GetItemNum(uint64_t nItemID, int64_t &nUnBindNum, int64_t &nBindNum);
    //��ȡָ����Ʒ�󶨺ͷǰ�����
    virtual int64_t GetPackageItemNum(uint64_t nItemID, int64_t &nUnBindNum, int64_t &nBindNum);
    //��ȡ����ָ����Ʒ�󶨺ͷǰ�����
    virtual int64_t GetPackageItemNum(uint64_t nItemID, int64_t &nUnBindNum, int64_t &nBindNum, ORDER_MAP_UINT16_INT64 &mapUnbindGridHas, ORDER_MAP_UINT16_INT64 &mapBindGridHas);
    
    //��ȡ����λ����Ʒ
    virtual const NFItem *GetItem(uint16_t nIndex);
    //��ȡ��һ��ָ��ID����Ʒ
    virtual NFItem *GetFirstItemById(uint64_t item_id);
    //��ȡ��Ʒ������tem.xlsx ���� subType �ֶ� ��ȡ��Ʒ, ֻ�����ҵ��ĵ�һ����Ʒ��
    virtual const NFItem *GetItemByType(int32_t itemSubType);
public:
    //�Ƿ������Ʒ
    virtual bool HasItem(const LIST_ITEM &lstItem);
    //�����Ƿ������Ʒ
    virtual bool HasPackageItem(const LIST_ITEM &lstItem, ORDER_MAP_UINT16_INT64 &mapGridReduceNum);
    //�����Ƿ������Ʒ
    virtual bool HasPackageItem(const MAP_UINT16_INT64 &mapIdxNum, ORDER_MAP_UINT16_INT64 &mapGridReduceNum);
    //�Ƿ������Ʒ
    virtual bool HasBagItem(const LIST_ITEM &lstItem, ORDER_MAP_UINT16_INT64 &mapGridReduceNum);
    //�Ƿ������Ʒ
    virtual bool HasBagItem(const MAP_UINT16_INT64 &mapIdxNum, ORDER_MAP_UINT16_INT64 &mapGridReduceNum);
public:
    //�Ƴ���Ʒ
    virtual bool RemoveItem(uint64_t nItemID, int64_t nNum, SCommonSource &sourceParam, int8_t byBind = (int8_t) EBindState::EBindState_all);
    //�Ƴ���Ʒ sourceParam : ��Ʒ�Ƴ���Դ����ʱû�У�
    virtual bool RemoveItem(LIST_ITEM &lstItem, SCommonSource &sourceParam);
    //�Ƴ���Ʒ index,num sourceParam : ��Ʒ�Ƴ���Դ����ʱû�У�
    virtual bool RemoveItemByIndex(uint16_t nIndex, int64_t nNum, SCommonSource &sourceParam);
    //�Ƴ���Ʒ index-num nGetType : ��Ʒ�Ƴ���Դ����ʱû�У�
    virtual bool RemoveItem(MAP_UINT16_INT64 &mapIndexItem, SCommonSource &sourceParam);
    //�Ƴ�������Ʒ
    virtual bool RemovePackageItem(ORDER_MAP_UINT16_INT64 &mapGridReduceNum, MAP_UINT64_INT64 &mapItemNum);
    //�Ƴ���Ʒ
    virtual bool RemoveBagItem(ORDER_MAP_UINT16_INT64 &mapGridReduceNum, MAP_UINT64_INT64 &mapItemNum);
public:
    //�Ƿ��������Ʒ
    virtual bool CanAddItem(LIST_ITEM &lstItem);
    //�ܷ������Ʒ
    virtual bool CanAddItem(VEC_ITEM_PROTO_EX &vecProtoItemsEx);
    //�ܷ���ӽ�����
    virtual bool CanPackageAddItem(MAP_LABEL_LIST_ITEM_EX &mapInLabelItem, MAP_LABEL_VEC_ITEM_PROTO_EX &mapInLabelVecItemProtoEx, MAP_UINT16_INT64 &mapOutGridAddNum, MAP_INDEX_ITEM_PROTO_EX &mapOutNewIdxItemProtoEx);
    //�ܷ���ӵ�����  mapOutGridAddNum:�ɶѵ��ĸ��ӣ�ÿ�����������ӵ����� index - num
    virtual bool CanBagAddItem(MAP_LABEL_LIST_ITEM_EX &mapInLabelItem, MAP_LABEL_VEC_ITEM_PROTO_EX &mapInLabelVecItemProtoEx, MAP_UINT16_INT64 &mapOutGridAddNum, MAP_INDEX_ITEM_PROTO_EX &mapOutNewIdxItemProtoEx);
public:
    //�����Ʒ
    virtual bool AddItem(uint64_t nItemID, int64_t nNum, SCommonSource &sourceParam, int8_t byBind = (int8_t) EBindState::EBindState_no);
    //�����Ʒ sourceParam : ��Ʒ��Դ����ʱû�У�
    virtual bool AddItem(LIST_ITEM &lstItem, SCommonSource &sourceParam, bool update = true, bool tip = true);
    
    //�����Ʒ sourceParam : ��Ʒ��Դ����ʱû�У�
    virtual bool AddItem(VEC_ITEM_PROTO &vecProtoItems, SCommonSource &sourceParam, bool update = true, bool tip = true);
    //�����Ʒ sourceParam : ��Ʒ��Դ����ʱû�У�
    virtual bool AddItem(VEC_ITEM_PROTO_EX &vecProtoItemsEx, SCommonSource &sourceParam, bool update = true, bool tip = true);
    //�����Ʒ sourceParam : ��Ʒ��Դ����ʱû�У�,����ӿ���ͨ������������Ʒ˳�����ǰ����Ʒtip��ʾ
    virtual bool AddItemEx(VEC_ITEM_PROTO_EX &vecProtoItemsEx, SCommonSource &sourceParam);
    //�����Ʒ sourceParam : ��Ʒ��Դ����ʱû�У� vecProtoItemsOut:�����������ⲿ��Ҫ����Ʒ����
    virtual bool AddItem(VEC_ITEM_PROTO_EX &vecProtoItemsEx, VEC_ITEM_PROTO_EX &vecProtoItemsOut, SCommonSource &sourceParam, bool update = true, bool tip = true);
    //��ӱ�����Ʒ
    virtual bool AddPackageItem(MAP_UINT16_INT64 &mapOutGridAddNum, MAP_INDEX_ITEM_PROTO_EX &mapOutNewIdxItemProtoEx, const VEC_ITEM_PROTO_EX &vecProtoTipItemsEx, bool update = true);
    //�����Ʒ�¼�����
    virtual void OnAddItemEvent(MAP_UINT16_INT64 &mapOutGridAddNum, MAP_INDEX_ITEM_PROTO_EX &mapOutNewIdxItemProtoEx, SCommonSource &sourceParam);
    //��䴫������
    virtual void OnAddItemParam(MAP_UINT16_INT64 &mapOutGridAddNum, MAP_INDEX_ITEM_PROTO_EX &mapOutNewIdxItemProtoEx, VEC_ITEM_PROTO_EX &vecProtoExParam);
public:
    //��ձ���������Ʒ
    virtual bool ClearPackage();
    //��չ����
    virtual int32_t Expand(int32_t &nNum);
    virtual int32_t GetMaxGridNum();
    
    //��ȡĳ����Ʒʣ��ɵ��ӵ�����
    virtual int64_t GetItemLeftPile(uint64_t nItemID, int64_t &nUnBindNum, int64_t &nBindNum, ORDER_MAP_UINT16_INT64 &mapUnbindGridHas, ORDER_MAP_UINT16_INT64 &mapBindGridHas);
    //��ȡĳ����ǩ����Ʒʣ��ɵ�������
    virtual int64_t GetLabelItemLeftPile(uint8_t byLabel, uint64_t nItemID, int64_t stackLimit, int64_t &nUnBindNum, int64_t &nBindNum, ORDER_MAP_UINT16_INT64 &mapUnbindGridHas, ORDER_MAP_UINT16_INT64 &mapBindGridHas);

public:
    //�Ƴ�����ָ�����͵���Ʒ������ tem.xlsx ���� subType �ֶ� �Ƴ���Ʒ��
    virtual bool RemoveAllByType(int32_t itemSubType, SCommonSource &sourceParam);
    virtual bool RemoveAllByItemID(uint64_t nItemID, SCommonSource &sourceParam);
    //�ֿ�򱳰�����ȫ������
    virtual void AllOpenGrid();
    //��ȡ��ǩҳ�ո��������б� nNum:Ϊ0��ʾ��ǩҳ���еĿո��ӣ�����0ʱ ֻ���� nNum ���ո���
    virtual uint32_t GetEmptyGrid(VEC_UINT16 &vecGrid);
    virtual uint32_t GetEmptyGrid();

public:
    //���ø�����Ϣ
    virtual bool SetUpdateItemInfo(NFItem *pItem, proto_ff::ItemProtoInfo *protoItemInfo, bool bDel = false);
    //��ӱ���������Ϣ
    virtual bool AddPackageUpdateInfo(NFItem *pItem, proto_ff::NotifyPackageUpdate &ret, bool bDel = false);
    //���±���
    virtual void UpdatePackageInfo(proto_ff::NotifyPackageUpdate &protoUpdate);
    //������չ
    virtual bool UpdatePackageExpand();
    
    //�Ƴ���Ʒ�¼�����
    virtual void OnRemoveItemEvent(MAP_UINT64_INT64 &mapItemNum, SCommonSource &sourceParam);
    //����ռ���Ʒ����Ҫ�����������ռ���Ʒ��
    virtual void AddCollectItem(proto_ff::ItemProtoInfo &itemProto, VEC_ITEM_PROTO_EX &vCollectItems);
    //�ռ���Ʒ�¼�����
    virtual void CollectItemEvent(VEC_ITEM_PROTO_EX &vCollectItems, SCommonSource &sourceParam, int32_t nOperateType = EItemOpetateType_None);
    
    //��Ʒ��־
    virtual void ItemLog(MAP_UINT16_INT64 &items, MAP_INDEX_ITEM_PROTO_EX &mapOutNewIdxItemProtoEx, SCommonSource &source);
    virtual void ItemLog(MAP_UINT64_INT64 &items, SCommonSource &source);
    //���������¼
    virtual void ClearIdxRecord();
    //get ������¼
    virtual SET_UINT16 &GetIdxRecord();

public:
    virtual bool SortItem();
    virtual bool BagItemSort();
    //�ѵ��ϲ�
    uint16_t PileMerge(VEC_PACKAGE_ITEM vecItems, VEC_ITEM_PROTO_EX &vecPileProtoEx);
public:
    //
    virtual void ItemSell(proto_ff::PackageSellReq &req);
    //������Ʒ
    virtual void ProcessItem(const LIST_ITEM &lstItem, LIST_ITEM &outLstItem, MAP_UINT32_INT64 &mapAttr, bool addFlag = false);
    //������Ʒ
    virtual void ProcessItem(VEC_ITEM_PROTO_EX &vecProtoItemsEx, VEC_ITEM_PROTO_EX &vecOutProtoItemsEx, MAP_UINT32_INT64 &mapAttr, bool addFlag);

public:
    //�ϲ�������Ʒ�б�
    virtual void MergePackageItemList(LIST_ITEM &inlstItem, VEC_ITEM_PROTO_EX &vecinProtoEx, MAP_LABEL_LIST_ITEM_EX &mapOutLabelItem, MAP_LABEL_VEC_ITEM_PROTO_EX &mapOutLabelVecItemProtoEx);
    //�ϲ���Ʒ�б�
    virtual void MergeItemList(LIST_ITEM &inlstItem, VEC_ITEM_PROTO_EX &vecinProtoEx, MAP_LABEL_LIST_ITEM_EX &mapOutLabelItem, MAP_LABEL_VEC_ITEM_PROTO_EX &mapOutLabelVecItemProtoEx);
public:
    uint64_t calcDynExp(int32_t type, int32_t num);
protected:
    uint64_t m_nLastSortTime;                        //�ϴ������ʱ��
    uint16_t m_nExpandNum;                            //��չ������
    uint16_t m_nOpenGrid;                            //��������������
    uint16_t m_nPackageType;
    
    int32_t m_initGrid;                                //��ʼ�����Ӵ�С
    int32_t m_maxGrid;                                //�����Ӵ�С
    NFShmPtr<NFPlayer> m_pMaster;
    SET_UINT16 m_setIdxRecord;                        //��¼ÿ�μ���Ʒ ���������ӵĸ�����������ÿ�μ���Ʒ֮ǰ�����
};