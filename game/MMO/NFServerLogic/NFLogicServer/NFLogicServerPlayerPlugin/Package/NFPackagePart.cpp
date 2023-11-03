// -------------------------------------------------------------------------
//    @FileName         :    NFPackagePart.cpp
//    @Author           :    gaoyi
//    @Date             :    23-10-20
//    @Email			:    445267987@qq.com
//    @Module           :    NFPackagePart
//
// -------------------------------------------------------------------------

#include "NFPackagePart.h"

NFPackagePart::NFPackagePart()
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

NFPackagePart::~NFPackagePart()
{
}

int NFPackagePart::CreateInit()
{
    return 0;
}

int NFPackagePart::ResumeInit()
{
    return 0;
}

int NFPackagePart::Init(NFPlayer *pMaster, uint32_t partType, const proto_ff::RoleDBData &dbData)
{
    m_commonBag.Init(this, pMaster);
    return NFPart::Init(pMaster, partType, dbData);
}

int NFPackagePart::UnInit()
{
    return NFPart::UnInit();
}

int NFPackagePart::LoadFromDB(const proto_ff::RoleDBData &data)
{
    return NFPart::LoadFromDB(data);
}

int NFPackagePart::InitConfig(const proto_ff::RoleDBData &data)
{
    return NFPart::InitConfig(data);
}

int NFPackagePart::SaveDB(proto_ff::RoleDBData &dbData)
{
    return NFPart::SaveDB(dbData);
}

int NFPackagePart::OnLogin()
{
    return NFPart::OnLogin();
}

int NFPackagePart::OnLogin(proto_ff::PlayerInfoRsp &playerInfo)
{
    return NFPart::OnLogin(playerInfo);
}

int NFPackagePart::RegisterMessage()
{
    return NFPart::RegisterMessage();
}

int NFPackagePart::OnHandleClientMessage(uint32_t msgId, NFDataPackage &packet)
{
    return NFPart::OnHandleClientMessage(msgId, packet);
}

int NFPackagePart::OnHandleServerMessage(uint32_t msgId, NFDataPackage &packet)
{
    return NFPart::OnHandleServerMessage(msgId, packet);
}

uint32_t NFPackagePart::GetLastErrorCode()
{
    return m_nLastErrorCode;
}

uint32_t NFPackagePart::GetPackageNotSpaceErrorCode(uint32_t nPackageType)
{
    uint32_t nRetCode = proto_ff::RET_SUCCESS;
    
    switch (nPackageType)
    {
        case proto_ff::EPackageType_Storage:
        {
            nRetCode = proto_ff::RET_PACKAGE_STORAGE_SPACE_NOT_ENOUGH;
            break;
        }
        default:
        {
            nRetCode = proto_ff::RET_PACKAGE_COMMON_SPACE_NOT_ENOUGH;
            break;
        }
    }
    
    return nRetCode;
}

int64_t NFPackagePart::GetItemNum(uint64_t nItemID, int64_t &nUnBindNum, int64_t &nBindNum)
{
    NFPackageBag* pBag = GetItemPackageBag(nItemID);
    if (pBag)
    {
        return pBag->GetItemNum(nItemID, nUnBindNum, nBindNum);
    }
    return 0;
}

bool NFPackagePart::HasItem(LIST_ITEM &lstItem)
{
    map<uint32_t, LIST_ITEM> itemGroups;
    
    for (auto iter = lstItem.begin(); iter != lstItem.end(); iter++)
    {
        uint32_t nPackageType = GetItemPackageType(iter->nItemID);
        if (IsValidPackage(nPackageType))
        {
            itemGroups[nPackageType].push_back(*iter);
        }
        else
        {
            return false;
        }
    }
    
    for (auto it = itemGroups.begin(); it != itemGroups.end();it++)
    {
        NFPackageBag* pBag = GetPackageByType(it->first);
        if (pBag == nullptr)
            return false;
        if (!pBag->HasItem(it->second))
            return false;
    }
    
    return true;
}

bool NFPackagePart::RemoveItem(uint64_t nItemID, int64_t nNum, SCommonSource &sourceParam, int8_t byBind)
{
    NFPackageBag* pBag = GetItemPackageBag(nItemID);
    if (pBag)
    {
        return pBag->RemoveItem(nItemID, nNum, sourceParam, byBind);
    }
    return false;
}

