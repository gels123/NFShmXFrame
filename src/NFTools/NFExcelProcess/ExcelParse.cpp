// -------------------------------------------------------------------------
//    @FileName         :    ExcelParse.cpp
//    @Author           :    gaoyi
//    @Date             :    23-8-26
//    @Email			:    445267987@qq.com
//    @Module           :    ExcelParse
//
// -------------------------------------------------------------------------

#include "ExcelParse.h"

#include <NFComm/NFCore/NFTimeUtil.h>
#include <NFComm/NFPluginModule/NFJson2PB/NFJsonToPb.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include "ExcelJsonParse.h"

ExcelParse::ExcelParse()
{
}

ExcelParse::~ExcelParse()
{
}

/**
 * 初始化Excel解析对象
 *
 * @param excel Excel文件的路径
 * @param outPath 输出文件的路径
 * @param all 是否处理所有工作表的标志（目前未使用）
 *
 * @return 成功返回0，失败返回-1
 *
 * 此函数主要负责初始化Excel解析过程所需的文件和路径 它首先将Excel文件路径和输出路径保存到成员变量中，
 * 然后对这些路径进行修剪和标准化处理以确保它们的正确性 最后，检查Excel文件是否存在，输出路径是否为有效目录，
 * 并打开Excel文件以供后续读取
 */
int ExcelParse::Init(const std::string& excel, const std::string& outPath, bool all)
{
    // 保存Excel文件路径和输出路径到成员变量
    m_excel = excel;
    m_outPath = outPath;

    // 对路径进行修剪处理，去除前后空格
    NFStringUtility::Trim(m_excel);
    NFStringUtility::Trim(m_outPath);

    // 对输出路径进行标准化处理，确保路径格式的统一性
    m_outPath = NFFileUtility::NormalizePath(m_outPath);

    // 提取Excel文件名（不带扩展名）作为表名
    m_excelName = NFFileUtility::GetFileNameWithoutExt(m_excel);

    // 检查Excel文件是否存在
    CHECK_EXPR(NFFileUtility::IsFileExist(m_excel), -1, "excel:{} not exist", m_excel);

    // 检查输出路径是否为有效目录
    CHECK_EXPR(NFFileUtility::IsDir(m_outPath), -1, "outPath:{} not exist", m_outPath);

    // 打开Excel文件以供后续读取
    m_excelReader.open(m_excel);

    return 0;
}


/**
 * 处理Excel文件的主函数
 *
 * 本函数负责依次调用处理Sheet列表、工作表和索引的成员函数
 * 它还记录了每个处理步骤的耗时，以便性能监控和优化
 *
 * @return 返回处理结果，0表示成功，非0表示失败
 */
int ExcelParse::HandleExcel()
{
    // 记录开始处理Sheet列表的时间
    uint64_t startTime = NFGetTime();

    // 处理Sheet列表
    int iRet = HandleSheetList();
    // 如果处理失败，直接返回错误码
    if (iRet != 0)
    {
        return iRet;
    }

    // 计算处理Sheet列表的耗时
    double useTime = (NFGetTime() - startTime) / 1000.0;
    // 日志输出处理Sheet列表的耗时信息
    NFLogInfo(NF_LOG_DEFAULT, 0, "HandleSheetList use time:{}", useTime)

    // 记录开始处理工作表的时间
    startTime = NFGetTime();

    // 处理工作表
    iRet = HandleSheetWork();
    // 如果处理失败，直接返回错误码
    if (iRet != 0)
    {
        return iRet;
    }

    // 计算处理工作表的耗时
    useTime = (NFGetTime() - startTime) / 1000.0;
    // 日志输出处理工作表的耗时信息
    NFLogInfo(NF_LOG_DEFAULT, 0, "HandleSheetWork use time:{}", useTime)

    // 记录开始处理Sheet索引的时间
    startTime = NFGetTime();

    // 处理Sheet索引
    iRet = HandleSheetIndex();
    // 如果处理失败，直接返回错误码
    if (iRet != 0)
    {
        return iRet;
    }

    // 计算处理Sheet索引的耗时
    useTime = (NFGetTime() - startTime) / 1000.0;
    // 日志输出处理Sheet索引的耗时信息
    NFLogInfo(NF_LOG_DEFAULT, 0, "HandleSheetIndex use time:{}", useTime)

    // 调用处理Sheet协议信息的函数
    OnHandleSheetProtoInfo();

    // 返回成功码
    return 0;
}


int ExcelParse::HandleSheetList()
{
    //首先处理list
    //获取工作簿中的所有工作表名称
    XLWorkbook wxbook = m_excelReader.workbook();
    std::vector<std::string> vecSheet = wxbook.worksheetNames();

    //遍历所有工作表名称
    for (int i = 0; i < (int)vecSheet.size(); i++)
    {
        std::string sheetname = vecSheet[i];
        XLWorksheet sheet = wxbook.worksheet(sheetname);

        //如果工作表名称为"main"，则记录日志信息
        if (sheetname == "main")
        {
            NFLogInfo(NF_LOG_DEFAULT, 0, "handle excel:{} sheet:{}", m_excel, sheet.name());
        }
        //如果工作表名称为"list"，则处理该工作表并根据处理结果决定是否继续
        else if (sheetname == "list")
        {
            NFLogInfo(NF_LOG_DEFAULT, 0, "handle excel:{} sheet:{}", m_excel, sheet.name());
            int iRet = HandleSheetList(sheet);
            if (iRet != 0)
            {
                return iRet;
            }
        }
    }

    //所有工作表处理完毕，返回成功
    return 0;
}

// 解析Excel表格的索引字符串，以支持复杂的索引操作
// 参数indexStr是索引字符串，以分号分隔不同的索引列组合
// 参数unique指示索引是否唯一
int ExcelSheet::ParseIndex(const std::string& indexStr, bool unique)
{
    // 将索引字符串按分号分割，得到每一组索引列字符串
    std::vector<std::string> col_index_str_list;
    NFStringUtility::Split(indexStr, ";", &col_index_str_list);

    // 遍历每一组索引列字符串
    for (int j = 0; j < (int)col_index_str_list.size(); j++)
    {
        std::string col_index_str_list_str = col_index_str_list[j];
        NFStringUtility::Trim(col_index_str_list_str);
        if (col_index_str_list_str.empty()) continue;

        // 将每一组索引列字符串按逗号分割，得到单独的索引列
        std::vector<std::string> col_index_str_vec;
        NFStringUtility::Split(col_index_str_list_str, ",", &col_index_str_vec);

        // 如果只有一个索引列，创建一个简单的ExcelSheetIndex对象
        if (col_index_str_vec.size() == 1)
        {
            std::string key = col_index_str_vec[0];
            NFStringUtility::Trim(key);
            if (key.empty()) continue;

            ExcelSheetIndex& excelSheetIndex = m_indexMap[key];
            excelSheetIndex.m_key = key;
            excelSheetIndex.m_unique = unique;
            excelSheetIndex.m_colIndex = -1;
            excelSheetIndex.m_varName = "m_" + NFStringUtility::ToChangeVarNameGood(key) + "IndexMap";

            NFLogInfo(NF_LOG_DEFAULT, 0, "excel:{} sheet:{} add index:{}", m_excelName, m_sheetName, key);
        }
        else
        {
            // 如果有多个索引列，创建一个复合的ExcelSheetComIndex对象
            ExcelSheetComIndex& excelSheetComIndex = m_comIndexMap[col_index_str_list_str];
            excelSheetComIndex.m_key = m_uniqueIndexStr;
            excelSheetComIndex.m_unique = unique;
            excelSheetComIndex.m_varName = "m_";
            std::string tempName;

            // 遍历每一个索引列，创建简单的ExcelSheetIndex对象，并添加到复合索引对象中
            for (int x = 0; x < (int)col_index_str_vec.size(); x++)
            {
                std::string key = col_index_str_vec[x];
                NFStringUtility::Trim(key);
                if (key.empty()) continue;

                ExcelSheetIndex excelSheetIndex;
                excelSheetIndex.m_key = key;
                excelSheetIndex.m_unique = unique;
                excelSheetIndex.m_colIndex = -1;
                if (x == 0)
                {
                    tempName += NFStringUtility::ToChangeVarNameGood(key);
                }
                else
                {
                    tempName += NFStringUtility::ToFirstUpperChangeVarNameGood(key);
                }
                excelSheetComIndex.m_index.push_back(excelSheetIndex);
            }

            excelSheetComIndex.m_varName += tempName + "ComIndexMap";
            excelSheetComIndex.m_structComName += m_otherName + NFStringUtility::FirstUpper(tempName);
            NFLogInfo(NF_LOG_DEFAULT, 0, "excel:{} sheet:{} add com_index:{}", m_excelName, m_sheetName, col_index_str_list_str);
        }
    }
    return 0;
}


int ExcelSheet::ParseUniqueIndex(const std::string& indexStr)
{
    m_uniqueIndexStr = indexStr;
    return ParseIndex(m_uniqueIndexStr, true);
}

int ExcelSheet::ParseMultiIndex(const std::string& indexStr)
{
    m_multiIndexStr = indexStr;
    return ParseIndex(m_multiIndexStr, false);
}

// 解析Excel表格中的关系字符串
// 参数: col_index_str - 包含列索引信息的字符串
// 返回值: 成功返回0，失败返回-1
int ExcelSheet::ParseRelation(const std::string& col_index_str)
{
    // 将输入的关系字符串保存到成员变量中
    m_relationStr = col_index_str;

    // 分割输入字符串，获取每一项关系字符串
    std::vector<std::string> col_index_str_list;
    NFStringUtility::Split(col_index_str, ";", &col_index_str_list);

    // 遍历每一项关系字符串
    for (int j = 0; j < (int)col_index_str_list.size(); j++)
    {
        // 获取当前关系字符串并去除前后空格
        std::string relation_str = col_index_str_list[j];
        NFStringUtility::Trim(relation_str);
        // 如果关系字符串为空，则跳过当前项
        if (relation_str.empty()) continue;

        // 分割关系字符串，获取关系的两部分
        std::vector<std::string> relation_str_vec;
        NFStringUtility::Split(relation_str, ":", &relation_str_vec);

        // 如果关系字符串的两部分不正确，则记录错误日志并返回失败
        if (relation_str_vec.size() != 2)
        {
            NFLogError(NF_LOG_DEFAULT, 0, "the relation:{} is not right", relation_str);
            return -1;
        }

        // 获取关系的两部分字符串
        std::string my_col_name = relation_str_vec[0];
        std::string dst_relation_str = relation_str_vec[1];

        // 如果关系的两部分字符串为空，则记录错误日志并返回失败
        if (my_col_name.empty() || dst_relation_str.empty())
        {
            NFLogError(NF_LOG_DEFAULT, 0, "the relation:{} is not right", relation_str);
            return -1;
        }

        // 分割我的列名字符串，获取每一级列名
        std::vector<std::string> my_col_str_vec;
        NFStringUtility::Split(my_col_name, ".", &my_col_str_vec);
        if (my_col_str_vec.size() <= 0)
        {
            NFLogError(NF_LOG_DEFAULT, 0, "the relation:{} is not right", relation_str);
            return -1;
        }

        // 分割目标关系字符串，获取每一个目标关系
        std::vector<std::string> dst_relation_str_vec;
        NFStringUtility::Split(dst_relation_str, "|", &dst_relation_str_vec);

        // 如果目标关系为空，则记录错误日志并返回失败
        if (dst_relation_str_vec.size() <= 0)
        {
            NFLogError(NF_LOG_DEFAULT, 0, "the relation:{} is not right", relation_str);
            return -1;
        }

        // 获取头部字段名
        std::string headFieldName = my_col_str_vec[0];
        if (headFieldName.size() <= 0)
        {
            NFLogError(NF_LOG_DEFAULT, 0, "the relation:{} is not right, headFieldName empty", relation_str);
            return -1;
        }

        // 根据头部字段名获取或创建关系对象
        ExcelRelation* pRelation = NULL;
        ExcelRelation* pLastestRelation = NULL;
        if (m_colRelationMap.find(headFieldName) == m_colRelationMap.end())
        {
            pRelation = &m_colRelationMap[headFieldName];
            pRelation->m_myColName = headFieldName;
        }
        else
        {
            pRelation = &m_colRelationMap[headFieldName];
        }

        // 根据我的列名字符串构建或获取最后一级关系对象
        if (my_col_str_vec.size() == 1)
        {
            pLastestRelation = pRelation;
        }
        else
        {
            pLastestRelation = pRelation;
            for (int i = 1; i < (int)my_col_str_vec.size(); i++)
            {
                std::string fieldName = my_col_str_vec[i];
                if (pLastestRelation->m_relationMap.find(fieldName) == pLastestRelation->m_relationMap.end())
                {
                    pLastestRelation->m_relationMap[fieldName] = new ExcelRelation();
                    pLastestRelation = pLastestRelation->m_relationMap[fieldName];
                    pLastestRelation->m_myColName = fieldName;
                }
                else
                {
                    pLastestRelation = pLastestRelation->m_relationMap[fieldName];
                }
            }
        }

        // 设置最后一级关系对象的关系字符串
        pLastestRelation->m_releationStr = relation_str;

        // 遍历每一个目标关系，构建目标关系对象
        for (int i = 0; i < (int)dst_relation_str_vec.size(); i++)
        {
            std::vector<std::string> dst_relation_str_vec_vec;
            NFStringUtility::Split(dst_relation_str_vec[i], ".", &dst_relation_str_vec_vec);

            // 如果目标关系的组成部分不正确，则记录错误日志并返回失败
            if (dst_relation_str_vec_vec.size() < 2)
            {
                NFLogError(NF_LOG_DEFAULT, 0, "the relation:{} is not right", relation_str);
                return -1;
            }

            // 构建目标关系对象并设置其属性
            ExcelRelationDst relationDst;
            relationDst.m_excelName = dst_relation_str_vec_vec[0];
            relationDst.m_sheetName = dst_relation_str_vec_vec[1];
            ExcelSheetInfo* pExcelSheetInfo = ExcelJsonParse::Instance()->GetSheetInfo(relationDst.m_excelName, relationDst.m_sheetName);
            if (pExcelSheetInfo && pExcelSheetInfo->otherName.size() > 0)
            {
                relationDst.m_descName = pExcelSheetInfo->otherName;
            }
            else
            {
                relationDst.m_descName = NFStringUtility::FirstUpper(relationDst.m_excelName) + NFStringUtility::FirstUpper(relationDst.m_sheetName);
            }
            pLastestRelation->m_dst.push_back(relationDst);
        }

        // 记录成功解析关系的日志
        NFLogInfo(NF_LOG_DEFAULT, 0, "excel:{} sheet:{} add relation:{}:{}", m_excelName, m_sheetName, my_col_name,
                  dst_relation_str);
    }
    // 解析成功，返回0
    return 0;
}


