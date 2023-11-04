// -------------------------------------------------------------------------
//    @FileName         :    NFMissionPart.h
//    @Author           :    gaoyi
//    @Date             :    23-10-20
//    @Email			:    445267987@qq.com
//    @Module           :    NFMissionPart
//
// -------------------------------------------------------------------------

#pragma once


#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFShmCore/NFShmObj.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "Part/NFPart.h"
#include "NFLogicCommon/NFLogicShmTypeDefines.h"
#include "NFComm/NFShmCore/NFISharedMemModule.h"
#include "NFLogicCommon/NFMissionDefine.h"
#include "DescStoreEx/TaskDescEx.h"

#define PLAYER_TRACK_MISSION_MAX_MISSION_COUNT MISSION_MAX_ACCEPT_NUM*2

class NFMissionPart : public NFShmObjTemplate<NFMissionPart, EOT_LOGIC_PART_ID+PART_MISSION, NFPart>
{
public:
public:
    // eventtype - level - dynamicid
    typedef NFShmHashMap<uint32_t, NFShmHashMap<int32_t, NFShmHashSet<uint64_t, PLAYER_TRACK_MISSION_MAX_MISSION_COUNT>,
                                                PLAYER_TRACK_MISSION_MAX_MISSION_COUNT>, 10> EventTabal;
    
    
    typedef NFShmHashMap<uint64_t, MissionTrack, PLAYER_TRACK_MISSION_MAX_MISSION_COUNT> PlayerTrackMissionMap;
    typedef NFShmHashMap<int32_t, DyMissionTrack, NF_MISSION_TYPE_MAX_COUNT> PlayerDyMissionTrackMap;
public:
    NFMissionPart();

    virtual ~NFMissionPart();

    int CreateInit();

    int ResumeInit();
public:
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
    virtual int LoadFromDB(const proto_ff::RoleDBData& data);
    
    /**
     * @brief �������г�ʼ������
     * @return
     */
    virtual int InitConfig(const proto_ff::RoleDBData& data) { return 0; }
    
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
    virtual int OnLogin(proto_ff::PlayerInfoRsp& playerInfo) { return 0; }
    
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
    virtual int FillFacadeProto(proto_ff::RoleFacadeProto& outproto) { return 0; }
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
    /**
     * @brief �����������
     * @param notify
     */
    void CheckTrunkMission(bool notify = true);

public: //��Ϣ�ص��ӿ�
    /**
     * @brief NPC��ȡ����
     * @param msgId
     * @param packet
     */
    int HandleNpcAcceptMission(uint32_t msgId, NFDataPackage &packet);
    
    /**
     * @brief �ύ����
     * @param msgId
     * @param packet
     */
    int HandleSubmitMission(uint32_t msgId, NFDataPackage &packet);
    
    /**
     * @brief NPC�Ի�,Ѱ�˽ӿ�
     * @param msgId
     * @param packet
     */
    int HandleTalkWithNpc(uint32_t msgId, NFDataPackage &packet);
    
    /**
     * @brief �ύ������Ʒ
     * @param msgId
     * @param packet
     */
    int HandleSubmitMissionGoods(uint32_t msgId, NFDataPackage &packet);
    
    /**
     * @brief һ���������
     * @param msgId
     * @param packet
     */
    int HandleOnekeyFinishMission(uint32_t msgId, NFDataPackage &packet);
    
    /**
     * @brief �������
     * @param msgId
     * @param packet
     */
    int HandleOnceFinishMission(uint32_t msgId, NFDataPackage &packet);
    
    /**
     * @brief ��ȡ���⽱��
     * @param msgId
     * @param packet
     */
    int HandleMissionRecvSpecialReward(uint32_t msgId, NFDataPackage &packet);
    
    /**
     * @brief ���������б���Ϣ
     * @param msgId
     * @param packet
     */
    int SendMissionList(uint32_t msgId, NFDataPackage &packet);
    
    void SendMissionInfo();
public:
    /**
     * @brief ��ȡ����
     * @param missionId
     * @param notify
     * @return
     */
    int32_t OnAccept(uint64_t missionId, bool notify);
    