bool NFPackagePart::RemoveItem(LIST_ITEM &lstItem, SCommonSource &sourceParam)
{
    if (!HasItem(lstItem))
        return false;
    
    
    map<uint32_t, LIST_ITEM> itemGroups;
    
    for (auto iter = lstItem.begin(); iter != lstItem.end(); iter++)
    {
        uint32_t nPackageType = GetItemPackageType(iter->nItemID);
        if (IsValidPackage(nPackageType))
        {
            itemGroups[nPackageType].push_back(*iter);
        }
    }
    
    for (auto it = itemGroups.begin(); it != itemGroups.end(); it++)
    {
        NFPackageBag* pBag = GetPackageByType(it->first);
        if (pBag == nullptr)
            return false;
        if (!pBag->RemoveItem(it->second,sourceParam))
            return false;
    }
    
    return true;
}

bool NFPackagePart::RemoveItem(uint32_t nPackageType, LIST_ITEM &lstItem, SCommonSource &sourceParam)
{
    NFPackageBag* pBag = GetPackageByType(nPackageType);
    if (pBag)
    {
        return pBag->RemoveItem(lstItem, sourceParam);
    }
    return false;
}

bool NFPackagePart::RemoveItemByIndex(uint32_t nPackageType, uint16_t nIndex, int64_t nNum, SCommonSource &sourceParam)
{
    NFPackageBag* pBag = GetPackageByType(nPackageType);
    if (pBag)
    {
        return pBag->RemoveItemByIndex(nIndex, nNum,sourceParam);
    }
    return false;
}

bool NFPackagePart::RemoveItem(uint32_t nPackageType, MAP_UINT16_INT64 &mapIndexItem, SCommonSource &sourceParam)
{
    NFPackageBag* pBag = GetPackageByType(nPackageType);
    if (pBag)
    {
        return pBag->RemoveItem(mapIndexItem, sourceParam);
    }
    return false;
}

bool NFPackagePart::CanAddItem(LIST_ITEM &lstItem)
{
    uint32_t nPackageType = proto_ff::EPackageType_Common;
    std::unordered_map<uint32_t, LIST_ITEM> itemGroups;
    
    for (auto iter = lstItem.begin(); iter != lstItem.end(); iter++)
    {
        nPackageType = GetItemPackageType(iter->nItemID);
        if (IsValidPackage(nPackageType))
        {
            itemGroups[nPackageType].push_back(*iter);
        }
    }
    
    for (auto it = itemGroups.begin(); it != itemGroups.end(); it++)
    {
        NFPackageBag* pBag = GetPackageByType(it->first);
        if (pBag == nullptr)
            return false;
        if (!pBag->CanAddItem(it->second))
        {
            nPackageType = pBag->GetPackageType();
            m_nLastErrorCode = GetPackageNotSpaceErrorCode(nPackageType);
            return false;
        }
    }
    
    return true;
}

bool NFPackagePart::CanAddItem(VEC_ITEM_PROTO_EX &vecProtoItemsEx)
{
    uint32_t nPackageType = proto_ff::EPackageType_Common;
    map<uint32_t, VEC_ITEM_PROTO_EX> itemGroups;
    
    for (auto iter = vecProtoItemsEx.begin(); iter != vecProtoItemsEx.end(); iter++)
    {
        nPackageType = GetItemPackageType(iter->item_id());
        if (IsValidPackage(nPackageType))
        {
            itemGroups[nPackageType].push_back(*iter);
        }
    }
    
    for (auto it = itemGroups.begin(); it != itemGroups.end(); it++)
    {
        NFPackageBag* pBag = GetPackageByType(it->first);
        if (pBag == nullptr)
            return false;
        if (!pBag->CanAddItem(it->second))
        {
            nPackageType = pBag->GetPackageType();
            m_nLastErrorCode = GetPackageNotSpaceErrorCode(nPackageType);
            return false;
        }
    }
    
    return true;
}

bool NFPackagePart::AddItem(uint64_t nItemID, int64_t nNum, SCommonSource &sourceParam, int8_t byBind)
{
    NFPackageBag* pBag = GetItemPackageBag(nItemID);
    if (pBag)
    {
        bool bRet = pBag->AddItem(nItemID, nNum, sourceParam, byBind);
        if (!bRet)
        {
            m_nLastErrorCode = GetPackageNotSpaceErrorCode(pBag->GetPackageType());
        }
        return bRet;
    }
    return false;
}

