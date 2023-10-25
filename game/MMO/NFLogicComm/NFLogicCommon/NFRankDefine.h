// -------------------------------------------------------------------------
//    @FileName         :    NFRankDefine.h
//    @Author           :    gaoyi
//    @Date             :    23-10-25
//    @Email			:    445267987@qq.com
//    @Module           :    NFRankDefine
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFGameCommon/NFComTypeDefine.h"

//���а��������
#define RANK_MAX_SIZE 100
//���а�ʵ����������
#define RANK_REAL_SIZE 150
//����ȼ�ȡ���а�ǰ����ƽ��ֵ
#define RANK_WORLDLEVEL_RANK 5

enum enRankType
{
    RANK_TYPE_POWER = 1,	//ս����
    RANK_TYPE_LEVEL = 2,	//�ȼ�
    RANK_TYPE_GUILD = 3,	//����
    RANK_TYPE_EQUIP = 4,    //װ��
    RANK_TYPE_XIUZHEN = 5,    //�����
    RANK_TYPE_FANGREN_XIUZHEN = 6,    //���������
    RANK_TYPE_BATTLE_PASS_XIANCE = 7, //��Ե�ɲ�
    RANK_TYPE_BATTLE_PASS_BAODING = 8, //��������
    RANK_TYPE_PET_FIGHT = 11, //����ս��
    RANK_TYPE_MOUNT_FIGHT = 12, //�������
    RANK_TYPE_YUANMAO_SUM = 13, //Ԫ���ۼƻ�ȡ
    RANK_TYPE_WING_FIGHT = 14, //����ȼ�
    RANK_BAOSHI_LEVEL = 15, //��ʯ�ܵȼ�
};


//���а������ʾ����
enum enRankPlayerParam
{
    RANK_PLAYER_PARAM_RANK = 1,				//1����
    RANK_PLAYER_PARAM_PT_STAGE= 2,	//2������澳��
    RANK_PLAYER_PARAM_NAME = 3,				//3�������
    RANK_PLAYER_PARAM_JOB = 4,				//4���ְҵ
    RANK_PLAYER_PARAM_POWER = 5,			//5ս����
    RANK_PLAYER_PARAM_LEVEL = 6,			//6�ȼ�
    RANK_PLAYER_PARAM_GUILD_RANK = 7,		//7��������
    RANK_PLAYER_PARAM_GUILD_NAME = 8,		//8��������
    RANK_PLAYER_PARAM_GUILD_LEVEL = 9,		//9���ɵȼ�
    RANK_PLAYER_PARAM_GUILD_POWER = 10,		//10����ս��
    RANK_PLAYER_PARAM_EQUIP_SCORE = 11,		//11װ������
    RANK_PLAYER_PARAM_RANK_PT_STAGE = 12,		//12������澳��
    RANK_PLAYER_PARAM_FANGREN_LAYER = 13,	//13��������ͨ������
};


struct RankNode
{
    uint64_t cid;
    VEC_INT64 paramInt;
    VEC_STRING paramStr;
    RankNode()
    {
        clear();
    }
    void clear()
    {
        cid = 0;
        paramInt.clear();
        paramStr.clear();
    }
    RankNode & operator = (const RankNode & other)
    {
        if (this != &other)
        {
            cid = other.cid;
            paramInt.clear();
            paramStr.clear();
            paramInt.assign(other.paramInt.begin(), other.paramInt.end());
            paramStr.assign(other.paramStr.begin(), other.paramStr.end());
            
        }
        return *this;
    }
};

typedef std::multimap<uint64_t, RankNode, greater<uint64_t> > mapRankNode; //��������
