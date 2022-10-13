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
#include "NFLoginQueue.h"
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
     * @brief �Ŷ�
     */

    uint32_t GetCurrentLoginNum() const;

    void SetCurrentLoginNum(uint32_t currentLoginNum);

    uint32_t GetMaxQueueNum() const;

    void SetMaxQueueNum(uint32_t maxQueueNum);

    uint32_t GetStartQueueNum() const;

    void SetStartQueueNum(uint32_t startQueueNum);

    /**
     * @brief �Ƿ���Ҫ�Ŷ�
     * @return
     */
    bool IsNeedLoginQueue() const;

    /**
     * @brief �Ƿ��ŶӶ����Ѿ�����
     * @return
     */
    bool IsLoginQueueFull() const;

    /**
     * @brief �Ƿ����ŶӶ�����
     * @param playerId
     * @return
     */
    bool IsInLoginQueue(uint64_t playerId) const;

     /**
      * @brief ���뵽��½�Ŷ���
      * @param playerId
      * @return
      */
    bool InsertLoginQueue(uint64_t playerId);

    /**
     * @brief ͨ�����ID����Ŷ�����
     * @param playerId
     * @return
     */
    NFLoginQueue* GetLoginQueue(uint64_t playerId);

    /**
     * @brief ��õ�ǰ�Ŷ�����
     * @return
     */
    uint32_t GetLoginQueueNum() const;

    /**
     * @brief ɾ����ҵ��Ŷ���Ϣ
     * @param playerId
     * @return
     */
    int DeleteLoginQueue(uint64_t playerId);
public:

private:
    /**
     * @brief ���ticker��ʱ
     */
    int m_playerTickTimer;

    /**
     * @brief ���charId��playerIdӳ�䣬֧���������=WORLD_SERVER_MAX_ONLINE_COUNT
     */
    NFShmHashMap<uint64_t, uint64_t, WORLD_SERVER_MAX_ONLINE_COUNT> m_charIdToPlayerIdMap;

private:
    /**
     * @brief �Ŷ�
     */

    /**
     * @brief ��¼�Ŷ�uid����
     */
    NFShmHashMap<uint64_t, NFLoginQueue, WORLD_SERVER_MAX_ONLINE_COUNT> m_loginQueueMap;

    /**
     * @brief �����Ѿ�����ĵ�½������  ������ѡ��ɫ����ͽ�����Ϸ��
     */
    uint32_t m_currentLoginNum;

    /**
     * @brief ����Ŷӵ�����
     */
    uint32_t m_maxQueueNum;

    /**
     * @brief ��ʼ�Ŷ�����
     */
    uint32_t m_startQueueNum;
DECLARE_IDCREATE(NFWorldPlayerMgr)
};