bool NFPackagePart::AddItem(LIST_ITEM &lstItem, SCommonSource &sourceParam, bool update, bool tip)
{
    if (!CanAddItem(lstItem))
    {
        return false;
    }
    
    map<uint32_t, LIST_ITEM> itemGroups;
    
    for (auto iter = lstItem.begin(); iter != lstItem.end(); iter++)
    {
        uint32_t nPackageType = GetItemPackageType(iter->nItemID);
        if (IsValidPackage(nPackageType))
        {
            itemGroups[nPackageType].push_back(*iter);
        }
    }
    
    for (auto it = itemGroups.begin(); it != itemGroups.end(); it++)
    {
        NFPackageBag* pBag = GetPackageByType(it->first);
        if (pBag == nullptr)
            return false;
        if (!pBag->AddItem(it->second,sourceParam,update,tip))
            return false;
    }
    
    return true;
}

bool NFPackagePart::AddItem(VEC_ITEM_PROTO &vecProtoItems, SCommonSource &sourceParam, bool update, bool tip)
{
    VEC_ITEM_PROTO_EX vecProtoEx;
    vecProtoEx.clear();
    VEC_ITEM_PROTO::iterator iter = vecProtoItems.begin();
    for (; iter != vecProtoItems.end(); ++iter)
    {
        proto_ff::ItemProtoInfo *proto = (*iter);
        if (nullptr != proto)
        {
            proto_ff::ItemProtoInfo protoEx(*proto);
            vecProtoEx.push_back(protoEx);
        }
    }
    
    if (!CanAddItem(vecProtoEx))
        return false;
    
    return AddItem(vecProtoEx, sourceParam, update);
}

bool NFPackagePart::AddItem(VEC_ITEM_PROTO_EX &vecProtoItemsEx, SCommonSource &sourceParam, bool update, bool tip)
{
    if (!CanAddItem(vecProtoItemsEx))
        return false;
    
    map<uint32_t, VEC_ITEM_PROTO_EX> itemGroups;
    
    for (auto iter = vecProtoItemsEx.begin(); iter != vecProtoItemsEx.end(); iter++)
    {
        uint32_t nPackageType = GetItemPackageType(iter->item_id());
        if (IsValidPackage(nPackageType))
        {
            itemGroups[nPackageType].push_back(*iter);
        }
    }
    
    for (auto it = itemGroups.begin(); it != itemGroups.end(); it++)
    {
        NFPackageBag* pBag = GetPackageByType(it->first);
        if (pBag == nullptr)
            return false;
        if (!pBag->AddItem(it->second,sourceParam,update,tip))
            return false;
    }
    
    return true;
}

bool NFPackagePart::AddItemEx(VEC_ITEM_PROTO_EX &vecProtoItemsEx, SCommonSource &sourceParam)
{
    if (!CanAddItem(vecProtoItemsEx))
        return false;
    
    map<uint32_t, VEC_ITEM_PROTO_EX> itemGroups;
    
    for (auto iter = vecProtoItemsEx.begin(); iter != vecProtoItemsEx.end(); iter++)
    {
        uint32_t nPackageType = GetItemPackageType(iter->item_id());
        if (IsValidPackage(nPackageType))
        {
            itemGroups[nPackageType].push_back(*iter);
        }
    }
    
    for (auto it = itemGroups.begin(); it != itemGroups.end(); it++)
    {
        NFPackageBag* pBag = GetPackageByType(it->first);
        if (pBag == nullptr)
            return false;
        if (!pBag->AddItemEx(it->second, sourceParam))
            return false;
    }
    
    return true;
}

bool NFPackagePart::AddItem(VEC_ITEM_PROTO_EX &vecProtoItemsEx, VEC_ITEM_PROTO_EX &vecProtoItemsOut, SCommonSource &sourceParam, bool update, bool tip)
{
    if (!CanAddItem(vecProtoItemsEx))
        return false;
    
    map<uint32_t, VEC_ITEM_PROTO_EX> itemGroups;
    
    for (auto iter = vecProtoItemsEx.begin(); iter != vecProtoItemsEx.end(); iter++)
    {
        uint32_t nPackageType = GetItemPackageType(iter->item_id());
        if (IsValidPackage(nPackageType))
        {
            itemGroups[nPackageType].push_back(*iter);
        }
    }
    
    for (auto it = itemGroups.begin(); it != itemGroups.end(); it++)
    {
        NFPackageBag* pBag = GetPackageByType(it->first);
        if (pBag == nullptr)
            return false;
        if (!pBag->AddItem(it->second,vecProtoItemsOut, sourceParam,update,tip))
            return false;
    }
    
    return true;
}

