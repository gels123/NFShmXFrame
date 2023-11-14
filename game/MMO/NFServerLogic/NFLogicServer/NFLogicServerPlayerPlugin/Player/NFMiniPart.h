// -------------------------------------------------------------------------
//    @FileName         :    NFMiniPart.h
//    @Author           :    gaoyi
//    @Date             :    23-10-20
//    @Email			:    445267987@qq.com
//    @Module           :    NFMiniPart
//
// -------------------------------------------------------------------------

#pragma once


#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFShmCore/NFShmObj.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "Part/NFPart.h"
#include "NFLogicCommon/NFLogicShmTypeDefines.h"
#include "NFComm/NFShmCore/NFISharedMemModule.h"

#define MAX_ARENA_CHALL_RESULT_NUM 50

enum HangUpDropItem
{
    HANG_UP_DROP_NORMAL_TYPE = 1, //1Ϊ��ͨ���͵��߳�ȡ��
    HANG_UP_DROP_FUSHI_TYPE = 2, //2Ϊ��Ʒ��ʯ���߳�ȡ��
};

/**
 * @brief ڤ������
 */
class HangUpData
{
public:
    HangUpData()
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
    
    ~HangUpData()
    {
    
    }
    
    int CreateInit()
    {
        m_hangup_time = 0;		//���ߵ����߹һ�ʱ��
        m_login_surplus_hangup_time = 0;
        m_add_hangup_exp = 0;
        m_hangup_before_lv = 0;
        m_hangup_after_lv = 0;
        m_hangup_before_fight = 0;
        m_hangup_after_fight = 0;
        
        m_online_hangup_time = 0; //���߹һ�ʱ��
        m_add_online_hangup_exp = 0;
        return 0;
    }
    
    int ResumeInit()
    {
        return 0;
    }
public:
    // ڤ������
    int32_t m_hangup_time;		//���ߵ����߹һ�ʱ��
    int32_t m_login_surplus_hangup_time;
    uint64_t m_add_hangup_exp;
    int32_t m_hangup_before_lv;
    int32_t m_hangup_after_lv;
    uint64_t m_hangup_before_fight;
    uint64_t m_hangup_after_fight;
    MAP_INT64_INT32  m_dropItem;
    
    int32_t m_online_hangup_time; //���߹һ�ʱ��
    MAP_INT64_INT32  m_onlineDropItem; //���߹һ�����
    uint64_t m_add_online_hangup_exp;
};

class NFMiniPart : public NFShmObjTemplate<NFMiniPart, EOT_LOGIC_PART_ID+PART_MINI, NFPart>
{
public:
    NFMiniPart();

    virtual ~NFMiniPart();

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
     * @brief ��������ڤ������
     * @return
     */
    int OnHandleLoginHangUpDataReq(uint32_t msgId, NFDataPackage &packet);
public:
    void SendHangupData();
private:
    /**
     * @brief ڤ������
     */
    HangUpData m_hangUpData;
};