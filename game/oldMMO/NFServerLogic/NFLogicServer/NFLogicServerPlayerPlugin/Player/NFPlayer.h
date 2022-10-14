// -------------------------------------------------------------------------
//    @FileName         :    NFUserDetail.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFUserDetail.h
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFShmCore/NFShmObj.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmCore/NFISharedMemModule.h"
#include "NFLogicCommon/NFServerFrameTypeDefines.h"
#include "NFComm/NFShmCore/NFSeqOP.h"
#include "NFComm/NFCore/NFTime.h"

typedef enum
{
    PLAYER_STATUS_NONE = 0,     //
    PLAYER_STATUS_ONLINE = 1,    //
    PLAYER_STATUS_OFFLINE = 2,   //
    PLAYER_STATUS_LOGOUT = 3,    //
} PLAYER_STATUS;

class NFPlayer : public NFShmObj, public NFSeqOP
{
public:
    NFPlayer(NFIPluginManager *pPluginManager);

    virtual ~NFPlayer();

    int CreateInit();

    int ResumeInit();

public:
    /**
     * @brief ��ǰ�������ʹ�õ�trans���� +1
     */
    void IncreaseTransNum();

    /**
     * @brief ��ǰ�������ʹ�õ�trans���� -1
     */
    void DecreaseTransNum();

    /**
     * @brief ��ǰ�������ʹ�õ�trans����
     * @return
     */
    int GetTransNum() const;

    /**
     * @brief ��ɫID
     * @return
     */
    uint64_t GetRoleId() const;

    /**
     * @brief
     * @param roleId
     */
    void SetRoleId(uint64_t roleId);

    /**
     * @brief ���ID
     * @return
     */
    uint64_t GetPlayerId() const;

    /**
     * @brief
     * @param playerId
     */
    void SetPlayerId(uint64_t playerId);
private:
    /**
     * @brief ��������Ƿ��ʼ��
     */
    bool m_bIsInited;

    /**
     * @brief ��ǰ�������ʹ�õ�trans����
     */
    int m_iTransNum;

    /**
     * @brief ��ɫID
     */
    uint64_t m_roleId;

    /**
     * @brief ���ID
     */
    uint64_t m_playerId;
DECLARE_IDCREATE(NFPlayer)
};
