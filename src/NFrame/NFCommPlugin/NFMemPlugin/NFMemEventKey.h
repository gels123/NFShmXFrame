// -------------------------------------------------------------------------
//    @FileName         :    NFMemEventKey
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFMemEventKey
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include <NFComm/NFCore/NFHash.hpp>
#include "NFComm/NFObjCommon/NFShmMgr.h"


#pragma pack(push)
#pragma pack(1)

class NFMemEventKey
{
public:
    NFMemEventKey()
    {
        if (EN_OBJ_MODE_INIT == NFShmMgr::Instance()->GetCreateMode())
        {
            CreateInit();
        }
        else
        {
            ResumeInit();
        }
    }

    int CreateInit()
    {
        m_srcId = 0;
        m_eventId = 0;
        m_srcType = 0;
        m_serverType = 0;
        return 0;
    }

    int ResumeInit()
    {
        return 0;
    }

    /**
    *@brief 事件主要的key，主要指玩家，生物唯一id
    */
    uint64_t m_srcId;

    /**
    *@brief 事件Id
    */
    uint32_t m_eventId;

    /**
    *@brief src类型, 用来区别玩家，怪物的类型
    */
    uint32_t m_srcType;

    /**
     * @brief 服务器类型，用来区分AllServer模式下，不同服务器的事件
     */
    uint32_t m_serverType;

    /**
    *@brief 判断是否相等
    */
    bool operator==(const NFMemEventKey& eventKey) const
    {
        return m_serverType == eventKey.m_serverType &&
            m_eventId == eventKey.m_eventId &&
            m_srcType == eventKey.m_srcType &&
            m_srcId == eventKey.m_srcId;
    }

    std::string ToString() const
    {
        return NF_FORMAT("nServerType:{} nEventID:{}, nSrcID:{}, bySrcType:{}", m_serverType, m_eventId, m_srcId, m_srcType);
    }
};

#pragma pack(pop)

/**
*@brief 求hash值
*/
namespace std
{
    template <>
    struct hash<NFMemEventKey>
    {
        size_t operator()(const NFMemEventKey& eventKey) const noexcept
        {
            return NFHash::hash_combine(eventKey.m_serverType, eventKey.m_eventId, eventKey.m_srcType, eventKey.m_srcId);
        }
    };
}