int ExcelSheet::ParseEnum(const std::string& col_index_str)
{
    m_enumStr = col_index_str;
    std::vector<std::string> col_enum_str_list;
    NFStringUtility::Split(col_index_str, ";", &col_enum_str_list);

    for (int j = 0; j < (int)col_enum_str_list.size(); j++)
    {
        std::string col_enum_str_list_str = col_enum_str_list[j];
        NFStringUtility::Trim(col_enum_str_list_str);
        if (col_enum_str_list_str.empty()) continue;

        std::vector<std::string> col_enum_str_vec;
        NFStringUtility::Split(col_enum_str_list_str, ":", &col_enum_str_vec);

        if (col_enum_str_vec.size() != 2)
        {
            NFLogError(NF_LOG_DEFAULT, 0, "the enum:{} is not right", col_enum_str_list_str);
            return -1;
        }

        std::string colEnName = col_enum_str_vec[0];
        std::string enumPbName = col_enum_str_vec[1];

        if (colEnName.empty() || enumPbName.empty())
        {
            NFLogError(NF_LOG_DEFAULT, 0, "the enum:{} is not right", col_enum_str_list_str);
            return -1;
        }

        NFStringUtility::Replace(colEnName, ".", "_");

        ExcelColPbEnum colPbEnum;
        colPbEnum.m_myColName = colEnName;
        colPbEnum.m_enumPbName = enumPbName;

        if (m_colEnumMap.find(colPbEnum.m_myColName) != m_colEnumMap.end())
        {
            NFLogError(NF_LOG_DEFAULT, 0, "the enum:{} is not right, repeated", col_enum_str_list_str);
            return -1;
        }
        m_colEnumMap.emplace(colPbEnum.m_myColName, colPbEnum);
    }
    return 0;
}

int ExcelSheet::ParseImportProto(const std::string& proto)
{
    std::vector<std::string> col_index_str_list;
    NFStringUtility::Split(proto, ";", &col_index_str_list);

    for (int j = 0; j < (int)col_index_str_list.size(); j++)
    {
        std::string proto_str = col_index_str_list[j];
        NFStringUtility::Trim(proto_str);
        if (proto_str.empty()) continue;

        m_protoFileMap.insert(proto_str);
    }
    return 0;
}

int ExcelSheet::ParseAddField(const std::string& proto)
{
    std::vector<std::string> col_index_str_list;
    NFStringUtility::Split(proto, ";", &col_index_str_list);

    for (int j = 0; j < (int)col_index_str_list.size(); j++)
    {
        std::string proto_str = col_index_str_list[j];
        NFStringUtility::Trim(proto_str);
        if (proto_str.empty()) continue;

        m_addFileVec.push_back(proto_str);
    }
    return 0;
}

int ExcelSheet::ParseColMessageType(const std::string& colMessageType)
{
    m_colMessageTypeStr = colMessageType;
    std::vector<std::string> col_message_type_str_list;
    NFStringUtility::Split(colMessageType, ";", &col_message_type_str_list);

    for (int j = 0; j < (int)col_message_type_str_list.size(); j++)
    {
        std::string col_message_type_str_list_str = col_message_type_str_list[j];
        NFStringUtility::Trim(col_message_type_str_list_str);
        if (col_message_type_str_list_str.empty()) continue;

        std::vector<std::string> col_message_type_str_vec;
        NFStringUtility::Split(col_message_type_str_list_str, ":", &col_message_type_str_vec);

        if (col_message_type_str_vec.size() != 2)
        {
            NFLogError(NF_LOG_DEFAULT, 0, "the col message type:{} is not right", col_message_type_str_list_str);
            return -1;
        }

        std::string colEnName = col_message_type_str_vec[0];
        std::string messagePbName = col_message_type_str_vec[1];

        if (colEnName.empty() || messagePbName.empty())
        {
            NFLogError(NF_LOG_DEFAULT, 0, "the col message type:{} is not right", col_message_type_str_list_str);
            return -1;
        }

        NFStringUtility::Replace(colEnName, ".", "_");

        ExcelColMessageType colPbMessage;
        colPbMessage.m_myColName = colEnName;
        colPbMessage.m_messagePbName = messagePbName;

        if (m_colMessageTypeMap.find(colPbMessage.m_myColName) != m_colMessageTypeMap.end())
        {
            NFLogError(NF_LOG_DEFAULT, 0, "the col message type:{} is not right", col_message_type_str_list_str);
            return -1;
        }
        m_colMessageTypeMap.emplace(colPbMessage.m_myColName, colPbMessage);
    }
    return 0;
}

int ExcelParse::HandleSheetList(const XLWorksheet sheet)
{
    if (sheet.name() != "list") return -1;

    int maxRows = sheet.rowCount();
    int maxCols = sheet.columnCount();

    if (maxRows < 1 || maxCols < 1)
    {
        return 0;
    }

    //list表的第一行第一个自动有sheet_name的话，这一行只是用来提示功能，第二列是unique_index唯一索引，multi_index不唯一所有，check用来检查列关联到的表数据是否存在
    int start_row = SHEET_START_ROW;
    std::string cell0Value = GetSheetValue(sheet, SHEET_LIST_TITLE_ROW,SHEET_LIST_SHEET_NAME);
    if (cell0Value == "sheet_name")
    {
        start_row = SHEET_START_ROW + 1;
    }

    for (int row = start_row; row <= (int)maxRows; row++)
    {
        std::string sheet_name = GetSheetValue(sheet, row, SHEET_LIST_SHEET_NAME);
        if (sheet_name.empty())
        {
            continue;
        }

        if (!m_excelReader.workbook().worksheetExists(sheet_name))
        {
            NFLogError(NF_LOG_DEFAULT, 0, "excel:{} list has sheet:{}, but can't find in the excel", m_excel, sheet_name);
            continue;
        }

        auto realSheet = m_excelReader.workbook().worksheet(sheet_name);

        ExcelSheet& excelSheet = m_sheets[sheet_name];
        excelSheet.m_excelName = m_excelName;
        excelSheet.m_sheetName = sheet_name;
        excelSheet.m_maxRows = realSheet.rowCount();
        excelSheet.m_maxCols = realSheet.columnCount();

        ExcelSheetInfo* pExcelSheetInfo = ExcelJsonParse::Instance()->GetSheetInfo(m_excelName, sheet_name);
        if (pExcelSheetInfo)
        {
            //must first other name
            excelSheet.m_otherName = pExcelSheetInfo->otherName;
            excelSheet.m_sheetMsgNameStr = pExcelSheetInfo->sheetMsgName;
            excelSheet.m_protoMsgNameStr = pExcelSheetInfo->protoMsgName;
            OnHandleSheetProtoInfo(excelSheet);
            int iRetCode = excelSheet.ParseUniqueIndex(pExcelSheetInfo->unique_index);
            CHECK_RET(iRetCode, "ParseUniqueIndex:{} Failed, From jsonfile", pExcelSheetInfo->unique_index);

            iRetCode = excelSheet.ParseMultiIndex(pExcelSheetInfo->multi_index);
            CHECK_RET(iRetCode, "ParseMultiIndex:{} Failed, From jsonfile", pExcelSheetInfo->multi_index);

            excelSheet.m_relationStr = pExcelSheetInfo->relation;

            iRetCode = excelSheet.ParseEnum(pExcelSheetInfo->field_enum);
            CHECK_RET(iRetCode, "ParseEnum:{} Failed", pExcelSheetInfo->field_enum);

            iRetCode = excelSheet.ParseImportProto(pExcelSheetInfo->import_proto);
            CHECK_RET(iRetCode, "ParseImportProto:{} Failed", pExcelSheetInfo->import_proto);

            iRetCode = excelSheet.ParseColMessageType(pExcelSheetInfo->colMessageType);
            CHECK_RET(iRetCode, "ParseColMessageType:{} Failed", pExcelSheetInfo->colMessageType);

            iRetCode = excelSheet.ParseAddField(pExcelSheetInfo->add_field);
            CHECK_RET(iRetCode, "ParseAddField:{} Failed", pExcelSheetInfo->add_field);
        }
        else {
           OnHandleSheetProtoInfo(excelSheet);
        }
    }

    return 0;
}

int ExcelParse::HandleSheetWork()
{
    XLWorkbook wxbook = m_excelReader.workbook();
    std::vector<std::string> vecSheet = wxbook.worksheetNames();
    for (int i = 0; i < (int)vecSheet.size(); i++)
    {
        std::string sheetname = vecSheet[i];
        XLWorksheet sheet = wxbook.worksheet(sheetname);
        if (m_sheets.find(sheet.name()) != m_sheets.end())
        {
            NFLogInfo(NF_LOG_DEFAULT, 0, "handle excel:{} sheet:{}", m_excel, sheet.name());
            int iRet = HandleSheetWork(sheet);
            if (iRet != 0)
            {
                return iRet;
            }
        }
    }
    NFLogInfo(NF_LOG_DEFAULT, 0, "parse excel:{} success", m_excel);

    return 0;
}

std::string ExcelParse::GetSheetValue(const XLCellValue& cell)
{
    switch (cell.type())
    {
        case XLValueType::Empty:
        {
            return std::string();
        }
        break;
        case XLValueType::Boolean:
        {
            bool temp = cell.get<bool>();
            return NFCommon::tostr(temp);
        }
        break;
        case XLValueType::Integer:
        {
            int64_t temp = cell.get<int64_t>();
            return NFCommon::tostr(temp);
        }
        break;
        case XLValueType::Float:
        {
            float temp = cell.get<float>();
            return NFCommon::tostr(temp);
        }
        break;
        case XLValueType::String:
        {
            return cell.get<std::string>();
        }
        break;
        case XLValueType::Error:
        {
            return std::string();
        }
        break;
        default:
        {
            break;
        }
    }
    return std::string();
}

std::string ExcelParse::GetSheetValue(const XLWorksheet sheet, int row, int col)
{
    XLCell cell = sheet.cell(row, col);
    return GetSheetValue(cell.value());
}

