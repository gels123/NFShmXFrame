// -------------------------------------------------------------------------
//    @FileName         :    NFLoginQueue.h
//    @Author           :    gaoyi
//    @Date             :    2022/10/13
//    @Email			:    445267987@qq.com
//    @Module           :    NFLoginQueue
//
// -------------------------------------------------------------------------

#pragma once


#include "NFComm/NFCore/NFPlatform.h"

class NFLoginQueue
{
public:
    NFLoginQueue();

    virtual ~NFLoginQueue();

    int CreateInit();

    int ResumeInit();
private:
    /**
     * @brief �Ŷ����UID
     */
    uint64_t m_playerId;
public:
    uint64_t GetPlayerId() const;

    void SetPlayerId(uint64_t playerId);

    uint64_t GetLastReqTime() const;

    void SetLastReqTime(uint64_t lastReqTime);

private:

    /**
     * @brief ��һ�������Ŷ����ε�ʱ��
     */
    uint64_t m_lastReqTime;
};