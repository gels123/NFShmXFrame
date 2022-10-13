//
// Created by gaoyi on 2022/9/21.
//

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFCore/NFSingleton.hpp"

#define MAX_ZONE_COUNT 16383//1024*8 14bit��ʾ ���Ϊ2��14�η���1,���ﲻҪ����޸�

//ԭ����64λ  zid bit[64-50] bit[50] =0  cid bit[1-49]
//���ڿͻ���LUA �� int64_t �������ֻ�ܱ�ʾ 2��53�η������Խ�ɫcid��ԭ����64λ�ĳ����ڵ�51λ��ʾ(14λ����ID+37λ���)
//���ڣ�51λ  zid bit(38-51) cid bit[1-37]
#define CHAR_DB_UID(zid,idx) (( ((uint64_t)zid) << 37) | idx)
//ͨ�����DBΨһID��ȡ����ID
#define CHAR_DB_ZID(cid) (cid >> 37)
//ͨ�����DBΨһID��ȡDB����
#define CHAR_DB_IDX(cid) (cid & 0x1FFFFFFFFF)


#define MAX_CHARACTER_COUNT 137438953471 //һ������ɫCID�������ֵ 2��37�η������ﲻҪ����޸�
#define MAX_ZONE_CONNECT_TIME_SEC   60000  //�ͻ�������zone�����ʱ����(1����,���ﵥλms)
#define MAX_CENTER_LOGIN_TIME_SEC 60000  //�ͻ��˵�½center�����ʱ����(1����,��λms)

class NFLogicCommon {
public:
    NFLogicCommon();
    virtual ~NFLogicCommon();
public:
    /**
     * @brief ʹ��uid �Լ� timeStamp ��ͬһ��md5 �� token
     * @param uid
     * @param timeStamp
     * @return
     */
    static std::string MakeToken(uint32_t uid, uint64_t timeStamp);
};


