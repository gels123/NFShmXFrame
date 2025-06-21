// -------------------------------------------------------------------------
//    @FileName         :    NFMemTransMng.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFMemTransMng.cpp
//
// -------------------------------------------------------------------------

#include "NFMemTransMng.h"
#include "NFComm/NFObjCommon/NFTransBase.h"
#include "NFComm/NFPluginModule/NFIMemMngModule.h"
#include "NFComm/NFObjCommon/NFShmMgr.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include "NFComm/NFPluginModule/NFCheck.h"

NFMemTransMng::NFMemTransMng()
{
    if (NFShmMgr::Instance()->GetCreateMode() == EN_OBJ_MODE_INIT)
    {
        CreateInit();
    }
    else
    {
        ResumeInit();
    }
}

NFMemTransMng::~NFMemTransMng()
{
}

int NFMemTransMng::CreateInit()
{
    return 0;
}

int NFMemTransMng::ResumeInit()
{
    return 0;
}

NFTransBase* NFMemTransMng::CreateTrans(uint32_t bTransObjType)
{
    if (m_aiTransObjIdList.size() >= m_aiTransObjIdList.max_size())
    {
        NFLogFatal(NF_LOG_DEFAULT, 0, "TransMng is FULL err, objType:{}, TotalNum:{}, MaxNum:{}", bTransObjType, m_aiTransObjIdList.size(), m_aiTransObjIdList.max_size());
        return nullptr;
    }

    NFTransBase* pTransBase = CreateTransObj(bTransObjType);
    CHECK_EXPR(pTransBase, NULL, "CreateTransObj Failed, TransObjType:{}", bTransObjType);

    m_aiTransObjIdList.push_back(pTransBase->GetGlobalId());
    NFLogDebug(NF_LOG_DEFAULT, 0, "Create Trans TotalNum:{} Info:{} Pointer:{}", m_aiTransObjIdList.size(), pTransBase->DebugString(), static_cast<void*>(pTransBase));

    return pTransBase;
}

NFTransBase* NFMemTransMng::GetTransBase(uint64_t ullTransId) const
{
    CHECK_EXPR(ullTransId < INT_MAX, NULL, "TrandID Max:{} IntMax:{}", ullTransId, INT_MAX);
    return NFTransBase::GetObjByGlobalId(ullTransId, true);
}

NFTransBase* NFMemTransMng::CreateTransObj(uint32_t bTransObjType) const
{
    return dynamic_cast<NFTransBase*>(FindModule<NFIMemMngModule>()->CreateObj(static_cast<int>(bTransObjType)));
}

NFTransBase* NFMemTransMng::GetTransObj(int iIndex) const
{
    NFTransBase* pTransBase = nullptr;
    if (iIndex >= 0 && iIndex < static_cast<int>(m_aiTransObjIdList.size()))
    {
        pTransBase = GetTransBase(m_aiTransObjIdList[iIndex]);
        if (nullptr == pTransBase)
        {
            LOG_ERR(0, -1, "This Trans  %d Is Invalid", m_aiTransObjIdList[iIndex]);
        }
    }

    return pTransBase;
}

int NFMemTransMng::CheckAllTransFinished(bool& bAllTransFinished) const
{
    bAllTransFinished = true;
    int iTransIndex = 0;
    while (iTransIndex < static_cast<int>(m_aiTransObjIdList.size()))
    {
        bAllTransFinished = false;
        NFTransBase* pTransBase = GetTransBase(m_aiTransObjIdList[iTransIndex]);
        if (pTransBase)
        {
            LOG_INFO(0, "Exist: TransID %d %d, Index %d, Class Type %d",
                     pTransBase->GetGlobalId(), m_aiTransObjIdList[iTransIndex], iTransIndex, pTransBase->GetClassType());
        }
        else
        {
            LOG_INFO(0, "NonExist: TransID %d Index %d.", m_aiTransObjIdList[iTransIndex], iTransIndex);
        }
        iTransIndex++;
    }
    return 0;
}

int NFMemTransMng::DoTick(uint32_t dwCurRunIndex, bool bIsTickAll)
{
    int iPerTickNumThisTime = m_iNumPerTick;

    while (m_iTickedNum < iPerTickNumThisTime || bIsTickAll)
    {
        if (m_iLastTickIndex >= static_cast<int>(m_aiTransObjIdList.size()))
        {
            m_iLastTickIndex = 0;
            m_bIsTickFinished = true;
            break;
        }

        NFTransBase* pTransBase = GetTransObj(m_iLastTickIndex);

        if (pTransBase)
        {
            if (pTransBase->GetGlobalId() == m_aiTransObjIdList[m_iLastTickIndex])
            {
                if (pTransBase->IsTimeOut())
                {
                    pTransBase->SetFinished(NFrame::ERR_CODE_SVR_SYSTEM_TIMEOUT); //time out
                }

                if (pTransBase->IsCanRelease())
                {
                    NFLogDebug(NF_LOG_DEFAULT, 0, "Free Trans END Index:{} Pointer:{} Info:{}", m_iLastTickIndex, static_cast<void*>(pTransBase), pTransBase->DebugString());
                    FindModule<NFIMemMngModule>()->DestroyObj(pTransBase);
                    m_aiTransObjIdList[m_iLastTickIndex] = m_aiTransObjIdList.back();
                    m_aiTransObjIdList.back() = 0;
                    m_aiTransObjIdList.pop_back();
                }
                else
                {
                    if (!pTransBase->IsFinished())
                    {
                        pTransBase->ProcessTick();
                    }
                }
            }
            else
            {
                NFLogFatal(NF_LOG_DEFAULT, 0, "Trans Index Err ObjGlobalID:{} != IndexGlobalID:{} ObjPointer:{} Info:{}", pTransBase->GetGlobalId(), m_aiTransObjIdList[m_iLastTickIndex], static_cast<void*>(pTransBase), pTransBase->DebugString());
            }

            m_iTickedNum++;
        }

        m_iLastTickIndex++;
    }

    return 0;
}

bool NFMemTransMng::CheckStopServer() const
{
    bool bAllTransFinished;
    CheckAllTransFinished(bAllTransFinished);
    return bAllTransFinished;
}

bool NFMemTransMng::StopServer()
{
    return true;
}