bool NFPackagePart::ClearPackage()
{
    for (int i = proto_ff::EPackageType_Common; i < proto_ff::EPackageType_Limit; i++)
    {
        NFPackageBag* pBag = GetPackageByType(i);
        if (pBag)
        {
            pBag->ClearPackage();
        }
    }
    
    return true;
}

int32_t NFPackagePart::Expand(int32_t &nNum)
{
    NFPackageBag* pBag = GetPackageByType(proto_ff::EPackageType_Common);
    if (pBag)
    {
        pBag->Expand(nNum);
    }
    return proto_ff::RET_FAIL;
}

int32_t NFPackagePart::ExpandStorage(int32_t &nNum)
{
    NFPackageBag* pBag = GetPackageByType(proto_ff::EPackageType_Storage);
    if (pBag)
    {
        pBag->Expand(nNum);
    }
    return proto_ff::RET_FAIL;
}

const NFItem *NFPackagePart::GetItem(uint16_t nIndex)
{
    return GetPackageItemByIndex(proto_ff::EPackageType_Common, nIndex);
}

const NFItem *NFPackagePart::GetItem(uint32_t packageType, uint16_t nIndex)
{
    return GetPackageItemByIndex(packageType, nIndex);
}

const NFItem *NFPackagePart::GetPackageItemByIndex(uint32_t nPackageType, uint32_t nIndex)
{
    NFPackageBag* pBag = GetPackageByType(nPackageType);
    if (pBag)
    {
        return pBag->GetItemByIndex(nIndex);
    }
    return nullptr;
}

NFItem *NFPackagePart::GetFirstItemById(uint32_t nPackageType, uint64_t item_id)
{
    NFPackageBag* pBag = GetPackageByType(nPackageType);
    if (pBag)
    {
        return pBag->GetFirstItemById(item_id);
    }
    return nullptr;
}

bool NFPackagePart::RemoveAllByType(int32_t itemSubType, SCommonSource &sourceParam)
{
    NFPackageBag* pBag = GetPackageByType(proto_ff::EPackageType_Common);
    if (pBag)
    {
        return pBag->RemoveAllByType(itemSubType, sourceParam);
    }
    return false;
}

bool NFPackagePart::RemoveAllByType(uint32_t nPackageType, int32_t itemSubType, SCommonSource &sourceParam)
{
    NFPackageBag* pBag = GetPackageByType(nPackageType);
    if (pBag)
    {
        return	pBag->RemoveAllByType(itemSubType, sourceParam);
    }
    return false;
}

int32_t NFPackagePart::UseItem(NFItem *pItem, int64_t &nNum, proto_ff::UseItemArgProto &protoArg)
{
    int32_t nRetCode = proto_ff::RET_FAIL;
    if (nullptr == pItem)
    {
        return proto_ff::RET_FAIL;
    }
    else if ((int64_t)pItem->GetNum() < nNum)
    {
        //�������� RET_PACKAGE_ITEM_NUM_LACK
        return proto_ff::RET_PACKAGE_ITEM_NUM_LACK;
    }
    else
    {
        auto pItemCfg = ItemItemDesc::Instance()->GetDesc(pItem->GetItemID());
        if (nullptr == pItemCfg)
        {
            //��Ʒ������
            return proto_ff::RET_PACKAGE_ITEM_NOT_EXITS;
        }
        else
        {
            //������Ʒ��ʹ���߼����������
/*            nRetCode = g_GetItemUseMgr()->UseItem(pItem, nNum, m_pMaster, protoArg);
            if (proto_ff::RET_SUCCESS == nRetCode || proto_ff::RET_PACKAGE_USE_ITEM_NOT_REMOVE == nRetCode)
            {
                //ʹ����Ʒ�¼�
                ItemUseEvent useEvent;
                useEvent.itemId = pItemCfg->id;
                useEvent.num = nNum;
                g_GetEvent()->FireExecute(EVENT_ITEM_USE, m_pMaster->Cid(), CREATURE_PLAYER, &useEvent, sizeof(ItemUseEvent));
            }*/
        }
    }
    
    return nRetCode;
}

