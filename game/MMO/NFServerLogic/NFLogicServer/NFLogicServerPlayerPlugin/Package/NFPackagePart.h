// -------------------------------------------------------------------------
//    @FileName         :    NFPackagePart.h
//    @Author           :    gaoyi
//    @Date             :    23-10-20
//    @Email			:    445267987@qq.com
//    @Module           :    NFPackagePart
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFShmCore/NFShmObj.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "Part/NFPart.h"
#include "NFLogicCommon/NFLogicShmTypeDefines.h"
#include "NFComm/NFShmCore/NFISharedMemModule.h"
#include "NFBagPage.hpp"

class NFPackagePart : public NFShmObjTemplate<NFPackagePart, EOT_LOGIC_PART_ID + PART_PACKAGE, NFPart>
{
public:
    NFPackagePart();
    
    virtual ~NFPackagePart();
    
    int CreateInit();
    
    int ResumeInit();
public:
    //******************part���ýӿ�******************
    /**
     * @brief ��ʼ��part
     * @param pMaster
     * @param partType
     * @param dbData
     * @param bCreatePlayer
     * @return
     */
    virtual int Init(NFPlayer *pMaster, uint32_t partType, const proto_ff::RoleDBData &dbData);
    
    /**
     * @brief �ͷ�part��Դ
     * @return
     */
    virtual int UnInit();
public:
    /**
     * @brief �����ݿ��м�������
     * @param data
     * @return
     */
    virtual int LoadFromDB(const proto_ff::RoleDBData &data);
    
    /**
     * @brief �������г�ʼ������
     * @return
     */
    virtual int InitConfig(const proto_ff::RoleDBData &data);
    
    /**
     * @brief �洢DB�������
     * @param proto
     * @return
     */
    virtual int SaveDB(proto_ff::RoleDBData &dbData);
    
    /**
     * @brief ��½���
     * @return
     */
    virtual int OnLogin();
    virtual int OnLogin(proto_ff::PlayerInfoRsp &playerInfo);
    
    /**
     * @brief �ǳ����
     * @return
     */
    virtual int OnLogout() { return 0; }
    
    /**
     * @brief �������
     * @return
     */
    virtual int OnDisconnect() { return 0; }
    
    /**
     * @brief �������
     * @return
     */
    virtual int OnReconnect() { return 0; }
    
    ////////////////////////////////// ÿ����� ÿ��һ��� ˢ�½ӿ� ///////////////////////////////////
    /**
     * @brief ÿ����� ˢ�½ӿ�
     * @return
     */
    virtual int DailyZeroUpdate() { return 0; }
    
    /**
     * @brief ÿ����� ˢ�½ӿ�
     * @return
     */
    virtual int WeekZeroUpdate() { return 0; }
    
    /**
     * @brief ÿ��ˢ�½ӿ�
     * @return
     */
    virtual int MonthZeroUpdate() { return 0; };
    
    /**
     * ���������Ϣ
     * @param outproto
     */
    virtual int FillFacadeProto(proto_ff::RoleFacadeProto &outproto) { return 0; }

public:
    /**
     * @brief update
     */
    virtual int Update() { return 0; }

public:
    /**
     * @brief ע��Ҫ�������Ϣ
     * @return
     */
    virtual int RegisterMessage();
public:
    /**
     * @brief ����ͻ�����Ϣ
     * @param unLinkId
     * @param packet
     * @return
     */
    virtual int OnHandleClientMessage(uint32_t msgId, NFDataPackage &packet);
    
