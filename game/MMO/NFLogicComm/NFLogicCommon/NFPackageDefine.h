// -------------------------------------------------------------------------
//    @FileName         :    NFPackageDefine.h
//    @Author           :    gaoyi
//    @Date             :    23-10-28
//    @Email			:    445267987@qq.com
//    @Module           :    NFPackageDefine
//
// -------------------------------------------------------------------------

#pragma once

//�������ѵ���
//#define PACKAGE_MAX_PILE_NUM 99
//��������ʼ������
#define COMMON_PACKAGE_INIT_GRID_NUM 100
//��������������(��Ҫ���Ķ�)
#define COMMON_PACKAGE_MAX_GRID_NUM 400
//�ֿ��ʼ������
#define STORAGE_PACKAGE_INIT_GRID_NUM 100
//�����ʼ������
#define MOUNT_PACKAGE_INIT_GRID_NUM 100
//�ֿ���������
#define STORAGE_PACKAGE_MAX_GRID_NUM 400
//������������
#define MOUNT_PACKAGE_MAX_GRID_NUM 100
//������������
#define DEITY_EQUP_PACKAGE_INIT_GRID_NUM 100
//������������
#define DEITY_EQUIP_PACKAGE_MAX_GRID_NUM 200
//�ؾ���ͼ����������
#define HANDBOOK_PACKAGE_INIT_GRID_NUM 300
//�ؾ���ͼ����������
#define HANDBOOK_PACKAGE_MAX_GRID_NUM 300
//������������
#define BEAST_EQUP_PACKAGE_INIT_GRID_NUM 100
//������������
#define BEAST_EQUIP_PACKAGE_MAX_GRID_NUM 100
//������������
#define LONG_HUN_PACKAGE_INIT_GRID_NUM 100
//������������
#define LONG_HUN_PACKAGE_MAX_GRID_NUM 100
//�����ǳ�4��������
#define STAR_HUN_PACKAGE_MAX_GRID_NUM 100
//�������ͱ�����ʼ��������
#define OTHER_PACKAGE_INIT_GRID_NUM 300
//�������ͱ��������������
#define OTHER_PACKAGE_MAX_GRID_NUM 300
//��񱳰������������
#define SOUL_PACKAGE_MAX_GRID_NUM 500
//���±��������������
#define STAR_PACKAGE_MAX_GRID_NUM 500
//���򱳰������������
#define FAIMATERI_PACKAGE_MAX_GRID_NUM 500
//ÿ����չ������
#define PACKAGE_EXPAND_GRID_NUM 5
//���������ֳ���
#define MAX_EQUIP_MAKER_NAME_LEN 35
//װ����Ƕ���������
#define MAX_EQUIP_HOLE_NUM 6
//װ���ϳ�ʱ ��¼����װ�����������
#define MAX_COMPOSE_COST_EQUIP_NUM 5
//һ�α��汳����Ʒ��
#define UNIT_SAVE_BAG_ITEM_SIZE  200
//����װ�����800��
#define TURN_PACKAGE_MAX_GRID_NUM 800
//���ʯ��λ��
#define MAX_STONE_SLOT 5
//ϴ������λ��
#define MAX_WASH_SLOT 4

//ͨ�ñ�����С
#define COM_BAG_SIZE 200
//��������С
#define EGG_BAG_SIZE 100

//���ﻯ��
#define MOUNT_KUN_PACKAGE_MAX_GRID_NUM 800

enum EBindState
{
    EBindState_no = 0,		//δ��
    EBindState_bind = 1,	//��
    EBindState_all = 2,		//���У������󶨣�δ�󶨣�ֻ��Ϊ�˱����Ǳ߲��ҽӿڲ����õģ�
};

//�ѵ�״̬
enum class EStackState
{
    EStackState_no = 0,		//���ɶѵ�
    EStackState_yes = 1,	//�ɶѵ�
};
//cd����
enum class ECdType
{
    ECdType_none,
    ECdType_nocd = 1,			//û����ȴ
    ECdType_independent = 2,	//������ȴ
    ECdType_common = 3,			//������ȴ
    ECdType_limit,
};

//��Ʒ�Ƿ��ʹ�õ�����
enum class EItemCanUseType
{
    EItemCanUseType_NoUse = 0,	//����ʹ��
    EItemCanUseType_CanUse = 1,	//��ʹ��
    EItemCanUseType_SysUse = 2, //ϵͳʹ��
};

