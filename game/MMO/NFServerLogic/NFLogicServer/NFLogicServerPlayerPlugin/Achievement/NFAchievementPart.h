// -------------------------------------------------------------------------
//    @FileName         :    NFAchievementPart.h
//    @Author           :    gaoyi
//    @Date             :    23-10-20
//    @Email			:    445267987@qq.com
//    @Module           :    NFAchievementPart
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFShmCore/NFShmObj.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmCore/NFISharedMemModule.h"
#include "Part/NFPart.h"
#include "NFLogicCommon/NFLogicShmTypeDefines.h"

class NFAchievementPart : public NFShmObjTemplate<NFAchievementPart, EOT_LOGIC_PART_ID + PART_ACHIEVEMENT, NFPart>
{

public:
    NFAchievementPart();
    
    virtual ~NFAchievementPart();
    
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
    virtual int Init(NFPlayer* pMaster, uint32_t partType, const proto_ff::RoleDBData& dbData);
    
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
    virtual int LoadFromDB(const proto_ff::RoleDBData& data) { return 0; }
    
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
    virtual int SaveDB(proto_ff::RoleDBData& dbData) { return 0; }
    
    /**
     * @brief ��½���
     * @return
     */
    virtual int OnLogin() { return 0; }
    
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
    virtual int OnHandleClientMessage(uint32_t msgId, NFDataPackage& packet);
    
    /**
     * @brief �������Է���������Ϣ
     * @param unLinkId
     * @param packet
     * @return
     */
    virtual int OnHandleServerMessage(uint32_t msgId, NFDataPackage& packet);
public:
    /**
     * @brief
     * @param msgId
     * @param packet
     * @return
     */
    int OnHandleAchievementInfoReq(uint32_t msgId, NFDataPackage& packet);
public:
    void SendAchievementInfo();
public:
    void OnCommonAddNum(int32_t num, int32_t eventType) { }
    
    void OnCommonAddNum(int32_t num, int32_t param1, int32_t eventType) { }
    
    void OnCommonAddNumWithLess(int32_t num, int32_t param1, int32_t eventType) { }
    
    void OnCommonAddNumWithLess(int32_t num, int32_t param1, int32_t param2, int32_t eventType) { }
    
    void OnCommonAddNumWithLess(int32_t num, int32_t param1, int32_t param2, int32_t param3, int32_t eventType) { }
    
    void OnCommonFinishNum(int32_t num, int32_t eventType) { }
    
    void OnCommonFinishNum(int32_t num, int32_t param, int32_t eventType) { }
    
    void OnCommonFinishNum(int32_t num, int32_t param1, int32_t param2, int32_t eventType) { }
    
    void OnCommonFinishNum(int32_t num, int32_t param1, int32_t param2, int32_t param3, int32_t eventType) { }
    
    void OnCommonFinishNumWithLess(int32_t num, int32_t param, int32_t eventType) { }
    
    void OnCommonFinishNumWithLess(int32_t num, int32_t param1, int32_t param2, int32_t eventType) { }
    
    void OnCommonFinishNumWithLess(int32_t num, int32_t param1, int32_t param2, int param3, int32_t eventType) { }
    
    void OnCommonClear(int32_t eventType) { }
private:
    std::unordered_map<int64_t, proto_ff::OneAchievementData> m_achMap;
    std::unordered_map<int64_t, proto_ff::OneAchievementData> m_achTotalMap;
};