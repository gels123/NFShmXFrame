// -------------------------------------------------------------------------
//    @FileName         :    WorldCommonDefine.h
//    @Author           :    gaoyi
//    @Date             :    2022/10/12
//    @Email			:    445267987@qq.com
//    @Module           :    WorldCommonDefine
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"

enum WorldPlayerState
{
    PLAYER_STATE_NULL,
    PLAYER_STATE_LOGIN,			// ��¼��Ϸ
    PLAYER_STATE_QUEUE,			// �Ŷ�״̬
    PLAYER_STATE_LOADCHARLIS,	// ���ؽ�ɫ�б�
    PLAYER_STATE_ENTER,			// ������Ϸ
    PLAYER_STATE_GAMING,		// ��Ϸ״̬
    PLAYER_STATE_SWITCH,		// �л�����
    PLAYER_STATE_LOGOUT,		// �ǳ���Ϸ
    PLAYER_STATE_DISCONNECT,    // ����״̬
};

#define WORLD_SERVER_MAX_ONLINE_COUNT 10000