//װ��������
enum class EEquipBindType
{
    EEquipBindType_none = 0,
    EEquipBindType_get = 1,			//��ȡ��
    EEquipBindType_dress = 2,		//������
    EEquipBindType_limit,
};


//װ������
enum EEquipFromType
{
    EEquipFromType_None = 0,
    EEquipFromType_Bag = 1,			//����װ��
    EEquipFromType_Dress = 2,		//����װ��
};


//ʱװ����
enum EEquipFashionType
{
    EEquipFashionType_None = 0,
    EEquipFashionType_Weapon = 1,			//ʱװ����
    EEquipFashionType_Clothes = 2,			//ʱװ�·�
    EEquipFashionType_Limit,
};

//��Ʒ��;������������Ʒ��װ����
enum EItemBindWay
{
    EItemBindWay_None = -1,
    EItemBindWay_UnBind = 0,	//�ǰ�
    EItemBindWay_Get = 1,		//��ȡ�󶨣���ȡ֮ǰ�ǰ󶨣�
    EItemBindWay_Bind = 2,		//���ð�
    EItemBindWay_Dress = 3,		//�����󶨣���ȡ֮ǰ�ǰ󶨣�
    EItemBindWay_Limit,
};

//��Ʒ��ֹ����
enum EItemForbidType
{
    EItemForbidType_None = -1,
    EItemForbidType_Follow = 0, //����󶨹���
    EItemForbidType_Forbid = 1,	//��ֹ
    EItemForbidType_Limit,
};

//��Ʒʹ��֮��㲥��Ч����Ч����
enum EItemUseEffectType
{
    EItemUseEffectType_Server = 1,	//ȫ��
    EItemUseEffectType_Scene = 2,	//����
};


/*��Ʒ�������Ͷ�Ӧ�Ĳ���
0���޹���
1��ʹ���޸����ԡ�������ֵ�����������޵���Ʒ����Ҵ�������ѫ�£�
30���ڹҿ�
38, ������Ʒ
*/
//��Ʒ��������
enum EItemFuncType
{
    EItemFuncType_None					= -1,
    EItemFuncType_NoFunc				= 0,			//0���޹���
    EItemFuncType_ModifyAttr			= 1,			//1��ʹ���޸����ԡ�������ֵ��Ѫƿ�����������޵���Ʒ����Ҵ�������ѫ�£�
    EItemFuncType_AddDupGroupNum		= 2,			//2����Ӹ�������
    EItemFuncType_AddBossNum			= 3,			//2�����Boss����(boss������Ҫ��)
    EItemFuncType_Stone					= 6,			//6, ��ʯ
    EItemFuncType_AddAttrForever		= 7,			//7,ʹ����Ʒ��������
    EItemFuncType_Box					= 9,			//9������Ʒ�����Կ���������Ʒ����Ʒ��
    EItemFuncType_SelectBox				= 10,			//10����ѡ����
    EItemFuncType_Fashion				= 21,			//21��ʹ��ʱװ
    EItemFuncType_HangCard				= 30,			//30���ڹҿ�,functionType=30 ��Чʱ�䣨��дСʱ��������д-1��
    EItemFuncType_addSkill				= 35,			//35,��Ӽ���
    EItemFuncType_Virtual				= 38,			//38,������Ʒ(��Դ)
    EItemFuncType_dynExp				= 39,			//39,��̬������
    EItemFuncType_AddDyMission          = 52,           //34, ���һ����̬����
    EItemFuncType_ActiveVip				= 54,			//����vip
    EItemFuncType_AddVipExp				= 55,			//���vip���鿨
    EItemFuncType_AddHuaKun				= 56,           //56,ʹ����Ʒ�������
    EItemFuncType_AddRoleExp			= 57,			//57��ӹ̶�����
    EItemFuncType_AddRoleRaceExp		= 58,			//58��ӰٷֱȾ���
    EItemFuncType_Limit,
};


/*
������ʹ��Ҫ�����Ʒ
����Ҫ�����ͣ�����ֵ
1��ָ������
2��ָ������
3��ָ��Ŀ��
*/
//��Ʒ������������
enum class EItemOtherParamType
{
    EItemOtherParamType_None,
    EItemOtherParamType_NoType = 0,
    EItemOtherParamType_Scene = 1,
    EItemOtherParamType_Area = 2,
    EItemOtherParamType_Target = 3,
    EItemOtherParamType_Limit,
};


