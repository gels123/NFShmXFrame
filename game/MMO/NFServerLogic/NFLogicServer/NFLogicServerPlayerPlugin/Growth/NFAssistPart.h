// -------------------------------------------------------------------------
//    @FileName         :    NFAssistPart.h
//    @Author           :    gaoyi
//    @Date             :    23-10-20
//    @Email			:    445267987@qq.com
//    @Module           :    NFAssistPart
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFShmCore/NFShmObj.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "Part/NFPart.h"
#include "NFLogicCommon/NFLogicShmTypeDefines.h"
#include "NFComm/NFShmCore/NFISharedMemModule.h"

class NFAssistPart : public NFShmObjTemplate<NFAssistPart, EOT_LOGIC_PART_ID + PART_ASSIST, NFPart>
{
public:
    NFAssistPart();
    
    virtual ~NFAssistPart();
    
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
    virtual int OnHandleClientMessage(uint32_t msgId, NFDataPackage& packet);
    
    /**
     * @brief �������Է���������Ϣ
     * @param unLinkId
     * @param packet
     * @return
     */
    virtual int OnHandleServerMessage(uint32_t msgId, NFDataPackage& packet);

public:
    void NotifyData();

public:
    int32_t m_dayPrestige = 0;            //���ջ�õ�����
    int32_t m_sendNum = 0;                //����������
    int32_t m_helpNum = 0;                //����Ԯ����
    int32_t m_allSendNum = 0;            //����������ܴ�����ͳ����)
    int32_t m_allHelpNum = 0;            //���Ԯ�����ܴ���(ͳ����)
    uint64_t m_resetTime = 0;            //����ʱ��
    int32_t m_recvSendGiftNum = 0;        //������ȡ��Ԯ�������
    int32_t m_recvHelpGiftNum = 0;        //������ȡԮ���������
    SET_INT32 m_rewardIds;                //����ȡ�������id
    
    MAP_INT64_INT32 m_thanks;                //��л��CID ״̬ cid->�Ƿ������ȡ���
};