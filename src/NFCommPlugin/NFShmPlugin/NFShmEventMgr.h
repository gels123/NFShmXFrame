// -------------------------------------------------------------------------
//    @FileName         :    NFShmEventMgr.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFShmEventMgr
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include <stddef.h>
#include <string.h>
#include <NFComm/NFPluginModule/NFEventTemplate.h>
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmCore/NFShmPtr.h"
#include "NFComm/NFShmCore/NFShmHashMap.h"
#include "NFComm/NFShmCore/NFShmNodeList.h"
#include "NFShmEventKey.h"
#include "NFShmSubscribeInfo.h"

#define NF_SHM_EVENT_KEY_MAX_NUM 50000
#define NF_SHM_OBJ_MAX_EVENT_NUM 100

class NFShmEventMgr : public NFShmObj
{
public:
    NFShmEventMgr(NFIPluginManager *pPluginManager);

    virtual ~NFShmEventMgr();

public:
    int CreateInit();

    int ResumeInit();

    /**
    * @brief �����¼�
    *
    * @param pSink		���Ķ���
    * @param nEventID	�¼�ID
    * @param nSrcID		�¼�ԴID��һ�㶼����ң�����Ψһid
    * @param bySrcType	�¼�Դ���ͣ�������ͣ���������֮���
    * @param desc		�¼����������ڴ�ӡ����ȡ��Ϣ���鿴BUG֮���
    * @return			�����¼��Ƿ�ɹ�
    */
    int Subscribe(NFShmObj *pSink, uint32_t nEventID, uint64_t nSrcID, uint32_t bySrcType, const std::string &desc)
    {
        if (nullptr == pSink) return -1;

        NFShmEventKey skey;
        skey.nEventID = nEventID;
        skey.nSrcID = nSrcID;
        skey.bySrcType = bySrcType;

        /**
        *@brief ���ж�ָ��pSink������û��ע�ᣬȻ���skey����
        *       ���ָ��ĵļ�������skey�Ѿ����ڣ�
        *       ˵���Ѿ����룬ֱ���˳�
        */
        auto pArray = m_mapAllSubscribeKey.Find(pSink->GetGlobalID());
        if (pArray == NULL)
        {
            pArray = m_mapAllSubscribeKey.Insert(pSink->GetGlobalID());
            CHECK_EXPR(pArray, -1, "m_mapAllSubscribeKey Insert Failed, Space Not Enough, desc:{}", desc);
            int ret = pArray->Add(skey);
            CHECK_EXPR(ret >= 0, -1, "m_mapAllSubscribeKey Arrray Add Failed, Space Not Enough, desc:{}", desc);
        }
        else
        {
            int ret = pArray->Add(skey);
            CHECK_EXPR(ret >= 0, -1, "m_mapAllSubscribeKey Arrray Add Failed, Space Not Enough, desc:{}", desc);
        }

        /**
        *@brief �ж�skey��û�д��ڣ��Ѷ������skey��������
        */
        NFShmSubscribeInfo *pInfo = dynamic_cast<NFShmSubscribeInfo *>(FindModule<NFISharedMemModule>()->CreateObj(EOT_TYPE_SUBSCRIBEINFO_OBJ));
        CHECK_EXPR(pInfo, -1, "CreateObj NFShmSubscribeInfo Failed, desc:{}", desc);
        pInfo->pSink = pSink;
        pInfo->szDesc = desc;
        auto pNodeList = m_mapAllSubscribeObj.Find(skey);
        if (pNodeList)
        {
            pNodeList->AddNode(m_pObjPluginManager, pInfo);
        }
        else
        {
            pNodeList = m_mapAllSubscribeObj.Insert(skey);
            CHECK_EXPR(pNodeList, -1, "m_mapAllSubscribeObj Insert Failed, Space Not Enough, desc:{}", desc);
            pNodeList->AddNode(m_pObjPluginManager, pInfo);
        }
        return 0;
    }

    /**
    * @brief ȡ�������¼�
    *
    * @param pSink		���Ķ���
    * @param nEventID	�¼�ID
    * @param nSrcID		�¼�ԴID��һ�㶼����ң�����Ψһid
    * @param bySrcType	�¼�Դ���ͣ�������ͣ���������֮���
    * @return			ȡ�������¼��Ƿ�ɹ�
    */
    int UnSubscribe(NFShmObj *pSink, uint32_t nEventID, uint64_t nSrcID, uint32_t bySrcType)
    {
        if (nullptr == pSink) return -1;

        NFShmEventKey skey;
        skey.nEventID = nEventID;
        skey.nSrcID = nSrcID;
        skey.bySrcType = bySrcType;

        /**
        *@brief �ж�pSinkָ�������û�д��ڣ�������ֱ���˳�
        *		���ڵĻ���ɾ����Ӧ��key, ���pSink����Ϊ�յĻ���
        *       ɾ��pSink
        */
        auto pArray = m_mapAllSubscribeKey.Find(pSink->GetGlobalID());
        if (pArray == NULL)
        {
            return -1;
        }

        bool notHas = true;
        for (int i = 0; i < pArray->Size();)
        {
            if (skey == (*pArray)[i])
            {
                pArray->Del(i);
                notHas = false;
            }
            else {
                i++;
            }
        }

        if(notHas)
        {
            return -1;
        }

        if (pArray->Size() <= 0)
        {
            m_mapAllSubscribeKey.Erase(pSink->GetGlobalID());
        }

        /**
        *@brief ɾ��skey�������pSink
        */
        DelSubcribeInfo(pSink, skey);

        return 0;
    }