int32_t NFPackagePart::UseItem(uint16_t nIndex, int64_t &nNum, proto_ff::UseItemArgProto &protoArg)
{
    NFItem *pItem = GetItemByPackageType(proto_ff::EPackageType_Common, nIndex);
    if (nullptr == pItem)
    {
        return proto_ff::RET_PACKAGE_ITEM_NOT_EXITS;
    }
    auto pItemCfg = ItemItemDesc::Instance()->GetDesc(pItem->GetItemID());
    if (nullptr == pItemCfg)
    {
        //��Ʒ������
        return proto_ff::RET_PACKAGE_ITEM_NOT_EXITS;
    }
    
    int32_t ret = UseItem(pItem, nNum, protoArg);
    if (proto_ff::RET_SUCCESS != ret && proto_ff::RET_PACKAGE_USE_ITEM_NOT_REMOVE != ret)
    {
        return ret;
    }
    //����Ʒ
    if (ret != proto_ff::RET_PACKAGE_USE_ITEM_NOT_REMOVE)
    {
        SCommonSource sourceParam;
        sourceParam.src = S_USE_ITEM;
        sourceParam.param1 = pItemCfg->m_id;
        RemoveItemByIndex(proto_ff::EPackageType_Common, nIndex, nNum, sourceParam);
    }
    
    return proto_ff::RET_SUCCESS;
}

void NFPackagePart::AllOpenGrid(uint32_t nPackageType)
{
    NFPackageBag* pBag = GetPackageByType(nPackageType);
    if (pBag)
    {
        pBag->AllOpenGrid();
    }
}

bool NFPackagePart::GetEmptyGrid(uint32_t nPackageType, VEC_UINT16 &vecGrid)
{
    NFPackageBag* pBag = GetPackageByType(nPackageType);
    if (pBag)
    {
        return pBag->GetEmptyGrid(vecGrid)>0;
    }
    return false;
}

int32_t NFPackagePart::GetEmptyGrid(uint32_t nPackageType)
{
    NFPackageBag* pBag = GetPackageByType(nPackageType);
    if (pBag)
    {
        return pBag->GetEmptyGrid();
    }
    return 0;
}

uint16_t NFPackagePart::SetItemByIndex(uint32_t nPackageType, uint16_t nIndex, const NFItem &item)
{
    NFPackageBag* pBag = GetPackageByType(nPackageType);
    if (pBag)
    {
        return pBag->SetItemByIndex(nIndex, item);
    }
    return 0;
}

void NFPackagePart::ItemSortByPackageType(uint32_t nPackageType)
{
    NFPackageBag* pBag = GetPackageByType(nPackageType);
    if (pBag)
    {
        pBag->SortItem();
    }
}

void NFPackagePart::SendUpdatePackageByIndex(uint32_t nPackageType, NFItem *pItem, bool bDel)
{
    proto_ff::NotifyPackageUpdate update_ret;
    AddPackageUpdateInfo(pItem, update_ret, bDel);
    //���±���
    NFPackageBag* pBag = GetPackageByType(nPackageType);
    if (pBag)
    {
        pBag->UpdatePackageInfo(update_ret);
    }
}

bool NFPackagePart::AddPackageUpdateInfo(NFItem *pItem, proto_ff::NotifyPackageUpdate &ret, bool bDel)
{
    if (nullptr == pItem)
    {
        return false;
    }
    proto_ff::ItemProtoInfo *protoItemInfo = ret.add_item_list();
    if (nullptr != protoItemInfo)
    {
        SetUpdateItemInfo(pItem, protoItemInfo, bDel);
    }
    return true;
}

void NFPackagePart::UpdatePackage(uint32_t nPackageType, proto_ff::NotifyPackageUpdate &ret)
{
    NFPackageBag* pBag = GetPackageByType(nPackageType);
    if (pBag)
    {
        pBag->UpdatePackageInfo(ret);
    }
}

NFItem *NFPackagePart::GetItemByPackageType(int8_t byPackageType, uint16_t nIndex)
{
    NFPackageBag* pBag = GetPackageByType(byPackageType);
    if (pBag)
    {
        return pBag->GetItemByIndex(nIndex);
    }
    
    return nullptr;
}

