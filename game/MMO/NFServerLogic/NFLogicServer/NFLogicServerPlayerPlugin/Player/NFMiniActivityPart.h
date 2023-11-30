// -------------------------------------------------------------------------
//    @FileName         :    NFMIniActivity.h
//    @Author           :    gaoyi
//    @Date             :    23-10-20
//    @Email			:    445267987@qq.com
//    @Module           :    NFMIniActivity
//
// -------------------------------------------------------------------------

#pragma once


#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFShmCore/NFShmObj.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "Part/NFPart.h"
#include "NFLogicCommon/NFLogicShmTypeDefines.h"
#include "NFComm/NFShmCore/NFISharedMemModule.h"

/**
1,����
2.boss
3.����
4.������
5.���ᴫ��
6.����
 */
enum {
    BACK_TYPE_DUP_GROUP = 1, //���� ��������������
    BACK_TYPE_BOSS = 2, //boss  ����boss������
    BACK_TYPE_TASK = 3, //���� ����taskdynamic��taskType
    BACK_TYPE_EXPQIFU = 4, //������
    BACK_TYPE_FactionTrans = 5, //���ᴫ��
    BACK_TYPE_YaoChi = 6, //����
};

class NFMiniActivityPart : public NFShmObjTemplate<NFMiniActivityPart, EOT_LOGIC_PART_ID+PART_MINI_ACTIVITY, NFPart>
{
public:
    NFMiniActivityPart();

    virtual ~NFMiniActivityPart();

    int CreateInit();

    int ResumeInit();
public:
    void AddBackTask(uint32_t type, uint32_t typeParam, uint32_t left_num, uint32_t external_left_num, uint64_t lastTime, uint32_t max_num, uint32_t external_max_num) { return; }
};