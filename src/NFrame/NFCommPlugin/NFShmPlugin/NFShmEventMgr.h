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
#include <NFComm/NFPluginModule/NFEventTemplate.h>
#include <NFComm/NFShmStl/NFShmHashMap.h>

#include "NFComm/NFObjCommon/NFNodeList.h"
#include "NFShmEventKey.h"
#include "NFShmSubscribeInfo.h"

#define NF_SHM_EVENT_KEY_MAX_NUM 50000
#define NF_SHM_OBJ_MAX_EVENT_NUM 100

class NFShmEventMgr final : public NFObjectTemplate<NFShmEventMgr, EOT_TYPE_EVENT_MGR, NFObject>
{
public:
    NFShmEventMgr();

    ~NFShmEventMgr() override;

public:
    int CreateInit();

    int ResumeInit();

    /**
    * @brief 订阅事件
    *
    * @param pSink		订阅对象
    * @param serverType
    * @param eventId	事件ID
    * @param srcType	事件源类型，玩家类型，怪物类型之类的
    * @param srcId		事件源ID，一般都是玩家，生物唯一id
    * @param desc		事件描述，用于打印，获取信息，查看BUG之类的
    * @return			订阅事件是否成功
    */
    int Subscribe(NFObject* pSink, NF_SERVER_TYPE serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const std::string& desc);

    /**
    * @brief 取消订阅事件
    *
    * @param pSink		订阅对象
    * @param serverType
    * @param eventId	事件ID
    * @param srcType	事件源类型，玩家类型，怪物类型之类的
    * @param srcId		事件源ID，一般都是玩家，生物唯一id
    * @return			取消订阅事件是否成功
    */
    int UnSubscribe(const NFObject* pSink, NF_SERVER_TYPE serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId);

    /**
    * @brief 取消pSink所有订阅事件
    *
    * @param pSink		订阅对象
    * @return			取消订阅事件是否成功
    */
    int UnSubscribeAll(const NFObject* pSink);
    int UnSubscribeAll(int globalId);

    /**
    * @brief 发送事件,并执行收到事件的对象的对应函数
    *
    * @param serverType
    * @param eventId		事件ID
    * @param srcType		事件源类型，玩家类型，怪物类型之类的
    * @param srcId			事件源ID，一般都是玩家，生物唯一id
    * @param message	事件传输的数据
    * @return				执行是否成功
    */
    int Fire(NF_SERVER_TYPE serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const google::protobuf::Message& message);

private:
    /**
    * @brief 删除skey的链表里的pSink
    *
    * @param pLastNode
    * @return			删除skey的链表里的pSink是否成功
    */
    int DelEventKeyListSubscribeInfo(NFShmSubscribeInfo* pLastNode);

    /**
    * @brief 执行所有订阅事件key的函数
    *
    * @param key			事件合成key，skey.nsrcid可能为0，可能=nEventID
    * @param serverType     服务器类型
    * @param eventId		事件ID
    * @param srcType		事件源类型，玩家类型，怪物类型之类的
    * @param srcId			事件源ID，一般都是玩家，生物唯一id
    * @param message	    事件传输的数据
    * @return				执行是否成功
    */
    int Fire(const NFShmEventKey& key, uint32_t serverType, uint32_t eventId, uint32_t srcType, uint64_t srcId, const google::protobuf::Message& message);

private:
    /**
     * @brief
     */
    NFShmHashMap<NFShmEventKey, NFNodeObjMultiList<NFShmSubscribeInfo>, NF_SHM_EVENT_KEY_MAX_NUM> m_eventKeyAllSubscribe;

    /**
     * @brief
     */
    NFShmHashMap<int, NFNodeObjMultiList<NFShmSubscribeInfo>, NF_SHM_EVENT_KEY_MAX_NUM> m_shmObjAllSubscribe;

    /**
     * @brief
     */
    int32_t m_nFireLayer;
};