uint16_t NFPackagePart::SetItemByPackageType(int8_t byPackageType, uint16_t nIndex, const NFItem& item)
{
    NFPackageBag* pBag = GetPackageByType(byPackageType);
    if (pBag)
    {
        return pBag->SetItemByIndex(nIndex, item);
    }
    return 0;
}

bool NFPackagePart::ValidIndexByPackageType(uint16_t nIndex, int8_t byPackageType)
{
    NFPackageBag* pBag = GetPackageByType(byPackageType);
    if (pBag)
    {
        return pBag->ValidIndex(nIndex);
    }
    
    return false;
}

bool NFPackagePart::ValidPackageType(int8_t byPackageType)
{
    if (byPackageType > proto_ff::EPackageType_None && byPackageType < proto_ff::EPackageType_Limit)
    {
        return true;
    }
    return false;
}

void NFPackagePart::FirstLogin()
{

}

void NFPackagePart::StorageOperate(const char *data, uint32_t len)
{

}

bool NFPackagePart::SetUpdateItemInfo(NFItem *pItem, proto_ff::ItemProtoInfo *protoItemInfo, bool bDel)
{
    if (bDel)
    {
        return NFItemMgr::SetItemProtoInfo(pItem, protoItemInfo, 0);
    }
    return NFItemMgr::SetItemProtoInfo(pItem, protoItemInfo);
}

void NFPackagePart::ClearIdxRecord()
{
    m_setIdxRecord.clear();
}

SET_UINT16 &NFPackagePart::GetIdxRecord()
{
    return m_setIdxRecord;
}

void NFPackagePart::OnUseItem(uint64_t nItemId, uint32_t num)
{

}

bool NFPackagePart::AddItemAttr(uint64_t nItemId, uint32_t nTimes)
{
    return false;
}

void NFPackagePart::SetPackageSaveFlag(bool saveFlag)
{
    MarkDirty();
}

bool NFPackagePart::IsValidPackage(uint32_t nPackageType)
{
    return nPackageType > proto_ff::EPackageType_None && nPackageType < proto_ff::EPackageType_Limit;
}

NFPackageBag *NFPackagePart::GetPackageByItemID(uint64_t nItemID)
{
    uint32_t nPackageType = GetItemPackageType(nItemID);
    return GetPackageByType(nPackageType);
}

NFPackageBag *NFPackagePart::GetItemPackageBag(uint64_t nItemID)
{
    uint32_t nPackageType = GetItemPackageType(nItemID);
    return GetPackageByType(nPackageType);
}

uint32_t NFPackagePart::GetItemPackageType(uint64_t nItemID)
{
    uint32_t nPackageType = proto_ff::EPackageType_None;
    auto *pItemCfg = ItemItemDesc::Instance()->GetDesc(nItemID);
    if (nullptr == pItemCfg)
    {
        auto pEquipCfg = EquipEquipDesc::Instance()->GetDesc(nItemID);
        if (nullptr == pEquipCfg)
        {
            return proto_ff::EPackageType_None;
        }
        return pEquipCfg->m_type;
    }
    
    switch (pItemCfg->m_itemType)
    {
        case proto_ff::EItemType_Material:
            nPackageType = proto_ff::EPackageType_Common;
            break;
        case proto_ff::EItemType_Other:
            nPackageType = proto_ff::EPackageType_Common;
            break;
        case proto_ff::EItemType_MountEgg:
            nPackageType = proto_ff::EPackageType_MountEgg;
            break;
        case proto_ff::EItemType_PetEgg:
            nPackageType = proto_ff::EPackageType_PetEgg;
            break;
        case proto_ff::EItemType_HandBook:
            nPackageType = proto_ff::EPackageType_HandBook;
            break;
        default:
            break;
    }
    
    return nPackageType;
}

NFPackageBag *NFPackagePart::GetPackageByType(uint32_t nPackageType)
{
    if (!IsValidPackage(nPackageType)) return nullptr;
    switch (nPackageType)
    {
        case proto_ff::EPackageType_Common:
            return &m_commonBag;
        case proto_ff::EPackageType_Storage:
            return &m_storeageBag;
        default:
            return &m_commonBag;
    }
    return nullptr;
}
