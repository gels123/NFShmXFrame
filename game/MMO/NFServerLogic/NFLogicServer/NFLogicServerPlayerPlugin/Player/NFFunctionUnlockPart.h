// -------------------------------------------------------------------------
//    @FileName         :    NFFunctionUnlockPart.h
//    @Author           :    gaoyi
//    @Date             :    23-10-20
//    @Email			:    445267987@qq.com
//    @Module           :    NFFunctionUnlockPart
//
// -------------------------------------------------------------------------

#pragma once


#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFShmCore/NFShmObj.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "Part/NFPart.h"
#include "NFLogicCommon/NFLogicShmTypeDefines.h"
#include "NFComm/NFShmCore/NFISharedMemModule.h"
#include "E_Functionunlock_s.h"

class NFMissionPart;
class NFFunctionUnlockPart : public NFShmObjTemplate<NFFunctionUnlockPart, EOT_LOGIC_PART_ID+PART_FUNCTIONUNLOCK, NFPart>
{
public:
    NFFunctionUnlockPart();

    virtual ~NFFunctionUnlockPart();

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
    
    virtual int OnExecute(uint32_t serverType, uint32_t nEventID, uint32_t bySrcType, uint64_t nSrcID, const google::protobuf::Message* pMessage);
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
    virtual int InitConfig(const proto_ff::RoleDBData& data);
    
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
    virtual int OnLogin(proto_ff::PlayerInfoRsp& playerInfo);
    
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
    virtual int DailyZeroUpdate();
    
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
    int OnHandleGetReward(uint32_t msgId, NFDataPackage &packet);
public:
    //�������й���
    bool UnlockAllFunc();
    //�����Ƿ����  FunctionUnlock_ID_TYPE
    bool isFunctionUnlock(uint64_t functionId);
    void Unlock(int64_t functionId);
    void UnlockSendAdd(int64_t functionId);
private:
    //������
    void checkUnlock(uint32_t nType, int64_t nValue);
    //�������ݵ��ͻ���
    void sendFunctionUnlockInfo(VEC_UINT64 *pList);
    //�Ƴ���������
    void DelUnlock(uint64_t unlockid);
    //��ӽ�������
    void AddUnlock(uint64_t unlockid);
    //������еĹ����Ƿ�ɽ���
    bool CheckALLFunctions(bool sync = true);
    //��ȡתְ�ȼ�
    uint32_t GetOccupationGrade(uint64_t occupationId);
    //��鹦���Ƿ����
    bool CheckUnlock(int32_t Lev, int32_t occupationLev, uint32_t opendays, int32_t viplev, int32_t totalrmb, NFMissionPart *pMissionPart, const  proto_ff_s::E_FunctionunlockFunctionunlock_s &cfg);
    //��������
    uint32_t OpenDays();
private:
    NFShmHashMap<uint64_t, bool, DEFINE_SHEET_FUNCTIONUNLOCKFUNCTIONUNLOCK_E_FUNCTIONUNLOCKFUNCTIONUNLOCK_LIST_MAX_NUM> m_mapUnLock; //����ѽ����Ĺ���
};