//��Ʒ����Ŀ�� 1�������ɫ��2����Ŀ�ꣻ3��������ҽ�ɫ��4�����5��NPC��6������Ŀ��
enum class EItemTarge
{
    EItemTarge_SelfPlayer = 1,			//1�������ɫ
    EItemTarge_NoTarge = 2,				//2����Ŀ��
    EItemTarge_OtherPlayer = 3,			//3��������ҽ�ɫ
    EItemTarge_Monster = 4,				//4������
    EItemTarge_Npc = 5,					//5��NPC
    EItemTarge_RandTarget = 6,			//6������Ŀ��
};


//����Ŀ��������ϵ 1�����ޣ�2���жԣ�3���Ѻ�
enum class EItemRelation
{
    EItemRelation_NoLimit = 1,		//1������
    EItemRelation_Enemy = 2,		//2���ж�
    EItemRelation_Friend = 3,		//3���Ѻ�
};


enum EEquipType
{
    EEquipType_none = 0,		//��
    EEquipType_nomal = 1,		//��ͨװ��
    EEquipType_xq = 2,			//����
    EEquipType_deity = 3,       //����
};

enum EEquipTotalType
{
    EEquipTotalType_normal_strong = 1,	//��ͨװ��ǿ���ܵȼ�
    EEquipTotalType_xq_strong = 2,		//����װ��ǿ���ܵȼ�
    EEquipTotalType_stone = 3,			//��ʯ��Ƕ�ܵȼ�
    EEquipTotalType_star = 4,			//װ�����Ǽ�
};

/*
	������ϣ�subType=1��
	תְ���ϣ�subType=2��
	��ʱδ�������ͣ�subType=3��
	��ʯ�����ࣨsubType=4��
	����װʯ�ࣨsubType=5��
	Ů��װʯ�ࣨsubType=6��
	�߼���װʯ��subType=7��
	������װʯ��subType=8��
	ϴ���ࣨsubType=9��
	�����ࣨsubType=10��
	��ħ�ࣨsubType=11��
	�������ࣨsubType=12��
	����鵤�ࣨsubType=13��
	���������ࣨsubType=14��
	��鷨���ࣨsubType=15��
	���������ࣨsubType=16��
	���������ࣨsubType=17��
	�������ࣨsubType=18��
	��������ࣨsubType=19��
	��ָ���ɲ��ϣ�subType=20��
	�������ɲ��ϣ�subType=21��
	ʱװ��������ְҵ��subType=22��
	ʱװ�·�������Ů��subType=23��
	�������ְҵ��subType=24��
	���û����Σ�subType=25��
	�������Σ�subType=26��
	�����û����Σ�subType=27��
	������Σ�subType=28��
	�����û����Σ�subType=29��
	������չ����subType=30��
	С����С���ɣ�subType=31��
	���鿨��subType=32��
	ɨ��ȯ��subType=33��
	���鵤��subType=34��
	����ӳɵ��ߣ�subType=35��
	��һ����ߣ�subType=36��
	Bossͼ����subType=37��
	����ͼ����subType=38��
	������ʯ���ͣ�subType=39��
	������ʯ���ͣ�subType=40��
	��ֵ�ࣨsubType=99��
*/
//��Ʒ������
enum class EItemSubType
{
    EItemSubType_none,
    EItemSubType_RealmMaterial		= 1,	//�������
    EItemSubType_TransferMaterial	= 2,	//תְ����
    EItemSubType_StoneRefine		= 4,	//��ʯ������
    EItemSubType_ManSuitStone		= 5,	//����װʯ
    EItemSubType_FemaleSuitStone	= 6,	//Ů��װʯ
    EItemSubType_AdvSuitStone		= 7,	//�߼���װʯ
    EItemSubType_FaiSuitStone		= 8,	//������װʯ
    EItemSubType_Wash				= 9,	//ϴ����
    EItemSubType_Wake				= 10,	//������
    EItemSubType_Enchant			= 11,	//��ħ��
    EItemSubType_PartnerAdv			= 12,	//������
    EItemSubType_PartnerPil			= 13,	//����鵤
    EItemSubType_MagWeaponAdv		= 14,	//����������
    EItemSubType_PartnerSpirit		= 15,	//��鷨��
    EItemSubType_ArtifactAdv		= 16,	//��������
    EItemSubType_ArtifactSpirit		= 17,	//��������
    EItemSubType_WingAdv			= 18,	//��������
    EItemSubType_WingSpirit			= 19,	//���������
    EItemSubType_RingMaterial		= 20,	//��ָ���ɲ���
    EItemSubType_BabyMaterial		= 21,	//�������ɲ���
    