int ExcelParse::HandleSheetWork(const XLWorksheet sheet)
{
    int rows = sheet.rowCount();
    int cols = sheet.columnCount();

    CHECK_EXPR(rows >= MAX_HEAD_ROWS && cols >= SHEET_START_COL, -1, "sheet:{} rows:{} cols:{} not supported", sheet.name(), rows, cols);

    ExcelSheet* pSheet = &m_sheets[sheet.name()];
    pSheet->m_rows = 0;

    std::vector<XLCellValue> enNameRow = sheet.row(SHEET_TITLE_EN_NAME_ROW).values();
    std::vector<XLCellValue> chNameRow = sheet.row(SHEET_TITLE_CH_NAME_ROW).values();
    std::vector<XLCellValue> typeNameRow = sheet.row(SHEET_TITLE_TYPE_ROW).values();
    std::vector<XLCellValue> selNameRow = sheet.row(SHEET_TITLE_CLIENTSERVER_ROW).values();

    for (int col_index = SHEET_START_COL; col_index <= cols; col_index++)
    {
        std::string col_en_name;
        std::string col_cn_name;
        std::string col_type;
        std::string col_sel_str;
        if (col_index - 1 < (int)enNameRow.size())
        {
            col_en_name = GetSheetValue(enNameRow[col_index - 1]);
        }
        if (col_index - 1 < (int)chNameRow.size())
        {
            col_cn_name = GetSheetValue(chNameRow[col_index - 1]);
        }
        if (col_index - 1 < (int)typeNameRow.size())
        {
            col_type = GetSheetValue(typeNameRow[col_index - 1]);
        }
        if (col_index - 1 < (int)selNameRow.size())
        {
            col_sel_str = GetSheetValue(selNameRow[col_index - 1]);
        }
        col_en_name = NFStringUtility::RemoveSpace(col_en_name);
        col_cn_name = NFStringUtility::RemoveSpace(col_cn_name);
        col_type = NFStringUtility::RemoveSpace(col_type);
        col_sel_str = NFStringUtility::RemoveSpace(col_sel_str);

        if (col_en_name.empty() && col_cn_name.empty() && col_type.empty() && col_sel_str.empty())
        {
            pSheet->m_protoInfo.m_endCol = col_index;
            break;
        }

        if (col_en_name.empty() || col_cn_name.empty() || col_type.empty() || col_sel_str.empty())
        {
            continue;
        }

        int col_sel = NFCommon::strto<int>(col_sel_str);
        if (col_sel != 2 && col_sel != 3)
            continue;

        NF_ENUM_COL_TYPE colTypeEnum = GetColType(col_type, NULL, NULL, NULL);
        if (colTypeEnum == NF_ENUM_COL_TYPE_UNKNOWN)
        {
            LOG_ERR(0, -1, "col:{} colType:{} is NF_ENUM_COL_TYPE_UNKNOWN", col_index, col_type);
            return -1;
        }

        if (pSheet->m_protoInfo.m_startCol == 0)
        {
            pSheet->m_protoInfo.m_startCol = col_index;
        }

        pSheet->m_protoInfo.m_endCol = col_index;

        if (NFStringUtility::StartsWith(col_en_name, "m_") || NFStringUtility::Contains(col_en_name, ".") || NFStringUtility::Contains(col_en_name, "["))
        {
            std::vector<std::string> col_en_name_list;
            std::vector<std::string> col_cn_name_list;
            std::vector<int> col_struct_num_list;
            SplitColHead(col_en_name, col_cn_name, col_en_name_list, col_cn_name_list, col_struct_num_list);
            int iRet = HandleNewColMsg(pSheet, NULL, col_index, col_type, col_en_name_list, col_cn_name_list, col_struct_num_list);
            CHECK_EXPR(iRet == 0, -1, "excel:{} sheet:{} col_en_name:{} col_cn_name:{} is not right", m_excel,
                       pSheet->m_sheetName, col_en_name, col_cn_name);
        }
        else
        {
            std::vector<std::string> col_en_name_list;
            NFStringUtility::Split(col_en_name, "_", &col_en_name_list);
            for (int i = 0; i < (int)col_en_name_list.size(); i++)
            {
                NFStringUtility::Trim(col_en_name_list[i]);
            }

            std::vector<std::string> col_cn_name_list;
            NFStringUtility::SplitDigit(col_cn_name, &col_cn_name_list);
            for (int i = 0; i < (int)col_cn_name_list.size(); i++)
            {
                NFStringUtility::Trim(col_cn_name_list[i]);
            }

            if (col_en_name_list.size() == 2 && col_cn_name_list.size() == 3)
            {
                /** 处理正常数组
                 * @brief item_id item_num item_id item_num
                 *        物品1Id  物品1Num 物品2Id    物品2Num
                 */
                if (col_en_name_list[0].size() > 0 && col_en_name_list[1].size() > 0
                    && col_cn_name_list[0].size() > 0 && col_cn_name_list[1].size() > 0 && col_cn_name_list[2].size() > 0)
                {
                    std::string struct_en_name = col_en_name_list[0];
                    std::string struct_en_sub_name = col_en_name_list[1];
                    std::string struct_cn_name = col_cn_name_list[0];
                    uint32_t struct_num = NFCommon::strto<uint32_t>(col_cn_name_list[1]);
                    std::string struct_cn_sub_name = col_cn_name_list[2];

                    int iRet = HandleNewColMsg(pSheet, NULL, col_index, col_type, {struct_en_name, struct_en_sub_name}, {struct_cn_name, struct_cn_sub_name}, {(int)struct_num, -1});
                    CHECK_EXPR(iRet == 0, -1, "excel:{} sheet:{} col_en_name:{} col_cn_name:{} is not right", m_excel,
                               pSheet->m_sheetName, col_en_name, col_cn_name);
                }
                else if (col_en_name_list[0].size() > 0 && col_en_name_list[1].size() > 0
                    && col_cn_name_list[0].size() == 0 && col_cn_name_list[1].size() > 0 && col_cn_name_list[2].size() > 0)
                {
                    std::string struct_en_name = col_en_name_list[0];
                    std::string struct_en_sub_name = col_en_name_list[1];
                    std::string struct_cn_name = col_cn_name_list[0];
                    uint32_t struct_num = NFCommon::strto<uint32_t>(col_cn_name_list[1]);
                    std::string struct_cn_sub_name = col_cn_name_list[2];

                    int iRet = HandleNewColMsg(pSheet, NULL, col_index, col_type, {struct_en_name, struct_en_sub_name}, {struct_cn_name, struct_cn_sub_name}, {(int)struct_num, -1});
                    CHECK_EXPR(iRet == 0, -1, "excel:{} sheet:{} col_en_name:{} col_cn_name:{} is not right", m_excel,
                               pSheet->m_sheetName, col_en_name, col_cn_name);
                }
                /** 处理数组特殊情况
                 * @brief item_id item_num item_id item_num
                 *        物品1    物品1Num     物品2    物品2Num
                 */
                /** 处理数组特殊情况
                 * @brief item_id item_num item_id item_num
                 *        物品1    物x1     物品2    物x2
                 */
                else if (col_en_name_list[0].size() > 0 && col_en_name_list[1].size() > 0
                    && col_cn_name_list[0].size() > 0 && col_cn_name_list[1].size() > 0 && col_cn_name_list[2].size() == 0)
                {
                    bool has_diff = false;

                    for (int col_index_temp = SHEET_START_COL; col_index_temp <= cols; col_index_temp++)
                    {
                        if (col_index_temp == col_index) continue;

                        std::string col_en_name_temp;
                        std::string col_cn_name_temp;
                        std::string col_type_temp;
                        std::string col_sel_temp_str;
                        if (col_index_temp - 1 < (int)enNameRow.size())
                        {
                            col_en_name_temp = GetSheetValue(enNameRow[col_index_temp - 1]);
                        }
                        if (col_index_temp - 1 < (int)chNameRow.size())
                        {
                            col_cn_name_temp = GetSheetValue(chNameRow[col_index_temp - 1]);
                        }
                        if (col_index_temp - 1 < (int)typeNameRow.size())
                        {
                            col_type_temp = GetSheetValue(typeNameRow[col_index_temp - 1]);
                        }
                        if (col_index_temp - 1 < (int)selNameRow.size())
                        {
                            col_sel_temp_str = GetSheetValue(selNameRow[col_index_temp - 1]);
                        }

                        NFStringUtility::Trim(col_en_name_temp);
                        NFStringUtility::Trim(col_cn_name_temp);
                        NFStringUtility::Trim(col_type_temp);
                        NFStringUtility::Trim(col_sel_temp_str);

                        int col_temp_sel = NFCommon::strto<int>(col_sel_temp_str);
                        if (col_temp_sel != 2 && col_temp_sel != 3)
                            continue;

                        std::vector<std::string> col_en_name_list_temp;
                        NFStringUtility::Split(col_en_name_temp, "_", &col_en_name_list_temp);
                        for (int i = 0; i < (int)col_en_name_list_temp.size(); i++)
                        {
                            NFStringUtility::Trim(col_en_name_list_temp[i]);
                        }

                        std::vector<std::string> col_cn_name_list_temp;
                        NFStringUtility::SplitDigit(col_cn_name_temp, &col_cn_name_list_temp);
                        for (int i = 0; i < (int)col_cn_name_list_temp.size(); i++)
                        {
                            NFStringUtility::Trim(col_cn_name_list_temp[i]);
                        }

                        /** 处理数组特殊情况
                         * @brief item_id item_num item_id item_num
                         *        物品1    物品1Num     物品2    物品2Num
                         */
                        if (col_en_name_list_temp.size() == 2 && col_en_name_temp != col_en_name && col_en_name_list_temp[0] == col_en_name_list[0]
                            && col_cn_name_list_temp.size() == 3 && col_cn_name_list_temp[1].size() > 0)
                        {
                            has_diff = true;
                            break;
                        }
                    }

                    /** 处理数组特殊情况
                     * @brief item_id item_num item_id item_num
                     *        物品1    物品1Num     物品2    物品2Num
                     */
                    if (has_diff)
                    {
                        std::string struct_en_name = col_en_name_list[0];
                        std::string struct_en_sub_name = col_en_name_list[1];
                        std::string struct_cn_name = col_cn_name_list[0];
                        uint32_t struct_num = NFCommon::strto<uint32_t>(col_cn_name_list[1]);
                        std::string struct_cn_sub_name = col_cn_name_list[2];

                        int iRet = HandleNewColMsg(pSheet, NULL, col_index, col_type, {struct_en_name, struct_en_sub_name}, {struct_cn_name, struct_cn_sub_name}, {(int)struct_num, -1});
                        CHECK_EXPR(iRet == 0, -1, "excel:{} sheet:{} col_en_name:{} col_cn_name:{} is not right", m_excel,
                                   pSheet->m_sheetName, col_en_name, col_cn_name);
                    }
                    /** 其余情况
                     * @brief item_id itemnum
                     *        物品1    物x1
                     */
                    else
                    {
                        int struct_num = NFCommon::strto<int>(col_cn_name_list[1]);
                        std::string struct_en_name = col_en_name;
                        std::string struct_cn_name = col_cn_name_list[0];
                        int iRet = HandleNewColMsg(pSheet, NULL, col_index, col_type, {struct_en_name}, {struct_cn_name}, {(int)struct_num});
                        CHECK_EXPR(iRet == 0, -1, "excel:{} sheet:{} col_en_name:{} col_cn_name:{} is not right", m_excel,
                                   pSheet->m_sheetName, col_en_name, col_cn_name);
                    }
                }
                else if (col_en_name_list[0].size() > 0 && col_en_name_list[1].size() > 0
                    && col_cn_name_list[0].size() == 0 && col_cn_name_list[1].size() > 0 && col_cn_name_list[2].size() == 0)
                {
                    bool has_diff = false;

                    for (int col_index_temp = SHEET_START_COL; col_index_temp <= cols; col_index_temp++)
                    {
                        if (col_index_temp == col_index) continue;

                        std::string col_en_name_temp;
                        std::string col_cn_name_temp;
                        std::string col_type_temp;
                        std::string col_sel_temp_str;
                        if (col_index_temp - 1 < (int)enNameRow.size())
                        {
                            col_en_name_temp = GetSheetValue(enNameRow[col_index_temp - 1]);
                        }
                        if (col_index_temp - 1 < (int)chNameRow.size())
                        {
                            col_cn_name_temp = GetSheetValue(chNameRow[col_index_temp - 1]);
                        }
                        if (col_index_temp - 1 < (int)typeNameRow.size())
                        {
                            col_type_temp = GetSheetValue(typeNameRow[col_index_temp - 1]);
                        }
                        if (col_index_temp - 1 < (int)selNameRow.size())
                        {
                            col_sel_temp_str = GetSheetValue(selNameRow[col_index_temp - 1]);
                        }

                        NFStringUtility::Trim(col_en_name_temp);
                        NFStringUtility::Trim(col_cn_name_temp);
                        NFStringUtility::Trim(col_type_temp);
                        NFStringUtility::Trim(col_sel_temp_str);

                        int col_temp_sel = NFCommon::strto<int>(col_sel_temp_str);
                        if (col_temp_sel != 2 && col_temp_sel != 3)
                            continue;

                        std::vector<std::string> col_en_name_list_temp;
                        NFStringUtility::Split(col_en_name_temp, "_", &col_en_name_list_temp);
                        for (int i = 0; i < (int)col_en_name_list_temp.size(); i++)
                        {
                            NFStringUtility::Trim(col_en_name_list_temp[i]);
                        }

                        std::vector<std::string> col_cn_name_list_temp;
                        NFStringUtility::SplitDigit(col_cn_name_temp, &col_cn_name_list_temp);
                        for (int i = 0; i < (int)col_cn_name_list_temp.size(); i++)
                        {
                            NFStringUtility::Trim(col_cn_name_list_temp[i]);
                        }

                        /** 处理数组特殊情况
                         * @brief item_id item_num item_id item_num
                         *        物品1    物品1Num     物品2    物品2Num
                         */
                        if (col_en_name_list_temp.size() == 2 && col_en_name_temp != col_en_name && col_en_name_list_temp[0] == col_en_name_list[0]
                            && col_cn_name_list_temp.size() == 3 && col_cn_name_list_temp[1].size() > 0)
                        {
                            has_diff = true;
                            break;
                        }
                    }

                    /** 处理数组特殊情况
                     * @brief item_id item_num item_id item_num
                     *        1    物品1Num     2    物品2Num
                     */
                    if (has_diff)
                    {
                        std::string struct_en_name = col_en_name_list[0];
                        std::string struct_en_sub_name = col_en_name_list[1];
                        std::string struct_cn_name = col_en_name_list[0];
                        uint32_t struct_num = NFCommon::strto<uint32_t>(col_cn_name_list[1]);
                        std::string struct_cn_sub_name = col_en_name_list[2];

                        int iRet = HandleNewColMsg(pSheet, NULL, col_index, col_type, {struct_en_name, struct_en_sub_name}, {struct_cn_name, struct_cn_sub_name}, {(int)struct_num, -1});
                        CHECK_EXPR(iRet == 0, -1, "excel:{} sheet:{} col_en_name:{} col_cn_name:{} is not right", m_excel,
                                   pSheet->m_sheetName, col_en_name, col_cn_name);
                    }
                    /** 其余情况
                     * @brief item_id itemnum
                     *        物品1    物x1
                     */
                    else
                    {
                        int struct_num = NFCommon::strto<int>(col_cn_name_list[1]);
                        std::string struct_en_name = col_en_name;
                        std::string struct_cn_name = col_en_name;
                        int iRet = HandleNewColMsg(pSheet, NULL, col_index, col_type, {struct_en_name}, {struct_cn_name}, {(int)struct_num});
                        CHECK_EXPR(iRet == 0, -1, "excel:{} sheet:{} col_en_name:{} col_cn_name:{} is not right", m_excel,
                                   pSheet->m_sheetName, col_en_name, col_cn_name);
                    }
                }
                else
                {
                    NFLogError(NF_LOG_DEFAULT, 0, "excel:{} sheet:{} sub_name:{} cn_name:{} is not right", m_excel, pSheet->m_sheetName, col_en_name,
                               col_cn_name);
                    return -1;
                }
            }
            else if (col_en_name_list.size() == 1 && col_cn_name_list.size() == 3)
            {
                /** 处理特殊情况
                 * @brief itemid itemid
                 *        物品1   物品2
                 */
                if (col_en_name_list[0].size() > 0 && col_cn_name_list[0].size() > 0 && col_cn_name_list[1].size() > 0 && col_cn_name_list[2].size() > 0)
                {
                    bool has_diff = false;
                    for (int col_index_temp = SHEET_START_COL; col_index_temp <= cols; col_index_temp++)
                    {
                        if (col_index_temp == col_index) continue;

                        std::string col_en_name_temp;
                        std::string col_cn_name_temp;
                        std::string col_type_temp;
                        std::string col_sel_temp_str;
                        if (col_index_temp - 1 < (int)enNameRow.size())
                        {
                            col_en_name_temp = GetSheetValue(enNameRow[col_index_temp - 1]);
                        }
                        if (col_index_temp - 1 < (int)chNameRow.size())
                        {
                            col_cn_name_temp = GetSheetValue(chNameRow[col_index_temp - 1]);
                        }
                        if (col_index_temp - 1 < (int)typeNameRow.size())
                        {
                            col_type_temp = GetSheetValue(typeNameRow[col_index_temp - 1]);
                        }
                        if (col_index_temp - 1 < (int)selNameRow.size())
                        {
                            col_sel_temp_str = GetSheetValue(selNameRow[col_index_temp - 1]);
                        }

                        NFStringUtility::Trim(col_en_name_temp);
                        NFStringUtility::Trim(col_cn_name_temp);
                        NFStringUtility::Trim(col_type_temp);
                        NFStringUtility::Trim(col_sel_temp_str);

                        int col_temp_sel = NFCommon::strto<int>(col_sel_temp_str);
                        if (col_temp_sel != 2 && col_temp_sel != 3)
                            continue;

                        std::vector<std::string> col_en_name_list_temp;
                        NFStringUtility::Split(col_en_name_temp, "_", &col_en_name_list_temp);
                        for (int i = 0; i < (int)col_en_name_list_temp.size(); i++)
                        {
                            NFStringUtility::Trim(col_en_name_list_temp[i]);
                        }

                        std::vector<std::string> col_cn_name_list_temp;
                        NFStringUtility::SplitDigit(col_cn_name_temp, &col_cn_name_list_temp);
                        for (int i = 0; i < (int)col_cn_name_list_temp.size(); i++)
                        {
                            NFStringUtility::Trim(col_cn_name_list_temp[i]);
                        }

                        /** 处理数组特殊情况
                         * @brief item item_num item    item_num
                         *        物品1 物品1Num  物品2    物品2Num
                         */
                        /** 处理数组特殊情况
                         * @brief item item_num item    item_num
                         *        物品1 物x1Num  物品2    物品2Num
                         */
                        /** 处理数组特殊情况
                         * @brief item item_num item item_num
                         *        物品1    物x1     物品2    物x2
                         */
                        if (col_en_name_list_temp.size() == 2 && col_en_name_temp != col_en_name && col_en_name_list_temp[0] == col_en_name_list[0]
                            && col_cn_name_list_temp.size() == 3 && col_cn_name_list_temp[1].size() > 0)
                        {
                            has_diff = true;
                            break;
                        }
                    }

                    /** 处理数组特殊情况
                     * @brief item item_num item    item_num
                     *        物品1 物品1Num  物品2    物品2Num
                     */
                    if (has_diff)
                    {
                        std::string struct_en_name = col_en_name_list[0];
                        std::string struct_en_sub_name = col_en_name_list[0];
                        std::string struct_cn_name = col_cn_name_list[0];
                        uint32_t struct_num = NFCommon::strto<uint32_t>(col_cn_name_list[1]);
                        std::string struct_cn_sub_name = col_cn_name_list[2];

                        int iRet = HandleNewColMsg(pSheet, NULL, col_index, col_type, {struct_en_name, struct_en_sub_name}, {struct_cn_name, struct_cn_sub_name}, {(int)struct_num, -1});
                        CHECK_EXPR(iRet == 0, -1, "excel:{} sheet:{} col_en_name:{} col_cn_name:{} is not right", m_excel,
                                   pSheet->m_sheetName, col_en_name, col_cn_name);
                    }
                    else
                    {
                        std::string struct_en_name = col_en_name;
                        std::string struct_cn_name = col_cn_name_list[0] + col_cn_name_list[2];
                        int struct_num = NFCommon::strto<int>(col_cn_name_list[1]);
                        int iRet = HandleNewColMsg(pSheet, NULL, col_index, col_type, {struct_en_name}, {struct_cn_name}, {(int)struct_num});
                        CHECK_EXPR(iRet == 0, -1, "excel:{} sheet:{} col_en_name:{} col_cn_name:{} is not right", m_excel,
                                   pSheet->m_sheetName, col_en_name, col_cn_name);
                    }
                }
                /** 处理特殊情况
                 * @brief itemid itemid
                 *        物品1ID 物品2ID
                 */
                else if (col_en_name_list[0].size() > 0
                    && col_cn_name_list[0].size() == 0 && col_cn_name_list[1].size() > 0 && col_cn_name_list[2].size() > 0)
                {
                    bool has_diff = false;
                    for (int col_index_temp = SHEET_START_COL; col_index_temp <= cols; col_index_temp++)
                    {
                        if (col_index_temp == col_index) continue;

                        std::string col_en_name_temp;
                        std::string col_cn_name_temp;
                        std::string col_type_temp;
                        std::string col_sel_temp_str;
                        if (col_index_temp - 1 < (int)enNameRow.size())
                        {
                            col_en_name_temp = GetSheetValue(enNameRow[col_index_temp - 1]);
                        }
                        if (col_index_temp - 1 < (int)chNameRow.size())
                        {
                            col_cn_name_temp = GetSheetValue(chNameRow[col_index_temp - 1]);
                        }
                        if (col_index_temp - 1 < (int)typeNameRow.size())
                        {
                            col_type_temp = GetSheetValue(typeNameRow[col_index_temp - 1]);
                        }
                        if (col_index_temp - 1 < (int)selNameRow.size())
                        {
                            col_sel_temp_str = GetSheetValue(selNameRow[col_index_temp - 1]);
                        }

                        NFStringUtility::Trim(col_en_name_temp);
                        NFStringUtility::Trim(col_cn_name_temp);
                        NFStringUtility::Trim(col_type_temp);
                        NFStringUtility::Trim(col_sel_temp_str);

                        int col_temp_sel = NFCommon::strto<int>(col_sel_temp_str);
                        if (col_temp_sel != 2 && col_temp_sel != 3)
                            continue;

                        std::vector<std::string> col_en_name_list_temp;
                        NFStringUtility::Split(col_en_name_temp, "_", &col_en_name_list_temp);
                        for (int i = 0; i < (int)col_en_name_list_temp.size(); i++)
                        {
                            NFStringUtility::Trim(col_en_name_list_temp[i]);
                        }

                        std::vector<std::string> col_cn_name_list_temp;
                        NFStringUtility::SplitDigit(col_cn_name_temp, &col_cn_name_list_temp);
                        for (int i = 0; i < (int)col_cn_name_list_temp.size(); i++)
                        {
                            NFStringUtility::Trim(col_cn_name_list_temp[i]);
                        }

                        /** 处理数组特殊情况
                         * @brief item item_num item item_num
                         *        物品1ID    物品1Num     物品2ID    物品2Num
                         */
                        if (col_en_name_list_temp.size() == 2 && col_en_name_temp != col_en_name && col_en_name_list_temp[0] == col_en_name_list[0]
                            && col_cn_name_list_temp.size() == 3 && col_cn_name_list_temp[1].size() > 0)
                        {
                            has_diff = true;
                            break;
                        }
                    }

                    /** 处理数组特殊情况
                     * @brief item item_num item item_num
                     *        物品1ID    物品1Num     物品2ID    物品2Num
                     */
                    if (has_diff)
                    {
                        std::string struct_en_name = col_en_name_list[0];
                        std::string struct_en_sub_name = col_en_name_list[0];
                        std::string struct_cn_name = col_cn_name_list[0];
                        uint32_t struct_num = NFCommon::strto<uint32_t>(col_cn_name_list[1]);
                        std::string struct_cn_sub_name = col_cn_name_list[2];

                        int iRet = HandleNewColMsg(pSheet, NULL, col_index, col_type, {struct_en_name, struct_en_sub_name}, {struct_cn_name, struct_cn_sub_name}, {(int)struct_num, -1});
                        CHECK_EXPR(iRet == 0, -1, "excel:{} sheet:{} col_en_name:{} col_cn_name:{} is not right", m_excel,
                                   pSheet->m_sheetName, col_en_name, col_cn_name);
                    }
                    else
                    {
                        std::string struct_en_name = col_en_name_list[0];
                        std::string struct_cn_name = col_cn_name_list[0] + col_cn_name_list[2];
                        int struct_num = NFCommon::strto<int>(col_cn_name_list[1]);
                        int iRet = HandleNewColMsg(pSheet, NULL, col_index, col_type, {struct_en_name}, {struct_cn_name}, {(int)struct_num});
                        CHECK_EXPR(iRet == 0, -1, "excel:{} sheet:{} col_en_name:{} col_cn_name:{} is not right", m_excel,
                                   pSheet->m_sheetName, col_en_name, col_cn_name);
                    }
                }
                else if (col_en_name_list[0].size() > 0
                    && col_cn_name_list[0].size() > 0 && col_cn_name_list[1].size() > 0 && col_cn_name_list[2].size() == 0)
                {
                    bool has_diff = false;
                    for (int col_index_temp = SHEET_START_COL; col_index_temp <= cols; col_index_temp++)
                    {
                        if (col_index_temp == col_index) continue;

                        std::string col_en_name_temp;
                        std::string col_cn_name_temp;
                        std::string col_type_temp;
                        std::string col_sel_temp_str;
                        if (col_index_temp - 1 < (int)enNameRow.size())
                        {
                            col_en_name_temp = GetSheetValue(enNameRow[col_index_temp - 1]);
                        }
                        if (col_index_temp - 1 < (int)chNameRow.size())
                        {
                            col_cn_name_temp = GetSheetValue(chNameRow[col_index_temp - 1]);
                        }
                        if (col_index_temp - 1 < (int)typeNameRow.size())
                        {
                            col_type_temp = GetSheetValue(typeNameRow[col_index_temp - 1]);
                        }
                        if (col_index_temp - 1 < (int)selNameRow.size())
                        {
                            col_sel_temp_str = GetSheetValue(selNameRow[col_index_temp - 1]);
                        }

                        NFStringUtility::Trim(col_en_name_temp);
                        NFStringUtility::Trim(col_cn_name_temp);
                        NFStringUtility::Trim(col_type_temp);
                        NFStringUtility::Trim(col_sel_temp_str);

                        int col_temp_sel = NFCommon::strto<int>(col_sel_temp_str);
                        if (col_temp_sel != 2 && col_temp_sel != 3)
                            continue;

                        std::vector<std::string> col_en_name_list_temp;
                        NFStringUtility::Split(col_en_name_temp, "_", &col_en_name_list_temp);
                        for (int i = 0; i < (int)col_en_name_list_temp.size(); i++)
                        {
                            NFStringUtility::Trim(col_en_name_list_temp[i]);
                        }

                        std::vector<std::string> col_cn_name_list_temp;
                        NFStringUtility::SplitDigit(col_cn_name_temp, &col_cn_name_list_temp);
                        for (int i = 0; i < (int)col_cn_name_list_temp.size(); i++)
                        {
                            NFStringUtility::Trim(col_cn_name_list_temp[i]);
                        }

                        /** 处理数组特殊情况
                         * @brief item item_num item item_num
                         *        1ID    物品1Num     2ID    物品2Num
                         */
                        if (col_en_name_list_temp.size() == 2 && col_en_name_temp != col_en_name && col_en_name_list_temp[0] == col_en_name_list[0]
                            && col_cn_name_list_temp.size() == 3 && col_cn_name_list_temp[1].size() > 0)
                        {
                            has_diff = true;
                            break;
                        }
                    }

                    /** 处理数组特殊情况
                     * @brief item item_num item item_num
                     *        物品1ID    物品1Num     物品2ID    物品2Num
                     */
                    if (has_diff)
                    {
                        std::string struct_en_name = col_en_name_list[0];
                        std::string struct_en_sub_name = col_en_name_list[0];
                        std::string struct_cn_name = col_cn_name_list[0];
                        uint32_t struct_num = NFCommon::strto<uint32_t>(col_cn_name_list[1]);
                        std::string struct_cn_sub_name = col_cn_name_list[2];

                        int iRet = HandleNewColMsg(pSheet, NULL, col_index, col_type, {struct_en_name, struct_en_sub_name}, {struct_cn_name, struct_cn_sub_name}, {(int)struct_num, -1});
                        CHECK_EXPR(iRet == 0, -1, "excel:{} sheet:{} col_en_name:{} col_cn_name:{} is not right", m_excel,
                                   pSheet->m_sheetName, col_en_name, col_cn_name);
                    }
                    else
                    {
                        std::string struct_en_name = col_en_name_list[0];
                        std::string struct_cn_name = col_cn_name_list[0] + col_cn_name_list[2];
                        int struct_num = NFCommon::strto<int>(col_cn_name_list[1]);
                        int iRet = HandleNewColMsg(pSheet, NULL, col_index, col_type, {struct_en_name}, {struct_cn_name}, {(int)struct_num});
                        CHECK_EXPR(iRet == 0, -1, "excel:{} sheet:{} col_en_name:{} col_cn_name:{} is not right", m_excel,
                                   pSheet->m_sheetName, col_en_name, col_cn_name);
                    }
                }
                else if (col_en_name_list[0].size() > 0
                    && col_cn_name_list[0].size() == 0 && col_cn_name_list[1].size() > 0 && col_cn_name_list[2].size() == 0)
                {
                    bool has_diff = false;
                    for (int col_index_temp = SHEET_START_COL; col_index_temp <= cols; col_index_temp++)
                    {
                        if (col_index_temp == col_index) continue;

                        std::string col_en_name_temp;
                        std::string col_cn_name_temp;
                        std::string col_type_temp;
                        std::string col_sel_temp_str;
                        if (col_index_temp - 1 < (int)enNameRow.size())
                        {
                            col_en_name_temp = GetSheetValue(enNameRow[col_index_temp - 1]);
                        }
                        if (col_index_temp - 1 < (int)chNameRow.size())
                        {
                            col_cn_name_temp = GetSheetValue(chNameRow[col_index_temp - 1]);
                        }
                        if (col_index_temp - 1 < (int)typeNameRow.size())
                        {
                            col_type_temp = GetSheetValue(typeNameRow[col_index_temp - 1]);
                        }
                        if (col_index_temp - 1 < (int)selNameRow.size())
                        {
                            col_sel_temp_str = GetSheetValue(selNameRow[col_index_temp - 1]);
                        }

                        NFStringUtility::Trim(col_en_name_temp);
                        NFStringUtility::Trim(col_cn_name_temp);
                        NFStringUtility::Trim(col_type_temp);
                        NFStringUtility::Trim(col_sel_temp_str);

                        int col_temp_sel = NFCommon::strto<int>(col_sel_temp_str);
                        if (col_temp_sel != 2 && col_temp_sel != 3)
                            continue;

                        std::vector<std::string> col_en_name_list_temp;
                        NFStringUtility::Split(col_en_name_temp, "_", &col_en_name_list_temp);
                        for (int i = 0; i < (int)col_en_name_list_temp.size(); i++)
                        {
                            NFStringUtility::Trim(col_en_name_list_temp[i]);
                        }

                        std::vector<std::string> col_cn_name_list_temp;
                        NFStringUtility::SplitDigit(col_cn_name_temp, &col_cn_name_list_temp);
                        for (int i = 0; i < (int)col_cn_name_list_temp.size(); i++)
                        {
                            NFStringUtility::Trim(col_cn_name_list_temp[i]);
                        }

                        /** 处理数组特殊情况
                         * @brief item item_num item item_num
                         *        1ID    物品1Num     2ID    物品2Num
                         */
                        if (col_en_name_list_temp.size() == 2 && col_en_name_temp != col_en_name && col_en_name_list_temp[0] == col_en_name_list[0]
                            && col_cn_name_list_temp.size() == 3 && col_cn_name_list_temp[1].size() > 0)
                        {
                            has_diff = true;
                            break;
                        }
                    }

                    /** 处理数组特殊情况
                     * @brief item item_num item item_num
                     *        1    1        2    2
                     */
                    if (has_diff)
                    {
                        std::string struct_en_name = col_en_name_list[0];
                        std::string struct_en_sub_name = col_en_name_list[0];
                        std::string struct_cn_name = col_en_name_list[0];
                        uint32_t struct_num = NFCommon::strto<uint32_t>(col_cn_name_list[1]);
                        std::string struct_cn_sub_name = col_en_name_list[2];

                        int iRet = HandleNewColMsg(pSheet, NULL, col_index, col_type, {struct_en_name, struct_en_sub_name}, {struct_cn_name, struct_cn_sub_name}, {(int)struct_num, -1});
                        CHECK_EXPR(iRet == 0, -1, "excel:{} sheet:{} col_en_name:{} col_cn_name:{} is not right", m_excel,
                                   pSheet->m_sheetName, col_en_name, col_cn_name);
                    }
                    else
                    {
                        std::string struct_en_name = col_en_name_list[0];
                        std::string struct_cn_name = col_cn_name_list[0] + col_cn_name_list[2];
                        int struct_num = NFCommon::strto<int>(col_cn_name_list[1]);
                        int iRet = HandleNewColMsg(pSheet, NULL, col_index, col_type, {struct_en_name}, {struct_cn_name}, {(int)struct_num});
                        CHECK_EXPR(iRet == 0, -1, "excel:{} sheet:{} col_en_name:{} col_cn_name:{} is not right", m_excel,
                                   pSheet->m_sheetName, col_en_name, col_cn_name);
                    }
                }
                else
                {
                    NFLogError(NF_LOG_DEFAULT, 0, "excel:{} sheet:{} sub_name:{} cn_name:{} is not right", m_excel, pSheet->m_sheetName, col_en_name,
                               col_cn_name);
                    return -1;
                }
            }
            else if (col_en_name_list.size() >= 2 && col_cn_name_list.size() >= 3)
            {
                std::vector<std::string> vecEnName = col_en_name_list;
                std::vector<std::string> vecCnName;
                std::vector<int> vecStructName;
                for (int i = 0; i < (int)col_cn_name_list.size(); i++)
                {
                    std::string temp = col_cn_name_list[i];
                    if (NFStringUtility::IsDigital(temp))
                    {
                        vecStructName.push_back(NFCommon::strto<int>(temp));
                    }
                    else
                    {
                        if (!temp.empty())
                        {
                            vecCnName.push_back(temp);
                        }
                    }
                }

                if (vecStructName.size() < vecCnName.size())
                {
                    vecStructName.push_back(-1);
                }

                if (vecEnName.size() != vecCnName.size() && vecCnName.size() == 1 && vecStructName.size() == 1)
                {
                    std::string struct_en_name = col_en_name;
                    std::string struct_cn_name = vecCnName[0];
                    int struct_num = vecStructName[0];
                    int iRet = HandleNewColMsg(pSheet, NULL, col_index, col_type, {struct_en_name}, {struct_cn_name}, {(int)struct_num});
                    CHECK_EXPR(iRet == 0, -1, "excel:{} sheet:{} col_en_name:{} col_cn_name:{} is not right", m_excel,
                               pSheet->m_sheetName, col_en_name, col_cn_name);
                }
                else
                {
                    int iRet = HandleNewColMsg(pSheet, NULL, col_index, col_type, vecEnName, vecCnName, vecStructName);
                    CHECK_EXPR(iRet == 0, -1, "excel:{} sheet:{} col_en_name:{} col_cn_name:{} is not right", m_excel,
                               pSheet->m_sheetName, col_en_name, col_cn_name);
                }
            }
            else
            {
                std::string struct_en_name = col_en_name;
                std::string struct_cn_name = col_cn_name;
                int struct_num = -1;
                int iRet = HandleNewColMsg(pSheet, NULL, col_index, col_type, {struct_en_name}, {struct_cn_name}, {struct_num});
                CHECK_EXPR(iRet == 0, -1, "excel:{} sheet:{} col_en_name:{} col_cn_name:{} is not right", m_excel,
                           pSheet->m_sheetName, col_en_name, col_cn_name);
            }
        }
    }

    CHECK_EXPR(pSheet->m_protoInfo.m_startCol > 0 && pSheet->m_protoInfo.m_endCol > 0 && pSheet->m_protoInfo.m_endCol >= pSheet->m_protoInfo.m_startCol, 0,
               "sheet:{} start_col:{} m_endCol:{}", sheet.name(), pSheet->m_protoInfo.m_startCol, pSheet->m_protoInfo.m_endCol);

    for (auto& row_vec : sheet.rows())
    {
        int row = row_vec.rowNumber();
        if (row < SHEET_CONTENT_START_ROW || row > rows)
        {
            continue;
        }

        bool flag = true;
        std::vector<XLCellValue> readValues = row_vec.values();
        for (int col = pSheet->m_protoInfo.m_startCol; col <= (int)pSheet->m_protoInfo.m_endCol; col++)
        {
            std::string value;
            if (col - 1 < (int)readValues.size())
            {
                value = GetSheetValue(readValues[col - 1]);
                NFStringUtility::Trim(value);
            }
            if (!value.empty())
            {
                flag = false;
                break;
            }
        }

        if (flag)
        {
            break;
        }
        pSheet->m_rows++;
    }
    return 0;
}


