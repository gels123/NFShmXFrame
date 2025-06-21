// -------------------------------------------------------------------------
//    @FileName         :    NFError.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFCore
//
// -------------------------------------------------------------------------

#include "NFError.h"
#include "FrameEnum.pb.h"
#include "NFComm/NFCore/NFCommon.h"

std::string NFError::GetErrorStr(int32_t retCode)
{
    if (NFrame::FrameErrorCode_IsValid(retCode))
    {
        return NFrame::FrameErrorCode_Name(static_cast<NFrame::FrameErrorCode>(retCode));
    }
    else
    {
        if (m_func)
        {
            return m_func(retCode);
        }
        else
        {
            return NFCommon::tostr(retCode);
        }
    }
}

void NFError::SetErrorFunction(const NFErrorFunction& func)
{
    m_func = func;
}

std::string GetErrorStr(int32_t retCode)
{
    return NFError::Instance()->GetErrorStr(retCode);
}




