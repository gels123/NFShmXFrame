// -------------------------------------------------------------------------
//    @FileName         :    NFNodeList.cpp
//    @Author           :    gaoyi
//    @Date             :    22-11-14
//    @Email			:    445267987@qq.com
//    @Module           :    NFNodeList
//
// -------------------------------------------------------------------------

#include "NFNodeList.h"

int NFIListNodeObj::m_iListCheckIdSeq = 0;

int NFIListNodeObj::GetListCheckIdSeq()
{
    m_iListCheckIdSeq++;

    if (m_iListCheckIdSeq <= 0)
    {
        m_iListCheckIdSeq = 1;
    }

    return m_iListCheckIdSeq;
}