int ExcelParse::HandleNewColMsg(ExcelSheet* pSheet, ExcelSheetColInfo* pLastColInfo, int col_index, const std::string& col_type, const std::vector<std::string>& vecEnName, const std::vector<std::string>& vecCnName, const std::vector<int>& vecStructNum)
{
    if (vecEnName.size() == 0) return 0;

    CHECK_EXPR(vecEnName.size() > 0 && vecEnName.size() == vecCnName.size() && vecEnName.size() == vecStructNum.size(), -1, "");

    std::string struct_en_name = vecEnName[0];
    std::string struct_cn_name = vecCnName[0];
    int struct_num = vecStructNum[0];

    ExcelSheetColInfo* pColInfo = NULL;
    if (pLastColInfo == NULL)
    {
        if (pSheet->m_colInfoMap.find(struct_en_name) == pSheet->m_colInfoMap.end())
        {
            pColInfo = new ExcelSheetColInfo();
            pColInfo->m_colIndex = col_index;
            pSheet->m_colInfoVec.emplace(col_index, pColInfo);
            pSheet->m_colInfoMap.emplace(struct_en_name, pColInfo);
            pColInfo->m_structEnName = struct_en_name;
            pColInfo->m_structCnName = struct_cn_name;
            pColInfo->m_pbDescTempName = NFStringUtility::FirstUpper(pSheet->m_otherName) + NFStringUtility::FirstUpper(GetPbName(struct_en_name));
            pColInfo->m_pbDescName = "E_" + pColInfo->m_pbDescTempName;
            pColInfo->m_colFullName = struct_en_name;
            if (pSheet->m_colMessageTypeMap.find(pColInfo->m_colFullName) != pSheet->m_colMessageTypeMap.end())
            {
                pColInfo->m_pbDescName = pSheet->m_colMessageTypeMap[pColInfo->m_colFullName].m_messagePbName;
            }
            pColInfo->m_colType = col_type;
            pColInfo->m_colCppType = GetColType(col_type, &pColInfo->m_strParseRepeatedNum, &pColInfo->m_strParseMessage, &pColInfo->m_fieldParseType);
            CHECK_EXPR(pColInfo->m_colCppType != NF_ENUM_COL_TYPE::NF_ENUM_COL_TYPE_UNKNOWN, -1, "sheet:{} add col info, col:{} en_name:{} cn_name:{}, but col_type:{} is not support",
                       pSheet->m_sheetName, col_index,
                       pColInfo->m_structEnName, pColInfo->m_structCnName, pColInfo->m_colType);

            pColInfo->m_maxSubNum = struct_num;
            if (struct_num > 0)
            {
                pColInfo->m_strutNumFromZero = false;
            }
            pColInfo->m_colTypeStrMaxSize = 64;

            HandleColTypeString(pColInfo->m_colType, pColInfo->m_colTypeStrMaxSize);

            if (struct_num >= 0)
            {
                pColInfo->m_isArray = true;
                CHECK_EXPR(struct_num == 0 || struct_num == 1, -1, "sheet:{} add col info, col:{} en_name:{} cn_name:{} col_type:{}, but the first struct_num is not 1",
                           pSheet->m_sheetName, pColInfo->m_colIndex,
                           pColInfo->m_structEnName, pColInfo->m_structCnName, pColInfo->m_colType);
            }

            NFLogInfo(NF_LOG_DEFAULT, 0, "sheet:{} add col info, col:{} en_name:{} cn_name:{} col_type:{}", pSheet->m_sheetName, col_index,
                      pColInfo->m_structEnName, pColInfo->m_structCnName, pColInfo->m_colType);
        }
        else
        {
            pColInfo = pSheet->m_colInfoMap[struct_en_name];
        }

        CHECK_EXPR(pSheet->m_allColInfoList.find(col_index) == pSheet->m_allColInfoList.end(), -1, "sheet:{} col:{} repeated parse", pSheet->m_sheetName,
                   col_index);
        ExcelSheetColIndex& sheetColIndex = pSheet->m_allColInfoList[col_index];
        sheetColIndex.m_colIndex = col_index;
        sheetColIndex.m_pColInfo = pColInfo;
        sheetColIndex.m_fullEnName = struct_en_name;
        sheetColIndex.m_colEnName = struct_en_name;
        if (struct_num >= 0)
        {
            if (pColInfo->m_strutNumFromZero)
            {
                sheetColIndex.m_fullEnName += "_" + NFCommon::tostr(struct_num);
            }
            else
            {
                sheetColIndex.m_fullEnName += "_" + NFCommon::tostr(struct_num - 1);
            }
        }
    }
    else
    {
        if (pLastColInfo->m_colInfoMap.find(struct_en_name) == pLastColInfo->m_colInfoMap.end())
        {
            pColInfo = new ExcelSheetColInfo();
            pColInfo->m_colIndex = col_index;
            pLastColInfo->m_colInfoMap.emplace(struct_en_name, pColInfo);
            pLastColInfo->m_colInfoList.emplace(col_index, pColInfo);
            pColInfo->m_structEnName = struct_en_name;
            pColInfo->m_structCnName = struct_cn_name;
            pColInfo->m_pbDescTempName = pLastColInfo->m_pbDescTempName + NFStringUtility::FirstUpper(GetPbName(struct_en_name));
            pColInfo->m_pbDescName = "E_" + pColInfo->m_pbDescTempName;

            pColInfo->m_colFullName = pLastColInfo->m_colFullName + "_" + struct_en_name;
            if (pSheet->m_colMessageTypeMap.find(pColInfo->m_colFullName) != pSheet->m_colMessageTypeMap.end())
            {
                pColInfo->m_pbDescName = pSheet->m_colMessageTypeMap[pColInfo->m_colFullName].m_messagePbName;
            }
            pColInfo->m_colType = col_type;
            pColInfo->m_colCppType = GetColType(col_type, &pColInfo->m_strParseRepeatedNum, &pColInfo->m_strParseMessage, &pColInfo->m_fieldParseType);
            CHECK_EXPR(pColInfo->m_colCppType != NF_ENUM_COL_TYPE::NF_ENUM_COL_TYPE_UNKNOWN, -1, "sheet:{} add col info, col:{} en_name:{} cn_name:{}, but col_type:{} is not support",
                       pSheet->m_sheetName, col_index,
                       pColInfo->m_structEnName, pColInfo->m_structCnName, pColInfo->m_colType);
            pColInfo->m_maxSubNum = struct_num;
            if (struct_num > 0)
            {
                pColInfo->m_strutNumFromZero = false;
            }
            pColInfo->m_colTypeStrMaxSize = 64;
            HandleColTypeString(pColInfo->m_colType, pColInfo->m_colTypeStrMaxSize);

            if (struct_num >= 0)
            {
                pColInfo->m_isArray = true;
                CHECK_EXPR(struct_num == 0 || struct_num == 1, -1, "sheet:{} add col info, col:{} en_name:{} cn_name:{} col_type:{}, but the first struct_num is not 1",
                           pSheet->m_sheetName, pColInfo->m_colIndex,
                           pColInfo->m_structEnName, pColInfo->m_structCnName, pColInfo->m_colType);
            }

            NFLogInfo(NF_LOG_DEFAULT, 0, "sheet:{} add col info, col:{} en_name:{} cn_name:{} col_type:{}", pSheet->m_sheetName, col_index + 1,
                      pColInfo->m_structEnName, pColInfo->m_structCnName, pColInfo->m_colType);
        }
        else
        {
            pColInfo = pLastColInfo->m_colInfoMap[struct_en_name];
        }

        CHECK_EXPR(pSheet->m_allColInfoList.find(col_index) != pSheet->m_allColInfoList.end(), -1, "sheet:{} col:{} parse failed", pSheet->m_sheetName,
                   col_index + 1);
        ExcelSheetColIndex& sheetColIndex = pSheet->m_allColInfoList[col_index];
        sheetColIndex.m_fullEnName += "_" + struct_en_name;
        sheetColIndex.m_colEnName += "_" + struct_en_name;
        if (struct_num >= 0)
        {
            if (pColInfo->m_strutNumFromZero)
            {
                sheetColIndex.m_fullEnName += "_" + NFCommon::tostr(struct_num);
            }
            else
            {
                sheetColIndex.m_fullEnName += "_" + NFCommon::tostr(struct_num - 1);
            }
        }
    }

    if (pColInfo->m_structCnName != struct_cn_name)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "excel:{} sheet:{} col info, col:{} en_name:{} cn_name:{} the format wrong -- col:{} en_name:{} cn_name:{}", m_excelName, pSheet->m_sheetName, pColInfo->m_colIndex + 1,
                   pColInfo->m_structEnName, pColInfo->m_structCnName, col_index, struct_en_name, struct_cn_name);
    }

    pColInfo->m_colIndexVec.push_back(col_index);

    if (pColInfo->m_structEnName == struct_en_name)
    {
        if (struct_num > pColInfo->m_maxSubNum)
        {
            CHECK_EXPR(pColInfo->m_maxSubNum >= 0 && struct_num == pColInfo->m_maxSubNum + 1, -1,
                       "sheet:{} add col info, col:{} en_name:{} cn_name:{} col_type:{} , but the struct_num is not the last struct_num + 1",
                       pSheet->m_sheetName, col_index,
                       pColInfo->m_structEnName, pColInfo->m_structCnName, pColInfo->m_colType);

            pColInfo->m_maxSubNum = struct_num;
        }
    }

    if (vecEnName.size() == 1)
    {
        CHECK_EXPR(pSheet->m_allColInfoList.find(col_index) != pSheet->m_allColInfoList.end(), -1, "sheet:{} can not find the colIndex:{} Data", pSheet->m_sheetName, col_index);
        ExcelSheetColIndex& sheetColIndex = pSheet->m_allColInfoList[col_index];
        sheetColIndex.m_pColInfo = pColInfo;
    }

    return HandleNewColMsg(pSheet, pColInfo, col_index, col_type, std::vector<std::string>(vecEnName.begin() + 1, vecEnName.end()), std::vector<std::string>(vecCnName.begin() + 1, vecCnName.end()), std::vector<int>(vecStructNum.begin() + 1, vecStructNum.end()));
}

