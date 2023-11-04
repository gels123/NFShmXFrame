#pragma once

#include "NFServerComm/NFServerCommon/NFIDescStoreEx.h"
#include "NFServerComm/NFServerCommon/NFIDescTemplate.h"
#include "NFComm/NFShmCore/NFResDb.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmStl/NFShmHashMap.h"
#include "NFComm/NFShmStl/NFShmVector.h"
#include "NFLogicCommon/NFDescStoreTypeDefines.h"
#include "NFLogicCommon/NFMissionDefine.h"
#include "DescStore/TaskdynamicTaskdynamicDesc.h"
#include "DescStore/TaskdynamicTaskcomcondDesc.h"
#include "DescStore/TaskTaskDesc.h"
#include "DescStore/TaskrewardTaskrewardDesc.h"
#include "DescStore/TaskdynamicTasktextDesc.h"

//ͨ����λ��϶�̬����text������ text ���� Ŀ�곡��(���11λʮ�������֣���Ӧ�Ķ�����λ�� 37bit)+��������(6bit)+�������(20bit)
#define DY_MISSION_TEXT_KEY(type, cond) ((type << 20) | cond )
#define DY_MISSION_TEXT_TYPE(key) (key >> 20)
#define DY_MISSION_TEXT_COND(key) ( key & 0xFFFFF)

//��̬���������key
#define DY_MISSION_REWARD_KEY(type, lv) ((lv << 8) | type)

#define NF_MISSION_TYPE_MAX_COUNT 20
#define NF_MISSION_TYPE_MAX_MISSION_COUNT (MAX_TASK_TASK_NUM+MAX_TASKDYNAMIC_TASKDYNAMIC_NUM)//ͨ����λ��϶�̬����text������ text ���� Ŀ�곡��(���11λʮ�������֣���Ӧ�Ķ�����λ�� 37bit)+��������(6bit)+�������(20bit)
#define DY_MISSION_TEXT_KEY(type, cond) ((type << 20) | cond )
#define DY_MISSION_TEXT_TYPE(key) (key >> 20)
#define DY_MISSION_TEXT_COND(key) ( key & 0xFFFFF)

//��̬���������key
#define DY_MISSION_REWARD_KEY(type, lv) ((lv << 8) | type)

#define NF_MISSION_TYPE_MAX_COUNT 20
#define NF_MISSION_TYPE_MAX_MISSION_COUNT (MAX_TASK_TASK_NUM+MAX_TASKDYNAMIC_TASKDYNAMIC_NUM)

class TaskDescEx : public NFShmObjGlobalTemplate<TaskDescEx, EOT_CONST_TASK_DESC_EX_ID, NFIDescStoreEx>
{
public:
    typedef NFShmHashMap<uint64_t, DyMissionInfo, MAX_TASKDYNAMIC_TASKDYNAMIC_NUM> DyMissionInfoMap;
    typedef NFShmHashMap<uint64_t, DyConditionInfo, MAX_TASKDYNAMIC_TASKCOMCOND_NUM> DyCondtionInfoMap;
    typedef NFShmHashMap<uint64_t, MissionInfo, MAX_TASK_TASK_NUM> MissionInfoMap;
    //��̬������ key����ҵȼ�(32 - 9)+��������(8 - 1) ���
    typedef NFShmHashMap<uint32_t, TASK_REWARD, MAX_TASKREWARD_TASKREWARD_NUM> DyTaskRewardMap;
    typedef NFShmHashMap<int32_t, NFShmHashSet<uint64_t, MAX_TASK_TASK_NUM>, NF_MISSION_TYPE_MAX_COUNT> FirstMissionMap;
    typedef NFShmHashMap<int32_t, NFShmHashSet<uint64_t, MAX_TASKDYNAMIC_TASKDYNAMIC_NUM>, NF_MISSION_TYPE_MAX_COUNT> DyMissionTypeMap;
public:
	TaskDescEx();
	virtual ~TaskDescEx();
	int CreateInit();
	int ResumeInit();
public:
	virtual int Load() override;
	virtual int CheckWhenAllDataLoaded() override;
public:
    bool ProcessDyMission();//����̬��������
    bool ProcessDyCondition();//����̬����
    bool ProcessDyText();    //����̬����ǰ����ʾ
    bool ProcessReward();    //����̬������
    bool ProcessTask();

public:
    const DyMissionInfo *GetDyMissionCfgInfo(uint64_t missionId);
    
