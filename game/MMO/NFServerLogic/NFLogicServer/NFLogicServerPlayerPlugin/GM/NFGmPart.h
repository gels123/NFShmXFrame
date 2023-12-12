// -------------------------------------------------------------------------
//    @FileName         :    NFGmPart.h
//    @Author           :    gaoyi
//    @Date             :    23-12-12
//    @Email			:    445267987@qq.com
//    @Module           :    NFGmPart
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFLogicCommon/NFLogicShmTypeDefines.h"
#include "NFComm/NFShmCore/NFShmObjTemplate.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmCore/NFShmObj.h"
#include "Part/NFPart.h"

class NFGmPart : public NFShmObjTemplate<NFGmPart, EOT_LOGIC_PART_ID + PART_GM, NFPart>
{
public:
    NFGmPart();
    
    virtual ~NFGmPart();
    
    int CreateInit();
    
    int ResumeInit();

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
     * @brief �����������gm��Ϣ
     * @param msgId
     * @param packet
     * @return
     */
    int OnHandleServerGmRsp(uint32_t msgId, NFDataPackage& packet);
    
    /**
     * @brief ����gm����
     * @param cmd
     * @param vecarg
     * @return
     */
    int HandleGMCommand(const string& cmd, const VEC_STRING& vecarg);

public:
    /**
     * @brief ��ȡ����
     * @param vecarg
     * @return
     */
    int GetItem(const VEC_STRING& vecarg);
    
    /**
     * @brief �����ȼ�
     * @param vecarg
     * @return
     */
    int UpgradeLevel(const VEC_STRING& vecarg);
    
    /**
     * @brief ���Ӿ���
     * @param vecarg
     * @return
     */
    int AddExp(const VEC_STRING& vecarg);
    
    /**
     * @brief ���ӽ��
     * @param vecarg
     * @return
     */
    int AddGold(const VEC_STRING& vecarg);
    
    /**
     * @brief ������ʯ
     * @param vecarg
     * @return
     */
    int AddDia(const VEC_STRING& vecarg);
    
    /**
     * @brief ���Ӱ���
     * @param vecarg
     * @return
     */
    int AddBdDia(const VEC_STRING& vecarg);
    
    /**
     * @brief
     * @param vecarg
     * @return
     */
    int AddMagic(const VEC_STRING& vecarg);
    
    /**
     * @brief
     * @param vecarg
     * @return
     */
    int AddHunLing(const VEC_STRING& vecarg);
    
    /**
     * @brief ��������
     * @param vecarg
     * @return
     */
    int AddPrestige(const VEC_STRING& vecarg);
    
    /**
     * @brief ���ӹ���ֵ
     * @param vecarg
     * @return
     */
    int AddContri(const VEC_STRING& vecarg);
    
    /**
     * @brief ����
     * @param vecarg
     * @return
     */
    int GotoMapId(const VEC_STRING& vecarg);
    
    /**
     * @brief ��������
     * @param vecarg
     * @return
     */
    int CreateMonster(const VEC_STRING& vecarg);
    
    /**
     * @brief ��Ӽ���
     * @param vecarg
     * @return
     */
    int AddSkill(const VEC_STRING& vecarg);
};