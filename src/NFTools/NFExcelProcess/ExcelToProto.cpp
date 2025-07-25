// -------------------------------------------------------------------------
//    @FileName         :    ExcelToProto.cpp
//    @Author           :    gaoyi
//    @Date             :    23-8-19
//    @Email			:    445267987@qq.com
//    @Module           :    ExcelToProto
//
// -------------------------------------------------------------------------

#include "ExcelToProto.h"

#include "ExcelJsonParse.h"


ExcelToProto::ExcelToProto()
{
}

ExcelToProto::~ExcelToProto()
{
}

int ExcelToProto::HandleExcel()
{
    int iRet = ExcelParse::HandleExcel();
    if (iRet != 0)
    {
        return iRet;
    }

    uint64_t startTime = NFGetTime();
    WriteExcelProto();
    WriteSheetDescStore();
    WriteMakeFile();
    double useTime = (NFGetTime() - startTime) / 1000.0;
    NFLogInfo(NF_LOG_DEFAULT, 0, "write execel proto use time:{}", useTime)

    return iRet;
}

void ExcelToProto::WriteExcelProto()
{
    std::string write_str;

    std::string excel_proto_file = m_outPath + "E_" + NFStringUtility::FirstUpper(m_excelName) + ".proto";

    write_str += "syntax = \"proto2\";\n\n";
    write_str += "package proto_ff;\n\n";
    write_str += "import \"nanopb.proto\";\n";

    XLWorkbook wxbook = m_excelReader.workbook();
    std::vector<std::string> vecSheet = wxbook.worksheetNames();
    std::unordered_set<std::string> protoFileMap;
    for (int i = 0; i < (int)vecSheet.size(); i++)
    {
        std::string sheetname = vecSheet[i];
        XLWorksheet sheet = wxbook.worksheet(sheetname);
        if (m_sheets.find(sheet.name()) != m_sheets.end() && m_sheets[sheet.name()].m_colInfoMap.size() > 0)
        {
            auto pSheet = &m_sheets[sheet.name()];
            for (auto iter = pSheet->m_protoFileMap.begin(); iter != pSheet->m_protoFileMap.end(); iter++)
            {
                protoFileMap.insert(*iter);
            }
        }
    }

    for (auto iter = protoFileMap.begin(); iter != protoFileMap.end(); iter++)
    {
        std::string proto_file_name = *iter;
        write_str += "import \"" + proto_file_name + "\";\n";
    }

    write_str += "\n";

    for (int i = 0; i < (int)vecSheet.size(); i++)
    {
        std::string sheetname = vecSheet[i];
        XLWorksheet sheet = wxbook.worksheet(sheetname);
        if (m_sheets.find(sheet.name()) != m_sheets.end() && m_sheets[sheet.name()].m_colInfoMap.size() > 0)
        {
            auto pSheet = &m_sheets[sheet.name()];
            if (pSheet->m_sheetMsgNameStr.empty() && pSheet->m_protoMsgNameStr.empty())
            {
                WriteSheetProto(pSheet, write_str);
            }
        }
    }

    NFFileUtility::WriteFile(excel_proto_file, write_str);
}

void ExcelToProto::WriteSheetDescStore()
{
    XLWorkbook wxbook = m_excelReader.workbook();
    std::vector<std::string> vecSheet = wxbook.worksheetNames();
    for (int i = 0; i < (int)vecSheet.size(); i++)
    {
        std::string sheetname = vecSheet[i];
        XLWorksheet sheet = wxbook.worksheet(sheetname);
        if (m_sheets.find(sheet.name()) != m_sheets.end() && m_sheets[sheet.name()].m_colInfoMap.size() > 0)
        {
            WriteSheetDescStoreH(&m_sheets[sheet.name()]);
            WriteSheetDescStoreCpp(&m_sheets[sheet.name()]);
        }
    }

    WriteSheetDescStoreExH();
    WriteSheetDescStoreExCpp();

    WriteDestStoreDefine();
}

