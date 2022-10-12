// -------------------------------------------------------------------------
//    @FileName         :    NFWorldPlayerIDHashTable.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFWorldPlayerIDHashTable.h
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFShmCore/NFShmObj.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmCore/NFISharedMemModule.h"
#include "NFLogicCommon/NFServerFrameTypeDefines.h"
#include "NFComm/NFShmCore/NFShmHashMap.h"
#include "NFWorldCommonDefine.h"
#include <vector>

class NFWorldPlayer;
class NFWorldPlayerMgr : public NFShmObj {
public:
    NFWorldPlayerMgr(NFIPluginManager* pPluginManager);
    virtual ~NFWorldPlayerMgr();

    int CreateInit();
    int ResumeInit();

    virtual void OnTimer(int timeId, int callcount) override;
public:
    /**
     * @brief
     * @return
     */
    int PlayerTick();
public:

    /**
     * @brief ͨ��playerId��ȡ�������
     * @param playerId
     * @return
     */
    NFWorldPlayer *GetPlayer(uint64_t playerId);

    /**
     * @brief ͨ��playerId�����������, �����Ҵ��ڣ�����NULL
     * @param playerId
     * @return
     */
    NFWorldPlayer *CreatePlayer(uint64_t playerId);

    /**
     * @brief ɾ���������
     * @param pPlayer
     * @return
     */
    int DeletePlayer(NFWorldPlayer *pPlayer);

public:
    /**
     * @brief ͨ��cid�ҵ����
     * @param cid ��ɫID
     * @return
     */
    NFWorldPlayer *GetPlayerByCid(uint64_t cid);

    /**
     * @brief ɾ��cid
     * @param cid
     * @return
     */
    int EraseCid(uint64_t cid);

public:
    /**
     * @brief
     * @param playerId
     * @return
     */
    int DeleteClientIdByPlayerId(uint64_t playerId);

    /**
     * @brief
     * @param playerId
     * @param clientId
     * @return
     */
    int InsertClientByPlayerId(uint64_t playerId, uint64_t clientId);

    /**
     * @brief
     * @param playerId
     * @return
     */
    int GetClientIdByPlayerId(uint64_t playerId);


private:
    /**
     * @brief ���ticker��ʱ
     */
    int m_playerTickTimer;

    /**
     * @brief ���charId��playerIdӳ�䣬֧���������=WORLD_SERVER_MAX_ONLINE_COUNT
     */
    NFShmHashMap<uint64_t, uint64_t, WORLD_SERVER_MAX_ONLINE_COUNT> m_charIdToPlayerIdMap;

    /**
     * @brief ���playerId��ClientIdӳ�䣬֧���������=WORLD_SERVER_MAX_ONLINE_COUNT
     */
    NFShmHashMap<uint64_t, uint64_t, WORLD_SERVER_MAX_ONLINE_COUNT> m_playerIdToClientIdMap;

    /**
     * @brief ClientId��PlayerIdӳ�䣬֧���������=WORLD_SERVER_MAX_ONLINE_COUNT
     */
    NFShmHashMap<uint64_t, uint64_t, WORLD_SERVER_MAX_ONLINE_COUNT> m_clientIdtoPlayerIdMap;
DECLARE_IDCREATE(NFWorldPlayerMgr)
};