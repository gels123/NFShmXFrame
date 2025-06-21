// -------------------------------------------------------------------------
//    @FileName         :    ExcelToProto.h
//    @Author           :    gaoyi
//    @Date             :    23-8-19
//    @Email			:    445267987@qq.com
//    @Module           :    ExcelToProto
//
// -------------------------------------------------------------------------

#pragma once

#include "ExcelParse.h"

class ExcelToProto : public ExcelParse, public NFSingleton<ExcelToProto>
{
public:
    ExcelToProto();
    virtual ~ExcelToProto();
public:
    virtual int HandleExcel();
public:
    void WriteExcelProto();
    int WriteSheetProto(ExcelSheet* pSheet, std::string& write_str);
    int WriteSheetProto(ExcelSheet* pSheet, std::string& write_str, ExcelSheetColInfo *pColInfo);
    void WriteSheetDescStoreH(ExcelSheet* pSheet);
    void WriteSheetDescStoreExH();
    void WriteSheetDescStoreCpp(ExcelSheet* pSheet);
    void WriteSheetDescStoreExCpp();
    void WriteSheetDescStore();
    void WriteMakeFile();
    void WriteDestStoreDefine();
    int WriteRelation(ExcelSheet* pSheet, std::string& write_str, ExcelRelation* pRelation, ExcelSheetColInfo *pColInfo, int depth);
};

