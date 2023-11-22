// -------------------------------------------------------------------------
//    @FileName         :    NFRankMgr.h
//    @Author           :    gaoyi
//    @Date             :    23-11-21
//    @Email			:    445267987@qq.com
//    @Module           :    NFRankMgr
//
// -------------------------------------------------------------------------

#pragma once

#include <Rank.pb.h>
#include <NFComm/NFShmCore/NFShmPtr.h>
#include <NFComm/NFShmStl/NFShmHashMap.h>
#include <NFLogicCommon/NFRankDefine.h>
#include <ObService/NFSnsObService.h>

#include "NFCommonRank.h"
#include "NFComm/NFCore/NFPlatform.h"
#include "NFLogicCommon/NFLogicShmTypeDefines.h"
#include "NFComm/NFShmCore/NFShmObjTemplate.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmCore/NFShmObj.h"

class NFRankMgr : public NFShmObjTemplate<NFRankMgr, EOT_SNS_SERVICE_ID+SNS_OB_SERVICE_RANK, NFSnsObService>
{
public:
    NFRankMgr();
    
    virtual ~NFRankMgr();
    
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
    //����������а�
    int OnUpdateRankReq(uint32_t msgId, NFDataPackage& packet);
    //�������а�����
    int OnRankInfoReq(uint32_t msgId, NFDataPackage& packet);
public:
    //�������а�����  enRankType
    void UpdateRankData(const proto_ff::GWUpdateRankReq& req);
    void UpdateRankData(uint32_t nType, uint64_t charID, uint64_t nValue);
    //�������а����ݸ��ͻ���
    int SendRankData(uint32_t nType, uint64_t charID, uint64_t selfValue, uint64_t unionId);
    //���ýڵ�proto����
    bool SetRankNodeProtoByCharId(proto_ff::RankNodeData* pNode, uint32_t rankType, uint32_t rank, uint64_t charID, uint64_t value);
    //���ýڵ�proto����
    bool SetRankNodeProtoByUnionId(proto_ff::RankNodeData* pNode, uint32_t rankType, uint32_t rank, uint64_t unionID, uint64_t value);
public:
    virtual int LoadFromDB(const std::string& dbData);

    virtual int SaveToDB(std::string& dbData);

    virtual int InitConfig();

    virtual int LoadAllRank();
public:
    //�������а�����
    NFCommonRank* CreateRankList(uint32_t nType);
    //��ȡĳһ���͵����а����
    NFCommonRank* GetRankList(uint32_t nType);
public:
    NFShmHashMap<uint32_t, NFShmPtr<NFCommonRank>, RANK_TYPE_MAX> m_rankData;
};