    /** ��ȡ����
     * @brief
     * @param missionId
     * @param kind
     */
    void OnAcceptType(uint64_t missionId, uint32_t kind);
    
    /**
     * @brief ��ȡ�ɽ������б�(��֧��)
     * @param missionId
     * @return
     */
    int32_t CanAccept(uint64_t missionId);
    
    /**
     * @brief �Ƿ�ƥ���ȡ���������
     * @param cond
     * @param param
     * @return
     */
    int32_t CanMatchAcceptCond(MissionInfo *pMissionInfo);

public:
    /**
     * @brief ��ȡ��̬����
     * @param missionId
     * @param notify
     * @return
     */
    int32_t OnAcceptDy(uint64_t missionId, bool notify);
    
    /**
     * @brief ��ȡ����(��̬����)
     * @param dymissionId
     * @param missionType
     */
    void OnAcceptDyType(uint64_t dymissionId, int32_t missionType);
    
    /**
     * @brief �Ƿ��ܽ�ȡ��̬���� dyCfgId:��̬���������ID
     * @param dyCfgId
     * @return
     */
    int32_t CanAcceptDy(uint64_t dyCfgId);
    
    /**
     * @brief
     * @param pDyMissionInfo
     * @return
     */
    int32_t CanAcceptDy(const DyMissionInfo *pDyMissionInfo);
    
    /**
     * @brief ��Ӷ�̬�����ȡ����
     * @param missionType
     * @param count
     */
    void OnAddAcceptDyCount(int32_t missionType, uint32_t count);

public:
    /////////////////////////////////////////��ͨ����ӿ�/////////////////////////////////////////////
    /**
     * @brief ͨ���������ͽ�ȡ����
     * @param missionType
     * @param notify
     * @return
     */
    int32_t AcceptMissionByType(int32_t missionType, bool notify);
    
    /**
     * @brief �ܷ�ͨ���������ͽ�ȡ����
     * @param missionType
     * @return
     */
    int32_t CanAcceptMissionByType(int32_t missionType);
    
    /**
     * @brief ͨ�����������������,����������������� notify���Ƿ���Ҫ֪ͨ�ͻ���ɾ������
     * @param missionType
     * @param notify
     * @return
     */
    int32_t ClearMissionByType(int32_t missionType, bool notify);


public:
    /**
     * @brief ��ȡ���������
     * @param missionType
     * @param level
     * @return
     */
    int32_t DyMaxCount(int32_t missionType, int32_t level);
    
    /**
     * @brief �����������ͻ�ȡ��̬�����Ѿ���ȡ����
     * @param missionType
     * @return
     */
    uint32_t GetDyMissionAceeptCnt(int32_t missionType);
    
    /**
     * @brief �������(��̬����)
     * @param dymissionId
     * @param dymissionType
     */
    void OnFinishDy(uint64_t dymissionId, uint32_t dymissionType);
    
    /**
     * @brief �����������ͻ�ȡ��̬�����¼��Ϣ
     * @param missionType
     * @return
     */
    DyMissionTrack *GetDyMissionTrack(int32_t missionType);
    
    /**
     * @brief ��̬������
     * @param missionType
     * @param missionId
     * @param missionReward
     * @return
     */
    int32_t OnAddDyMissionReward(int32_t missionType, uint64_t missionId, SMissionReward &missionReward);
    
    /**
     * @brief ���¶�̬�����ȡ����
     * @param setMissionType
     */
    void NotifyDyAcceptCount(SET_UINT32 &setMissionType);
    
    /**
     * @brief �Ƿ�����Ч�Ķ�̬����ID
     * @param dyMissionId
     * @return
     */
    bool ValidDyMissionId(uint64_t dyMissionId);
    
    /**
     * @brief ����һ����̬����ID
     * @return
     */
    uint64_t AllocNewDyMisssionId();
    
    /**
     * @brief ����һ����̬����ID
     * @param dyMissionId
     */
    void FreeDyMissionId(uint64_t dyMissionId);
    
