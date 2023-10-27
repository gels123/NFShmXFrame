#pragma once

#include "NFServerComm/NFServerCommon/NFIDescStoreEx.h"
#include "NFServerComm/NFServerCommon/NFIDescTemplate.h"
#include "NFComm/NFShmCore/NFResDb.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmStl/NFShmHashMap.h"
#include "NFComm/NFShmStl/NFShmVector.h"
#include "NFLogicCommon/NFDescStoreTypeDefines.h"

enum AchievementEventType
{
    ACHIEVEMENT_EVENT_LEVELUP = 1,   //����ȼ�
    ACHIEVEMENT_EVENT_WING_ADVANCE = 2, //����
    ACHIEVEMENT_EVENT_TREASURE_ADVANCE = 3, //����
    ACHIEVEMENT_EVENT_MOUNT_LEVELUP = 4, //����
    ACHIEVEMENT_EVENT_ARTIFACT_ADVANCE = 5, //���
//���� 6
    ACHIEVEMENT_EVENT_XIUZHEN_LEVELUP = 6,
    ACHIEVEMENT_EVENT_OCCUPATION_LEVELUP = 7, //תְ
//�ǳ� 8
    ACHIEVEMENT_EVENT_GET_PET = 9, //�������
    ACHIEVEMENT_EVENT_PET_CALL = 10, //��������
    ACHIEVEMENT_EVENT_GET_PET_WITH_QUALITY = 11, //����Ʒ��
    ACHIEVEMENT_EVENT_PET_LV = 12, //��������
    ACHIEVEMENT_EVENT_PET_LV_WITH_NUM = 13, //�������
    ACHIEVEMENT_EVENT_PET_STARLV = 14, //��������
    ACHIEVEMENT_EVENT_EQUIP_DRESS_SUIT_WITH_150 = 150, //150����ͨ��װ)  ���Ǽ�װ����param����1����ɫװ����5   ��ɫװ����6   ��ɫװ����7
    ACHIEVEMENT_EVENT_EQUIP_DRESS_SUIT_WITH_151 = 151, //151 (��ͨ��װ)��param����1����ɫװ����5   ��ɫװ����6   ��ɫװ����7, param����2��1��װ����1   2�ǣ�2   3�ǣ�3��
    ACHIEVEMENT_EVENT_EQUIP_DRESS_SUIT_WITH_152 = 152, //152 (����)  param����1����ɫװ����5   ��ɫװ����6   ��ɫװ����7
    ACHIEVEMENT_EVENT_EQUP_STREN_WITH160 = 160, //160����ͨ��װ)  ���Ǽ�װ����param����1��
    ACHIEVEMENT_EVENT_EQUP_STREN_WITH161 = 161, //161 (����)  param����1
    ACHIEVEMENT_EVENT_WASH_QUALITY = 17, //װ��ϴ��
    //18 װ����Ʒ
    //19 װ����װ
    ACHIEVEMENT_EVENT_STONE_INLAY = 20, //��ʯ
    //21 ����
    //22 ���
    
};

