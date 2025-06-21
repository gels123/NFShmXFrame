// -------------------------------------------------------------------------
//    @FileName         :    NFShmSubcribeInfo.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFShmSubcribeInfo.h
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include <NFComm/NFObjCommon/NFTypeDefines.h>
#include "NFComm/NFObjCommon/NFObject.h"
#include "NFComm/NFObjCommon/NFObjPtr.h"
#include "NFComm/NFShmStl/NFShmString.h"
#include "NFComm/NFObjCommon/NFNodeList.h"
#include "NFMemEventKey.h"

enum
{
    NF_SHM_SUBSCRIBEINFO_EVENT_KEY_INDEX_0 = 0,
    NF_SHM_SUBSCRIBEINFO_SHM_OBJ_INDEX_1 = 1,
    NF_SHM_SUBSCRIBEINFO_MAX_INDEX_NUM = 2,
};

/**
 *@brief 事件描述信息
 */
class NFMemSubscribeInfo final : public NFObjectTemplate<NFMemSubscribeInfo, EOT_TYPE_SUBSCRIBEINFO_OBJ, NFObject>, public NFMultiListNodeObjWithGlobalId<NFMemSubscribeInfo, NF_SHM_SUBSCRIBEINFO_MAX_INDEX_NUM>
{
public:
    NFMemSubscribeInfo();

    int CreateInit();

    int ResumeInit();

    /**
    *@brief 增加引用
    */
    void Add();

    /**
    *@brief 减少引用
    */
    void Sub();

    std::string ToString() const;

public:
    /**
    *@brief 事件对象
    */
    NFObjPtr<NFObject> m_pSink;

    /**
    *@brief 引用次数
    */
    int32_t m_refCount;

    /**
    *@brief 移除标志
    */
    bool m_removeFlag;

    /**
    *@brief 描述信息
    */
    NFShmString<32> m_szDesc;

    /**
     * @brief
     */
    NFMemEventKey m_eventKey;

    /**
     * @brief
     */
    int m_shmObjId;
};