    /**
    * @brief ȡ��pSink���ж����¼�
    *
    * @param pSink		���Ķ���
    * @return			ȡ�������¼��Ƿ�ɹ�
    */
    int UnSubscribeAll(NFShmObj *pSink)
    {
        if (nullptr == pSink) return false;

        auto pArray = m_mapAllSubscribeKey.Find(pSink->GetGlobalID());
        if (pArray == NULL)
        {
            return -1;
        }

        for (int i = 0; i < (int)pArray->Size(); i++)
        {
            DelSubcribeInfo(pSink, (*pArray)[i]);
        }

        m_mapAllSubscribeKey.Erase(pSink->GetGlobalID());

        return true;
    }

    /**
    * @brief �����¼�,��ִ���յ��¼��Ķ���Ķ�Ӧ����
    *
    * @param nEventID		�¼�ID
    * @param nSrcID			�¼�ԴID��һ�㶼����ң�����Ψһid
    * @param bySrcType		�¼�Դ���ͣ�������ͣ���������֮���
    * @param pEventContext	�¼����������
    * @return				ִ���Ƿ�ɹ�
    */
    /*
    * ������в�����ܵ�������, �����ᵼ�±���, ��������Ԥ��Ĳ�һ��:
    * ����1:��������Fire�¼����ͬ��key��ɾ����ͬ��pSink,
    *		���ܵ��½�Ҫִ�е��¼���ɾ�������������Ԥ�����Ʋ�һ��
    * ����2:��������Fire�¼����ͬ��key��ɾ����ͬ��pSink, �����¼�ϵͳ����SubscribeInfo��Add,Sub���ü�������Ԥ����
    *       ��������������ɾ�������ᵼ��std::list������ʧЧ�� ����ɾ�����ᵼ������
    * ����3:��������Fire�¼�� Fire�˱���¼����ᵼ�µ������⣬�¼�ϵͳ�Ѿ�������Ԥ���� ��ͬ���¼���������5�Σ�
    *       ���е�Fire�¼�������20��
    */
    bool Fire(uint32_t nEventID, uint64_t nSrcID, uint32_t bySrcType, const google::protobuf::Message &message)
    {
        SEventKey skey;
        skey.nEventID = nEventID;
        skey.nSrcID = nSrcID;
        skey.bySrcType = bySrcType;

        /**
        * @brief ��ִ����ȫƥ���
        */
        if (skey.nSrcID != 0)
        {
            bool bRes = Fire(skey, nEventID, nSrcID, bySrcType, message);
            if (!bRes)
            {
                return false;
            }
        }

        /**
        * @brief ��ִ�У� ��������¼�nEventID,����ΪbySrcType
        * ���綩��ʱ�������������������¼��������Ƕ�һ����ҵ��¼���
        * ����ʱ��nSrcId=0�����ܵ�������Ҳ����ĸ����¼�
        */
        skey.nSrcID = 0;
        bool bRes = Fire(skey, nEventID, nSrcID, bySrcType, message);
        if (!bRes)
        {
            return false;
        }
        return true;
    }

private:
    /**
    * @brief ɾ��skey���������pSink
    *
    * @param pSink		���Ķ���
    * @param SEventKey	�¼��ϳ�key
    * @return			ɾ��skey���������pSink�Ƿ�ɹ�
    */
    bool DelSubcribeInfo(NFShmObj *pSink, const NFShmEventKey &skey)
    {
        auto pNodeList = m_mapAllSubscribeObj.Find(skey);
        if (pNodeList)
        {
            NFShmSubscribeInfo* pNode = pNodeList->GetHeadNodeObj(m_pObjPluginManager);
            while(pNode)
            {
                if (pNode->pSink.GetPoint() == pSink)
                {
                    if (pNode->nRefCount == 0)
                    {
                        pNodeList->RemoveNode(m_pObjPluginManager, pNode);
                    }
                    else
                    {
                        pNode->bRemoveFlag = true;
                    }
                    break;
                }
                pNode = pNodeList->GetNextNodeObj(m_pObjPluginManager, pNode);
            }

            if (pNodeList->GetNodeCount() == 0)
            {
                m_mapAllSubscribeObj.Erase(skey);
            }
        }

        return true;
    }

