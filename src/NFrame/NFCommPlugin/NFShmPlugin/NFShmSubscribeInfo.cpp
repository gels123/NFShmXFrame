// -------------------------------------------------------------------------
//    @FileName         :    NFShmSubcribeInfo.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFShmSubcribeInfo.h
//
// -------------------------------------------------------------------------

#include "NFShmSubscribeInfo.h"
#include "NFComm/NFObjCommon/NFTypeDefines.h"
#include "NFComm/NFPluginModule/NFIMemMngModule.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"

NFShmSubscribeInfo::NFShmSubscribeInfo()
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

int NFShmSubscribeInfo::CreateInit()
{
    m_pSink = NULL;
    m_refCount = 0;
    m_removeFlag = false;
    m_shmObjId = INVALID_ID;
    return 0;
}

int NFShmSubscribeInfo::ResumeInit()
{
    return 0;
}

/**
*@brief 增加引用
*/
void NFShmSubscribeInfo::Add()
{
    m_refCount++;
}

/**
*@brief 减少引用
*/
void NFShmSubscribeInfo::Sub()
{
    --m_refCount;
}

std::string NFShmSubscribeInfo::ToString() const
{
    return NF_FORMAT("refCount:{},removeFlag:{},desc:{},eventKey:{},shmObjId:{}", m_refCount, m_removeFlag, m_szDesc.ToString(), m_eventKey.ToString(), m_shmObjId);
}
