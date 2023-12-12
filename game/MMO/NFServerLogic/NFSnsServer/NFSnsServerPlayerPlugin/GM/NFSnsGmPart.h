// -------------------------------------------------------------------------
//    @FileName         :    NFSnsGmMgr.h
//    @Author           :    gaoyi
//    @Date             :    23-12-12
//    @Email			:    445267987@qq.com
//    @Module           :    NFSnsGmMgr
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFLogicCommon/NFLogicShmTypeDefines.h"
#include "NFComm/NFShmCore/NFShmObjTemplate.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmCore/NFShmObj.h"
#include "NFGameCommon/NFComTypeDefine.h"
#include "Part/NFSnsPart.h"

class NFSnsGmPart : public NFShmObjTemplate<NFSnsGmPart, EOT_SNS_PART_ID+SNS_PART_GM, NFSnsPart>
{
public:
    NFSnsGmPart();
    
    virtual ~NFSnsGmPart();
    
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
     * @brief ����gm������
     */
    int OnHandleGmReq(NFDataPackage& packet);
    
    /**
     * @brief ����gm������
     */
    int ParseGmHandle(const std::string& cmd, const VEC_STRING& vecarg);
    
    /**
     * @brief ����gm������
     */
    int HandleGmCommand(const std::string& cmd, const VEC_STRING& vecarg);
public:
    /**
     * @brief gm����ʱ��
     * @param vecarg
     * @return
     */
    int GmSetTime(const VEC_STRING& vecarg);
};