    /**
    * @brief ִ�����ж����¼�key�ĺ���
    *
    * @param skey			�¼��ϳ�key��skey.nsrcid����Ϊ0������=nEventID
    * @param nEventID		�¼�ID
    * @param nSrcID			�¼�ԴID��һ�㶼����ң�����Ψһid
    * @param bySrcType		�¼�Դ���ͣ�������ͣ���������֮���
    * @param pEventContext	�¼����������
    * @return				ִ���Ƿ�ɹ�
    */
    bool Fire(const SEventKey &skey, uint32_t nEventID, uint64_t nSrcID, uint32_t bySrcType,
              const google::protobuf::Message &message)
    {
        m_nFireLayer++;
        if (m_nFireLayer >= EVENT_FIRE_MAX_LAYER)
        {
            NFLogError(NF_LOG_SYSTEMLOG, 0,
                       "[Event] m_nFireLayer >= EVENT_FIRE_MAX_LAYER.....nEventID:{}, nSrcID:{}, bySrcType:{}, fireLayer:{}",
                       nEventID, nSrcID, bySrcType, m_nFireLayer);
            m_nFireLayer--;
            return false;
        }

        auto iterLst = m_mapAllSubscribeObj.find(skey);
        if (iterLst != m_mapAllSubscribeObj.end())
        {
            for (auto iter = iterLst->second.begin(); iter != iterLst->second.end();)
            {
                SubscribeInfo *pSubscribeInfo = &(*iter);
                if (pSubscribeInfo->nRefCount >= EVENT_REF_MAX_CNT)
                {
                    NFLogError(NF_LOG_SYSTEMLOG, 0,
                               "[Event] pSubscribeInfo->nRefCount >= EVENT_REF_MAX_CNT....eventid:{}, srcid:{}, type:{}, refcont:{}, removeflag:{}, szdesc:{}",
                               nEventID, nSrcID, bySrcType, pSubscribeInfo->nRefCount,
                               static_cast<int32_t>(pSubscribeInfo->bRemoveFlag), pSubscribeInfo->szDesc);
                    m_nFireLayer--;
                    return false;
                }
                if (!pSubscribeInfo->bRemoveFlag)
                {
                    int bRes = 0;
                    try
                    {
                        pSubscribeInfo->Add();
                        bRes = m_FireEventObj(pSubscribeInfo->pSink, nEventID, nSrcID, bySrcType, message);
                        pSubscribeInfo->Sub();
                    }
                    catch (...)
                    {
                        NFLogError(NF_LOG_SYSTEMLOG, 0,
                                   "[Event] pSubscribeInfo->nRefCount >= EVENT_REF_MAX_CNT....eventid:{}, srcid:{}, type:{}, refcont:{}, removeflag:{}, szdesc:{}",
                                   nEventID, nSrcID, bySrcType, pSubscribeInfo->nRefCount,
                                   static_cast<int32_t>(pSubscribeInfo->bRemoveFlag), pSubscribeInfo->szDesc);
                        m_nFireLayer--;
                        return false;
                    }
                    if (pSubscribeInfo->bRemoveFlag && 0 == pSubscribeInfo->nRefCount)
                    {
                        iter = iterLst->second.erase(iter);
                    }
                    else
                    {
                        ++iter;
                    }
                    if (bRes != 0)
                    {
                        NFLogError(NF_LOG_SYSTEMLOG, 0,
                                   "[Event] ret != 0 ....eventid:{}, srcid:{}, type:{}, refcont:{}, removeflag:{}, szdesc:{}",
                                   nEventID, nSrcID, bySrcType, pSubscribeInfo->nRefCount,
                                   static_cast<int32_t>(pSubscribeInfo->bRemoveFlag), pSubscribeInfo->szDesc);
                    }
                } // end of if (!subInfo.bRemoveFlag)
                else
                {
                    if (0 == pSubscribeInfo->nRefCount)
                    {
                        iter = iterLst->second.erase(iter);
                    }
                    else
                    {
                        ++iter;
                    }
                }
            } // end of for (; iter != pLstSubscribe->end();)

            if (iterLst->second.empty())
            {
                m_mapAllSubscribeObj.erase(iterLst);
            }
        } // enf of if (iterLst != m_mapAllSubscribeObj.end())

        m_nFireLayer--;

        return true;
    }

public:
    //
    NFShmHashMap<NFShmEventKey, NFShmNodeObjList<NFShmSubscribeInfo>, NF_SHM_EVENT_KEY_MAX_NUM> m_mapAllSubscribeObj;
    //
    NFShmHashMap<int, NFArray<NFShmEventKey, NF_SHM_OBJ_MAX_EVENT_NUM>, NF_SHM_EVENT_KEY_MAX_NUM> m_mapAllSubscribeKey;
    //
    int32_t m_nFireLayer;
DECLARE_IDCREATE(NFShmEventMgr)
};