int ExcelParse::CheckRelation(ExcelSheet& sheet, ExcelRelation* pRelation, ExcelSheetColInfo* pColInfo)
{
    if ((pRelation->m_relationMap.empty() && !pColInfo->m_colInfoMap.empty()) || (!pRelation->m_relationMap.empty() && pColInfo->m_colInfoMap.empty()))
    {
        CHECK_EXPR(false, -1, "excel:{} sheet:{} the relation:{} is not right", m_excelName, sheet.m_sheetName, pRelation->m_releationStr);
    }

    for (auto iter = pRelation->m_relationMap.begin(); iter != pRelation->m_relationMap.end(); iter++)
    {
        auto pLastRelation = iter->second;
        CHECK_NULL(0, pLastRelation);
        CHECK_EXPR(pColInfo->m_colInfoMap.find(pLastRelation->m_myColName) != pColInfo->m_colInfoMap.end(), -1, "excel:{} sheet:{} can't find the relation:{}", m_excelName, sheet.m_sheetName, pLastRelation->m_releationStr);
        auto pLastColInfo = pColInfo->m_colInfoMap[pLastRelation->m_myColName];
        CHECK_NULL(0, pLastColInfo);
        int iRetCode = CheckRelation(sheet, pLastRelation, pLastColInfo);
        CHECK_RET(iRetCode, "CheckRelation Failed");
    }

    pRelation->m_noFindError = "";
    for (int i = 0; i < (int)pRelation->m_dst.size(); i++)
    {
        ExcelRelationDst& relationDst = pRelation->m_dst[i];
        pRelation->m_noFindError += " excel:" + relationDst.m_excelName + " sheet:" + relationDst.m_sheetName;
        if (pRelation->m_dst.size() >= 2 && i != (int)pRelation->m_dst.size() - 1)
        {
            pRelation->m_noFindError += " or ";
        }
    }

    return 0;
}

