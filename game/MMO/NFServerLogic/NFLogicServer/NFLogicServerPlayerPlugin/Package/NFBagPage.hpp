// -------------------------------------------------------------------------
//    @FileName         :    NFBagPage.h
//    @Author           :    gaoyi
//    @Date             :    23-10-28
//    @Email			:    445267987@qq.com
//    @Module           :    NFBagPage
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFShmCore/NFSeqOP.h"
#include "NFLogicCommon/NFLogicShmTypeDefines.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmCore/NFRawShmObj.h"
#include "NFLogicCommon/NFPackageDefine.h"
#include "NFComm/NFShmStl/NFShmVector.h"
#include "NFGameCommon/NFComTypeDefine.h"
#include "NFLogicCommon/Item/NFItem.h"
#include "NFLogicCommon/Item/NFItemMgr.h"
#include "Package.pb.h"

typedef std::list<SItem> LIST_ITEM_EX;
// label - LIST_ITEM_EX(��ǩ-��Ʒ�б�)
typedef std::unordered_map<uint8_t, LIST_ITEM_EX> MAP_LABEL_LIST_ITEM_EX;
// index - SItemEx
typedef std::map<uint16_t, SItem> MAP_INDEX_ITEM_EX;
// label - MAP_INDEX_ITEM_EX
typedef std::map<uint8_t, MAP_INDEX_ITEM_EX> MAP_LABEL_MAP_INDEX_ITEM_EX;

// index - SItem
typedef std::map<uint16_t, SItem> MAP_INDEX_ITEM;

//label - VEC_ITEM_PROTO_EX
typedef std::unordered_map<uint8_t, VEC_ITEM_PROTO_EX> MAP_LABEL_VEC_ITEM_PROTO_EX;
//index - ItemProtoInfo
typedef std::map<uint16_t, proto_ff::ItemProtoInfo> MAP_INDEX_ITEM_PROTO_EX;

template<class ItemType, int MaxGridNum>
class NFBagPage : public NFRawShmObj, public NFSeqOP
{
    typedef NFShmVector<ItemType, MaxGridNum> VEC_PACKAGE_ITEM;
public:
    NFBagPage()
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
    
    virtual ~NFBagPage()
    {
    
    }
    