    /**
     * @brief ���������������һ������ID
     * @param missionType
     * @return
     */
    uint64_t DyRandMissionId(int32_t missionType);

public:
    /** ����ύ�б������Ƿ���ָ�����͵�����
     * @brief
     * @param missionType
     * @return
     */
    bool HaveRecentSubmit(int32_t missionType);
    
    /**
     * @brief  �����������ͻ�ȡ��ǰ�����б��д��ڵ�����
     * @param missionType
     * @return
     */
    int32_t MissionNumByType(int32_t missionType);
    
    /**
     * @brief �������Ƿ��Ѿ����չ���
     * @param missionId
     * @return
     */
    bool HaveAccept(const uint64_t &missionId);
    
    /**
     * @brief �Ƿ��Ѿ��ύ��������
     * @param missionId
     * @return
     */
    bool HaveSubmited(uint64_t missionId);

public:
    int32_t OnExtractCond(MissionInfo *pMissionInfo, MissionTrack *pMissionTrack);
    
    /**
     * @brief ����Ԥ�ж�(��Щ������ȡ����ʱ���Ѿ������)
     * @param cond
     */
    void OnPreUpdateProgress(ItemInfo &cond);
    
    /**
     * @brief ������������
     * @param data
     * @param cond
     * @param notify
     */
    void OnUpdateCondProcess(const ExecuteData &data, ItemInfo &cond, bool &notify);
    
    /**
     * @brief ��ȡ��̬��������
     * @param pMissionInfo
     * @param pMissionTrack
     * @param preUpdate
     * @return
     */
    int32_t OnExtractDyCond(const DyMissionInfo *pMissionInfo, MissionTrack *pMissionTrack, bool preUpdate = true);
    
    /**
     * @brief ������������
     * @param pDyMissionInfo
     * @param pDyConditionInfo
     * @param cond
     * @param textId
     * @return
     */
    int32_t OnGeneralCond(const DyMissionInfo *pDyMissionInfo, const DyConditionInfo *pDyConditionInfo, ItemInfo &cond, uint64_t &textId);
    
    /**
     * @brief ��ȡ��̬�������Խ���
     * @param pNewMissinTrack
     * @return
     */
    int32_t OnExtractDyAttrReward(MissionTrack *pNewMissinTrack);

public:
    /**
     * @brief ���ӷ�����ʱ����Ʒ����
     * @param pMissionTrack
     * @return
     */
    int32_t AddReward(uint64_t missionId, int32_t kind, TASK_REWARD &reward, float ration = 1);
    
    /**
     * @brief �Ƿ�������������
     * @param pPlayer
     * @param reward
     * @param param
     * @param lstOutItem
     * @return
     */
    bool CanAddReward(uint64_t missionId, int32_t kind, TASK_REWARD &reward, LIST_ITEM &lstOutItem);

public:
    /**
     * @brief �Ƴ�����
     * @param pMissionInfo
     * @return
     */
    int32_t RemoveMission(MissionInfo *pMissionInfo);
    
    /**
     * @brief �Ƴ�����
     * @param pMissinTrack
     * @param pMissionInfo
     * @return
     */
    int32_t RemoveMission(MissionTrack *pMissinTrack, MissionInfo *pMissionInfo);
    
    /**
     * @brief �Ƴ���̬����
     * @param dymissionId
     * @param notify
     * @return
     */
    int32_t RemoveDyMission(uint64_t dymissionId, bool notify);

public:
    /**
     * @brief ���½���
     * @param missionId
     * @param data
     * @return
     */
    int32_t OnUpdateProgress(uint64_t missionId, const ExecuteData &data);
    
    /**
     * @brief �����������
     * @param missionId
     */
    void UpdateMissionProgress(uint64_t missionId);
    
    /** ɾ������
     * @brief
     * @param dymissionId
     */
    void NotifyDelMission(uint64_t dymissionId);
    
    /**
     * @brief �Ƴ�������Ʒ
     * @param pMissionTrack
     * @return
     */
    int32_t OnDelMissionItem(MissionTrack *pMissionTrack);

public:
    /**
     * @brief ����������
     * @param pMissionTrack
     * @param progressLev
     * @return
     */
    int32_t OnAddMissionDrop(MissionTrack *pMissionTrack, int32_t progressLev);
    
