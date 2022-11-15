// -------------------------------------------------------------------------
//    @FileName         :    NFShmEventKey
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFShmEventKey
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFPluginModule/NFLogMgr.h"
#include <stddef.h>
#include <string.h>
#include <NFComm/NFCore/NFHash.hpp>
#include "NFComm/NFShmCore/NFShmMgr.h"

class NFShmEventKey
{
public:
    NFShmEventKey()
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
        nSrcID = 0;
        nEventID = 0;
        bySrcType = 0;
        return 0;
    }

    int ResumeInit()
    {
        return 0;
    }

    /**
    *@brief �¼���Ҫ��key����Ҫָ��ң�����Ψһid
    */
    uint64_t nSrcID;

    /**
    *@brief �¼�Id
    */
    uint32_t nEventID;

    /**
    *@brief src����, ����������ң����������
    */
    uint32_t bySrcType;

    /**
    *@brief �ж��Ƿ����
    */
    bool operator ==(const NFShmEventKey& eventKey) const
    {
        return ((nSrcID == eventKey.nSrcID) &&
                (nEventID == eventKey.nEventID) &&
                (bySrcType == eventKey.bySrcType));
    }

    /**
    *@brief �ж��Ƿ�С��, ��֪����û�и��õ��ж�С�ڵķ���
    */
    bool operator <(const NFShmEventKey& eventKey) const
    {
        if (nSrcID < eventKey.nSrcID)
        {
            return true;
        }
        else if (nSrcID > eventKey.nSrcID)
        {
            return false;
        }
        else
        {
            if (nEventID < eventKey.nEventID)
            {
                return true;
            }
            else if (nEventID > eventKey.nEventID)
            {
                return false;
            }
            else
            {
                if (bySrcType < eventKey.bySrcType)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }
    }
};