enum XiuZhenRoadEventType
{
    XIUZHENROAD_EVENT_LEVELUP = 1,   //����ȼ�
    XIUZHENROAD_EVENT_OCCUPATION_LEVELUP = 2, //תְ
    XIUZHENROAD_EVENT_EQUIP = 3, //����װ��
    XIUZHENROAD_EVENT_XIAN_EQUIP_STREN = 4, //����ǿ��
    XIUZHENROAD_EVENT_EQUIP_STREN = 5, //װ��ǿ��
    XIUZHENROAD_EVENT_XIUZHEN_LEVELUP = 6, //�ɽ�
    XIUZHENROAD_EVENT_FANGREN_XIUZHEN_DUP = 7, //��������
    XIUZHENROAD_EVENT_OPEN_PET_SLOT = 8, //����XXX�����ﵰ������λ
    XIUZHENROAD_EVENT_MOJINGSHANGDIAN_COST = 9, //ħ���̳��ۼƻ���XXX�þ�ʯ
    XIUZHENROAD_EVENT_SHANGGUSHENGLING_ALL_LEVELUP = 10, //�Ϲ�����XXXȫ�������XXX��
    XIUZHENROAD_EVENT_PET_STEPLV = 11, // ӵ��xxx/xxx�����׵ȼ��ﵽ5�׵ĳ���
    XIUZHENROAD_EVENT_PET_QUALITY = 12,//ӵ��XXX��������XXXƷ�ʵĳ���
    XIUZHENROAD_EVENT_PengLaiTanBao = 13, //����̽��XXX��
    XIUZHENROAD_EVENT_TongXinSuo = 14,//װ��ͬ����
    XIUZHENROAD_EVENT_QingLV_DUP = 15,//�������¸���XXX��
    XIUZHENROAD_EVENT_LIGHTSUOL_DRESS = 16,//װ��XXX��XXXƷ�ʼ����ϵ�����
    XIUZHENROAD_EVENT_STONE_DRESS = 17, //��ǶXXX��XXX�������ϵı�ʯ
    XIUZHENROAD_EVENT_TONGXINSUO_LEVELUP = 18,//ͬ�����ﵽXXX�ף�XXX/XXX)
    XIUZHENROAD_EVENT_MARRY_XIANWA = 19,//ӵ��XXX���ﵽXXX�׵�����
    XIUZHENROAD_EVENT_PET_LEVELUP = 20,//ӵ��XXX���ȼ��ﵽXXX���ĳ���
    XIUZHENROAD_EVENT_SHENGMOJINGDI_DUP = 21,//������ħ����XXX��
    XIUZHENROAD_EVENT_WASH_QUALITY = 22, //ӵ��XXX��XXXƷ�ʼ�����Ʒ�ʵ�ϴ������
    XIUZHENROAD_EVENT_EQUIP_DRESS_SUIT = 23,//������ħ��װXXX����װЧ����XXX/XXX)
    XIUZHENROAD_EVENT_LIGHTSUOL_ACTIVE_SLOT = 24,//����XXX��������Ŀ�
    XIUZHENROAD_EVENT_KILL_MANGHUANGSHENGSHOU_BOSS = 25,//��ɱXXXֻXXX�׼����ϵ���������BOSS
    XIUZHENROAD_EVENT_DEITY_BATTLE = 26,//��սXXX������
    XIUZHENROAD_EVENT_ACTIVE_SHENGSHOU = 27,//��������-XXX��XXX/1)
    XIUZHENROAD_EVENT_GET_HUA_KUN = 28,//ӵ��XXX���ȼ��ﵽXXX������
    XIUZHENROAD_EVENT_COST_SHENGWANG = 29,//�����̳��ۼƻ���XXX����
    XIUZHENROAD_EVENT_GET_KUN_YUANSHENG = 30,//�ɻ�XXX��XXXƷ�ʼ����ϵ�����Ԫ��
    XIUZHENROAD_EVENT_WING_LEVEL = 31,//����ȼ��ﵽXXX��
    XIUZHENROAD_EVENT_ACTIVE_DEITY_FANTASY = 32,//��������-XXX��XXX/1)
    XIUZHENROAD_EVENT_GET_KUN = 33,//ӵ��XXX��XXXƷ�ʼ����ϵ���
    XIUZHENROAD_EVENT_GET_FANTASY_WING = 34,//ӵ��XXX����ɫ�����װ
    XIUZHENROAD_EVENT_BUY_DRAGONSOUL = 35,//Ԫ���̵깺��XXX����β��
    XIUZHENROAD_EVENT_BUY_BAGUA = 36,//Ԫ���̵깺��XXX����������
    XIUZHENROAD_EVENT_DRESS_BAGUA = 37,//װ��XXX��XXXƷ�ʼ����ϵİ���
    XIUZHENROAD_EVENT_GET_PET_SHANGWUXIANZI = 38,//��ó���-�������ӣ�XXX/1)
    XIUZHENROAD_EVENT_GET_FAQI_JIUFENGQINGLIANJIN = 39,//��÷������-�ŷ���������XXX/1)
    XIUZHENROAD_EVENT_STONE_LEVEL_SUM = 40,//��ʯ�ܵȼ��ﵽXXX/XXXX��
    XIUZHENROAD_EVENT_DRESS_EQUIP_POS = 41,//����ָ����λװ��
    XIUZHENROAD_EVENT_WELFARE_FIRSTCHARGE = 42,//���ָ���׳� ��дid����Ӧwelfare-fristcharge��id
    XIUZHENROAD_EVENT_WELFARE_LOGIN = 43,//���ָ���ĵ�¼� ��дid����Ӧwelfare-logon��id
    XIUZHENROAD_EVENT_SEND_FLOWER_TO_FRIEND = 44, //��������
    XIUZHENROAD_EVENT_KILL_BOSS_WORLD = 47, //��ɱXXXֻXXX�׼����ϵ�����BOSS
    XIUZHENROAD_EVENT_KILL_BOSS_HARMONY = 48, //��ɱXXXֻXXX�׼����ϵĺ���BOSS
    XIUZHENROAD_EVENT_KILL_BOSS_SELF = 49, //��ɱXXXֻXXX�׼����ϵ�ר��BOSS
    XIUZHENROAD_EVENT_FACTION_CHAT = 50, //����Ƶ������
    XIUZHENROAD_EVENT_WORLD_CHAT = 51, //����Ƶ������
    XIUZHENROAD_EVENT_BUY_DIALY_WELFARE_GIFT = 52, //����ָ��id�����
    XIUZHENROAD_EVENT_MISSION_HUNT_TREASURE= 53, //ʹ�òر�ͼ
    XIUZHENROAD_EVENT_MISSION_BOUNTY = 54, //�ͽ�����
    XIUZHENROAD_EVENT_QIFU = 55, //��
    XIUZHENROAD_EVENT_JOIN_PER_ACTIVITY = 56, //����ָ����淨id
    XIUZHENROAD_EVENT_JOIN_DAILY_ACTIVITY = 57, //����ָ���ճ���id
    XIUZHENROAD_EVENT_FACTION_JUANXIAN = 59, //��ɰ��ɾ���xxx��
    XIUZHENROAD_EVENT_BOSS_ASSIST = 60, //���bossЭ��xxx��
    XIUZHENROAD_EVENT_1V1_REWARD = 61, //��ȡ1v1����
    XIUZHENROAD_EVENT_3v3_REWARD = 62, //��ȡ3v3����
    XIUZHENROAD_EVENT_FACTION_DATI = 64, //������ɴ���
    
