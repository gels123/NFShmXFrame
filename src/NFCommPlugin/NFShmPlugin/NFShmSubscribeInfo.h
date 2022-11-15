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
#include <stddef.h>
#include <string.h>
#include <NFComm/NFShmCore/NFTypeDefines.h>
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmCore/NFShmObj.h"
#include "NFComm/NFShmCore/NFShmPtr.h"
#include "NFComm/NFShmCore/NFSizeString.h"
#include "NFComm/NFShmCore/NFShmNodeList.h"

/**
 *@brief �¼�������Ϣ
 */
class NFShmSubscribeInfo : public NFShmObj, public NFListNodeObjWithGlobalID<NFShmSubscribeInfo, EOT_TYPE_SUBSCRIBEINFO_OBJ>
{
public:
    NFShmSubscribeInfo(NFIPluginManager* pPluginManager);

    int CreateInit();

    int ResumeInit();

    /**
    *@brief ��������
    */
    void Add();

    /**
    *@brief ��������
    */
    void Sub();
public:
    /**
    *@brief �¼�����
    */
    NFShmPtr<NFShmObj> pSink;

    /**
    *@brief ���ô���
    */
    int32_t nRefCount;

    /**
    *@brief �Ƴ���־
    */
    bool bRemoveFlag;

    /**
    *@brief ������Ϣ
    */
    NFSizeString<16> szDesc;
DECLARE_IDCREATE(NFShmSubscribeInfo)
};