int ExcelParse::HandleSheetIndex()
{
    for (auto iter = m_sheets.begin(); iter != m_sheets.end(); iter++)
    {
        ExcelSheet& sheet = iter->second;
        for (auto iter_i = sheet.m_indexMap.begin(); iter_i != sheet.m_indexMap.end(); iter_i++)
        {
            std::string key = iter_i->first;
            auto find_iter = sheet.m_colInfoMap.find(key);
            if (find_iter != sheet.m_colInfoMap.end())
            {
                if (find_iter->second->m_isArray)
                {
                    NFLogError(NF_LOG_DEFAULT, 0, "excel:{} sheet:{} index key:{} is not right, this is a array, array can't do the index", m_excel,
                               sheet.m_sheetName, key);
                    return -1;
                }

                iter_i->second.m_colIndex = find_iter->second->m_colIndex;
            }
            else
            {
                NFLogError(NF_LOG_DEFAULT, 0, "excel:{} sheet:{} index key:{} not exist", m_excel, sheet.m_sheetName, key);
                return -1;
            }
        }

        for (auto iter_i = sheet.m_comIndexMap.begin(); iter_i != sheet.m_comIndexMap.end(); iter_i++)
        {
            ExcelSheetComIndex& comIndex = iter_i->second;
            for (int i = 0; i < (int)comIndex.m_index.size(); i++)
            {
                ExcelSheetIndex& index = comIndex.m_index[i];
                std::string key = index.m_key;
                auto find_iter = sheet.m_colInfoMap.find(key);
                if (find_iter != sheet.m_colInfoMap.end())
                {
                    if (find_iter->second->m_isArray)
                    {
                        NFLogError(NF_LOG_DEFAULT, 0, "excel:{} sheet:{} index key:{} is not right, this is a array, array can't do the index",
                                   m_excel, sheet.m_sheetName, key);
                        return -1;
                    }

                    index.m_colIndex = find_iter->second->m_colIndex;
                }
                else
                {
                    NFLogError(NF_LOG_DEFAULT, 0, "excel:{} sheet:{} index key:{} not exist", m_excel, sheet.m_sheetName, key);
                    return -1;
                }
            }
        }

        int iRetCode = sheet.ParseRelation(sheet.m_relationStr);
        CHECK_RET(iRetCode, "ParseEnum:{} Failed", sheet.m_relationStr);
        for (auto iter = sheet.m_colRelationMap.begin(); iter != sheet.m_colRelationMap.end(); iter++)
        {
            ExcelRelation* pRelation = &iter->second;
            CHECK_EXPR(sheet.m_colInfoMap.find(pRelation->m_myColName) != sheet.m_colInfoMap.end(), -1, "excel:{} sheet:{} can't find the colName:{} info in the relation", m_excelName, sheet.m_sheetName, pRelation->m_myColName);
            ExcelSheetColInfo* pColInfo = sheet.m_colInfoMap[iter->second.m_myColName];

            if ((pRelation->m_relationMap.empty() && !pColInfo->m_colInfoMap.empty()) || (!pRelation->m_relationMap.empty() && pColInfo->m_colInfoMap.empty()))
            {
                CHECK_EXPR(false, -1, "excel:{} sheet:{} the relation:{} is not right", m_excelName, sheet.m_sheetName, pRelation->m_releationStr);
            }

            int iRetCode = CheckRelation(sheet, pRelation, pColInfo);
            CHECK_RET(iRetCode, "CheckRelation Failed");
        }
    }
    return 0;
}