    const DyConditionInfo *GetDyConditionCfgInfo(uint64_t condId);
    
    MissionInfo *GetMissionCfgInfo(uint64_t missionId);

public:
    bool ParseMissionCond(MissionInfo *pMissionInfo, const std::string &strParam);              //���������ȡ����
    bool ParseTaskExecute(MissionInfo *pMissionInfo, const std::string &sExecute);      //���������������
    bool ParseTaskReceAdd(MissionInfo *pMissionInfo, const std::string &sReceAdd);
    
    bool ParseTaskSubAward(MissionInfo *pMissionInfo, int64_t sSubAward);

public:
    //������ȡ����(���ýӿ�)
    bool ParseAcceptCond(AcceptInfo &accept, const std::string &strAcceptParam, SParaseAcceptParam &param);
    //�����������(���ýӿ�)
    bool ParseFinishCond(InterExecute &conds, const std::string &strFinish, SParseFinishParam &param);
    //У���������(���ýӿ�)
    bool CheckFinishCond(InterItemPair &item, SParseFinishParam &param);
    
    bool CheckRewardParam(uint64_t missionId, uint32_t type, uint64_t id);

public:
    uint64_t ComposeTextKey(int32_t missionType, int32_t condType); //��װtext���key
    uint32_t ComposeDyRewardKey(int32_t taskType, int32_t level);//��װ��̬������key
public:
    //��ȡÿ���ߵĵ�һ������
    const FirstMissionMap &GetFirstMission() { return _missionFirstMap; }
    
    const NFShmHashSet<uint64_t, MAX_TASKDYNAMIC_TASKDYNAMIC_NUM> *GetDyMissionLstByType(int32_t missionType) const;
    
    const NFShmHashSet<uint64_t, MAX_TASK_TASK_NUM> *GetPreAcceptMission(uint64_t missionId);
    
    uint64_t GetDyTextId(uint64_t key);
    
    uint64_t GetDyTextId(int32_t missionType, uint32_t condType);
    
    //��ȡ��̬������
    const TASK_REWARD *GetDyMissionReward(int32_t missionType, int32_t level);
private:
    DyMissionTypeMap m_dymissionTypeMap;                    //��̬��������map
    DyMissionInfoMap m_dymissionInfoMap;                    //��̬��������
    DyCondtionInfoMap m_dycondtionInfoMap;                    //��̬��������
    MissionInfoMap m_missionInfoMap;                    //���������б�
    DyTaskRewardMap m_mapDyReward;                        //��̬������
    FirstMissionMap _missionFirstMap;                    //�������Ͷ�Ӧ�ĵ�һ�������б�
    NFShmHashMap<int32_t, NFShmHashSet<uint64_t, 100>, 1000> m_mapLevMission;                    //�ȼ�����
    
    NFShmHashMap<uint64_t, NFShmHashSet<uint64_t, MAX_TASK_TASK_NUM>, MAX_TASK_TASK_NUM> m_mapPreOrAcceptMap;                //���������е�ǰ�����������
    NFShmHashMap<uint64_t, NFShmHashSet<uint64_t, MAX_TASK_TASK_NUM>, MAX_TASK_TASK_NUM> m_mapPreAndAcceptMap;                //���������е�ǰ������������
    NFShmHashMap<uint64_t, NFShmHashSet<uint64_t, MAX_TASK_TASK_NUM>, MAX_TASK_TASK_NUM> m_mapPreAcceptMap;                    //��ȡ������ǰ��������ɿɽ�ȡ������
    
    NFShmHashMap<uint64_t, NFShmHashSet<uint64_t, 100>, MAX_TASKDYNAMIC_TASKTEXT_NUM> _dymissionTextMap;                    //��̬����ǰ����ʾ����
};