    /**
     * @brief �������Է���������Ϣ
     * @param unLinkId
     * @param packet
     * @return
     */
    virtual int OnHandleServerMessage(uint32_t msgId, NFDataPackage &packet);
public:
private:
    ////////////////////////////////////  ��ұ����߼�  //////////////////////////////////////
    //��Ϣ
    int PackageInfo(uint32_t msgId, NFDataPackage &packet);
    //����
    int ItemSort(uint32_t msgId, NFDataPackage &packet);
    //����
    int ItemSell(uint32_t msgId, NFDataPackage &packet);
    //ʹ��
    int ItemUse(uint32_t msgId, NFDataPackage &packet);
    //��չ
    int ExpandReq(uint32_t msgId, NFDataPackage &packet);
public:
    virtual uint32_t GetLastErrorCode();
    virtual uint32_t GetPackageNotSpaceErrorCode(uint32_t nPackageType);
    
    ///////////////////////////////// IPackage   /////////////////////////////////////////
    //��ȡָ����Ʒ����
    virtual int64_t GetItemNum(uint64_t nItemID, int64_t &nUnBindNum, int64_t &nBindNum);
    //�Ƿ������Ʒ
    virtual bool HasItem(LIST_ITEM &lstItem);
    //�Ƴ���Ʒ
    virtual bool RemoveItem(uint64_t nItemID, int64_t nNum, SCommonSource &sourceParam, int8_t byBind = (int8_t) EBindState::EBindState_all);
    //�Ƴ���Ʒ sourceParam : ��Ʒ�Ƴ���Դ����ʱû�У�
    virtual bool RemoveItem(LIST_ITEM &lstItem, SCommonSource &sourceParam);
    //�Ƴ���Ʒ
    virtual bool RemoveItem(uint32_t nPackageType, LIST_ITEM &lstItem, SCommonSource &sourceParam);
    //�Ƴ���Ʒ index,num sourceParam : ��Ʒ�Ƴ���Դ����ʱû�У�
    virtual bool RemoveItemByIndex(uint32_t nPackageType, uint16_t nIndex, int64_t nNum, SCommonSource &sourceParam);
    //�Ƴ���Ʒ index-num nGetType : ��Ʒ�Ƴ���Դ����ʱû�У�
    virtual bool RemoveItem(uint32_t nPackageType, MAP_UINT16_INT64 &mapIndexItem, SCommonSource &sourceParam);
    
    //�Ƿ��������Ʒ
    virtual bool CanAddItem(LIST_ITEM &lstItem);
    //�ܷ������Ʒ
    virtual bool CanAddItem(VEC_ITEM_PROTO_EX &vecProtoItemsEx);
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
    //��ձ���������Ʒ
    virtual bool ClearPackage();
    
    //��չ����
    virtual int32_t Expand(int32_t &nNum);
    //��չ�ֿ�
    virtual int32_t ExpandStorage(int32_t &nNum);
    
    //��ȡ����λ����Ʒ
    virtual NFItem *GetItem(uint16_t nIndex);
    virtual NFItem *GetItem(uint32_t packageType, uint16_t nIndex);
    virtual NFItem *GetPackageItemByIndex(uint32_t nPackageType, uint32_t nIndex);
    // 	//��ȡ��һ��ָ��ID����Ʒ
    virtual NFItem *GetFirstItemById(uint32_t nPackageType, uint64_t item_id);
    //�Ƴ�����ָ�����͵���Ʒ������ tem.xlsx ���� subType �ֶ� �Ƴ���Ʒ��
    virtual bool RemoveAllByType(int32_t itemSubType, SCommonSource &sourceParam);
    virtual bool RemoveAllByType(uint32_t nPackageType, int32_t itemSubType, SCommonSource &sourceParam);
    //ʹ����Ʒ �������޹ص�ʹ����Ʒ�ӿ� nNum:ʹ�õ�����,protoArg:ʹ����Ʒ��Ҫ�Ĳ���
    virtual int32_t UseItem(NFItem *pItem, int64_t &nNum, proto_ff::UseItemArgProto &protoArg);
    //ʹ����Ʒ nIndex:��Ʒ���ڱ�������,nNum:ʹ�õ�����,protoArg:ʹ����Ʒ��Ҫ�Ĳ���
    virtual int32_t UseItem(uint16_t nIndex, int64_t &nNum, proto_ff::UseItemArgProto &protoArg);
    //�ֿ�򱳰�����ȫ������
    virtual void AllOpenGrid(uint32_t nPackageType);
    