    /**
     * @brief ����������
     * @param dymissionId
     * @param monsId
     * @param dropId
     * @param boxId
     * @param progressLev
     * @return
     */
    bool AddMissionDrop(uint64_t dymissionId, uint64_t monsId, uint64_t dropId, uint64_t boxId, int32_t progressLev);
    
    /**
     * @brief ɾ���������
     * @param dymissionId
     * @param monsId
     * @return
     */
    bool DelMissionDrop(uint64_t dymissionId, uint64_t monsId);
    
    /**
     * @brief ��ȡ�������
     * @param monsterId
     * @return
     */
    MissionDropMap *GetMissionDrop(uint64_t monsterId);
    
    /**
     * @brief �Ƴ��������
     * @param pMissionTrack
     * @return
     */
    int32_t OnDelMissionDrop(MissionTrack *pMissionTrack);

public:
    /**
     * @brief �Ƴ������м���Ʒ
     * @param pMissionTrack
     */
    int RemoveReward(MissionTrack *pMissionTrack);
    
    /**
     * @brief �Ƴ���ȡ����ʱ���ŵ���Ʒ
     * @param pPlayer
     * @param missionId
     * @param reward
     * @return
     */
    int RemoveReward(uint64_t missionId, TASK_REWARD &reward);

public://�����¼�����ӿ�
    /**
     * @brief ע������¼���������ɹ���ע��
     * @param eventType
     * @param missionId
     * @param progressLev
     */
    void RegisterEvent(uint32_t eventType, uint64_t missionId, int32_t progressLev);
    
    /**
     * @brief �Ƴ��������ע��������¼�
     * @param missionId
     */
    void RemoveEvent(uint64_t missionId);
    
    /**
     * @brief ���������¼�������
     * @param eventType
     * @param data
     * @param dynamicId
     */
    void OnEvent(uint32_t eventType, const ExecuteData &data, uint64_t dynamicId = 0);

public:
    /**
     * @brief �ύ����
     * @param pPlayer
     * @param missionId
     * @param selidx
     * @return
     */
    int32_t OnSubmit(uint64_t missionId, uint32_t selidx);
    
    /**
     * @brief �ύ����
     * @param missionId
     * @param premissionId
     * @param kind
     */
    void OnSubmit(uint64_t missionId, uint64_t premissionId, uint32_t kind);
    
    /**
     * @brief ����ȡ������ǰ���������֮��Ŀɽ�����
     * @param missionId
     * @param notify
     */
    void CheckPreAcceptMission(uint64_t missionId, bool notify = true);

public:
    /**
     * @brief �ύ��̬����
     * @param dymissionId
     * @return
     */
    int32_t OnSubmitDy(uint64_t dymissionId);
    
    
    /**
     * @brief �ύ����(��̬����)
     * @param dymissionId
     * @param missionType
     */
    void OnSubmitDy(uint64_t dymissionId, int32_t missionType);
public:
    /**
     * @brief ���ŵ��ð��
     * @param count
     */
    void OnFinishLoopMission(int32_t count);
private:
    PlayerTrackMissionMap _playerTrackMissionMap;    //��ǰ�����б�
    NFShmHashMap<int32_t, NFShmHashSet<uint64_t, 100>, NF_MISSION_TYPE_MAX_COUNT> _mapRecentSubmit;        //����ύ������
    NFShmHashSet<uint64_t, NF_MISSION_TYPE_MAX_MISSION_COUNT> _setAlreadySubmit;        //�Ѿ��ύ������
    //��̬����
    PlayerDyMissionTrackMap _mapDyMissionTrack;        //��̬��������
    MissionAllDropMap _mapMissionAllDrop;        //�������
    //
    EventTabal _eventTabal;            //�����¼���
    NFShmVector<bool, MISSION_MAX_DYNAMIC_ALLOC + 1> _aryDyIdAlloc; //��̬����ID����
};