    XIUZHENROAD_EVENT_FIRST_LOGIN = 66, //�����״ε�½
    XIUZHENROAD_EVENT_ACTIVE_EXP = 67, //�����ճ���Ծ��
    XIUZHENROAD_EVENT_ONLINE_TIME = 68, //��������ʱ��
    XIUZHENROAD_EVENT_MARRY = 69,       //���һ�ν�Ե���Ѿ���Ե
    XIUZHENROAD_EVENT_GIVE_ITEM = 70,   //���ͻ�
    XIUZHENROAD_EVENT_EXPRESS = 71,     //����ָ��id��׵���
    XIUZHENROAD_EVENT_FINISH_MAIN_TASK = 72,     //����ض�����������
    XIUZHENROAD_EVENT_FINISH_DUP = 73,     //ͨ��ָ���ĸ���xxx��
    XIUZHENROAD_EVENT_FINISH_KIND_YONGJIUSHOUHU = 74,     //����X�������ػ�
    XIUZHENROAD_EVENT_FINISH_SHOP_BUY = 75,     //����
    XIUZHENROAD_EVENT_FINISH_SHANGGUSHENGLING_LEVELUP = 76,     //�Ϲ��������X��
    XIUZHENROAD_EVENT_FINISH_FIGHT_VALUE = 77,     //��ɫս���ﵽX
    XIUZHENROAD_EVENT_OWNER_LONG_SOUL_NUM = 78,     //ӵ��X������
    XIUZHENROAD_EVENT_SHENGLING_SHENGGE = 79,       //��������X�����ﵽX��
    XIUZHENROAD_EVENT_COLECT_NUM = 80, //�ɼ�X��
};

class AchievementDescEx : public NFShmObjGlobalTemplate<AchievementDescEx, EOT_CONST_ACHIEVEMENT_DESC_EX_ID, NFIDescStoreEx>
{
public:
	AchievementDescEx();
	virtual ~AchievementDescEx();
	int CreateInit();
	int ResumeInit();
public:
    virtual int Load() override;
    virtual int CheckWhenAllDataLoaded() override;
};