#define CASE_FIELD_TYPE(cpptype, jsontype) \
    case cpptype:\
    {\
        const RAPIDJSON_NAMESPACE::SizeType size = d.Size();\
        for (RAPIDJSON_NAMESPACE::SizeType index = 0; index < size; ++index)\
        {\
            const RAPIDJSON_NAMESPACE::Value& item = d[index];\
            if (!item.Is##jsontype())\
            {\
                LOG_ERR(0, -1, "Invalid value for repeated field: {}", colValue);\
                return false;\
            }\
        }\
        break;\
    }

bool ExcelParse::CheckValue(ExcelSheetColInfo* pColInfo, const std::string& colValue)
{
    NF_ENUM_COL_TYPE colType = pColInfo->m_colCppType;

    bool isRepeated = false;
    bool isJson = false;
    if (pColInfo->m_strParseRepeatedNum.size() > 0)
    {
        isJson = true;
        isRepeated = true;
    }

    if (pColInfo->m_fieldParseType != FPT_DEFAULT)
    {
        isJson = true;
    }

    switch (colType)
    {
        case NF_ENUM_COL_TYPE_INT8:
        case NF_ENUM_COL_TYPE_UINT8:
        case NF_ENUM_COL_TYPE_INT16:
        case NF_ENUM_COL_TYPE_UINT16:
        case NF_ENUM_COL_TYPE_INT32:
        {
            if (!isJson)
            {
                std::string newValue = colValue;
                if (newValue.size() > 0)
                {
                    if (newValue[0] == '+' || newValue[0] == '-')
                    {
                        newValue.erase(0, 1);
                    }
                }
                return NFStringUtility::CheckIsDigit(newValue);
            }
            else if (!NFProtobufCommon::CheckJsonValueToProtoField(colValue, pColInfo->m_colFullName, google::protobuf::FieldDescriptor::CPPTYPE_INT32, isRepeated))
            {
                LOG_ERR(0, -1, "Invalid value for repeated field: {}", colValue);
                return false;
            }
            break;
        }
        case NF_ENUM_COL_TYPE_UINT32:
        {
            if (!isJson)
            {
                std::string newValue = colValue;
                if (newValue.size() > 0)
                {
                    if (newValue[0] == '+' || newValue[0] == '-')
                    {
                        newValue.erase(0, 1);
                    }
                }
                return NFStringUtility::CheckIsDigit(newValue);
            }
            else if (!NFProtobufCommon::CheckJsonValueToProtoField(colValue, pColInfo->m_colFullName, google::protobuf::FieldDescriptor::CPPTYPE_UINT32, isRepeated))
            {
                LOG_ERR(0, -1, "Invalid value for repeated field: {}", colValue);
                return false;
            }
            break;
        }
        case NF_ENUM_COL_TYPE_INT64:
        {
            if (!isJson)
            {
                std::string newValue = colValue;
                if (newValue.size() > 0)
                {
                    if (newValue[0] == '+' || newValue[0] == '-')
                    {
                        newValue.erase(0, 1);
                    }
                }
                return NFStringUtility::CheckIsDigit(newValue);
            }
            else if (!NFProtobufCommon::CheckJsonValueToProtoField(colValue, pColInfo->m_colFullName, google::protobuf::FieldDescriptor::CPPTYPE_INT64, isRepeated))
            {
                LOG_ERR(0, -1, "Invalid value for repeated field: {}", colValue);
                return false;
            }
            break;
        }
        case NF_ENUM_COL_TYPE_UINT64:
        {
            if (!isJson)
            {
                std::string newValue = colValue;
                if (newValue.size() > 0)
                {
                    if (newValue[0] == '+' || newValue[0] == '-')
                    {
                        newValue.erase(0, 1);
                    }
                }
                return NFStringUtility::CheckIsDigit(newValue);
            }
            else if (!NFProtobufCommon::CheckJsonValueToProtoField(colValue, pColInfo->m_colFullName, google::protobuf::FieldDescriptor::CPPTYPE_UINT64, isRepeated))
            {
                LOG_ERR(0, -1, "Invalid value for repeated field: {}", colValue);
                return false;
            }
            break;
        }
        case NF_ENUM_COL_TYPE_BOOL:
        {
            if (!isJson)
            {
                if (NFStringUtility::CheckIsDigit(colValue))
                {
                    return true;
                }
                else
                {
                    if (NFStringUtility::ToLower(colValue) == "true")
                    {
                        return true;
                    }
                    else if (NFStringUtility::ToLower(colValue) == "false")
                    {
                        return true;
                    }
                }
                return false;
            }
            else if (!NFProtobufCommon::CheckJsonValueToProtoField(colValue, pColInfo->m_colFullName, google::protobuf::FieldDescriptor::CPPTYPE_BOOL, isRepeated))
            {
                LOG_ERR(0, -1, "Invalid value for repeated field: {}", colValue);
                return false;
            }
            break;
        }
        case NF_ENUM_COL_TYPE_FLOAT:
        {
            if (!isJson)
            {
                std::string newValue = colValue;
                if (newValue.size() > 0)
                {
                    if (newValue[0] == '+' || newValue[0] == '-')
                    {
                        newValue.erase(0, 1);
                    }
                }
                if (std::numeric_limits<float>::has_quiet_NaN && newValue == "NaN")
                {
                    return true;
                }
                if (std::numeric_limits<float>::has_infinity && newValue == "Infinity")
                {
                    return true;
                }
                if (std::numeric_limits<float>::has_infinity && newValue == "-Infinity")
                {
                    return true;
                }
                if (!NFStringUtility::CheckIsDigit(newValue))
                {
                    return NFStringUtility::IsFloatNumber(newValue);
                }
                break;
            }
            else if (!NFProtobufCommon::CheckJsonValueToProtoField(colValue, pColInfo->m_colFullName, google::protobuf::FieldDescriptor::CPPTYPE_FLOAT, isRepeated))
            {
                LOG_ERR(0, -1, "Invalid value for repeated field: {}", colValue);
                return false;
            }
            break;
        }
        case NF_ENUM_COL_TYPE_DOUBLE:
        {
            if (!isJson)
            {
                std::string newValue = colValue;
                if (newValue.size() > 0)
                {
                    if (newValue[0] == '+' || newValue[0] == '-')
                    {
                        newValue.erase(0, 1);
                    }
                }
                if (std::numeric_limits<double>::has_quiet_NaN && newValue == "NaN")
                {
                    return true;
                }
                if (std::numeric_limits<double>::has_infinity && newValue == "Infinity")
                {
                    return true;
                }
                if (std::numeric_limits<double>::has_infinity && newValue == "-Infinity")
                {
                    return true;
                }
                if (!NFStringUtility::CheckIsDigit(newValue))
                {
                    return NFStringUtility::IsFloatNumber(newValue);
                }
                break;
            }
            else if (!NFProtobufCommon::CheckJsonValueToProtoField(colValue, pColInfo->m_colFullName, google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE, isRepeated))
            {
                LOG_ERR(0, -1, "Invalid value for repeated field: {}", colValue);
                return false;
            }
            break;
        }
        case NF_ENUM_COL_TYPE_ENUM:
        {
            if (pColInfo->m_fieldParseType == FPT_DEFAULT)
            {
                std::string numberValue;
                if (!NFProtobufCommon::Instance()->FindEnumNumberByMacroName(pColInfo->m_strParseMessage, colValue, numberValue))
                {
                    LOG_ERR(0, -1, "Invalid value for field: {}", colValue);
                    return false;
                }
            }
            else
            {
                if (!NFProtobufCommon::CheckJsonValueToProtoField(colValue, pColInfo->m_strParseMessage, google::protobuf::FieldDescriptor::CPPTYPE_ENUM, isRepeated))
                {
                    LOG_ERR(0, -1, "Invalid value for repeated field: {}", colValue);
                    return false;
                }
            }
            break;
        }
        case NF_ENUM_COL_TYPE_MESSAGE:
        {
            ::google::protobuf::Message* pMessage = NFProtobufCommon::Instance()->CreateDynamicMessageByName(pColInfo->m_strParseMessage);
            if (pMessage == NULL)
            {
                LOG_ERR(0, -1, "can't create message {} to parse json:", pColInfo->m_strParseMessage, colValue);
                return false;
            }
            if (!NFProtobufCommon::CheckJsonToProtoMessage(colValue, pMessage, isRepeated))
            {
                LOG_ERR(0, -1, "CheckJsonToProtoMessage Failed, colValue:{}", colValue);
                delete pMessage;
                return false;
            }
            delete pMessage;
            break;
        }
        case NF_ENUM_COL_TYPE_DATE:
        case NF_ENUM_COL_TYPE_STRING:
        {
            return true;
        }
        default:
        {
            LOG_ERR(0, -1, "Invalid repeated field type:{}", colType);
            return false;
        }
    }

    return true;
}

NF_ENUM_COL_TYPE ExcelParse::GetColType(const std::string& colType, std::string* pStrJsonRepeatedNum, std::string* pStrJsonMessage, FieldParseType* pFieldParseType)
{
    std::string firstColType = colType;
    if (colType.find(":") != std::string::npos)
    {
        std::vector<std::string> vecColType;
        NFStringUtility::SplitStringToVector(colType, ":", vecColType);
        if (vecColType.size() == 2)
        {
            firstColType = NFStringUtility::Lower(vecColType[0]);
            if (firstColType == "enum")
            {
                if (pStrJsonMessage)
                {
                    *pStrJsonMessage = vecColType[1];
                }
            }
            else
            {
                return NF_ENUM_COL_TYPE_UNKNOWN;
            }
        }
        else if (vecColType.size() == 3)
        {
            firstColType = NFStringUtility::Lower(vecColType[0]);
            if (firstColType == "struct")
            {
                if (pStrJsonMessage)
                {
                    *pStrJsonMessage = vecColType[1];
                }

                if (NFStringUtility::Lower(vecColType[2]) == "json")
                {
                    if (pFieldParseType)
                    {
                        *pFieldParseType = FPT_JSON;
                    }
                }
                else if (NFStringUtility::Lower(vecColType[2]) == "xml")
                {
                    if (pFieldParseType)
                    {
                        *pFieldParseType = FPT_XML;
                    }
                }
                else if (NFStringUtility::Lower(vecColType[2]) == "lua")
                {
                    if (pFieldParseType)
                    {
                        *pFieldParseType = FPT_LUA;
                    }
                }
            }
            else
            {
                return NF_ENUM_COL_TYPE_UNKNOWN;
            }
        }
        else if (vecColType.size() == 4)
        {
            firstColType = NFStringUtility::Lower(vecColType[0]);
            if (firstColType == "struct")
            {
                return NF_ENUM_COL_TYPE_UNKNOWN;
            }
            else
            {
                if (NFStringUtility::Lower(vecColType[1]) == "json")
                {
                    if (pFieldParseType)
                    {
                        *pFieldParseType = FPT_JSON;
                    }
                }
                else if (NFStringUtility::Lower(vecColType[1]) == "xml")
                {
                    if (pFieldParseType)
                    {
                        *pFieldParseType = FPT_XML;
                    }
                }
                else if (NFStringUtility::Lower(vecColType[1]) == "lua")
                {
                    if (pFieldParseType)
                    {
                        *pFieldParseType = FPT_LUA;
                    }
                }

                if (NFStringUtility::Lower(vecColType[2]) == "array")
                {
                    if (pStrJsonRepeatedNum)
                    {
                        *pStrJsonRepeatedNum = vecColType[3];
                    }
                }
                else
                {
                    return NF_ENUM_COL_TYPE_UNKNOWN;
                }
            }
        }
        else if (vecColType.size() == 5)
        {
            firstColType = NFStringUtility::Lower(vecColType[0]);
            if (firstColType == "struct" || firstColType == "enum")
            {
                if (pStrJsonMessage)
                {
                    *pStrJsonMessage = vecColType[1];
                }

                if (NFStringUtility::Lower(vecColType[2]) == "json")
                {
                    if (pFieldParseType)
                    {
                        *pFieldParseType = FPT_JSON;
                    }
                }
                else if (NFStringUtility::Lower(vecColType[2]) == "xml")
                {
                    if (pFieldParseType)
                    {
                        *pFieldParseType = FPT_XML;
                    }
                }
                else if (NFStringUtility::Lower(vecColType[2]) == "lua")
                {
                    if (pFieldParseType)
                    {
                        *pFieldParseType = FPT_LUA;
                    }
                }

                if (NFStringUtility::Lower(vecColType[3]) == "array")
                {
                    if (pStrJsonRepeatedNum)
                    {
                        *pStrJsonRepeatedNum = vecColType[4];
                    }
                }
                else
                {
                    return NF_ENUM_COL_TYPE_UNKNOWN;
                }
            }
            else
            {
                return NF_ENUM_COL_TYPE_UNKNOWN;
            }
        }
        else
        {
            return NF_ENUM_COL_TYPE_UNKNOWN;
        }
    }
    else
    {
        firstColType = NFStringUtility::Lower(colType);
    }

    if (firstColType == "string" || firstColType == "string16" || firstColType == "string32" || firstColType == "string64" || firstColType == "string128" || firstColType == "string256" || firstColType == "string512")
    {
        return NF_ENUM_COL_TYPE_STRING;
    }
    else if (firstColType == "int8")
    {
        return NF_ENUM_COL_TYPE_INT8;
    }
    else if (firstColType == "uint8")
    {
        return NF_ENUM_COL_TYPE_UINT8;
    }
    else if (firstColType == "int16")
    {
        return NF_ENUM_COL_TYPE_INT16;
    }
    else if (firstColType == "uint16")
    {
        return NF_ENUM_COL_TYPE_UINT16;
    }
    else if (firstColType == "int" || firstColType == "int32")
    {
        return NF_ENUM_COL_TYPE_INT32;
    }
    else if (firstColType == "uint" || firstColType == "uint32")
    {
        return NF_ENUM_COL_TYPE_UINT32;
    }
    else if (firstColType == "int64")
    {
        return NF_ENUM_COL_TYPE_INT64;
    }
    else if (firstColType == "uint64")
    {
        return NF_ENUM_COL_TYPE_UINT64;
    }
    else if (firstColType == "bool")
    {
        return NF_ENUM_COL_TYPE_BOOL;
    }
    else if (firstColType == "float")
    {
        return NF_ENUM_COL_TYPE_FLOAT;
    }
    else if (firstColType == "double")
    {
        return NF_ENUM_COL_TYPE_DOUBLE;
    }
    else if (firstColType == "struct")
    {
        return NF_ENUM_COL_TYPE_MESSAGE;
    }
    else if (firstColType == "enum")
    {
        return NF_ENUM_COL_TYPE_ENUM;
    }
    else if (firstColType == "date")
    {
        return NF_ENUM_COL_TYPE_DATE;
    }
    else
    {
        return NF_ENUM_COL_TYPE_UNKNOWN;
    }
}

void ExcelParse::HandleColTypeString(std::string& colType, uint32_t& maxSize)
{
    if (colType == "string16")
    {
        colType = "string";
        maxSize = 16;
    }
    else if (colType == "string32")
    {
        colType = "string";
        maxSize = 32;
    }
    else if (colType == "string" || colType == "string64")
    {
        colType = "string";
        maxSize = 64;
    }
    else if (colType == "string128")
    {
        colType = "string";
        maxSize = 128;
    }
    else if (colType == "string256")
    {
        colType = "string";
        maxSize = 256;
    }
    else if (colType == "string512")
    {
        colType = "string";
        maxSize = 512;
    }
}

int ExcelParse::get_max_num(int num)
{
    int i = 1;
    while (true)
    {
        i = i * 2;
        if (i > num)
        {
            return i;
        }
    }
}

std::string ExcelParse::get_string_define(int num)
{
    int maxNum = num;
    if (maxNum <= 16)
    {
        return "MAX_STRING_LEN_16";
    }
    else if (maxNum == 32)
    {
        return "MAX_STRING_LEN_32";
    }
    else if (maxNum == 64)
    {
        return "MAX_STRING_LEN_64";
    }
    else if (maxNum == 128)
    {
        return "MAX_STRING_LEN_128";
    }
    else if (maxNum == 256)
    {
        return "MAX_STRING_LEN_256";
    }
    else if (maxNum >= 512)
    {
        return "MAX_STRING_LEN_512";
    }

    return "MAX_STRING_LEN_32";
}

ExcelSheet* ExcelParse::GetExcelSheet(const std::string& sheetName)
{
    auto iter = m_sheets.find(sheetName);
    if (iter != m_sheets.end())
    {
        return &iter->second;
    }
    return nullptr;
}

void ExcelParse::SplitColHead(const std::string& enName, const std::string& cnName, std::vector<std::string>& vecEnName, std::vector<std::string>& vecCnName, std::vector<int>& vecStructNum)
{
    std::vector<std::string> vecEnNameTemp;
    SplitColHead(enName, &vecEnNameTemp);
    std::vector<std::string> vecCnNameTemp;
    SplitColHead(cnName, &vecCnNameTemp);

    for (int i = 0; i < (int)vecEnNameTemp.size(); i++)
    {
        std::string enNameTemp = vecEnNameTemp[i];
        if (!NFStringUtility::IsDigital(enNameTemp) && !enNameTemp.empty())
        {
            vecEnName.push_back(enNameTemp);
            vecStructNum.push_back(-1);
        }
        else
        {
            if (NFStringUtility::IsDigital(enNameTemp))
            {
                if (i - 1 >= 0 && i - 1 < (int)vecStructNum.size())
                    vecStructNum[i - 1] = NFCommon::strto<int>(enNameTemp);
            }
        }
    }

    for (int i = 0; i < (int)vecCnNameTemp.size(); i++)
    {
        std::string cnNameTemp = vecCnNameTemp[i];
        if (!NFStringUtility::IsDigital(cnNameTemp) && !cnNameTemp.empty())
        {
            vecCnName.push_back(cnNameTemp);
        }
    }
}

void ExcelParse::SplitColHead(const std::string& str, std::vector<std::string>* result)
{
    if (str.empty() || result == NULL)
    {
        return;
    }

    std::string substr;
    bool flag = false;
    for (auto iter = str.begin(); iter != str.end(); iter++)
    {
        if ((*iter >= '0' && *iter <= '9') || *iter == '.' || *iter == '[' || *iter == ']')
        {
            if (flag == false)
            {
                flag = true;
                result->push_back(substr);
                substr.clear();
            }

            if (*iter >= '0' && *iter <= '9')
            {
                substr.append(1, *iter);
            }
        }
        else
        {
            if (flag == true)
            {
                flag = false;
                result->push_back(substr);
                substr.clear();
            }

            substr.append(1, *iter);
        }
    }

    result->push_back(substr);
    if (flag == true)
    {
        result->push_back("");
    }
}

std::string ExcelParse::GetPbName(const std::string& fieldName)
{
    std::string newFieldName = fieldName;
    NFStringUtility::Replace(newFieldName, "m_", "");
    NFStringUtility::Replace(newFieldName, "_", "");
    return newFieldName;
}

void ExcelParse::OnHandleSheetProtoInfo()
{
    for (auto iter = m_sheets.begin(); iter != m_sheets.end(); iter++)
    {
        ExcelSheet& sheet = iter->second;
        OnHandleSheetProtoInfo(sheet);
    }
}

void ExcelParse::OnHandleSheetProtoInfo(ExcelSheet& sheet)
{
    sheet.m_protoInfo.m_sheetName = sheet.m_sheetName;
    if (sheet.m_sheetMsgNameStr.size() > 0)
    {
        sheet.m_protoInfo.m_sheetMsgName = sheet.m_sheetMsgNameStr;
    }
    if (sheet.m_protoMsgNameStr.size() > 0)
    {
        sheet.m_protoInfo.m_protoMsgName = sheet.m_protoMsgNameStr;
    }
    if (sheet.m_otherName.empty())
    {
        if (sheet.m_sheetMsgNameStr.empty())
        {
            sheet.m_protoInfo.m_sheetMsgName = "Sheet_" + NFStringUtility::FirstUpper(m_excelName) + NFStringUtility::FirstUpper(sheet.m_sheetName);
        }
        if (sheet.m_protoMsgNameStr.empty())
        {
            sheet.m_protoInfo.m_protoMsgName = "E_" + NFStringUtility::FirstUpper(m_excelName) + NFStringUtility::FirstUpper(sheet.m_sheetName);
        }
        sheet.m_otherName = NFStringUtility::FirstUpper(m_excelName) + NFStringUtility::FirstUpper(sheet.m_sheetName);
    }
    else
    {
        if (sheet.m_sheetMsgNameStr.empty())
        {
            sheet.m_protoInfo.m_sheetMsgName = "Sheet_" + sheet.m_otherName;
        }
        if (sheet.m_protoMsgNameStr.empty())
        {
            sheet.m_protoInfo.m_protoMsgName = "E_" + sheet.m_otherName;
        }
    }

    if (sheet.m_protoMsgNameStr.empty())
    {
        sheet.m_protoInfo.m_binFileName = sheet.m_protoInfo.m_protoMsgName;
    }
    else
    {
        if (sheet.m_otherName.empty())
        {
            sheet.m_protoInfo.m_binFileName = "E_" + NFStringUtility::FirstUpper(m_excelName) + NFStringUtility::FirstUpper(sheet.m_sheetName);
        }
        else
        {
            sheet.m_protoInfo.m_binFileName = "E_" + sheet.m_otherName;
        }
    }
}