    //��ȡ��ǩҳ�ո��������б� nNum:Ϊ0��ʾ��ǩҳ���еĿո��ӣ�����0ʱ ֻ���� nNum ���ո���
    bool GetEmptyGrid(uint32_t nPackageType, VEC_UINT16 &vecGrid);
    int32_t GetEmptyGrid(uint32_t nPackageType);
    //������Ʒ
    uint16_t SetItemByIndex(uint32_t nPackageType, uint16_t nIndex, const NFItem& item);
    uint16_t SetItemByIndex(uint32_t nPackageType, uint16_t nIndex, const NFItem* pItem);
    //����
    void ItemSortByPackageType(uint32_t nPackageType);
    //���±�����Ʒ
    void SendUpdatePackageByIndex(uint32_t nPackageType, NFItem *pItem, bool bDel);
    
    //��ӱ���������Ϣ
    bool AddPackageUpdateInfo(NFItem *pItem, proto_ff::NotifyPackageUpdate &ret, bool bDel = false);
    void UpdatePackage(uint32_t nPackageType, proto_ff::NotifyPackageUpdate &ret);


private:
    
    //ͨ���������ͻ�ȡ��Ʒ byPackageType:�������� ��� EPackageTypeö��, nIndex:��Ӧ�������͸�������
    NFItem *GetItemByPackageType(int8_t byPackageType, uint16_t nIndex);
    // ͨ����������������Ʒ :�������� ��� EPackageTypeö�� , nIndex:��Ӧ�������͸�������
    uint16_t SetItemByPackageType(int8_t byPackageType, uint16_t nIndex, const NFItem& item);
    
    //ͨ�����������ж��Ƿ�����Ч������
    bool ValidIndexByPackageType(uint16_t nIndex, int8_t byPackageType);
    //�Ƿ�����Ч�İ�������
    bool ValidPackageType(int8_t byPackageType);
    
    //��һ�ε�½
    void FirstLogin();
    ////////////////////////////////  �ֿ�ӿ� //////////////////////////////////////////
    //�ֿ���������룬�Ƴ���
    void StorageOperate(const char *data, uint32_t len);

private:
    //���ø�����Ϣ
    bool SetUpdateItemInfo(NFItem *pItem, proto_ff::ItemProtoInfo *protoItemInfo, bool bDel = false);
    
    //���������¼
    void ClearIdxRecord();
    //get ������¼
    SET_UINT16 &GetIdxRecord();

public:
    virtual void OnUseItem(uint64_t nItemId, uint32_t num);
    bool AddItemAttr(uint64_t nItemId, uint32_t nTimes);
    NFPackageBag* GetPackageByType(uint32_t nPackageType);
private:
    
