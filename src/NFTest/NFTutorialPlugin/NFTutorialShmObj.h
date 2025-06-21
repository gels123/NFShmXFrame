// -------------------------------------------------------------------------
//    @FileName         :    NFTutorialShmObj.h
//    @Author           :    gaoyi
//    @Date             :    24-1-13
//    @Email			:    445267987@qq.com
//    @Module           :    NFTutorialShmObj
//
// -------------------------------------------------------------------------

#pragma once

#include <FrameTest.nanopb.h>
#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFObjCommon/NFObject.h"
#include "NFComm/NFShmStl/NFShmHashMapWithList.h"
#include "NFComm/NFShmStl/NFShmHashTableWithList.h"


class NFTutorialTestData
{
public:
    NFTutorialTestData()
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
        NFLogInfo(NF_LOG_DEFAULT, 0, "CreateInit");
        m_test = 0;
        m_intVec.push_back(1);
        m_intVec.push_back(2);
        m_intVec.push_back(3);
        m_intVec.push_back(4);
        return 0;
    }

    int ResumeInit()
    {
        m_test = 10;
        NFLogInfo(NF_LOG_DEFAULT, 0, "ResumeInit");
        for (int i = 0; i < (int)m_intVec.size(); i++)
        {
            std::cout << m_intVec[i] << std::endl;
        }
        return 0;
    }

    NFShmVector<int, 10> m_intVec;
    int m_test;
};

class NFTutorialShmObj : public NFObjectTemplate<NFTutorialShmObj, 100, NFObject>
{
public:
    NFTutorialShmObj();
    
    virtual ~NFTutorialShmObj();
    
    int CreateInit();
    int ResumeInit();

    virtual int OnTimer(int timeId, int callcount);
private:
    uint32_t m_idCount;
    int m_timerId;
    NFTutorialTestData m_data;
    NFShmVector<NFTutorialTestData, 10> m_test;
};