    int CreateInit()
    {
        m_nLastSortTime = 0;
        m_nExpandNum = 0;
        m_nOpenGrid = 0;
        m_setIdxRecord.clear();
        m_nPackageType = 0;
        m_initGrid = 0;
        m_maxGrid = 0;
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
public:
    int Init(NFShmObj *pShmObj, NFPlayer* pMaster, uint32_t nPackageType, uint32_t nInitGrid);
public:
    int32_t GetPackageType() { return m_nPackageType; }
public:
    //ͨ��������ȡ��Ʒ
    ItemType *GetItemByIndex(uint16_t nIndex);
    uint16_t SetItemByIndex(uint16_t nIndex, const ItemType& item);
    //��ȡָ����Ʒ����
    int64_t GetItemNum(uint64_t nItemID, int64_t &nUnBindNum, int64_t &nBindNum);
    //��ȡָ����Ʒ�󶨺ͷǰ�����
    int64_t GetPackageItemNum(uint64_t nItemID, int64_t &nUnBindNum, int64_t &nBindNum);
    //��ȡ����ָ����Ʒ�󶨺ͷǰ�����
    int64_t GetPackageItemNum(uint64_t nItemID, int64_t &nUnBindNum, int64_t &nBindNum, ORDER_MAP_UINT16_INT64 &mapUnbindGridHas, ORDER_MAP_UINT16_INT64 &mapBindGridHas);
    
    //��ȡ����λ����Ʒ
    virtual const ItemType *GetItem(uint16_t nIndex);
    //��ȡ��һ��ָ��ID����Ʒ
    virtual ItemType* GetFirstItemById(uint64_t item_id);
    //��ȡ��Ʒ������tem.xlsx ���� subType �ֶ� ��ȡ��Ʒ, ֻ�����ҵ��ĵ�һ����Ʒ��
    virtual const ItemType *GetItemByType(int32_t itemSubType);
public:
    //�Ƿ������Ʒ
    virtual bool HasItem(LIST_ITEM &lstItem);
    //�����Ƿ������Ʒ
    bool HasPackageItem(const LIST_ITEM &lstItem, ORDER_MAP_UINT16_INT64 &mapGridReduceNum);
    //�����Ƿ������Ʒ
    bool HasPackageItem(const MAP_UINT16_INT64 &mapIdxNum, ORDER_MAP_UINT16_INT64 &mapGridReduceNum);
    //�Ƿ������Ʒ
    bool HasBagItem(const LIST_ITEM &lstItem, ORDER_MAP_UINT16_INT64 &mapGridReduceNum);
    //�Ƿ������Ʒ
    bool HasBagItem(const MAP_UINT16_INT64 &mapIdxNum, ORDER_MAP_UINT16_INT64 &mapGridReduceNum);
public:
    //�Ƴ���Ʒ
    virtual bool RemoveItem(uint64_t nItemID, int64_t nNum, SCommonSource &sourceParam, int8_t byBind = (int8_t)EBindState::EBindState_all);
    //�Ƴ���Ʒ sourceParam : ��Ʒ�Ƴ���Դ����ʱû�У�
    virtual bool RemoveItem(LIST_ITEM &lstItem, SCommonSource &sourceParam);
    //�Ƴ���Ʒ index,num sourceParam : ��Ʒ�Ƴ���Դ����ʱû�У�
    virtual bool RemoveItemByIndex(uint16_t nIndex, int64_t nNum, SCommonSource &sourceParam);
    //�Ƴ���Ʒ index-num nGetType : ��Ʒ�Ƴ���Դ����ʱû�У�
    virtual bool RemoveItem(MAP_UINT16_INT64 &mapIndexItem, SCommonSource &sourceParam);
    //�Ƴ�������Ʒ
    bool RemovePackageItem(ORDER_MAP_UINT16_INT64 &mapGridReduceNum, MAP_UINT64_INT64 &mapItemNum);
    //�Ƴ���Ʒ
    bool RemoveBagItem(ORDER_MAP_UINT16_INT64 &mapGridReduceNum, MAP_UINT64_INT64 &mapItemNum);
public:
    //�Ƿ��������Ʒ
    virtual bool CanAddItem(LIST_ITEM &lstItem);
    //�ܷ������Ʒ
    virtual bool CanAddItem(VEC_ITEM_PROTO_EX &vecProtoItemsEx);
    //�ܷ���ӽ�����
    bool CanPackageAddItem(MAP_LABEL_LIST_ITEM_EX &mapInLabelItem, MAP_LABEL_VEC_ITEM_PROTO_EX &mapInLabelVecItemProtoEx, MAP_UINT16_INT64 &mapOutGridAddNum, MAP_INDEX_ITEM_PROTO_EX &mapOutNewIdxItemProtoEx);
    //�ܷ���ӵ�����  mapOutGridAddNum:�ɶѵ��ĸ��ӣ�ÿ�����������ӵ����� index - num
    bool CanBagAddItem(MAP_LABEL_LIST_ITEM_EX &mapInLabelItem, MAP_LABEL_VEC_ITEM_PROTO_EX &mapInLabelVecItemProtoEx, MAP_UINT16_INT64 &mapOutGridAddNum, MAP_INDEX_ITEM_PROTO_EX &mapOutNewIdxItemProtoEx);
public:
    //�����Ʒ
    virtual bool AddItem(uint64_t nItemID, int64_t nNum, SCommonSource &sourceParam, int8_t byBind = (int8_t)EBindState::EBindState_no);
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
    bool AddPackageItem(MAP_UINT16_INT64 &mapOutGridAddNum, MAP_INDEX_ITEM_PROTO_EX &mapOutNewIdxItemProtoEx, const VEC_ITEM_PROTO_EX &vecProtoTipItemsEx, bool update = true);
    //�����Ʒ�¼�����
    void OnAddItemEvent(MAP_UINT16_INT64 &mapOutGridAddNum, MAP_INDEX_ITEM_PROTO_EX &mapOutNewIdxItemProtoEx, SCommonSource &sourceParam);
    //��䴫������
    void OnAddItemParam(MAP_UINT16_INT64 &mapOutGridAddNum, MAP_INDEX_ITEM_PROTO_EX &mapOutNewIdxItemProtoEx, VEC_ITEM_PROTO_EX &vecProtoExParam);
public:
    //��ձ���������Ʒ
    virtual bool ClearPackage();
    //��չ����
    virtual int32_t Expand(int32_t& nNum);
    virtual int32_t GetMaxGridNum();
    
    //��ȡĳ����Ʒʣ��ɵ��ӵ�����
    int64_t GetItemLeftPile(uint64_t nItemID, int64_t &nUnBindNum, int64_t &nBindNum, ORDER_MAP_UINT16_INT64 &mapUnbindGridHas, ORDER_MAP_UINT16_INT64 &mapBindGridHas);
    //��ȡĳ����ǩ����Ʒʣ��ɵ�������
    int64_t GetLabelItemLeftPile(uint8_t byLabel, uint64_t nItemID, int64_t stackLimit, int64_t &nUnBindNum, int64_t &nBindNum, ORDER_MAP_UINT16_INT64 &mapUnbindGridHas, ORDER_MAP_UINT16_INT64 &mapBindGridHas);
    
public:
    //�Ƴ�����ָ�����͵���Ʒ������ tem.xlsx ���� subType �ֶ� �Ƴ���Ʒ��
    virtual bool RemoveAllByType(int32_t itemSubType, SCommonSource &sourceParam);
    virtual bool RemoveAllByItemID(uint64_t nItemID, SCommonSource& sourceParam);
    //�ֿ�򱳰�����ȫ������
    virtual void AllOpenGrid();
    //��ȡ��ǩҳ�ո��������б� nNum:Ϊ0��ʾ��ǩҳ���еĿո��ӣ�����0ʱ ֻ���� nNum ���ո���
    uint32_t GetEmptyGrid(VEC_UINT16 &vecGrid);
    uint32_t GetEmptyGrid();
    bool ValidIndex(uint16_t nIndex){ return nIndex < m_nOpenGrid; }
    void SetPackageSaveFlag(bool bSave=true);
public:
    //���ø�����Ϣ
    bool SetUpdateItemInfo(ItemType *pItem, proto_ff::ItemProtoInfo *protoItemInfo, bool bDel = false);
    //��ӱ���������Ϣ
    bool AddPackageUpdateInfo(ItemType *pItem, proto_ff::NotifyPackageUpdate &ret, bool bDel = false);
    //���±���
    void UpdatePackageInfo(proto_ff::NotifyPackageUpdate &protoUpdate);
    //������չ
    bool UpdatePackageExpand();
    
    //�Ƴ���Ʒ�¼�����
    void OnRemoveItemEvent(MAP_UINT64_INT64 &mapItemNum, SCommonSource &sourceParam);
    //����ռ���Ʒ����Ҫ�����������ռ���Ʒ��
    void AddCollectItem(proto_ff::ItemProtoInfo& itemProto, VEC_ITEM_PROTO_EX& vCollectItems);
    //�ռ���Ʒ�¼�����
    void CollectItemEvent(VEC_ITEM_PROTO_EX &vCollectItems, SCommonSource &sourceParam, int32_t nOperateType = EItemOpetateType_None);
    
    //��Ʒ��־
    void ItemLog(MAP_UINT16_INT64 &items, MAP_INDEX_ITEM_PROTO_EX &mapOutNewIdxItemProtoEx, SCommonSource &source);
    void ItemLog(MAP_UINT64_INT64 &items, SCommonSource &source);
    //���������¼
    void ClearIdxRecord();
    //get ������¼
    SET_UINT16& GetIdxRecord();
    
    uint32_t GetOpenGrid(){ return m_nOpenGrid; }
    bool SortItem();
    bool BagItemSort();
    //�ѵ��ϲ�
    uint16_t PileMerge(VEC_PACKAGE_ITEM vecItems, VEC_ITEM_PROTO_EX &vecPileProtoEx);
    //
    void ItemSell(proto_ff::PackageSellReq& req);
    void ShentiEquipSell(proto_ff::PackageSellReq& req);
    //������Ʒ
    void ProcessItem(LIST_ITEM& lstItem, LIST_ITEM& outLstItem, MAP_UINT32_INT64& mapAttr,bool addFlag = false);
    //������Ʒ
    void ProcessItem(VEC_ITEM_PROTO_EX& vecProtoItemsEx, VEC_ITEM_PROTO_EX& vecOutProtoItemsEx, MAP_UINT32_INT64& mapAttr, bool addFlag);

public:
    //�ϲ�������Ʒ�б�
    void MergePackageItemList(LIST_ITEM &inlstItem, VEC_ITEM_PROTO_EX &vecinProtoEx, MAP_LABEL_LIST_ITEM_EX &mapOutLabelItem, MAP_LABEL_VEC_ITEM_PROTO_EX &mapOutLabelVecItemProtoEx);
    //�ϲ���Ʒ�б�
    void MergeItemList(LIST_ITEM &inlstItem, VEC_ITEM_PROTO_EX &vecinProtoEx, MAP_LABEL_LIST_ITEM_EX &mapOutLabelItem, MAP_LABEL_VEC_ITEM_PROTO_EX &mapOutLabelVecItemProtoEx);

private:
    uint64_t m_nLastSortTime;                        //�ϴ������ʱ��
    uint16_t m_nExpandNum;                            //��չ������
    uint16_t m_nOpenGrid;                            //��������������
    VEC_PACKAGE_ITEM m_vecItems;                    //��������
    SET_UINT16 m_setIdxRecord;                        //��¼ÿ�μ���Ʒ ���������ӵĸ�����������ÿ�μ���Ʒ֮ǰ�����
    uint16_t m_nPackageType;
    
    int32_t m_initGrid;                                //��ʼ�����Ӵ�С
    int32_t m_maxGrid;                                //�����Ӵ�С

protected:
    NFShmPtr<NFPlayer> m_pMaster;
};


template<class ItemType, int MaxGridNum>
int NFBagPage<ItemType, MaxGridNum>::Init(NFShmObj *pShmObj, NFPlayer* pMaster, uint32_t nPackageType, uint32_t nInitGrid)
{
    InitShmObj(pShmObj);
    m_pMaster = pMaster;
    
    CHECK_EXPR(nInitGrid <= MaxGridNum, -1, "nInitGrid:{} maxGridNum:{} Error", nInitGrid, MaxGridNum);
    m_nLastSortTime = 0;
    m_nExpandNum = 0;
    m_nOpenGrid = nInitGrid;
    m_vecItems.resize(MaxGridNum);
    m_setIdxRecord.clear();
    m_nPackageType = nPackageType;
    m_initGrid = nInitGrid;
    m_maxGrid = MaxGridNum;
    return 0;
}

template<class ItemType, int MaxGridNum>
ItemType *NFBagPage<ItemType, MaxGridNum>::GetItemByIndex(uint16_t nIndex)
{
    if (nIndex < m_nOpenGrid)
    {
        return m_vecItems[nIndex];
    }
    return nullptr;
}

template<class ItemType, int MaxGridNum>
uint16_t NFBagPage<ItemType, MaxGridNum>::SetItemByIndex(uint16_t nIndex, const ItemType& item)
{
    if (nIndex < m_nOpenGrid)
    {
        m_vecItems[nIndex] = item;
        return nIndex;
    }
    return -1;
}

template<class ItemType, int MaxGridNum>
int64_t NFBagPage<ItemType, MaxGridNum>::GetItemNum(uint64_t nItemID, int64_t &nUnBindNum, int64_t &nBindNum)
{
    LIST_ITEM lstOutItem;
    MAP_UINT32_INT64 mapAttr;
    uint32_t attrId = 0;
    if (NFItemMgr::Instance(GetShmObj()->m_pObjPluginManager)->IsVirItem(nItemID, attrId))
    {
        return m_pMaster->GetAttr(attrId);
    }
    
    return GetPackageItemNum(nItemID, nUnBindNum, nBindNum);
}

template<class ItemType, int MaxGridNum>
int64_t NFBagPage<ItemType, MaxGridNum>::GetPackageItemNum(uint64_t nItemID, int64_t &nUnBindNum, int64_t &nBindNum)
{
    return 0;
}

template<class ItemType, int MaxGridNum>
int64_t NFBagPage<ItemType, MaxGridNum>::GetPackageItemNum(uint64_t nItemID, int64_t &nUnBindNum, int64_t &nBindNum, ORDER_MAP_UINT16_INT64 &mapUnbindGridHas, ORDER_MAP_UINT16_INT64 &mapBindGridHas)
{
    return 0;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::HasItem(LIST_ITEM &lstItem)
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::HasPackageItem(const LIST_ITEM &lstItem, ORDER_MAP_UINT16_INT64 &mapGridReduceNum)
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::HasPackageItem(const MAP_UINT16_INT64 &mapIdxNum, ORDER_MAP_UINT16_INT64 &mapGridReduceNum)
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::HasBagItem(const LIST_ITEM &lstItem, ORDER_MAP_UINT16_INT64 &mapGridReduceNum)
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::HasBagItem(const MAP_UINT16_INT64 &mapIdxNum, ORDER_MAP_UINT16_INT64 &mapGridReduceNum)
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::RemoveItem(uint64_t nItemID, int64_t nNum, SCommonSource &sourceParam, int8_t byBind)
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::RemoveItem(LIST_ITEM &lstItem, SCommonSource &sourceParam)
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::RemoveItemByIndex(uint16_t nIndex, int64_t nNum, SCommonSource &sourceParam)
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::RemoveItem(MAP_UINT16_INT64 &mapIndexItem, SCommonSource &sourceParam)
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::RemovePackageItem(ORDER_MAP_UINT16_INT64 &mapGridReduceNum, MAP_UINT64_INT64 &mapItemNum)
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::RemoveBagItem(ORDER_MAP_UINT16_INT64 &mapGridReduceNum, MAP_UINT64_INT64 &mapItemNum)
{
    return false;
}

template<class ItemType, int MaxGridNum>
const ItemType *NFBagPage<ItemType, MaxGridNum>::GetItem(uint16_t nIndex)
{
    return nullptr;
}

template<class ItemType, int MaxGridNum>
ItemType *NFBagPage<ItemType, MaxGridNum>::GetFirstItemById(uint64_t item_id)
{
    return nullptr;
}

template<class ItemType, int MaxGridNum>
const ItemType *NFBagPage<ItemType, MaxGridNum>::GetItemByType(int32_t itemSubType)
{
    return nullptr;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::CanAddItem(LIST_ITEM &lstItem)
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::CanAddItem(VEC_ITEM_PROTO_EX &vecProtoItemsEx)
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::CanPackageAddItem(MAP_LABEL_LIST_ITEM_EX &mapInLabelItem, MAP_LABEL_VEC_ITEM_PROTO_EX &mapInLabelVecItemProtoEx, MAP_UINT16_INT64 &mapOutGridAddNum, MAP_INDEX_ITEM_PROTO_EX &mapOutNewIdxItemProtoEx)
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::CanBagAddItem(MAP_LABEL_LIST_ITEM_EX &mapInLabelItem, MAP_LABEL_VEC_ITEM_PROTO_EX &mapInLabelVecItemProtoEx, MAP_UINT16_INT64 &mapOutGridAddNum, MAP_INDEX_ITEM_PROTO_EX &mapOutNewIdxItemProtoEx)
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::AddItem(uint64_t nItemID, int64_t nNum, SCommonSource &sourceParam, int8_t byBind)
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::AddItem(LIST_ITEM &lstItem, SCommonSource &sourceParam, bool update, bool tip)
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::AddItem(VEC_ITEM_PROTO &vecProtoItems, SCommonSource &sourceParam, bool update, bool tip)
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::AddItem(VEC_ITEM_PROTO_EX &vecProtoItemsEx, SCommonSource &sourceParam, bool update, bool tip)
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::AddItemEx(VEC_ITEM_PROTO_EX &vecProtoItemsEx, SCommonSource &sourceParam)
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::AddItem(VEC_ITEM_PROTO_EX &vecProtoItemsEx, VEC_ITEM_PROTO_EX &vecProtoItemsOut, SCommonSource &sourceParam, bool update, bool tip)
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::AddPackageItem(MAP_UINT16_INT64 &mapOutGridAddNum, MAP_INDEX_ITEM_PROTO_EX &mapOutNewIdxItemProtoEx, const VEC_ITEM_PROTO_EX &vecProtoTipItemsEx, bool update)
{
    return false;
}

template<class ItemType, int MaxGridNum>
void NFBagPage<ItemType, MaxGridNum>::OnAddItemEvent(MAP_UINT16_INT64 &mapOutGridAddNum, MAP_INDEX_ITEM_PROTO_EX &mapOutNewIdxItemProtoEx, SCommonSource &sourceParam)
{

}

template<class ItemType, int MaxGridNum>
void NFBagPage<ItemType, MaxGridNum>::OnAddItemParam(MAP_UINT16_INT64 &mapOutGridAddNum, MAP_INDEX_ITEM_PROTO_EX &mapOutNewIdxItemProtoEx, VEC_ITEM_PROTO_EX &vecProtoExParam)
{

}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::ClearPackage()
{
    return false;
}

template<class ItemType, int MaxGridNum>
int32_t NFBagPage<ItemType, MaxGridNum>::Expand(int32_t &nNum)
{
    return 0;
}

template<class ItemType, int MaxGridNum>
int32_t NFBagPage<ItemType, MaxGridNum>::GetMaxGridNum()
{
    return 0;
}

template<class ItemType, int MaxGridNum>
int64_t NFBagPage<ItemType, MaxGridNum>::GetItemLeftPile(uint64_t nItemID, int64_t &nUnBindNum, int64_t &nBindNum, ORDER_MAP_UINT16_INT64 &mapUnbindGridHas, ORDER_MAP_UINT16_INT64 &mapBindGridHas)
{
    return 0;
}

template<class ItemType, int MaxGridNum>
int64_t NFBagPage<ItemType, MaxGridNum>::GetLabelItemLeftPile(uint8_t byLabel, uint64_t nItemID, int64_t stackLimit, int64_t &nUnBindNum, int64_t &nBindNum, ORDER_MAP_UINT16_INT64 &mapUnbindGridHas, ORDER_MAP_UINT16_INT64 &mapBindGridHas)
{
    return 0;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::RemoveAllByType(int32_t itemSubType, SCommonSource &sourceParam)
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::RemoveAllByItemID(uint64_t nItemID, SCommonSource &sourceParam)
{
    return false;
}

template<class ItemType, int MaxGridNum>
void NFBagPage<ItemType, MaxGridNum>::AllOpenGrid()
{

}

template<class ItemType, int MaxGridNum>
uint32_t NFBagPage<ItemType, MaxGridNum>::GetEmptyGrid(VEC_UINT16 &vecGrid)
{
    return 0;
}

template<class ItemType, int MaxGridNum>
uint32_t NFBagPage<ItemType, MaxGridNum>::GetEmptyGrid()
{
    return 0;
}

template<class ItemType, int MaxGridNum>
void NFBagPage<ItemType, MaxGridNum>::SetPackageSaveFlag(bool bSave)
{

}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::SetUpdateItemInfo(ItemType *pItem, proto_ff::ItemProtoInfo *protoItemInfo, bool bDel)
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::AddPackageUpdateInfo(ItemType *pItem, proto_ff::NotifyPackageUpdate &ret, bool bDel)
{
    return false;
}

template<class ItemType, int MaxGridNum>
void NFBagPage<ItemType, MaxGridNum>::UpdatePackageInfo(proto_ff::NotifyPackageUpdate &protoUpdate)
{

}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::UpdatePackageExpand()
{
    return false;
}

template<class ItemType, int MaxGridNum>
void NFBagPage<ItemType, MaxGridNum>::OnRemoveItemEvent(MAP_UINT64_INT64 &mapItemNum, SCommonSource &sourceParam)
{

}

template<class ItemType, int MaxGridNum>
void NFBagPage<ItemType, MaxGridNum>::AddCollectItem(proto_ff::ItemProtoInfo &itemProto, VEC_ITEM_PROTO_EX &vCollectItems)
{

}

template<class ItemType, int MaxGridNum>
void NFBagPage<ItemType, MaxGridNum>::CollectItemEvent(VEC_ITEM_PROTO_EX &vCollectItems, SCommonSource &sourceParam, int32_t nOperateType)
{

}

template<class ItemType, int MaxGridNum>
void NFBagPage<ItemType, MaxGridNum>::ItemLog(MAP_UINT16_INT64 &items, MAP_INDEX_ITEM_PROTO_EX &mapOutNewIdxItemProtoEx, SCommonSource &source)
{

}

template<class ItemType, int MaxGridNum>
void NFBagPage<ItemType, MaxGridNum>::ItemLog(MAP_UINT64_INT64 &items, SCommonSource &source)
{

}

template<class ItemType, int MaxGridNum>
void NFBagPage<ItemType, MaxGridNum>::ClearIdxRecord()
{

}

template<class ItemType, int MaxGridNum>
SET_UINT16 &NFBagPage<ItemType, MaxGridNum>::GetIdxRecord()
{
    return m_setIdxRecord;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::SortItem()
{
    return false;
}

template<class ItemType, int MaxGridNum>
bool NFBagPage<ItemType, MaxGridNum>::BagItemSort()
{
    return false;
}

template<class ItemType, int MaxGridNum>
uint16_t NFBagPage<ItemType, MaxGridNum>::PileMerge(NFBagPage::VEC_PACKAGE_ITEM vecItems, VEC_ITEM_PROTO_EX &vecPileProtoEx)
{
    return 0;
}

template<class ItemType, int MaxGridNum>
void NFBagPage<ItemType, MaxGridNum>::ItemSell(proto_ff::PackageSellReq &req)
{

}

template<class ItemType, int MaxGridNum>
void NFBagPage<ItemType, MaxGridNum>::ShentiEquipSell(proto_ff::PackageSellReq &req)
{

}

template<class ItemType, int MaxGridNum>
void NFBagPage<ItemType, MaxGridNum>::ProcessItem(LIST_ITEM &lstItem, LIST_ITEM &outLstItem, MAP_UINT32_INT64 &mapAttr, bool addFlag)
{

}

template<class ItemType, int MaxGridNum>
void NFBagPage<ItemType, MaxGridNum>::ProcessItem(VEC_ITEM_PROTO_EX &vecProtoItemsEx, VEC_ITEM_PROTO_EX &vecOutProtoItemsEx, MAP_UINT32_INT64 &mapAttr, bool addFlag)
{

}

template<class ItemType, int MaxGridNum>
void NFBagPage<ItemType, MaxGridNum>::MergePackageItemList(LIST_ITEM &inlstItem, VEC_ITEM_PROTO_EX &vecinProtoEx, MAP_LABEL_LIST_ITEM_EX &mapOutLabelItem, MAP_LABEL_VEC_ITEM_PROTO_EX &mapOutLabelVecItemProtoEx)
{

}

template<class ItemType, int MaxGridNum>
void NFBagPage<ItemType, MaxGridNum>::MergeItemList(LIST_ITEM &inlstItem, VEC_ITEM_PROTO_EX &vecinProtoEx, MAP_LABEL_LIST_ITEM_EX &mapOutLabelItem, MAP_LABEL_VEC_ITEM_PROTO_EX &mapOutLabelVecItemProtoEx)
{

}
