// -------------------------------------------------------------------------
//    @FileName         :    NFDragonPart.h
//    @Author           :    gaoyi
//    @Date             :    23-10-20
//    @Email			:    445267987@qq.com
//    @Module           :    NFDragonPart
//
// -------------------------------------------------------------------------

#pragma once


#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFShmCore/NFShmObj.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "Part/NFPart.h"
#include "NFLogicCommon/NFLogicShmTypeDefines.h"
#include "NFComm/NFShmCore/NFISharedMemModule.h"

class NFDragonPart : public NFShmObjTemplate<NFDragonPart, EOT_LOGIC_PART_ID+PART_DRAGON, NFPart>
{
public:
    NFDragonPart();

    virtual ~NFDragonPart();

    int CreateInit();

    int ResumeInit();
    
    //ж�ص�ǰ�⻷
    bool UnDressHalo(bool sync_facade) { return true; }
};