// -------------------------------------------------------------------------
//    @FileName         :    NFShmTransMng.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFShmTransMng.h
//
// -------------------------------------------------------------------------

#pragma once

#include <NFComm/NFShmStl/NFShmVector.h>

#include "NFComm/NFObjCommon/NFObject.h"
#include "NFComm/NFObjCommon/NFTickByRunIndexOP.h"
#define MAX_TOTAL_TRANS_NUM 500000

class NFTransBase;

class NFShmTransMng final : public NFObjectTemplate<NFShmTransMng, EOT_TRANS_MNG, NFObject>, public NFTickByRunIndexOP
{
public:
    NFShmTransMng();
    ~NFShmTransMng() override;

    int CreateInit();
    int ResumeInit();

    NFTransBase* CreateTrans(uint32_t bTransObjType);
    NFTransBase* GetTransBase(uint64_t ullTransId) const;

    int CheckAllTransFinished(bool& bAllTransFinished) const;
    int DoTick(uint32_t dwCurRunIndex, bool bIsTickAll = false) override;

    int GetTotalTransNum() const { return m_aiTransObjIdList.size(); }

    /*
     * 停服之前，检查服务器是否满足停服条件
     * */
    bool CheckStopServer() const;

    /*
     * 停服之前，做一些操作，满足停服条件
     * */
    bool StopServer();

protected:
    NFTransBase* CreateTransObj(uint32_t bTransObjType) const;
    NFTransBase* GetTransObj(int iIndex) const;

protected:
    NFShmVector<int, MAX_TOTAL_TRANS_NUM> m_aiTransObjIdList;
};


