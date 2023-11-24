// -------------------------------------------------------------------------
//    @FileName         :    NFFestivalDefine.h
//    @Author           :    gaoyi
//    @Date             :    23-11-24
//    @Email			:    445267987@qq.com
//    @Module           :    NFFestivalDefine
//
// -------------------------------------------------------------------------

#pragma once

#pragma pack(push)
#pragma pack(1)


//�ҵ�(��������)��־��󳤶�
const uint32_t FEST_DIG_EGG_MAX_LOG_NUM = 300;

//ת�̶౶������־��󳤶�
const uint32_t FEST_RTURANBLE_MAX_LOG_NUM = 100;

const uint32_t FEST_DRAWPIZE_MAX_LOG_NUM = 100;

//ʱ������
enum class ETimeType
{
    permanent = 1,		//��פ
    openserver = 2,		//����ʱ��
    date = 3,			//����
    //create = 4,		//����ʱ��(ȥ��)
    limit,
};
//��Ч�ķ���������
enum class EServType
{
    newzone = 1,	//�·�
    merge = 2,		//�Ϸ�
    all = 3,		//���е�
    limit,
};

//�������������
enum class EPeriodType
{
    none = 0,
    fix = 1,		//�̶�
    dynamic = 2,	//��̬
    limit,
};


//ģ������
enum class ETplType
{
    none = 0,				//��
    bosskill = 1,			//boss��ɱ
    openrank = 2,			//�������
    collect = 3,			//���ֻ
    shop = 4,				//��ʱ�̵�(��������������ֱ��)
    activeplat = 5,			//��Ծת��
    opencheer = 6,			//������
    timeonline = 7,			//��ʱ����
    chargeplat = 8,			//��ֵת��
    totalcharge = 9,		//�ۻ���ֵ
    fish = 10,				//èè����
    dailytotalcharge = 11,	//�ۻ���ֵ
    sign = 12,				//ǩ��(��������)
    goldstore = 13,			//���(��������)
    digegg = 14,			//�ҵ�(��������)
    candle = 15,			//������ս(��������)
    btdragon = 16,			//��̬������(��������)
    total_recharge = 17,	//�ۼƳ�ֵ
    marry = 18,				//��������
    huimengxiyou = 20,      //��������
    drop = 22,				//���ն������
    mult_drop = 23,			//�౶����
    login = 24,				//��¼����
    conti_recharge = 25,	//������ֵ
    limit_sale = 26,		//��ʱ��ɱ
    day_totalecharge = 27,	//ÿ���۳�
    recharge_prize = 28, //�۳�����
    mystery = 29,			//��������
    field_boss = 30,		//Ұ��BOSS
    shoots_sun = 33,		//��������
    limit,
};

//�״̬
enum EFestState
{
    fest_not_open = 0,	//δ����
    fest_open = 1,		//�����
    fest_end = 2,		//�����
};


//�����������
enum class EFestRes
{
    success = 0,
    fail = 1,
    ignore = 2,
};




//���״̬��Ҫ�Ĳ���
struct ChkFestParam
{
    uint32_t zid = 0;		//����ID
    uint64_t curtime = 0;	//��ǰunixʱ��
    bool isret = false;		//�Ϸ�����Ƿ��web������
    bool ismerge = false;   //�Ƿ�Ϸ�
    uint32_t days = 0;		//�����ǿ���֮��ĵڼ���
    uint64_t zerotime = 0;	//������һ���0��ʱ��
};

#define XIYOU_MAX_TIME_LENGTH 20 * 60

enum {
    FestHuiMengXiYouStatus_Close = 0, //0:�ر� 1��׼���׶� 2:����Ϸ�׶� 3:��ɽ׶�
    FestHuiMengXiYouStatus_Ready = 1,
    FestHuiMengXiYouStatus_Play = 2,
    FestHuiMengXiYouStatus_Finish = 3,
};

enum {
    FestHuiMengXiYouState_READY = 1, //׼���������� ����Ϊmuban_play_game order 1
};


#pragma pack(pop)