    EItemSubType_FashWeaponProf		= 22,	//ʱװ��������ְҵ
    EItemSubType_FashWeaponSex		= 23,	//ʱװ���������Ա�
    EItemSubType_MagArms			= 24,	//�������ְҵ
    EItemSubType_PartnerFatastic	= 25,	//���û�
    
    EItemSubType_HorseFace			= 26,	//��������
    EItemSubType_MagWeaponFace		= 27,	//�����û�����
    EItemSubType_WingFace			= 28,	//�������
    EItemSubType_BabyFace			= 29,	//�����û�����
    
    EItemSubType_FaiExpandCard		= 30,	//������չ��
    
    EItemSubType_SmallBat			= 31,	//С����
    EItemSubType_ExpCard			= 32,	//���鿨
    EItemSubType_SweepTicket		= 33,	//ɨ��ȯ
    EItemSubType_ExpPil				= 34,	//���鵤
    EItemSubType_AddExpItem			= 35,	//����ӳɵ���
    EItemSubType_ActExchange		= 36,	//��һ�����
    EItemSubType_BossBook			= 37,	//BOSSͼ��
    EItemSubType_FaiBook			= 38,	//����ͼ��
    EItemSubType_AtkStone			= 39,	//������ʯ
    EItemSubType_HpStone			= 40,	//������ʯ
    EItemSubType_Blue_FieldCylstal  = 53,	//������ˮ��
    EItemSubType_Green_FieldCylstal  = 54,	//������ˮ��
    
    EItemSubType_limit,
};

//�ϳ��л�������
enum EComposeCurrency
{
    EComposeCurrency_gold = 1,			//���
    EComposeCurrency_small_spirit = 2,	//С���ǻ���
    EComposeCurrency_big_spirit = 3,	//�����ǻ���
    EComposeCurrency_star = 4,			//���ǻ���
    EComposeCurrency_binddia = 5,		//����
    EComposeCurrency_dia = 6,			//��ʯ
    EComposeCurrency_holydia = 11,		//�������
};


struct SItem
{
    uint64_t nItemID;
    int64_t nNum;			//��Ʒ������������ int64_t ������ҪΪ�˼��� ������Ʒ������(��ң���ʯ��)
    int8_t byBind;
    SItem() :nItemID(0), nNum(0), byBind((int8_t)EBindState::EBindState_no){}
    SItem(uint64_t itemId, int64_t num, int8_t bind = (int8_t)EBindState::EBindState_no) :nItemID(itemId), nNum(num), byBind(bind) {}
};

//��Ʒ�б�
using LIST_ITEM = list<SItem>;
//itemid - SItem
using MAP_UINT64_ITEM = unordered_map<uint64_t, SItem>;
//
using MAP_UINT32_MAP_UINT64_ITEM = unordered_map<uint32_t, MAP_UINT64_ITEM > ;

enum EItemOpetateType
{
    EItemOpetateType_None = 1,
    EItemOpetateType_Add,			//����
    EItemOpetateType_Del,			//����
    EItemOpetateType_Use,			//ʹ��
};

//��Ʒ��ʼ����������
enum class EInitAttrType
{
    None = 0,
    Common = 1, //ͨ�õ����Գ�ʼ��
    Limit,
};

//������Ʒ����
struct SItemCond
{
    EInitAttrType inittype = EInitAttrType::Common;			//��Ʒ��ʼ����������
    int32_t level = 1;										//�ȼ�(��ҵȼ�)
    int32_t prof = 0;										//ְҵ
    //
    string makeName;										//����������(��ħװ����ʼ��)
    uint64_t makeTime = 0;									//����ʱ��(��ħװ����ʼ��)
    
    //������������������������
};

