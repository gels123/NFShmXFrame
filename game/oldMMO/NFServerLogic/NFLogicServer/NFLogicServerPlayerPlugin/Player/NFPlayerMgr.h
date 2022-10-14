// -------------------------------------------------------------------------
//    @FileName         :    NFPlayerMgr.h
//    @Author           :    gaoyi
//    @Date             :    2022/10/14
//    @Email			:    445267987@qq.com
//    @Module           :    NFPlayerMgr
//
// -------------------------------------------------------------------------

#pragma once


#include "NFComm/NFShmCore/NFShmObj.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmCore/NFISharedMemModule.h"
#include "NFLogicCommon/NFServerFrameTypeDefines.h"
#include "NFComm/NFShmCore/NFShmHashMap.h"

class NFPlayer;
class NFPlayerMgr : public NFShmObj
{
public:
    NFPlayerMgr(NFIPluginManager *pPluginManager);

    virtual ~NFPlayerMgr();

    int CreateInit();

    int ResumeInit();

public:
    /**
     * @brief ͨ��roleId��ȡ�������
     * @param roleId
     * @return
     */
    NFPlayer *GetPlayerByRoleId(uint64_t roleId);

    /**
     * @brief ͨ��playerId��ȡ�������
     * @param playerId
     * @return
     */
    NFPlayer *GetPlayerByPlayerId(uint64_t playerId);

    /**
     * @brief ͨ��roleId�����������, �����Ҵ��ڣ�����NULL
     * @param roleId
     * @return
     */
    NFPlayer *CreatePlayer(uint64_t playerId, uint64_t roleId);

    /**
     * @brief ɾ���������
     * @param pPlayer
     * @return
     */
    int DeletePlayer(NFPlayer *pPlayer);

private:
    /**
     * @brief playerId => NFPlayer's globalId
     */
    NFShmHashMap<uint64_t, uint64_t, 1000000> m_playerIdMap;
DECLARE_IDCREATE(NFPlayerMgr);
};