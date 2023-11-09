// -------------------------------------------------------------------------
//    @FileName         :    NFSkillPart.h
//    @Author           :    gaoyi
//    @Date             :    23-10-20
//    @Email			:    445267987@qq.com
//    @Module           :    NFSkillPart
//
// -------------------------------------------------------------------------

#pragma once


#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFShmCore/NFShmObj.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "Part/NFPart.h"
#include "NFLogicCommon/NFLogicShmTypeDefines.h"
#include "NFComm/NFShmCore/NFISharedMemModule.h"

class NFSkillPart : public NFShmObjTemplate<NFSkillPart, EOT_LOGIC_PART_ID+PART_SKILL, NFPart>
{
public:
    NFSkillPart();

    virtual ~NFSkillPart();

    int CreateInit();

    int ResumeInit();
public:
    //�����ı�
    void OnHpChange(int64_t oldHp, int64_t curHp, int64_t maxHp) { }
    //��Ӽ���(��������)
    bool AddSkill(uint64_t skillid, int32_t level, const SCommonSource& src, bool sync = false, uint64_t use_msec = 0) { return true; }
    //������ս��
    int64_t SkillFight() { return 0; }
    //������� deityId������ID
    int32_t OnDeityChgFacade(int64_t deityId) { return 0; }
};