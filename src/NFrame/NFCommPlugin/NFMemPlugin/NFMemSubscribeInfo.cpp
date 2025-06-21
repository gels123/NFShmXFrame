// -------------------------------------------------------------------------
//    @FileName         :    NFShmSubcribeInfo.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFShmSubcribeInfo.h
//
// -------------------------------------------------------------------------

#include "NFMemSubscribeInfo.h"
#include "NFComm/NFObjCommon/NFTypeDefines.h"
#include "NFComm/NFPluginModule/NFIMemMngModule.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"

NFMemSubscribeInfo::NFMemSubscribeInfo()
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

int NFMemSubscribeInfo::CreateInit()
{
    m_pSink = NULL;
    m_refCount = 0;
    m_removeFlag = false;
    m_shmObjId = INVALID_ID;
    return 0;
}

int NFMemSubscribeInfo::ResumeInit()
{
    return 0;
}

/**
*@brief 增加引用
*/
void NFMemSubscribeInfo::Add()
{
    m_refCount++;
}

/**
*@brief 减少引用
*/
void NFMemSubscribeInfo::Sub()
{
    --m_refCount;
}

std::string NFMemSubscribeInfo::ToString() const
{
    return NF_FORMAT("refCount:{},removeFlag:{},desc:{},eventKey:{},shmObjId:{}", m_refCount, m_removeFlag, m_szDesc.ToString(), m_eventKey.ToString(), m_shmObjId);
}