int ExcelToProto::WriteSheetProto(ExcelSheet* pSheet, std::string& proto_file, ExcelSheetColInfo* pColInfo)
{
    CHECK_NULL(0, pColInfo);
    if (pColInfo->m_colInfoMap.empty()) return 0;
    if (pSheet->m_colMessageTypeMap.find(pColInfo->m_colFullName) != pSheet->m_colMessageTypeMap.end())
    {
        return 0;
    }

    for (auto iter = pColInfo->m_colInfoList.begin(); iter != pColInfo->m_colInfoList.end(); iter++)
    {
        std::string struct_en_name = pColInfo->m_structEnName;
        auto pNewColInfo = iter->second;
        CHECK_NULL(0, pNewColInfo);
        WriteSheetProto(pSheet, proto_file, pNewColInfo);
    }

    proto_file += "\nmessage " + pColInfo->m_pbDescName + "\n";
    proto_file += "{\n";

    int index = 0;

    for (auto iter = pColInfo->m_colInfoList.begin(); iter != pColInfo->m_colInfoList.end(); iter++)
    {
        index++;
        auto pSubColInfo = iter->second;
        CHECK_NULL(0, pSubColInfo);
        std::string col_en_name = pSubColInfo->m_structEnName;
        std::string col_cn_name = pSubColInfo->m_structCnName;
        NF_ENUM_COL_TYPE col_type = pSubColInfo->m_colCppType;
        std::string strJsonRepeatdNum = pSubColInfo->m_strParseRepeatedNum;
        std::string strJsonMessage = pSubColInfo->m_strParseMessage;
        uint32_t colTypeStrMaxSize = pSubColInfo->m_colTypeStrMaxSize;
        int32_t int_col_max_size = pColInfo->m_maxSubNum;
        if (pColInfo->m_strutNumFromZero)
        {
            int_col_max_size = pColInfo->m_maxSubNum + 1;
        }
        std::string col_max_size = NFCommon::tostr(int_col_max_size);

        if (!pSubColInfo->m_isArray && strJsonRepeatdNum.empty() && pSubColInfo->m_colInfoMap.empty())
        {
            if (col_type == NF_ENUM_COL_TYPE_INT32)
            {
                proto_file += "\toptional int32 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name + "\"";
            }
            else if (col_type == NF_ENUM_COL_TYPE_UINT32)
            {
                proto_file += "\toptional uint32 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name + "\"";
            }
            else if (col_type == NF_ENUM_COL_TYPE_INT64)
            {
                proto_file += "\toptional int64 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name + "\"";
            }
            else if (col_type == NF_ENUM_COL_TYPE_UINT64)
            {
                proto_file += "\toptional uint64 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name + "\"";
            }
            else if (col_type == NF_ENUM_COL_TYPE_FLOAT)
            {
                proto_file += "\toptional float " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name + "\"";
            }
            else if (col_type == NF_ENUM_COL_TYPE_DOUBLE)
            {
                proto_file += "\toptional double " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name + "\"";
            }
            else if (col_type == NF_ENUM_COL_TYPE_BOOL)
            {
                proto_file += "\toptional bool " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name + "\"";
            }
            else if (col_type == NF_ENUM_COL_TYPE_ENUM)
            {
                proto_file += "\toptional " + strJsonMessage + " " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name + "\"";
            }
            else if (col_type == NF_ENUM_COL_TYPE_STRING)
            {
                proto_file += "\toptional string " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\"," + " (nanopb).max_size_enum = \"" + get_string_define(colTypeStrMaxSize) + "\"";

                if (pSheet->m_createSql)
                {
                    proto_file += ", (nanopb).db_max_size_enum = \"" + get_string_define(colTypeStrMaxSize) + "\"";
                }
            }
            else if (col_type == NF_ENUM_COL_TYPE_DATE)
            {
                proto_file += "\toptional string " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\"," + " (nanopb).max_size_enum = \"" + get_string_define(colTypeStrMaxSize) + "\"];\n";

                index++;
                proto_file += "\toptional uint64 " + col_en_name + "_t = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name + "\"";

                if (pSheet->m_createSql)
                {
                    proto_file += ", (nanopb).db_max_size_enum = \"" + get_string_define(colTypeStrMaxSize) + "\"";
                }
            }
            else if (col_type == NF_ENUM_COL_TYPE_MESSAGE)
            {
                proto_file += "\toptional " + strJsonMessage + " " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", (nanopb).parse_type=FPT_JSON";
            }

            if (pSheet->m_createSql)
            {
                proto_file += ", (nanopb).db_comment = \"" + col_cn_name + "\"";
            }
        }
        else if (pSubColInfo->m_isArray && !strJsonRepeatdNum.empty() && pSubColInfo->m_colInfoMap.empty())
        {
            CHECK_EXPR(false, -1, "excel:{} sheet:{} not right, col:{} enName:{} cnName:{}", pSheet->m_excelName, pSheet->m_sheetName, pColInfo->m_colIndex, pColInfo->m_colType, pColInfo->m_structEnName, pColInfo->m_structCnName);
        }
        else if (pSubColInfo->m_isArray && strJsonRepeatdNum.empty() && pSubColInfo->m_colInfoMap.empty())
        {
            std::string nanopb_max_count = "(nanopb).max_count = " + col_max_size;
            if (!NFStringUtility::IsDigital(col_max_size))
            {
                nanopb_max_count = "(nanopb).max_count_enum = \"" + col_max_size + "\"";
            }
            if (col_type == NF_ENUM_COL_TYPE_INT32)
            {
                proto_file += "\trepeated int32 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_UINT32)
            {
                proto_file += "\trepeated uint32 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_INT64)
            {
                proto_file += "\trepeated int64 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_UINT64)
            {
                proto_file += "\trepeated uint64 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_FLOAT)
            {
                proto_file += "\trepeated float " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_DOUBLE)
            {
                proto_file += "\trepeated double " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_BOOL)
            {
                proto_file += "\trepeated bool " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_ENUM)
            {
                proto_file += "\trepeated " + strJsonMessage + " " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_STRING)
            {
                proto_file += "\trepeated string " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    + "\", " + nanopb_max_count + ", (nanopb).max_size_enum = \"" +
                    get_string_define(colTypeStrMaxSize) + "\"";

                if (pSheet->m_createSql)
                {
                    proto_file += ", (nanopb).db_max_size_enum = \"" + get_string_define(colTypeStrMaxSize) + "\"";
                }
            }
            else if (col_type == NF_ENUM_COL_TYPE_DATE)
            {
                proto_file += "\trepeated string " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    +"\", " + nanopb_max_count + ", (nanopb).max_size_enum = \"" +
                    get_string_define(colTypeStrMaxSize) + "\"];\n";

                index++;
                proto_file += "\trepeated uint64 " + col_en_name + "_t = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;

                if (pSheet->m_createSql)
                {
                    proto_file += ", (nanopb).db_max_size_enum = \"" + get_string_define(colTypeStrMaxSize) + "\"";
                }
            }
            else if (col_type == NF_ENUM_COL_TYPE_MESSAGE)
            {
                proto_file += "\trepeated " + strJsonMessage + " " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    +"\", " + nanopb_max_count + ", (nanopb).parse_type=FPT_JSON";

                if (pSheet->m_createSql)
                {
                    proto_file += ", (nanopb).db_max_size_enum = \"" + get_string_define(colTypeStrMaxSize) + "\"";
                }
            }

            if (pSheet->m_createSql)
            {
                proto_file += ", (nanopb).db_comment = \"" + col_cn_name + "\"";
                proto_file += ", (nanopb).db_max_count = " + col_max_size;
            }
        }
        else if (!pSubColInfo->m_isArray && !strJsonRepeatdNum.empty() && pSubColInfo->m_colInfoMap.empty())
        {
            if (!strJsonRepeatdNum.empty())
            {
                col_max_size = strJsonRepeatdNum;
            }

            std::string nanopb_max_count = "(nanopb).max_count = " + col_max_size;
            if (!NFStringUtility::IsDigital(col_max_size))
            {
                nanopb_max_count = "(nanopb).max_count_enum = \"" + col_max_size + "\"";
            }

            nanopb_max_count += ", (nanopb).parse_type=FPT_JSON";
            if (col_type == NF_ENUM_COL_TYPE_INT32)
            {
                proto_file += "\trepeated int32 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_UINT32)
            {
                proto_file += "\trepeated uint32 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_INT64)
            {
                proto_file += "\trepeated int64 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_UINT64)
            {
                proto_file += "\trepeated uint64 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_FLOAT)
            {
                proto_file += "\trepeated float " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_DOUBLE)
            {
                proto_file += "\trepeated double " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_BOOL)
            {
                proto_file += "\trepeated bool " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_ENUM)
            {
                proto_file += "\trepeated " + strJsonMessage + " " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_STRING)
            {
                proto_file += "\trepeated string " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    +"\", " + nanopb_max_count + ", (nanopb).max_size_enum = \"" +
                    get_string_define(colTypeStrMaxSize) + "\"";
            }
            else if (col_type == NF_ENUM_COL_TYPE_DATE)
            {
                proto_file += "\trepeated string " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    +"\", " + nanopb_max_count + ", (nanopb).max_size_enum = \"" +
                    get_string_define(colTypeStrMaxSize) + "\"];\n";

                index++;
                proto_file += "\trepeated uint64 " + col_en_name + "_t = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_MESSAGE)
            {
                proto_file += "\trepeated " + strJsonMessage + " " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    +"\", " + nanopb_max_count;
            }

            if (pSheet->m_createSql)
            {
                proto_file += ", (nanopb).db_max_size_enum = \"" + get_string_define(colTypeStrMaxSize) + "\"";
            }

            if (pSheet->m_createSql)
            {
                proto_file += ", (nanopb).db_comment = \"" + col_cn_name + "\"";
            }
        }
        else if (pSubColInfo->m_isArray && !pSubColInfo->m_colInfoMap.empty())
        {
            proto_file += "\trepeated " + pSubColInfo->m_pbDescName + " " + col_en_name + " = " + NFCommon::tostr(index) +
                "[(nanopb).field_cname = \"" + col_cn_name + "\"," + " (nanopb).max_count = " +
                col_max_size;

            if (pSheet->m_createSql)
            {
                proto_file += ", (nanopb).db_comment = \"" + col_cn_name + "\"";
                proto_file += ", (nanopb).db_max_count = " + col_max_size;
            }
        }
        else if (!pSubColInfo->m_isArray && !pSubColInfo->m_colInfoMap.empty())
        {
            proto_file += "\toptional " + pSubColInfo->m_pbDescName + " " + col_en_name + " = " + NFCommon::tostr(index) +
                "[(nanopb).field_cname = \"" + col_cn_name + "\"";

            if (pSheet->m_createSql)
            {
                proto_file += ", (nanopb).db_comment = \"" + col_cn_name + "\"";
                proto_file += ", (nanopb).db_message_expand = true";
            }
        }
        else
        {
            CHECK_EXPR(false, -1, "excel:{} sheet:{} not right, col:{} enName:{} cnName:{}", pSheet->m_excelName, pSheet->m_sheetName, pColInfo->m_colIndex, pColInfo->m_colType, pColInfo->m_structEnName, pColInfo->m_structCnName);
        }

        if (pSheet->m_colEnumMap.find(pSubColInfo->m_colFullName) != pSheet->m_colEnumMap.end())
        {
            proto_file += ", (nanopb).macro_type = \"" + pSheet->m_colEnumMap[pSubColInfo->m_colFullName].m_enumPbName + "\"";
        }

        proto_file += "];\n";
    }

    proto_file += "}\n";

    return 0;
}

int ExcelToProto::WriteSheetProto(ExcelSheet* pSheet, std::string& proto_file)
{
    std::string sheet_name = pSheet->m_sheetName;
    for (auto iter = pSheet->m_colInfoVec.begin(); iter != pSheet->m_colInfoVec.end(); iter++)
    {
        ExcelSheetColInfo* pColInfo = iter->second;
        WriteSheetProto(pSheet, proto_file, pColInfo);
    }

    proto_file += "\nmessage " + pSheet->m_protoInfo.m_protoMsgName + "\n";
    proto_file += "{\n";
    if (pSheet->m_createSql)
    {
        proto_file += "\toption (nanopb_msgopt).to_db_sql = true;\n";
    }

    int index = 0;
    for (auto iter = pSheet->m_colInfoVec.begin(); iter != pSheet->m_colInfoVec.end(); iter++)
    {
        ExcelSheetColInfo* pColInfo = iter->second;
        std::string col_en_name = pColInfo->m_structEnName;
        std::string col_cn_name = pColInfo->m_structCnName;
        NF_ENUM_COL_TYPE col_type = pColInfo->m_colCppType;
        std::string strJsonRepeatdNum = pColInfo->m_strParseRepeatedNum;
        std::string strJsonMessage = pColInfo->m_strParseMessage;
        uint32_t colTypeStrMaxSize = pColInfo->m_colTypeStrMaxSize;
        int32_t int_col_max_size = pColInfo->m_maxSubNum;
        if (pColInfo->m_strutNumFromZero)
        {
            int_col_max_size = pColInfo->m_maxSubNum + 1;
        }
        std::string col_max_size = NFCommon::tostr(int_col_max_size);

        if (pColInfo->m_colInfoMap.empty() && !pColInfo->m_isArray && strJsonRepeatdNum.empty())
        {
            index += 1;

            if (col_type == NF_ENUM_COL_TYPE_INT8 || col_type == NF_ENUM_COL_TYPE_UINT8)
            {
                proto_file +=
                    "\toptional int32 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).int_size = IS_8, (nanopb).field_cname = \"" + col_cn_name +
                    "\"";
            }
            if (col_type == NF_ENUM_COL_TYPE_INT16 || col_type == NF_ENUM_COL_TYPE_UINT16)
            {
                proto_file +=
                    "\toptional int32 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).int_size = IS_16, (nanopb).field_cname = \"" + col_cn_name +
                    "\"";
            }
            else if (col_type == NF_ENUM_COL_TYPE_INT32)
            {
                proto_file +=
                    "\toptional int32 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\"";
            }
            else if (col_type == NF_ENUM_COL_TYPE_UINT32)
            {
                proto_file +=
                    "\toptional uint32 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\"";
            }
            else if (col_type == NF_ENUM_COL_TYPE_INT64)
            {
                proto_file +=
                    "\toptional int64 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\"";
            }
            else if (col_type == NF_ENUM_COL_TYPE_UINT64)
            {
                proto_file +=
                    "\toptional uint64 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\"";
            }
            else if (col_type == NF_ENUM_COL_TYPE_FLOAT)
            {
                proto_file +=
                    "\toptional float " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\"";
            }
            else if (col_type == NF_ENUM_COL_TYPE_DOUBLE)
            {
                proto_file +=
                    "\toptional double " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\"";
            }
            else if (col_type == NF_ENUM_COL_TYPE_BOOL)
            {
                proto_file +=
                    "\toptional bool " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\"";
            }
            else if (col_type == NF_ENUM_COL_TYPE_ENUM)
            {
                proto_file +=
                    "\toptional " + strJsonMessage + " " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\"";
            }
            else if (col_type == NF_ENUM_COL_TYPE_STRING)
            {
                proto_file +=
                    "\toptional string " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\"," + " (nanopb).max_size_enum = \"" + get_string_define(colTypeStrMaxSize) + "\"";

                if (pSheet->m_createSql)
                {
                    proto_file += ", (nanopb).db_max_size_enum = \"" + get_string_define(colTypeStrMaxSize) + "\"";
                }
            }
            else if (col_type == NF_ENUM_COL_TYPE_DATE)
            {
                proto_file +=
                    "\toptional string " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\"," + " (nanopb).max_size_enum = \"" + get_string_define(colTypeStrMaxSize) + "\"];\n";
                index++;
                proto_file +=
                    "\toptional uint64 " + col_en_name + "_t = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\"";
                if (pSheet->m_createSql)
                {
                    proto_file += ", (nanopb).db_max_size_enum = \"" + get_string_define(colTypeStrMaxSize) + "\"";
                }
            }
            else if (col_type == NF_ENUM_COL_TYPE_MESSAGE)
            {
                proto_file +=
                    "\toptional " + strJsonMessage + " " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name + "\", (nanopb).parse_type=FPT_JSON";

                if (pSheet->m_createSql)
                {
                    proto_file += ", (nanopb).db_max_size_enum = \"" + get_string_define(colTypeStrMaxSize) + "\"";
                }
            }

            if (pSheet->m_colEnumMap.find(col_en_name) != pSheet->m_colEnumMap.end())
            {
                proto_file += ", (nanopb).macro_type = \"" + pSheet->m_colEnumMap[col_en_name].m_enumPbName + "\"";
            }

            if (pSheet->m_createSql)
            {
                proto_file += ", (nanopb).db_comment = \"" + col_cn_name + "\"";

                if (index == 1)
                {
                    proto_file += ", (nanopb).db_type = E_FIELD_TYPE_PRIMARYKEY";
                }
                else
                {
                    if (pSheet->m_indexMap.find(pColInfo->m_structEnName) != pSheet->m_indexMap.end())
                    {
                        if (pSheet->m_indexMap[pColInfo->m_structEnName].m_unique)
                        {
                            proto_file += ", (nanopb).db_type = E_FIELD_TYPE_UNIQUE_INDEX";
                        }
                        else
                        {
                            proto_file += ", (nanopb).db_type = E_FIELD_TYPE_INDEX";
                        }
                    }
                    else
                    {
                        for (auto com_iter = pSheet->m_comIndexMap.begin(); com_iter != pSheet->m_comIndexMap.end(); com_iter++)
                        {
                            bool find = false;
                            for (int com_i = 0; com_i < (int)com_iter->second.m_index.size(); com_i++)
                            {
                                if (com_iter->second.m_index[com_i].m_key == pColInfo->m_structEnName)
                                {
                                    proto_file += ", (nanopb).db_type = E_FIELD_TYPE_INDEX";
                                    find = true;
                                    break;
                                }
                            }

                            if (find)
                            {
                                break;
                            }
                        }
                    }
                }
            }
            proto_file += "];\n";
        }
        else if (pColInfo->m_colInfoMap.size() > 0 && pColInfo->m_isArray)
        {
            index += 1;

            if (pColInfo->m_colInfoMap.size() > 0)
            {
                proto_file += "\trepeated " + pColInfo->m_pbDescName + " " + col_en_name + " = " + NFCommon::tostr(index) +
                    "[(nanopb).field_cname = \"" + col_cn_name + "\"," + " (nanopb).max_count = " +
                    col_max_size;
            }

            if (pSheet->m_createSql)
            {
                proto_file += ", (nanopb).db_comment = \"" + col_cn_name + "\"";
                proto_file += ", (nanopb).db_max_count = " + col_max_size;
            }

            proto_file += "];\n";
        }
        else if (pColInfo->m_colInfoMap.empty() && pColInfo->m_isArray && !strJsonRepeatdNum.empty())
        {
            CHECK_EXPR(false, -1, "excel:{} sheet:{} not right, col:{} enName:{} cnName:{}", pSheet->m_excelName, pSheet->m_sheetName, pColInfo->m_colIndex, pColInfo->m_colType, pColInfo->m_structEnName, pColInfo->m_structCnName);
        }
        else if (pColInfo->m_colInfoMap.empty() && !pColInfo->m_isArray && !strJsonRepeatdNum.empty())
        {
            if (!strJsonRepeatdNum.empty())
            {
                col_max_size = strJsonRepeatdNum;
            }
            std::string nanopb_max_count = "(nanopb).max_count = " + col_max_size;
            if (!NFStringUtility::IsDigital(col_max_size))
            {
                nanopb_max_count = "(nanopb).max_count_enum = \"" + col_max_size + "\"";
            }

            nanopb_max_count += ", (nanopb).parse_type=FPT_JSON";

            index += 1;

            if (col_type == NF_ENUM_COL_TYPE_INT32)
            {
                proto_file +=
                    "\trepeated int32 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_UINT32)
            {
                proto_file +=
                    "\trepeated uint32 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_INT64)
            {
                proto_file +=
                    "\trepeated int64 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_UINT64)
            {
                proto_file +=
                    "\trepeated uint64 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_FLOAT)
            {
                proto_file +=
                    "\trepeated float " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_DOUBLE)
            {
                proto_file +=
                    "\trepeated double " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_BOOL)
            {
                proto_file +=
                    "\trepeated bool " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_ENUM)
            {
                proto_file +=
                    "\trepeated " + strJsonMessage + " " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_STRING)
            {
                proto_file +=
                    "\trepeated string " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    +"\", " + nanopb_max_count + ", (nanopb).max_size_enum = \"" +
                    get_string_define(colTypeStrMaxSize) + "\"";
            }
            else if (col_type == NF_ENUM_COL_TYPE_DATE)
            {
                proto_file +=
                    "\trepeated string " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    +"\", " + nanopb_max_count + ", (nanopb).max_size_enum = \"" +
                    get_string_define(colTypeStrMaxSize) + "\"];\n";
                index++;
                proto_file +=
                    "\trepeated uint64 " + col_en_name + "_t = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_MESSAGE)
            {
                proto_file +=
                    "\trepeated " + strJsonMessage + " " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    +"\", " + nanopb_max_count;
            }

            if (pSheet->m_createSql)
            {
                proto_file += ", (nanopb).db_max_size_enum = \"" + get_string_define(colTypeStrMaxSize) + "\"";
            }

            if (pSheet->m_colEnumMap.find(col_en_name) != pSheet->m_colEnumMap.end())
            {
                proto_file += ", (nanopb).macro_type = \"" + pSheet->m_colEnumMap[col_en_name].m_enumPbName + "\"";
            }

            if (pSheet->m_createSql)
            {
                proto_file += ", (nanopb).db_comment = \"" + col_cn_name + "\"";
                proto_file += ", (nanopb).db_max_count = " + col_max_size;
            }

            proto_file += "];\n";
        }
        else if (pColInfo->m_colInfoMap.empty() && pColInfo->m_isArray && strJsonRepeatdNum.empty())
        {
            std::string nanopb_max_count = "(nanopb).max_count = " + col_max_size;
            if (!NFStringUtility::IsDigital(col_max_size))
            {
                nanopb_max_count = "(nanopb).max_count_enum = \"" + col_max_size + "\"";
            }

            index += 1;

            if (col_type == NF_ENUM_COL_TYPE_INT32)
            {
                proto_file += "\trepeated int32 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_UINT32)
            {
                proto_file += "\trepeated uint32 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_INT64)
            {
                proto_file += "\trepeated int64 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_UINT64)
            {
                proto_file += "\trepeated uint64 " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_FLOAT)
            {
                proto_file += "\trepeated float " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_DOUBLE)
            {
                proto_file += "\trepeated double " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_BOOL)
            {
                proto_file += "\trepeated bool " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_ENUM)
            {
                proto_file += "\trepeated " + strJsonMessage + " " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
            }
            else if (col_type == NF_ENUM_COL_TYPE_STRING)
            {
                proto_file += "\trepeated string " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    +"\", " + nanopb_max_count + ", (nanopb).max_size_enum = \"" +
                    get_string_define(colTypeStrMaxSize) + "\"";

                if (pSheet->m_createSql)
                {
                    proto_file += ", (nanopb).db_max_size_enum = \"" + get_string_define(colTypeStrMaxSize) + "\"";
                }
            }
            else if (col_type == NF_ENUM_COL_TYPE_DATE)
            {
                proto_file += "\trepeated string " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    +"\", " + nanopb_max_count + ", (nanopb).max_size_enum = \"" +
                    get_string_define(colTypeStrMaxSize) + "\"];\n";
                index++;
                proto_file += "\trepeated uint64 " + col_en_name + "_t = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    "\", " + nanopb_max_count;
                if (pSheet->m_createSql)
                {
                    proto_file += ", (nanopb).db_max_size_enum = \"" + get_string_define(colTypeStrMaxSize) + "\"";
                }
            }
            else if (col_type == NF_ENUM_COL_TYPE_MESSAGE)
            {
                proto_file += "\trepeated " + strJsonMessage + " " + col_en_name + " = " + NFCommon::tostr(index) + "[(nanopb).field_cname = \"" + col_cn_name +
                    +"\", " + nanopb_max_count + ", (nanopb).parse_type=FPT_JSON";

                if (pSheet->m_createSql)
                {
                    proto_file += ", (nanopb).db_max_size_enum = \"" + get_string_define(colTypeStrMaxSize) + "\"";
                }
            }

            if (pSheet->m_colEnumMap.find(col_en_name) != pSheet->m_colEnumMap.end())
            {
                proto_file += ", (nanopb).macro_type = \"" + pSheet->m_colEnumMap[col_en_name].m_enumPbName + "\"";
            }

            if (pSheet->m_createSql)
            {
                proto_file += ", (nanopb).db_comment = \"" + col_cn_name + "\"";
                proto_file += ", (nanopb).db_max_count = " + col_max_size;
            }

            proto_file += "];\n";
        }
        else if (pColInfo->m_colInfoMap.size() > 0 && !pColInfo->m_isArray)
        {
            index += 1;


            if (pColInfo->m_colInfoMap.size() > 0)
            {
                proto_file += "\toptional " + pColInfo->m_pbDescName + " " + col_en_name + " = " + NFCommon::tostr(index) +
                    "[(nanopb).field_cname = \"" + col_cn_name + "\"";
            }

            if (pSheet->m_createSql)
            {
                proto_file += ", (nanopb).db_comment = \"" + col_cn_name + "\"";
                proto_file += ", (nanopb).db_message_expand = true";
            }

            proto_file += "];\n";
        }
    }

    if (pSheet->m_addFileVec.size() > 0)
    {
        proto_file += "////////////////////////////add field//////////////////////////////////////\n";
    }

    for (auto iter = pSheet->m_addFileVec.begin(); iter != pSheet->m_addFileVec.end(); iter++)
    {
        proto_file += "\t" + *iter + ";\n";
    }

    proto_file += "}\n";

    proto_file += "\n\nmessage " + pSheet->m_protoInfo.m_sheetMsgName + "\n";
    proto_file += "{\n";
    proto_file += "\trepeated " + pSheet->m_protoInfo.m_protoMsgName + " " + pSheet->m_protoInfo.m_protoMsgName + "_List = 1[(nanopb).max_count=" +
        NFCommon::tostr(get_max_num(pSheet->m_rows)) + "];\n";
    proto_file += "}\n";

    return 0;
}

void ExcelToProto::WriteSheetDescStoreExH()
{
    std::string desc_file_name = NFStringUtility::FirstUpper(m_excelName) + "DescEx.h";
    std::string desc_file_path = m_outPath + desc_file_name;
    std::string desc_file;
    desc_file += "#pragma once\n\n";
    desc_file += "#include \"NFServerComm/NFServerCommon/NFIDescStoreEx.h\"\n";
    desc_file += "#include \"NFServerComm/NFServerCommon/NFIDescTemplate.h\"\n";
    desc_file += "#include \"NFComm/NFObjCommon/NFResDb.h\"\n";
    desc_file += "#include \"NFComm/NFObjCommon/NFShmMgr.h\"\n";
    desc_file += "#include \"NFComm/NFShmStl/NFShmHashMap.h\"\n";
    desc_file += "#include \"NFComm/NFShmStl/NFShmVector.h\"\n";
    desc_file += "#include \"NFLogicCommon/NFDescStoreTypeDefines.h\"\n";

    desc_file += "\nclass " + NFStringUtility::FirstUpper(m_excelName) + "DescEx : public NFObjectGlobalTemplate<" +
        NFStringUtility::FirstUpper(m_excelName) + "DescEx, EOT_CONST_" +
        NFStringUtility::Upper(m_excelName) + "_DESC_EX_ID, NFIDescStoreEx>\n";
    desc_file += "{\n";
    desc_file += "public:\n";
    desc_file += "\t" + NFStringUtility::FirstUpper(m_excelName) + "DescEx();\n";
    desc_file += "\tvirtual ~" + NFStringUtility::FirstUpper(m_excelName) + "DescEx();\n";
    desc_file += "\tint CreateInit();\n";
    desc_file += "\tint ResumeInit();\n";
    desc_file += "public:\n";
    desc_file += "\tvirtual int Load() override;\n";
    desc_file += "\tvirtual int CheckWhenAllDataLoaded() override;\n";

    desc_file += "};\n";
    NFFileUtility::WriteFile(desc_file_path, desc_file);
}

void ExcelToProto::WriteSheetDescStoreH(ExcelSheet* pSheet)
{
    std::string sheet_name = pSheet->m_sheetName;
    std::string desc_file_name = pSheet->m_otherName + "Desc.h";
    std::string desc_file_path = m_outPath + desc_file_name;
    std::string desc_file;
    desc_file += "#pragma once\n\n";
    desc_file += "#include \"NFServerComm/NFServerCommon/NFIDescStore.h\"\n";
    desc_file += "#include \"NFServerComm/NFServerCommon/NFIDescTemplate.h\"\n";
    desc_file += "#include \"NFLogicCommon/NFDescStoreDefine.h\"\n";
    desc_file += "#include \"NFLogicCommon/NFDescStoreTypeDefines.h\"\n";
    desc_file += "#include \"E_" + NFStringUtility::FirstUpper(m_excelName) + ".nanopb.h\"\n";

    pSheet->m_define_max_rows = "MAX_" + NFStringUtility::Upper(m_excelName) + "_" + NFStringUtility::Upper(sheet_name) + "_NUM";

    desc_file += "#ifndef " + pSheet->m_define_max_rows + "\n";
    desc_file += "#define " + pSheet->m_define_max_rows + " " + NFCommon::tostr(get_max_num(pSheet->m_rows)) + "\n";
    desc_file += "#else\n";
    desc_file += "static_assert(" + pSheet->m_define_max_rows + " >= " + NFCommon::tostr(get_max_num(pSheet->m_rows)) + ", \"the define max rows < desc has rows\");\n";
    desc_file += "#endif\n";

    for (auto iter = pSheet->m_comIndexMap.begin(); iter != pSheet->m_comIndexMap.end(); iter++)
    {
        ExcelSheetComIndex& comIndex = iter->second;
        desc_file += "\nstruct ";
        desc_file += comIndex.m_structComName + "\n";
        desc_file += "{\n";
        desc_file += "\t" + comIndex.m_structComName + "()\n";
        desc_file += "\t{\n";
        desc_file += "\t\tif (EN_OBJ_MODE_INIT == NFShmMgr::Instance()->GetCreateMode()) {\n";
        desc_file += "\t\t\tCreateInit();\n";
        desc_file += "\t\t}\n";
        desc_file += "\t\telse {\n";
        desc_file += "\t\t\tResumeInit();\n";
        desc_file += "\t\t}\n";
        desc_file += "\t}\n";
        desc_file += "\tint CreateInit()\n";
        desc_file += "\t{\n";
        for (int i = 0; i < (int)comIndex.m_index.size(); i++)
        {
            ExcelSheetIndex& index = comIndex.m_index[i];
            std::string index_key = index.m_key;
            desc_file += "\t\t" + index_key + "=0;\n";
        }
        desc_file += "\t\treturn 0;\n";
        desc_file += "\t}\n";
        desc_file += "\tint ResumeInit()\n";
        desc_file += "\t{\n";
        desc_file += "\t\treturn 0;\n";
        desc_file += "\t}\n";
        for (int i = 0; i < (int)comIndex.m_index.size(); i++)
        {
            ExcelSheetIndex& index = comIndex.m_index[i];
            std::string index_key = index.m_key;
            desc_file += "\tint64_t " + index_key + ";\n";
        }
        desc_file += "\tbool operator==(const " + comIndex.m_structComName + "& data) const\n";
        desc_file += "\t{\n";
        desc_file += "\t\t return ";
        for (int i = 0; i < (int)comIndex.m_index.size(); i++)
        {
            ExcelSheetIndex& index = comIndex.m_index[i];
            std::string index_key = index.m_key;
            if (i != (int)comIndex.m_index.size() - 1)
            {
                desc_file += index_key + "== data." + index_key + " && ";
            }
            else
            {
                desc_file += index_key + "== data." + index_key + ";\n";
            }
        }
        desc_file += "\t}\n";
        desc_file += "\tstd::string ToString() const\n";
        desc_file += "\t{\n";
        desc_file += "\t\tstd::stringstream ios;\n";
        for (int i = 0; i < (int)comIndex.m_index.size(); i++)
        {
            ExcelSheetIndex& index = comIndex.m_index[i];
            std::string index_key = index.m_key;
            desc_file += "\t\tios << " + index_key + ";\n";
            if (i != (int)comIndex.m_index.size() - 1)
            {
                desc_file += "\t\tios << \",\";\n";
            }
        }
        desc_file += "\t\treturn ios.str();\n";
        desc_file += "\t}\n";
        desc_file += "};\n";

        desc_file += "\nnamespace std\n";
        desc_file += "{\n";
        desc_file += "\ttemplate<>\n";
        desc_file += "\tstruct hash<" + comIndex.m_structComName + ">\n";
        desc_file += "\t{\n";
        desc_file += "\t\tsize_t operator()(const " + comIndex.m_structComName + "& data) const\n";
        desc_file += "\t\t{\n";
        desc_file += "\t\t\treturn NFHash::hash_combine(";
        for (int i = 0; i < (int)comIndex.m_index.size(); i++)
        {
            ExcelSheetIndex& index = comIndex.m_index[i];
            std::string index_key = index.m_key;
            if (i != (int)comIndex.m_index.size() - 1)
            {
                desc_file += "data." + index_key + ",";
            }
            else
            {
                desc_file += "data." + index_key + ");\n";
            }
        }
        desc_file += "\t\t}\n";
        desc_file += "\t};\n";
        desc_file += "}\n\n";
    }

    desc_file += "\nclass " + pSheet->m_otherName + "Desc : public NFIDescTemplate<" +
        pSheet->m_otherName + "Desc, " +
        pSheet->m_protoInfo.m_protoMsgName + ", EOT_CONST_" +
        NFStringUtility::Upper(m_excelName) + "_" + NFStringUtility::Upper(sheet_name) + "_DESC_ID, MAX_" +
        NFStringUtility::Upper(m_excelName) + "_" + NFStringUtility::Upper(sheet_name) + "_NUM>\n";
    desc_file += "{\n";
    desc_file += "public:\n";
    desc_file += "\t" + pSheet->m_otherName + "Desc();\n";
    desc_file += "\tvirtual ~" + pSheet->m_otherName + "Desc();\n";
    desc_file += "\tint CreateInit();\n";
    desc_file += "\tint ResumeInit();\n";
    if (pSheet->m_createSql)
    {
        desc_file += "\tvirtual bool IsFileLoad() { return false; }\n";
    }
    desc_file += "public:\n";
    desc_file += "\tvirtual int Load(NFResDb *pDB) override;\n";
    desc_file += "\tvirtual int LoadDB(NFResDb *pDB) override;\n";
    desc_file += "\tvirtual int CheckWhenAllDataLoaded() override;\n";
    if (pSheet->m_protoMsgNameStr.size() > 0)
    {
        desc_file += "\tvirtual std::string GetFileName() { return \"" + pSheet->m_protoInfo.m_binFileName + "\"; }\n";
    }

    for (auto iter = pSheet->m_indexMap.begin(); iter != pSheet->m_indexMap.end(); iter++)
    {
        ExcelSheetIndex& index = iter->second;
        std::string index_key = iter->second.m_key;
        if (index.m_unique)
        {
            desc_file +=
                "\tconst " + pSheet->m_protoInfo.m_protoMsgName + "* GetDescBy" +
                NFStringUtility::ToFirstUpperChangeVarNameGood(index_key) + "(int64_t " + NFStringUtility::ToChangeVarNameGood(index_key) + ") const;\n";
        }
        else
        {
            desc_file += "\tstd::vector<const " + pSheet->m_protoInfo.m_protoMsgName +
                "*> GetDescBy" + NFStringUtility::ToFirstUpperChangeVarNameGood(index_key) + "(int64_t " + NFStringUtility::ToChangeVarNameGood(index_key) +
                ") const;\n";
        }
    }

    for (auto iter = pSheet->m_comIndexMap.begin(); iter != pSheet->m_comIndexMap.end(); iter++)
    {
        ExcelSheetComIndex& comIndex = iter->second;
        if (comIndex.m_unique)
        {
            desc_file += "\tconst " + pSheet->m_protoInfo.m_protoMsgName + "* GetDescBy";
        }
        else
        {
            desc_file += "\tstd::vector<const " + pSheet->m_protoInfo.m_protoMsgName + "*> GetDescBy";
        }

        for (int i = 0; i < (int)comIndex.m_index.size(); i++)
        {
            std::string index_key = comIndex.m_index[i].m_key;
            desc_file += NFStringUtility::ToFirstUpperChangeVarNameGood(index_key);
        }

        desc_file += "(";

        for (int i = 0; i < (int)comIndex.m_index.size(); i++)
        {
            std::string index_key = comIndex.m_index[i].m_key;
            if (i == (int)comIndex.m_index.size() - 1)
            {
                desc_file += "int64_t " + NFStringUtility::ToChangeVarNameGood(index_key);
            }
            else
            {
                desc_file += "int64_t " + NFStringUtility::ToChangeVarNameGood(index_key) + ", ";
            }
        }

        desc_file += ");\n";
    }

    if (pSheet->m_indexMap.size() > 0 || pSheet->m_comIndexMap.size() > 0)
        desc_file += "private:\n";

    for (auto iter = pSheet->m_indexMap.begin(); iter != pSheet->m_indexMap.end(); iter++)
    {
        ExcelSheetIndex& index = iter->second;
        if (index.m_unique)
        {
            desc_file += "\tNFShmHashMap<int64_t, uint64_t, " + pSheet->m_define_max_rows + "> " + index.m_varName + ";\n";
        }
        else
        {
            desc_file += "\tNFShmHashMultiMap<int64_t, uint64_t, " + pSheet->m_define_max_rows + "> " + index.m_varName + ";\n";
        }
    }

    for (auto iter = pSheet->m_comIndexMap.begin(); iter != pSheet->m_comIndexMap.end(); iter++)
    {
        ExcelSheetComIndex& comIndex = iter->second;
        if (comIndex.m_unique)
        {
            desc_file += "\tNFShmHashMap<" + comIndex.m_structComName;
        }
        else
        {
            desc_file += "\tNFShmHashMultiMap<" + comIndex.m_structComName;
        }
        desc_file += ", uint64_t, " + pSheet->m_define_max_rows + ">" + " " + comIndex.m_varName + ";\n";
    }

    desc_file += "};\n";

    NFFileUtility::WriteFile(desc_file_path, desc_file);
}

void ExcelToProto::WriteSheetDescStoreExCpp()
{
    std::string desc_file_name = NFStringUtility::FirstUpper(m_excelName) + "DescEx.cpp";
    std::string desc_file_path = m_outPath + desc_file_name;
    std::string desc_file;
    desc_file += "#include \"" + NFStringUtility::FirstUpper(m_excelName) + "DescEx.h\"\n\n";
    desc_file += NFStringUtility::FirstUpper(m_excelName) + "DescEx::" + NFStringUtility::FirstUpper(m_excelName) +
        "DescEx()\n";
    desc_file += "{\n";
    desc_file += "\tif (EN_OBJ_MODE_INIT == NFShmMgr::Instance()->GetCreateMode()) {\n";
    desc_file += "\t\tCreateInit();\n";
    desc_file += "\t}\n";
    desc_file += "\telse {\n";
    desc_file += "\t\tResumeInit();\n";
    desc_file += "\t}\n";
    desc_file += "}\n\n";
    //////////////////////////////////////////////////////////////////
    desc_file += NFStringUtility::FirstUpper(m_excelName) + "DescEx::~" +
        NFStringUtility::FirstUpper(m_excelName) + "DescEx()\n";
    desc_file += "{\n";
    desc_file += "}\n\n";
    ///////////////////////////////////////////////////////////
    desc_file += "int " + NFStringUtility::FirstUpper(m_excelName) + "DescEx::CreateInit()\n";
    desc_file += "{\n";
    desc_file += "\treturn 0;\n";
    desc_file += "}\n\n";
    ////////////////////////////////////////////////////////////////
    desc_file += "int " + NFStringUtility::FirstUpper(m_excelName) + "DescEx::ResumeInit()\n";
    desc_file += "{\n";
    desc_file += "\treturn 0;\n";
    desc_file += "}\n\n";
    ////////////////////////////////////////////////////////////////
    desc_file += "int " + NFStringUtility::FirstUpper(m_excelName) + "DescEx::Load()\n";
    desc_file += "{\n";
    desc_file += "\treturn 0;\n";
    desc_file += "}\n\n";
    ////////////////////////////////////////////////////////////////
    desc_file += "int " + NFStringUtility::FirstUpper(m_excelName) + "DescEx::CheckWhenAllDataLoaded()\n";
    desc_file += "{\n";
    desc_file += "\treturn 0;\n";
    desc_file += "}\n\n";
    NFFileUtility::WriteFile(desc_file_path, desc_file);
}

void WriteRelationHead(ExcelRelation* pRelation, std::set<std::string>& headFileHead)
{
    for (auto iter = pRelation->m_relationMap.begin(); iter != pRelation->m_relationMap.end(); iter++)
    {
        auto pLastRelation = iter->second;
        if (pLastRelation->m_relationMap.empty())
        {
            for (int i = 0; i < (int)pLastRelation->m_dst.size(); i++)
            {
                ExcelRelationDst& relationDst = pLastRelation->m_dst[i];
                headFileHead.insert(relationDst.m_descName);
            }
        }
        else
        {
            WriteRelationHead(pLastRelation, headFileHead);
        }
    }
}

void ExcelToProto::WriteSheetDescStoreCpp(ExcelSheet* pSheet)
{
    ExcelSheetColInfo* one_col_info = pSheet->m_colInfoVec.begin()->second;
    std::string key_en_name = NFStringUtility::Lower(one_col_info->m_structEnName);

    std::string sheet_name = pSheet->m_sheetName;
    std::string desc_file_name = pSheet->m_otherName + "Desc.cpp";
    std::string desc_file_path = m_outPath + desc_file_name;
    std::string desc_file;

    desc_file += "#include \"" + pSheet->m_otherName + "Desc.h\"\n";
    std::set<std::string> headFileHead;
    for (auto iter = pSheet->m_colRelationMap.begin(); iter != pSheet->m_colRelationMap.end(); iter++)
    {
        auto pLastRelation = &iter->second;
        if (pLastRelation->m_relationMap.empty())
        {
            for (int i = 0; i < (int)pLastRelation->m_dst.size(); i++)
            {
                ExcelRelationDst& relationDst = pLastRelation->m_dst[i];
                headFileHead.insert(relationDst.m_descName);
            }
        }
        else
        {
            WriteRelationHead(pLastRelation, headFileHead);
        }
    }

    for (auto iter = headFileHead.begin(); iter != headFileHead.end(); iter++)
    {
        desc_file += "#include \"" + *iter + "Desc.h\"\n";
    }

    desc_file += "#include \"NFComm/NFPluginModule/NFCheck.h\"\n";
    desc_file += "#include \"NFComm/NFCore/NFFileUtility.h\"\n";
    desc_file += "#include \"" + NFStringUtility::FirstUpper(pSheet->m_excelName) + "DescEx.h\"\n\n";

    //////////////////////////////////////////////////////////////
    desc_file +=
        pSheet->m_otherName + "Desc::" + pSheet->m_otherName + "Desc()\n";
    desc_file += "{\n";
    desc_file += "\tif (EN_OBJ_MODE_INIT == NFShmMgr::Instance()->GetCreateMode()) {\n";
    desc_file += "\t\tCreateInit();\n";
    desc_file += "\t}\n";
    desc_file += "\telse {\n";
    desc_file += "\t\tResumeInit();\n";
    desc_file += "\t}\n";
    desc_file += "}\n\n";
    //////////////////////////////////////////////////////////////////
    desc_file += pSheet->m_otherName + "Desc::~" +
        pSheet->m_otherName + "Desc()\n";
    desc_file += "{\n";
    desc_file += "}\n\n";
    ///////////////////////////////////////////////////////////
    desc_file += "int " + pSheet->m_otherName + "Desc::CreateInit()\n";
    desc_file += "{\n";
    desc_file += "\treturn 0;\n";
    desc_file += "}\n\n";
    ////////////////////////////////////////////////////////////////
    desc_file += "int " + pSheet->m_otherName + "Desc::ResumeInit()\n";
    desc_file += "{\n";
    desc_file += "\treturn 0;\n";
    desc_file += "}\n\n";
    ////////////////////////////////////////////////////////////////
    desc_file += "int " + pSheet->m_otherName + "Desc::LoadDB(NFResDb *pDB)\n";
    desc_file += "{\n";
    desc_file += "\tCHECK_EXPR(pDB != NULL, -1, \"pDB == NULL\");\n";
    desc_file += "\n";
    desc_file += "\tNFLogTrace(NF_LOG_DEFAULT, 0, \"" + pSheet->m_otherName +
        "Desc::LoadDB() strFileName = {}\", GetFileName());\n";
    desc_file += "\n";
    desc_file += "\tproto_ff::Sheet_" + pSheet->m_otherName + " table;\n";
    desc_file += "\tNFResTable* pResTable = pDB->GetTable(GetFileName());\n";
    desc_file += "\tCHECK_EXPR(pResTable != NULL, -1, \"pTable == NULL, GetTable:{} Error\", GetFileName());\n";
    desc_file += "\n";
    desc_file += "\tint iRet = 0;\n";
    desc_file += "\tiRet = pResTable->FindAllRecord(GetDBName(), &table);\n";
    desc_file += "\tCHECK_EXPR(iRet == 0, -1, \"FindAllRecord Error:{}\", GetFileName());\n";
    desc_file += "\n";
    desc_file += "\t//NFLogTrace(NF_LOG_DEFAULT, 0, \"Load From DB:{}\", table.Utf8DebugString());\n";
    desc_file += "\n";
    desc_file += "\tstd::string bin_file = GetFilePathName();\n\n";
    desc_file += "\tstd::string content = table.SerializePartialAsString();\n\n";
    desc_file += "\tNFFileUtility::WriteFile(bin_file, content);\n\n";
    desc_file += "\tNFJson2PB::Pb2JsonOptions options;\n";
    desc_file += "\toptions.pretty_json = true;\n";
    desc_file += "\tstd::string excelJson;\n";
    desc_file += "\tstd::string excelJsonError;\n";
    desc_file += "\tif (!NFProtobufCommon::ProtoMessageToJson(options, table, &excelJson, &excelJsonError))\n";
    desc_file += "\t{\n";
    desc_file += "\t\tCHECK_ERR(0, -1, \"ProtoMessageToJson Failed table:{} errorJson:{}\", GetFileName(), excelJsonError);\n";
    desc_file += "\t}\n";
    desc_file += "\tstd::string debug_file = NFFileUtility::GetExcludeFileExt(bin_file) + \"_debug.json\";\n\n";
    desc_file += "\tNFFileUtility::WriteFile(debug_file, excelJson);\n\n";
    desc_file += "\tSetDBLoaded(true);\n";
    desc_file += "\treturn 0;\n";
    desc_file += "}\n\n";
    ////////////////////////////////////////////////////////////////
    desc_file += "int " + pSheet->m_otherName + "Desc::Load(NFResDb *pDB)\n";
    desc_file += "{\n";
    desc_file += "\tCHECK_EXPR(pDB != NULL, -1, \"pDB == NULL\");\n";
    desc_file += "\n";
    desc_file += "\tNFLogTrace(NF_LOG_DEFAULT, 0, \"" + pSheet->m_otherName +
        "Desc::Load() strFileName = {}\", GetFileName());\n";
    desc_file += "\n";
    desc_file += "\tproto_ff::Sheet_" + pSheet->m_otherName + " table;\n";
    desc_file += "\tNFResTable* pResTable = pDB->GetTable(GetFileName());\n";
    desc_file += "\tCHECK_EXPR(pResTable != NULL, -1, \"pTable == NULL, GetTable:{} Error\", GetFileName());\n";
    desc_file += "\n";
    desc_file += "\tint iRet = 0;\n";
    desc_file += "\tiRet = pResTable->FindAllRecord(GetDBName(), &table);\n";
    desc_file += "\tCHECK_EXPR(iRet == 0, -1, \"FindAllRecord Error:{}\", GetFileName());\n";
    desc_file += "\n";
    desc_file += "\t//NFLogTrace(NF_LOG_DEFAULT, 0, \"{}\", table.Utf8DebugString());\n";
    desc_file += "\n";
    desc_file += "\tm_astDescVec.clear();\n";
    desc_file += "\tm_astDescMap.clear();\n";
    desc_file += "\tif ((table.e_" + NFStringUtility::Lower(pSheet->m_otherName) + "_list_size() < 0) || (table.e_" +
        NFStringUtility::Lower(pSheet->m_otherName) + "_list_size() > (int)(m_astDescVec.max_size())))\n";
    desc_file += "\t{\n";
    desc_file += "\t\tNFLogError(NF_LOG_DEFAULT, 0, \"Invalid TotalNum:{}\", table.e_" + NFStringUtility::Lower(pSheet->m_otherName) + "_list_size());\n";
    desc_file += "\t\treturn -2;\n";
    desc_file += "\t}\n";
    desc_file += "\n";
    desc_file += "\tif (IsReloading())\n";
    desc_file += "\t{\n";
    desc_file += "\t\tstd::set<int64_t> newKeys;\n";
    desc_file += "\t\tfor (int i = 0; i < (int)table.e_" + NFStringUtility::Lower(pSheet->m_otherName) + "_list_size(); i++)\n";
    desc_file += "\t\t{\n";
    desc_file += "\t\t\tconst proto_ff::" + pSheet->m_protoInfo.m_protoMsgName + "& desc = table.e_" +
        NFStringUtility::Lower(pSheet->m_otherName) + "_list(i);\n";
    desc_file += "\t\t\tif (desc.has_" + key_en_name + "() == false && desc.ByteSize() == 0)\n";
    desc_file += "\t\t\t{\n";
    desc_file += "\t\t\t\tcontinue;\n";
    desc_file += "\t\t\t}\n";
    desc_file += "\t\t\tnewKeys.insert(desc." + key_en_name + "());\n";
    desc_file += "\t\t\tif (m_astDescMap.find(desc." + key_en_name + "()) == m_astDescMap.end())\n";
    desc_file += "\t\t\t{\n";
    desc_file += "\t\t\t\tNFLogInfo(NF_LOG_DEFAULT, 0, \"reload add new col, id:{}\",  desc." + key_en_name + "());\n";
    desc_file += "\t\t\t}\n";
    desc_file += "\t\t}\n";
    desc_file += "\t\tfor (auto iter = m_astDescMap.begin(); iter != m_astDescMap.end(); iter++)\n";
    desc_file += "\t\t{\n";
    desc_file += "\t\t\tif (newKeys.find(iter->first) == newKeys.end())\n";
    desc_file += "\t\t\t{\n";
    desc_file += "\t\t\t\tNFLogError(NF_LOG_DEFAULT, 0, \"reload del old col, id:{}\", iter->first);\n";
    desc_file += "\t\t\t}\n";
    desc_file += "\t\t}\n";
    desc_file += "\t}\n\n";
    desc_file += "\tm_astDescVec.clear();\n";
    desc_file += "\tm_astDescMap.clear();\n";
    desc_file += "\tfor (int i = 0; i < (int)table.e_" + NFStringUtility::Lower(pSheet->m_otherName) + "_list_size(); i++)\n";
    desc_file += "\t{\n";
    desc_file += "\t\tconst proto_ff::" + pSheet->m_protoInfo.m_protoMsgName + "& desc = table.e_" +
        NFStringUtility::Lower(pSheet->m_otherName) + "_list(i);\n";
    desc_file += "\t\tif (desc.has_" + key_en_name + "() == false && desc.ByteSize() == 0)\n";
    desc_file += "\t\t{\n";
    desc_file += "\t\t\tNFLogError(NF_LOG_DEFAULT, 0, \"the desc no value, {}\", desc.Utf8DebugString());\n";
    desc_file += "\t\t\tcontinue;\n";
    desc_file += "\t\t}\n\n";

    desc_file += "\t\t//NFLogTrace(NF_LOG_DEFAULT, 0, \"{}\", desc.Utf8DebugString());\n";
    desc_file += "\t\tif (m_astDescMap.find(desc." + key_en_name + "()) != m_astDescMap.end())\n";
    desc_file += "\t\t{\n";
    desc_file += "\t\t\tNFLogError(NF_LOG_DEFAULT, 0, \"the desc:{} id:{} exist\", GetClassName(), desc." + key_en_name + "());\n";
    desc_file += "\t\t\treturn -1;\n";
    desc_file += "\t\t}\n";


    desc_file += "\t\tCHECK_EXPR_ASSERT(m_astDescVec.size() < m_astDescVec.max_size(), -1, \"m_astDescVec Space Not Enough\");\n";
    desc_file += "\t\tm_astDescVec.emplace_back();\n";
    desc_file += "\t\tauto pDesc = &m_astDescVec.back();\n";
    desc_file += "\t\tCHECK_EXPR_ASSERT(pDesc, -1, \"m_astDescVec Insert Failed desc.id:{}\", desc." + key_en_name + "());\n";
    desc_file += "\t\tpDesc->FromPb(desc);\n";
    desc_file += "\t\tCHECK_EXPR_ASSERT(m_astDescMap.size() < m_astDescMap.max_size(), -1, \"m_astDescMap Space Not Enough\");\n";
    desc_file += "\t\tm_astDescMap.insert(std::make_pair(desc." + key_en_name + "(), m_astDescVec.size()-1));\n";
    desc_file += "\t\tCHECK_EXPR_ASSERT(GetDesc(desc." + key_en_name + "()) == pDesc, -1, \"GetDesc != pDesc, id:{}\", desc." + key_en_name + "());\n";

    desc_file += "\t}\n\n";

    for (auto iter = pSheet->m_indexMap.begin(); iter != pSheet->m_indexMap.end(); iter++)
    {
        ExcelSheetIndex& index = iter->second;
        desc_file += "\t" + index.m_varName + ".clear();\n";
    }

    for (auto iter = pSheet->m_comIndexMap.begin(); iter != pSheet->m_comIndexMap.end(); iter++)
    {
        ExcelSheetComIndex& comIndex = iter->second;
        desc_file += "\t" + comIndex.m_varName + ".clear();\n";
    }

    if (pSheet->m_indexMap.size() > 0 || pSheet->m_comIndexMap.size() > 0)
    {
        desc_file += "\tfor(int i = 0; i < (int)m_astDescVec.size(); i++)\n";
        desc_file += "\t{\n";
        desc_file += "\t\tauto pDesc = &m_astDescVec[i];\n";
        for (auto iter = pSheet->m_indexMap.begin(); iter != pSheet->m_indexMap.end(); iter++)
        {
            ExcelSheetIndex& index = iter->second;
            std::string index_key = index.m_key;
            std::string index_key_en_name = index_key;
            std::string index_map = index.m_varName;
            desc_file += "\t\t{\n";
            if (index.m_unique)
            {
                desc_file += "\t\t\tCHECK_EXPR_ASSERT(" + index_map + ".find(pDesc->" + index_key_en_name + ") == " + index_map +
                    ".end(), -1, \"unique index:" + index_key + " repeat key:{}\", pDesc->" + index_key_en_name + ");\n";
                desc_file += "\t\t\tauto pair_iter = " + index_map + ".insert(std::make_pair(pDesc->" + index_key_en_name + ", i));\n";
                desc_file += "\t\t\tCHECK_EXPR_ASSERT(pair_iter.second && pair_iter.first != " + index_map + ".end(), -1, \"" + index_map + ".insert Failed pDesc->" +
                    index_key_en_name + ":{}, space not enough\", pDesc->" + index_key_en_name + ");\n";
            }
            else
            {
                desc_file += "\t\t\tauto iter = " + index_map + ".insert(std::make_pair(pDesc->" + index_key_en_name + ", i));\n";
                desc_file += "\t\t\tCHECK_EXPR_ASSERT(iter != " + index_map + ".end(), -1, \"" + index_map + ".insert Failed pDesc->" +
                    index_key_en_name + ":{}, space not enough\", pDesc->" + index_key_en_name + ");\n";
            }
            desc_file += "\t\t}\n";
        }

        for (auto iter = pSheet->m_comIndexMap.begin(); iter != pSheet->m_comIndexMap.end(); iter++)
        {
            ExcelSheetComIndex& comIndex = iter->second;
            std::string index_map = comIndex.m_varName;
            std::string comIndexClassName = comIndex.m_structComName;
            desc_file += "\t\t{\n";
            desc_file += "\t\t\t" + comIndexClassName + " data;\n";
            for (int i = 0; i < (int)comIndex.m_index.size(); i++)
            {
                ExcelSheetIndex& index = comIndex.m_index[i];
                std::string index_key = index.m_key;
                desc_file += "\t\t\tdata." + index_key + " = pDesc->" + index_key + ";\n";
            }

            if (comIndex.m_unique)
            {
                desc_file += "\t\t\tCHECK_EXPR_ASSERT(" + index_map + ".find(data) == " + index_map +
                    ".end(), -1, \"unique index:" + comIndexClassName + " repeat key:{}\", data.ToString());\n";
                desc_file += "\t\t\tauto key_iter = " + index_map + ".insert(std::make_pair(data, i));\n";
                desc_file += "\t\t\t\tCHECK_EXPR_ASSERT(key_iter.second && key_iter.first != " + index_map +
                    ".end(), -1, \"key:{} space not enough\", data.ToString());\n";
            }
            else
            {
                desc_file += "\t\t\tauto key_iter = " + index_map + ".insert(std::make_pair(data, i));\n";
                desc_file += "\t\t\t\tCHECK_EXPR_ASSERT(key_iter != " + index_map +
                    ".end(), -1, \"key:{} space not enough\", data.ToString());\n";
            }
            desc_file += "\t\t}\n";
        }

        desc_file += "\t}\n";
    }

    desc_file += "\tif (IsReloading())\n";
    desc_file += "\t{\n";
    desc_file += "\t\t" + NFStringUtility::FirstUpper(pSheet->m_excelName) + "DescEx::Instance()->SetReLoading(true);\n";
    desc_file += "\t}\n";
    desc_file += "\n";
    desc_file += "\tNFLogTrace(NF_LOG_DEFAULT, 0, \"load {}, num={}\", iRet, table.e_" + NFStringUtility::Lower(pSheet->m_otherName) + "_list_size());\n";
    desc_file += "\treturn 0;\n";
    desc_file += "}\n\n";
    ////////////////////////////////////////////////////////////////
    desc_file += "int " + pSheet->m_otherName + "Desc::CheckWhenAllDataLoaded()\n";
    desc_file += "{\n";
    if (pSheet->m_colRelationMap.size() > 0)
    {
        desc_file += "\tint result = 0;\n";
        desc_file += "\tfor(int i = 0; i < (int)m_astDescVec.size(); i++)\n";
        desc_file += "\t{\n";
        desc_file += "\t\tauto pDesc = &m_astDescVec[i];\n";
        for (auto iter = pSheet->m_colRelationMap.begin(); iter != pSheet->m_colRelationMap.end(); iter++)
        {
            auto pRelation = &iter->second;
            CHECK_EXPR_ASSERT(pSheet->m_colInfoMap.find(pRelation->m_myColName) != pSheet->m_colInfoMap.end(), , "");
            auto pColInfo = pSheet->m_colInfoMap[pRelation->m_myColName];

            WriteRelation(pSheet, desc_file, pRelation, pColInfo, 0);
        }
        desc_file += "\t}\n";
        desc_file += "\treturn result;\n";
    }
    else
    {
        desc_file += "\treturn 0;\n";
    }

    desc_file += "}\n\n";

    for (auto iter = pSheet->m_indexMap.begin(); iter != pSheet->m_indexMap.end(); iter++)
    {
        ExcelSheetIndex& index = iter->second;
        std::string index_key = index.m_key;
        std::string index_map = index.m_varName;
        if (index.m_unique)
        {
            desc_file += "const " + pSheet->m_protoInfo.m_protoMsgName + "* " +
                pSheet->m_otherName + "Desc::GetDescBy" +
                NFStringUtility::ToFirstUpperChangeVarNameGood(index_key) + "(int64_t " +  NFStringUtility::ToChangeVarNameGood(index_key) + ") const\n";
            desc_file += "{\n";
            desc_file += "\tauto iter = " + index_map + ".find(" + NFStringUtility::ToChangeVarNameGood(index_key) + ");\n";
            desc_file += "\tif(iter != " + index_map + ".end())\n";
            desc_file += "\t{\n";
            desc_file += "\t\treturn GetDescByIndex(iter->second);\n";
            desc_file += "\t}\n";
            desc_file += "\treturn nullptr;\n";
            desc_file += "}\n\n";
        }
        else
        {
            desc_file += "std::vector<const " + pSheet->m_protoInfo.m_protoMsgName +
                "*> " + pSheet->m_otherName + "Desc::GetDescBy" +
                NFStringUtility::ToFirstUpperChangeVarNameGood(index_key) + "(int64_t " + NFStringUtility::ToChangeVarNameGood(index_key) + ") const\n";
            desc_file += "{\n";
            desc_file += "\tstd::vector<const " + pSheet->m_protoInfo.m_protoMsgName +
                "*> m_vec;\n";
            desc_file += "\tauto iter_pair = " + index_map + ".equal_range(" + NFStringUtility::ToChangeVarNameGood(index_key) + ");\n";
            desc_file += "\tfor(auto iter = iter_pair.first; iter != iter_pair.second; iter++)\n";
            desc_file += "\t{\n";
            desc_file += "\t\tauto pDesc = GetDescByIndex(iter->second);\n";
            desc_file += "\t\tCHECK_EXPR_CONTINUE(pDesc, \"key:{} GetDesc error:{}\", " + NFStringUtility::ToChangeVarNameGood(index_key) +
                ", iter->second);\n";
            desc_file += "\t\tm_vec.push_back(pDesc);\n";
            desc_file += "\t}\n";
            desc_file += "\treturn m_vec;\n";
            desc_file += "}\n\n";
        }
    }

    for (auto iter = pSheet->m_comIndexMap.begin(); iter != pSheet->m_comIndexMap.end(); iter++)
    {
        ExcelSheetComIndex& comIndex = iter->second;
        std::string index_map = comIndex.m_varName;

        if (comIndex.m_unique)
        {
            desc_file += "const " + pSheet->m_protoInfo.m_protoMsgName + "* " +
                pSheet->m_otherName + "Desc::GetDescBy";
        }
        else
        {
            desc_file += "std::vector<const " + pSheet->m_protoInfo.m_protoMsgName +
                "*> " + pSheet->m_otherName + "Desc::GetDescBy";
        }

        for (int i = 0; i < (int)comIndex.m_index.size(); i++)
        {
            ExcelSheetIndex& index = comIndex.m_index[i];
            std::string index_key = index.m_key;
            desc_file += NFStringUtility::ToFirstUpperChangeVarNameGood(index_key);
        }

        desc_file += "(";

        for (int i = 0; i < (int)comIndex.m_index.size(); i++)
        {
            ExcelSheetIndex& index = comIndex.m_index[i];
            std::string index_key = index.m_key;
            if (i == (int)comIndex.m_index.size() - 1)
            {
                desc_file += "int64_t " + NFStringUtility::ToChangeVarNameGood(index_key);
            }
            else
            {
                desc_file += "int64_t " + NFStringUtility::ToChangeVarNameGood(index_key) + ", ";
            }
        }

        desc_file += ")\n";
        std::string className = comIndex.m_structComName;
        if (comIndex.m_unique)
        {
            desc_file += "{\n";
            desc_file += "\t" + className + " data;\n";
            for (int i = 0; i < (int)comIndex.m_index.size(); i++)
            {
                ExcelSheetIndex& index = comIndex.m_index[i];
                std::string index_key = index.m_key;
                desc_file += "\tdata." + index_key + " = " + NFStringUtility::ToChangeVarNameGood(index_key) + ";\n";
            }

            desc_file += "\tauto iter = " + index_map + ".find(data);\n";
            desc_file += "\tif(iter != " + index_map + ".end())\n";
            desc_file += "\t{\n";
            desc_file += "\t\tauto pDesc = GetDescByIndex(iter->second);\n";
            desc_file += "\t\tCHECK_EXPR(pDesc, nullptr, \"GetDesc failed:{}\", iter->second);\n";
            desc_file += "\t\treturn pDesc;\n";
            desc_file += "\t}\n";
            desc_file += "\treturn nullptr;\n";
            desc_file += "}\n\n";
        }
        else
        {
            desc_file += "{\n";
            desc_file += "\t" + className + " data;\n";
            for (int i = 0; i < (int)comIndex.m_index.size(); i++)
            {
                ExcelSheetIndex& index = comIndex.m_index[i];
                std::string index_key = index.m_key;
                desc_file += "\tdata." + index_key + " = " + NFStringUtility::ToChangeVarNameGood(index_key) + ";\n";
            }
            desc_file += "\tstd::vector<const " + pSheet->m_protoInfo.m_protoMsgName +
                "*> m_vec;\n";
            desc_file += "\tauto iter_pair = " + index_map + ".equal_range(data);\n";
            desc_file += "\tfor(auto iter = iter_pair.first; iter != iter_pair.second; iter++)\n";
            desc_file += "\t{\n";
            desc_file += "\t\tauto pDesc = GetDescByIndex(iter->second);\n";
            desc_file += "\t\tCHECK_EXPR_CONTINUE(pDesc, \"GetDesc failed:{}\", iter->second);\n";
            desc_file += "\t\tm_vec.push_back(pDesc);\n";

            desc_file += "\t}\n";
            desc_file += "\treturn m_vec;\n";
            desc_file += "}\n\n";
        }
    }

    NFFileUtility::WriteFile(desc_file_path, desc_file);
}

int ExcelToProto::WriteRelation(ExcelSheet* pSheet, std::string& desc_file, ExcelRelation* pRelation, ExcelSheetColInfo* pColInfo, int depth)
{
    CHECK_NULL(0, pRelation);
    CHECK_NULL(0, pColInfo);

    char var = 'i';
    var += depth + 1;
    std::string j = {var};
    char vark = 'i';
    vark += depth + 2;
    std::string k = {vark};
    std::string pDesc;
    if (depth == 0)
    {
        pDesc = "pDesc";
    }
    else
    {
        pDesc = "pDesc" + j;
    }

    if (pRelation->m_relationMap.empty())
    {
        CHECK_EXPR_ASSERT(pColInfo->m_colInfoMap.empty(), -1, "");
        if (!pColInfo->m_isArray)
        {
            for (int i = 0; i < depth + 2; i++)
            {
                desc_file += "\t";
            }
            desc_file += "CHECK_EXPR_MSG_RESULT(";
            for (int i = 0; i < (int)pRelation->m_dst.size(); i++)
            {
                ExcelRelationDst& relationDst = pRelation->m_dst[i];
                desc_file += "(" + pDesc + "->" + pRelation->m_myColName + " <= 0 || " + relationDst.m_descName +
                    "Desc::Instance()->GetDesc(" + pDesc + "->" + pRelation->m_myColName +
                    "))";

                if (i != (int)pRelation->m_dst.size() - 1)
                {
                    desc_file += " || ";
                }
            }


            desc_file += ", result, \"can't find the " + pRelation->m_myColName + ":{} in the " + pRelation->m_noFindError +
                "\", " + pDesc + "->" + pRelation->m_myColName + ");\n";
        }
        else
        {
            for (int i = 0; i < depth + 2; i++)
            {
                desc_file += "\t";
            }
            desc_file += "for(int " + j + " = 0; " + j + " < (int)" + pDesc + "->" + pRelation->m_myColName + ".size(); " + j + "++)\n";
            for (int i = 0; i < depth + 2; i++)
            {
                desc_file += "\t";
            }
            desc_file += "{\n";
            for (int i = 0; i < depth + 3; i++)
            {
                desc_file += "\t";
            }
            desc_file += "CHECK_EXPR_MSG_RESULT(";
            for (int i = 0; i < (int)pRelation->m_dst.size(); i++)
            {
                ExcelRelationDst& relationDst = pRelation->m_dst[i];
                desc_file += "(" + pDesc + "->" + pRelation->m_myColName + "[" + j + "] <= 0 || " + relationDst.m_descName +
                    "Desc::Instance()->GetDesc(" + pDesc + "->" + pRelation->m_myColName +
                    "[" + j + "]))";

                if (i != (int)pRelation->m_dst.size() - 1)
                {
                    desc_file += " || ";
                }
            }

            desc_file += ", result, \"can't find the " + pRelation->m_myColName + ":{} in the " + pRelation->m_noFindError +
                "\", " + pDesc + "->" + pRelation->m_myColName + "[" + j + "]);\n";
            for (int i = 0; i < depth + 2; i++)
            {
                desc_file += "\t";
            }
            desc_file += "}\n";
        }
    }
    else
    {
        CHECK_EXPR_ASSERT(!pColInfo->m_colInfoMap.empty(), -1, "");
        for (int i = 0; i < depth + 2; i++)
        {
            desc_file += "\t";
        }
        desc_file += "for(int " + j + " = 0; " + j + " < (int)" + pDesc + "->" + pRelation->m_myColName + ".size(); " + j + "++)\n";
        for (int i = 0; i < depth + 2; i++)
        {
            desc_file += "\t";
        }
        desc_file += "{\n";
        for (int i = 0; i < depth + 3; i++)
        {
            desc_file += "\t";
        }
        desc_file += "auto pDesc" + k + " = &" + pDesc + "->" + pRelation->m_myColName + "[" + j + "];\n";
        for (auto iter = pRelation->m_relationMap.begin(); iter != pRelation->m_relationMap.end(); iter++)
        {
            ExcelRelation* pLastRelation = iter->second;
            CHECK_EXPR_ASSERT(pColInfo->m_colInfoMap.find(pLastRelation->m_myColName) != pColInfo->m_colInfoMap.end(), -1, "");
            ExcelSheetColInfo* pLastColInfo = pColInfo->m_colInfoMap[pLastRelation->m_myColName];
            WriteRelation(pSheet, desc_file, pLastRelation, pLastColInfo, depth + 1);
        }
        for (int i = 0; i < depth + 2; i++)
        {
            desc_file += "\t";
        }
        desc_file += "}\n";
    }
    return 0;
}

void ExcelToProto::WriteMakeFile()
{
    std::string excel_src_file_name = NFFileUtility::GetFileName(m_excel);
    std::string sheet_makefile_name = m_outPath + m_excelName + "_gen.makefile";
    std::string makefile_file;
    makefile_file += "include ./define.makefile\n\n";
    makefile_file += ".PHONY:all\n\n";
    makefile_file += "all:${PROTOCGEN_FILE_PATH}/module_" + m_excelName + "_bin\n\n";

    makefile_file +=
        "${PROTOCGEN_FILE_PATH}/module_" + m_excelName + "_bin:${PROTOCGEN_FILE_PATH}/" + m_excelName + ".proto.ds ${RESDB_EXCELMMO_PATH}/" +
        excel_src_file_name + "\n";
    makefile_file += "\tmkdir -p ${PROTOCGEN_FILE_PATH}\n";
    makefile_file += "\trm -rf ${PROTOCGEN_FILE_PATH}/module_" + m_excelName + "_bin\n";
    makefile_file +=
        "\t${NFEXCELPROCESS} --work=\"exceltobin\" --excel_json=${RESDB_EXCEL_JSON} --src=${RESDB_EXCELMMO_PATH}/" + excel_src_file_name + "  --proto_ds=${PROTOCGEN_FILE_PATH}/" +
        m_excelName + ".proto.ds --dst=${PROTOCGEN_FILE_PATH}/;\n";

    makefile_file += "\t${FILE_COPY_EXE} --work=\"filecopy_notexist\" --src=\"${PROTOCGEN_FILE_PATH}/" + NFStringUtility::FirstUpper(m_excelName) +
        +"DescEx.h " + "${PROTOCGEN_FILE_PATH}/" +
        NFStringUtility::FirstUpper(m_excelName) +
        "DescEx.cpp\" --dst=${DESC_STORE_EX_PATH}/\n";
    XLWorkbook wxbook = m_excelReader.workbook();
    std::vector<std::string> vecSheet = wxbook.worksheetNames();
    for (int i = 0; i < (int)vecSheet.size(); i++)
    {
        std::string sheetname = vecSheet[i];
        XLWorksheet sheet = wxbook.worksheet(sheetname);
        if (m_sheets.find(sheet.name()) != m_sheets.end() && m_sheets[sheet.name()].m_colInfoMap.size() > 0)
        {
            ExcelSheet* pSheet = &m_sheets[sheet.name()];
            std::string sheet_name = pSheet->m_sheetName;

            makefile_file += "\t${FILE_COPY_EXE} --work=\"filecopy\" --src=\"${PROTOCGEN_FILE_PATH}/" + pSheet->m_protoInfo.m_binFileName + ".bin" + "\" --dst=${GAME_DATA_PATH}/\n";
            makefile_file += "\t${FILE_COPY_EXE} --work=\"filecopy\" --src=\"${PROTOCGEN_FILE_PATH}/" + pSheet->m_protoInfo.m_binFileName + "_debug.json" + "\" --dst=${GAME_DATA_PATH}/\n";
            makefile_file += "\t${FILE_COPY_EXE} --work=\"filecopy\" --src=\"${PROTOCGEN_FILE_PATH}/" + pSheet->m_otherName + "Desc.h " + "${PROTOCGEN_FILE_PATH}/" + pSheet->m_otherName + "Desc.cpp\" --dst=${DESC_STORE_PATH}/\n";

            if (pSheet->m_createSql)
            {
                makefile_file += "\t${FILE_COPY_EXE} --work=\"filecopy\" --src=\"${PROTOCGEN_FILE_PATH}/CreateTable_" + pSheet->m_protoInfo.m_binFileName + ".sql\" --dst=${GAME_SQL_PATH}/\n";
            }
        }
    }
    makefile_file += "\ttouch ${PROTOCGEN_FILE_PATH}/module_" + m_excelName + "_bin\n";

    NFFileUtility::WriteFile(sheet_makefile_name, makefile_file);
}

void ExcelToProto::WriteDestStoreDefine()
{
    std::string descStoreHeadFile = m_outPath + "NFDescStoreHead.h";
    std::string destStoreHeadFileRead;
    std::string descStoreHeadFileStr;
    if (!NFFileUtility::IsFileExist(descStoreHeadFile))
    {
        descStoreHeadFileStr += "#pragma once\n\n";
    }
    else
    {
        NFFileUtility::ReadFileContent(descStoreHeadFile, destStoreHeadFileRead);
        NFStringUtility::Trim(destStoreHeadFileRead);
        destStoreHeadFileRead += "\n";
    }

    std::string descStoreDefineFile = m_outPath + "NFDescStoreDefine.h";
    std::string descStoreDefineFileStr;
    std::string descStoreDefineFileRead;
    if (!NFFileUtility::IsFileExist(descStoreDefineFile))
    {
        descStoreDefineFileStr += "#pragma once\n\n";
        descStoreDefineFileStr += "#define EOT_DESC_STORE_ALL_ID_DEFINE ";
    }
    else
    {
        NFFileUtility::ReadFileContent(descStoreDefineFile, descStoreDefineFileRead);
        NFStringUtility::Trim(descStoreDefineFileRead);
        descStoreDefineFileRead += "\n";
    }

    std::string descStoreRegisterFile = m_outPath + "NFDescStoreRegister.h";
    std::string descStoreRegisterFileStr;
    std::string descStoreRegisterFileRead;
    if (!NFFileUtility::IsFileExist(descStoreRegisterFile))
    {
        descStoreRegisterFileStr += "#pragma once\n\n";
        descStoreRegisterFileStr += "#define EOT_DESC_STORE_ALL_REGISTER_DEFINE ";
    }
    else
    {
        NFFileUtility::ReadFileContent(descStoreRegisterFile, descStoreRegisterFileRead);
        NFStringUtility::Trim(descStoreRegisterFileRead);
        descStoreRegisterFileRead += "\n";
    }


    XLWorkbook wxbook = m_excelReader.workbook();
    std::vector<std::string> vecSheet = wxbook.worksheetNames();
    for (int i = 0; i < (int)vecSheet.size(); i++)
    {
        std::string sheetname = vecSheet[i];
        XLWorksheet sheet = wxbook.worksheet(sheetname);
        if (m_sheets.find(sheet.name()) != m_sheets.end() && m_sheets[sheet.name()].m_colInfoMap.size() > 0)
        {
            ExcelSheet* pSheet = &m_sheets[sheet.name()];
            std::string sheet_name = pSheet->m_sheetName;
            if (destStoreHeadFileRead.find("#include \"DescStore/" + pSheet->m_otherName + "Desc.h\"") == std::string::npos)
            {
                descStoreHeadFileStr += "#include \"DescStore/" + pSheet->m_otherName + "Desc.h\"\n";
            }
        }
    }

    if (destStoreHeadFileRead.find("#include \"DescStoreEx/" + NFStringUtility::FirstUpper(m_excelName) + "DescEx") == std::string::npos)
    {
        descStoreHeadFileStr += "#include \"DescStoreEx/" + NFStringUtility::FirstUpper(m_excelName) + "DescEx.h\"\n";
    }

    for (int i = 0; i < (int)vecSheet.size(); i++)
    {
        std::string sheetname = vecSheet[i];
        XLWorksheet sheet = wxbook.worksheet(sheetname);
        if (m_sheets.find(sheet.name()) != m_sheets.end() && m_sheets[sheet.name()].m_colInfoMap.size() > 0)
        {
            ExcelSheet* pSheet = &m_sheets[sheet.name()];
            std::string sheet_name = pSheet->m_sheetName;
            if (descStoreDefineFileRead.find("EOT_CONST_" + NFStringUtility::Upper(m_excelName) + "_" + NFStringUtility::Upper(sheet_name)) == std::string::npos)
            {
                descStoreDefineFileStr += "EOT_CONST_" + NFStringUtility::Upper(m_excelName) + "_" + NFStringUtility::Upper(sheet_name) + "_DESC_ID,\\\n";
            }
        }
    }

    if (descStoreDefineFileRead.find("EOT_CONST_" + NFStringUtility::Upper(m_excelName) + "_DESC_EX_ID") == std::string::npos)
    {
        descStoreDefineFileStr += "EOT_CONST_" + NFStringUtility::Upper(m_excelName) + "_DESC_EX_ID,\\\n";
    }

    for (int i = 0; i < (int)vecSheet.size(); i++)
    {
        std::string sheetname = vecSheet[i];
        XLWorksheet sheet = wxbook.worksheet(sheetname);
        if (m_sheets.find(sheet.name()) != m_sheets.end() && m_sheets[sheet.name()].m_colInfoMap.size() > 0)
        {
            ExcelSheet* pSheet = &m_sheets[sheet.name()];
            std::string sheet_name = pSheet->m_sheetName;
            if (descStoreRegisterFileRead.find("REGISTER_DESCSTORE(" + pSheet->m_otherName) ==
                std::string::npos)
            {
                descStoreRegisterFileStr += "REGISTER_DESCSTORE(" + pSheet->m_otherName + "Desc);\\\n";
            }
        }
    }

    if (descStoreRegisterFileRead.find("REGISTER_DESCSTORE_EX(" + NFStringUtility::FirstUpper(m_excelName) + "DescEx") ==
        std::string::npos)
    {
        descStoreRegisterFileStr += "REGISTER_DESCSTORE_EX(" + NFStringUtility::FirstUpper(m_excelName) + "DescEx);\\\n";
    }

    descStoreDefineFileStr += "\n\n";
    descStoreRegisterFileStr += "\n\n";
    destStoreHeadFileRead += descStoreHeadFileStr;
    descStoreDefineFileRead += descStoreDefineFileStr;
    descStoreRegisterFileRead += descStoreRegisterFileStr;

    NFFileUtility::WriteFile(descStoreHeadFile, destStoreHeadFileRead);
    NFFileUtility::WriteFile(descStoreDefineFile, descStoreDefineFileRead);
    NFFileUtility::WriteFile(descStoreRegisterFile, descStoreRegisterFileRead);
}
