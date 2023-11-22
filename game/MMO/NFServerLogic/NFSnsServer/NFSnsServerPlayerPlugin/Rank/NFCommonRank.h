// -------------------------------------------------------------------------
//    @FileName         :    NFCommonRank.h
//    @Author           :    gaoyi
//    @Date             :    23-11-21
//    @Email			:    445267987@qq.com
//    @Module           :    NFCommonRank
//
// -------------------------------------------------------------------------

#pragma once

#include <DBProxy2.pb.h>
#include <NFComm/NFShmStl/NFShmHashMap.h>
#include <NFComm/NFShmStl/NFShmMap.h>
#include <NFComm/NFShmStl/NFShmVector.h>
#include <NFLogicCommon/NFLogicCommon.h>
#include <NFServerComm/NFServerCommon/NFDBGlobalTemplate.h>

#include "NFComm/NFCore/NFPlatform.h"
#include "NFLogicCommon/NFLogicShmTypeDefines.h"
#include "NFComm/NFShmCore/NFShmObjTemplate.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmCore/NFShmObj.h"

//���а��������
#define RANK_MAX_SIZE 100
//���а�ʵ����������
#define RANK_REAL_SIZE 150
//����ȼ�ȡ���а�ǰ����ƽ��ֵ
#define RANK_WORLDLEVEL_RANK 5

class RankNode
{
public:
    uint64_t m_cid;
    uint64_t m_rank;
    NFShmVector<int64_t, 1> m_paramInt;
    NFShmVector<NFCommonStr, 1> m_paramStr;

    RankNode()
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

    RankNode(const RankNode& other)
    {
        if (this != &other)
        {
            CopyFrom(other);
        }
    }

    int CreateInit()
    {
        return 0;
    }

    int ResumeInit()
    {
        return 0;
    }

    void CopyFrom(const RankNode& other)
    {
        m_cid = other.m_cid;
        m_rank = other.m_rank;
        m_paramInt = other.m_paramInt;
        m_paramStr = other.m_paramStr;
    }

    RankNode& operator =(const RankNode& other)
    {
        if (this != &other)
        {
            CopyFrom(other);
        }
        return *this;
    }
};

class NFCommonRank : public NFDBGlobalTemplate<NFCommonRank, proto_ff::tbSnsRank, EOT_SNS_COMMON_RANK>
{
    typedef NFShmMultiMap<uint64_t, RankNode, RANK_REAL_SIZE, std::greater<uint64_t>> MapRankNode;
public:
    NFCommonRank();

    virtual ~NFCommonRank();

    int CreateInit();
    int ResumeInit();

public:
    int InitRank(uint32_t rankType);

public:
    virtual int LoadFromDB(const std::string& dbData);

    virtual int SaveToDB(std::string& dbData);

    virtual int InitConfig();

    virtual int GetDbId();
public:
    //���½ڵ�
    bool UpdateNode(uint64_t charID, uint64_t nValue, const std::vector<int64_t>& paramIntVec, const std::vector<string>& paramStrVec);
    //��ȡ�б�
    MapRankNode* GetNodeList();
    //��ȡ����
    uint32_t GetRank(uint64_t cid);
    //ɾ���ڵ�
    bool DeleteNode(uint64_t cid);
protected:
    //��ȡ���һ������ֵ
    uint64_t GetLowestValue();
    //��������µ���ҵ����а���ӳɹ�����������ʧ�ܷ���0
    void TryToAddNewNode(uint64_t charID, uint64_t nValue, const std::vector<int64_t>& paramIntVec, const std::vector<string>& paramStrVec);
    //���°����������
    void UpdateNodeData(uint64_t charID, uint64_t nValue, const std::vector<int64_t>& paramIntVec, const std::vector<string>& paramStrVec);
    void EraseLowestValue();
protected:
    uint32_t m_rankType;
    MapRankNode m_rankData;
    NFShmHashMap<uint64_t, MapRankNode::iterator, RANK_REAL_SIZE> m_cidInRank;
};
