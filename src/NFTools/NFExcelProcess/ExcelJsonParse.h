// -------------------------------------------------------------------------
//    @FileName         :    ExcelJsonParse.h
//    @Author           :    gaoyi
//    @Date             :    23-8-26
//    @Email			:    445267987@qq.com
//    @Module           :    ExcelJsonParse
//
// -------------------------------------------------------------------------

#pragma once

#include "FrameComm.nanopb.h"
#include "NFComm/NFCore/NFSingleton.hpp"

class ExcelJsonParse : public NFSingleton<ExcelJsonParse> {
public:
    int Init(const std::string& jsonFile);

    ExcelSheetInfo* GetSheetInfo(const std::string& excelName, const std::string& sheetName);
    ExcelPbInfo* GetExcelInfo(const std::string& excelName);
    ExcelParseInfo m_excelInfo;
};