    //���ñ������ݱ�����
    void SetPackageSaveFlag(bool saveFlag);
    bool IsValidPackage(uint32_t nPackageType);
    NFPackageBag *GetPackageByItemID(uint64_t nItemID);
    NFPackageBag *GetItemPackageBag(uint64_t nItemID);
    uint32_t GetItemPackageType(uint64_t nItemID);
private:
    NFBagPage<NFEquip, proto_ff::EPackageType_Common, COMMON_PACKAGE_INIT_GRID_NUM, COMMON_PACKAGE_MAX_GRID_NUM> m_commonBag;
    NFBagPage<NFEquip, proto_ff::EPackageType_Storage, STORAGE_PACKAGE_INIT_GRID_NUM, STORAGE_PACKAGE_MAX_GRID_NUM> m_storeageBag;
    NFBagPage<NFItem, proto_ff::EPackageType_MountEgg, MOUNT_PACKAGE_INIT_GRID_NUM, MOUNT_PACKAGE_MAX_GRID_NUM> m_mountEggBag;
    NFBagPage<NFItem, proto_ff::EPackageType_PetEgg, EGG_BAG_SIZE, EGG_BAG_SIZE> m_petEggBag;
    NFBagPage<NFDeityEquip, proto_ff::EPackageType_DeityEquip, DEITY_EQUP_PACKAGE_INIT_GRID_NUM, DEITY_EQUIP_PACKAGE_MAX_GRID_NUM> m_deityBag;
    NFBagPage<NFItem, proto_ff::EPackageType_HandBook, HANDBOOK_PACKAGE_INIT_GRID_NUM, HANDBOOK_PACKAGE_MAX_GRID_NUM> m_handBookBag;
    NFBagPage<NFBeastEquip, proto_ff::EPackageType_BeastEquip, BEAST_EQUP_PACKAGE_INIT_GRID_NUM, BEAST_EQUIP_PACKAGE_MAX_GRID_NUM> m_beastEquipBag;
    NFBagPage<NFLongHunEquip, proto_ff::EPackageType_Longhun, LONG_HUN_PACKAGE_INIT_GRID_NUM, LONG_HUN_PACKAGE_MAX_GRID_NUM> m_longHunEquipBag;
    NFBagPage<NFEquip, proto_ff::EPackageType_FindTreasure, COMMON_PACKAGE_INIT_GRID_NUM, COMMON_PACKAGE_MAX_GRID_NUM> m_findTreasureBag;
    NFBagPage<NFEquip, proto_ff::EPackageType_BigDragon, COMMON_PACKAGE_INIT_GRID_NUM, COMMON_PACKAGE_MAX_GRID_NUM> m_bigDragonBag;
    NFBagPage<NFShengjiEquip, proto_ff::EPackageType_shenji_aq, COMMON_PACKAGE_INIT_GRID_NUM, COMMON_PACKAGE_MAX_GRID_NUM> m_shengjiAqBag;
    NFBagPage<NFShengjiEquip, proto_ff::EPackageType_shenji_lj, COMMON_PACKAGE_INIT_GRID_NUM, COMMON_PACKAGE_MAX_GRID_NUM> m_shengjiLjBag;
    NFBagPage<NFGodEvilEquip, proto_ff::EPackageType_GodEvil, COMMON_PACKAGE_INIT_GRID_NUM, COMMON_PACKAGE_MAX_GRID_NUM> m_godEvilBag;
    NFBagPage<NFStarEquip, proto_ff::EPackageType_star, STAR_HUN_PACKAGE_MAX_GRID_NUM, STAR_HUN_PACKAGE_MAX_GRID_NUM> m_starBag;
    NFBagPage<NFTurnEquip, proto_ff::EPackageType_turn, TURN_PACKAGE_MAX_GRID_NUM, TURN_PACKAGE_MAX_GRID_NUM> m_turnBag;
    NFBagPage<NFMountKunEquip, proto_ff::EPackageType_MountKun, MOUNT_KUN_PACKAGE_MAX_GRID_NUM, MOUNT_KUN_PACKAGE_MAX_GRID_NUM> m_mountKunBag;
    NFBagPage<NFYaoHunEquip, proto_ff::EPackageType_YaoHun, COM_BAG_SIZE, COM_BAG_SIZE> m_yaoKunBag;
    NFBagPage<NFMoFaEquip, proto_ff::EPackageType_MoFa, COM_BAG_SIZE, COM_BAG_SIZE> m_moFaBag;
    NFBagPage<NFEquip, proto_ff::EPackageType_hungu, COM_BAG_SIZE, COM_BAG_SIZE> m_hunguBag;
    NFBagPage<NFEquip, proto_ff::EPackageType_rune, COM_BAG_SIZE, COM_BAG_SIZE> m_runeBag;
    
    uint32_t m_nLastErrorCode;
    SET_UINT16 m_setIdxRecord;                        //��¼ÿ�μ���Ʒ ���������ӵĸ�����������ÿ�μ���Ʒ֮ǰ�����
};