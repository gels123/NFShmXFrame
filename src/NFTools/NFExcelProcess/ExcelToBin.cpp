// -------------------------------------------------------------------------
//    @FileName         :    ExcelToBin.cpp
//    @Author           :    gaoyi
//    @Date             :    23-8-26
//    @Email			:    445267987@qq.com
//    @Module           :    ExcelToBin
//
// -------------------------------------------------------------------------

#include <cinttypes>
#include "ExcelToBin.h"
#include "NFComm/NFPluginModule/NFProtobufCommon.h"
#include "google/protobuf/util/json_util.h"
#include <inttypes.h>
#include <NFComm/NFCore/NFTimeUtil.h>

ExcelToBin::ExcelToBin()
{
}

ExcelToBin::~ExcelToBin()
{
}

int ExcelToBin::Init(const std::string& excel, const std::string& outPath, const std::string& proto_ds)
{
    int iRet = ExcelParse::Init(excel, outPath);
    if (iRet != 0)
    {
        return iRet;
    }

    m_proto_ds = proto_ds;
    iRet = NFProtobufCommon::Instance()->LoadProtoDsFile(m_proto_ds);
    if (iRet == 0)
    {
        NFLogInfo(NF_LOG_DEFAULT, 0, "Reload proto ds success:{}", m_proto_ds);
    }
    else
    {
        NFLogInfo(NF_LOG_DEFAULT, 0, "Reload proto ds fail:{}", m_proto_ds);
        return iRet;
    }

    return 0;
}

int ExcelToBin::HandleExcel()
{
    int iRet = ExcelParse::HandleExcel();
    if (iRet != 0)
    {
        return iRet;
    }

    OnHandleOtherInfo();

    iRet = WriteToBin();
    if (iRet != 0)
    {
        return iRet;
    }
    return 0;
}

int ExcelToBin::OnHandleOtherInfo()
{
    for (auto iter = m_sheets.begin(); iter != m_sheets.end(); iter++)
    {
        ExcelSheet& sheet = iter->second;
        if (sheet.m_colInfoMap.empty()) continue;

        OnHandleOtherInfo(sheet);
    }
    return 0;
}

int ExcelToBin::OnHandleOtherInfo(ExcelSheet& sheet)
{
    for (auto iter = sheet.m_colMessageTypeMap.begin(); iter != sheet.m_colMessageTypeMap.end(); iter++)
    {
        const ExcelColMessageType& colPbMessage = iter->second;
        const google::protobuf::Descriptor* pDesc = NFProtobufCommon::Instance()->FindDynamicMessageTypeByName(colPbMessage.m_messagePbName);
        if (pDesc == NULL)
        {
            pDesc = NFProtobufCommon::Instance()->FindDynamicMessageTypeByName(DEFINE_DEFAULT_PROTO_PACKAGE_ADD + colPbMessage.m_messagePbName);
        }

        if (pDesc == NULL)
        {
            continue;
        }

        std::unordered_map<std::string, std::string> vecFieldMap;
        NFProtobufCommon::GetMacroTypeFromMessage(pDesc, colPbMessage.m_myColName, vecFieldMap);

        for (auto iter = vecFieldMap.begin(); iter != vecFieldMap.end(); iter++)
        {
            ExcelColPbEnum colPbEnum;
            colPbEnum.m_myColName = iter->first;
            colPbEnum.m_enumPbName = iter->second;

            if (sheet.m_colEnumMap.find(colPbEnum.m_myColName) == sheet.m_colEnumMap.end())
            {
                sheet.m_colEnumMap.emplace(colPbEnum.m_myColName, colPbEnum);
            }
        }
    }
    return 0;
}

int ExcelToBin::WriteToBin()
{
    for (auto iter = m_sheets.begin(); iter != m_sheets.end(); iter++)
    {
        ExcelSheet& sheet = iter->second;
        if (sheet.m_colInfoMap.empty()) continue;

        NFLogInfo(NF_LOG_DEFAULT, 0, "start excel to bin:{} rows:{} col:{}", sheet.m_sheetName, sheet.m_rows, sheet.m_allColInfoList.size());
        uint64_t startTime = NFGetTime();
        int iRet = WriteToBin(sheet);
        double useTime = (NFGetTime() - startTime) / (double)1000;
        if (iRet != 0)
        {
            NFLogInfo(NF_LOG_DEFAULT, 0, "excel to bin:{} failed, use time:{}s rows:{} col:{}", sheet.m_sheetName, useTime, sheet.m_rows, sheet.m_allColInfoList.size());
            return iRet;
        }
        NFLogInfo(NF_LOG_DEFAULT, 0, "excel to bin:{} success, use time:{}s rows:{} col:{}", sheet.m_sheetName, useTime, sheet.m_rows, sheet.m_allColInfoList.size());
    }
    return 0;
}

int ExcelToBin::WriteToBin(ExcelSheet& sheet)
{
    // 构造完整的protobuf消息名称
    std::string full_name = sheet.m_protoInfo.m_protoPackage + "." + sheet.m_protoInfo.m_sheetMsgName;

    // 创建动态消息实例
    auto pSheetProto = NFProtobufCommon::Instance()->CreateDynamicMessageByName(full_name);
    CHECK_EXPR(pSheetProto, -1, "CreateDynamicMessageByName:{} Failed", full_name);

    // 获取消息的描述符
    const google::protobuf::Descriptor* pDesc = pSheetProto->GetDescriptor();
    CHECK_EXPR(pDesc, -1, "GetDescriptor:{} Failed", full_name);

    // 获取消息的反射对象
    const google::protobuf::Reflection* pReflect = pSheetProto->GetReflection();
    CHECK_EXPR(pReflect, -1, "pReflect:{} Failed", full_name);

    // 构造protobuf表格重复字段名称
    std::string proto_sheet_repeatedfieldname = sheet.m_protoInfo.m_protoMsgName + "_List";

    // 查找重复字段描述符
    const google::protobuf::FieldDescriptor* pFieldDesc = pDesc->FindFieldByName(proto_sheet_repeatedfieldname);
    CHECK_EXPR(pReflect, -1, "{} Can't find field:{} Failed", full_name, proto_sheet_repeatedfieldname);
    CHECK_EXPR(pFieldDesc->is_repeated(), -1, "{} field:{} is not repeated", full_name, proto_sheet_repeatedfieldname);
    CHECK_EXPR(pFieldDesc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE, -1, "{} field:{} is not message", full_name,
               proto_sheet_repeatedfieldname);

    // 获取Excel工作表
    auto excelSheet = m_excelReader.workbook().worksheet(sheet.m_sheetName);

    // 遍历Excel行
    for (auto& row_vec : excelSheet.rows())
    {
        int row = row_vec.rowNumber();
        if (row < (int)sheet.m_protoInfo.m_startRow || row >= (int)sheet.m_protoInfo.m_startRow + (int)sheet.m_rows)
        {
            continue;
        }

        // 读取行值
        std::vector<XLCellValue> readValues = row_vec.values();
        std::unordered_map<std::string, std::string> mapFields;
        bool allEmpty = true;

        // 遍历列信息
        for (auto iter = sheet.m_allColInfoList.begin(); iter != sheet.m_allColInfoList.end(); iter++)
        {
            int col = iter->first;
            ExcelSheetColIndex& sheelColIndex = iter->second;
            CHECK_EXPR(row <= (int)sheet.m_maxRows, -1, "excel:{} can't get data, row:{} col:{}", sheet.m_sheetName, row, col);
            CHECK_EXPR(col <= (int)sheet.m_maxCols, -1, "excel:{} can't get data, row:{} col:{}", sheet.m_sheetName, row, col);
            std::string value;
            if (col - 1 < (int)readValues.size())
            {
                value = GetSheetValue(readValues[col - 1]);
                if (sheelColIndex.m_pColInfo->m_colCppType != NF_ENUM_COL_TYPE_STRING)
                {
                    NFStringUtility::Trim(value);
                }

                if (sheelColIndex.m_pColInfo->m_colCppType == NF_ENUM_COL_TYPE_DATE)
                {
                    if (!value.empty())
                    {
                        time_t valueTime;
                        std::string strValue = value.data();
                        int count1 = std::count(strValue.begin(), strValue.end(), '-');
                        int count2 = std::count(strValue.begin(), strValue.end(), ':');
                        if (count1 == 2 && count2 == 2)
                        {
                            if (NFTimeUtil::DateStrToLocalTime(value.data(), &valueTime) != 0)
                            {
                                LOG_ERR(0, -1, "Invalid value for date field: {}", value);
                                return -1;
                            }
                        }
                        else if (count1 == 0 && count2 == 2)
                        {
                            if (NFTimeUtil::GetTimeSec(value.data(), &valueTime) != 0)
                            {
                                LOG_ERR(0, -1, "Invalid value for date field: {}", value);
                                return -1;
                            }
                        }
                        else if (count1 == 0 && count2 == 1)
                        {
                            if (NFTimeUtil::GetTimeSecEx(value.data(), &valueTime) != 0)
                            {
                                LOG_ERR(0, -1, "Invalid value for date field: {}", value);
                                return -1;
                            }
                        }
                        else
                        {
                            if (NFTimeUtil::GetTimeSecEx(value.data(), &valueTime) != 0)
                            {
                                LOG_ERR(0, -1, "Invalid value for date field: {}", value);
                                return -1;
                            }
                        }

                        mapFields.emplace(sheelColIndex.m_fullEnName+"_t",  NFCommon::tostr(valueTime));
                    }
                }
            }
            if (!value.empty())
            {
                allEmpty = false;
            }

            // 处理JSON消息类型的列
            if (sheelColIndex.m_pColInfo->m_fieldParseType != FPT_DEFAULT)
            {
                if (value.size() > 0 && !CheckValue(sheelColIndex.m_pColInfo, value))
                {
                    LOG_ERR(0, -1, "excel:{} get data row:{} col:{} colName:{}, CheckValue Failed, the colType:{} value:{} maybe type not right, maybe out of range", sheet.m_sheetName, row, col, sheelColIndex.m_fullEnName, sheelColIndex.m_pColInfo->m_colType, value);
                    return -1;
                }
                mapFields.emplace(sheelColIndex.m_fullEnName, value);
            }
            else
            {
                // 处理枚举类型的列
                if (value.size() > 0 && sheet.m_colEnumMap.find(sheelColIndex.m_colEnName) != sheet.m_colEnumMap.end())
                {
                    std::string newValue;
                    if (NFProtobufCommon::Instance()->FindEnumNumberByMacroName(sheet.m_colEnumMap[sheelColIndex.m_colEnName].m_enumPbName, value, newValue))
                    {
                        mapFields.emplace(sheelColIndex.m_fullEnName, newValue);
                    }
                    else
                    {
                        NFLogError(NF_LOG_DEFAULT, 0, "FindEnumNumberByMacroName Failed, enumPbName:{} value:{}", sheet.m_colEnumMap[sheelColIndex.m_colEnName].m_enumPbName, value);
                        return -1;
                    }
                }
                else
                {
                    if (value.size() > 0 && !CheckValue(sheelColIndex.m_pColInfo, value))
                    {
                        LOG_ERR(0, -1, "excel:{} get data row:{} col:{} colName:{}, CheckValue Failed, the colType:{} value:{} maybe type not right, maybe out of range", sheet.m_sheetName, row, col, sheelColIndex.m_fullEnName, sheelColIndex.m_pColInfo->m_colType, value);
                        return -1;
                    }
                    mapFields.emplace(sheelColIndex.m_fullEnName, value);
                }
            }
        }

        // 如果所有字段都为空，则返回
        if (allEmpty) return 0;

        // 添加行消息到消息列表
        google::protobuf::Message* pRowMessage = pReflect->AddMessage(pSheetProto, pFieldDesc);
        CHECK_EXPR(pRowMessage, -1, "{} addfield:{} Failed", pSheetProto->GetTypeName(), pFieldDesc->full_name());

        // 从字段映射中获取消息
        int iRetCode = NFProtobufCommon::GetMessageFromMapFields(mapFields, pRowMessage);
        CHECK_ERR(0, iRetCode, "GetMessageFromMapFields failed, excel:{} sheet:{} row:{}", sheet.m_excelName, sheet.m_sheetName, row);
    }

    // 生成二进制文件路径
    std::string bin_file = m_outPath + sheet.m_protoInfo.m_binFileName + ".bin";

    // 序列化消息为二进制内容
    std::string content = pSheetProto->SerializePartialAsString();

    // 写入二进制文件
    NFFileUtility::WriteFile(bin_file, content);

    // 设置JSON转换选项
    NFJson2PB::Pb2JsonOptions options;
    options.pretty_json = true;

    // 将消息转换为JSON格式
    std::string excelJson;
    std::string excelJsonError;
    if (!NFProtobufCommon::ProtoMessageToJson(options, *pSheetProto, &excelJson, &excelJsonError))
    {
        CHECK_ERR(0, -1, "ProtoMessageToJson Failed excel:{} sheet:{} errorJson:{}", sheet.m_excelName, sheet.m_sheetName, excelJsonError);
    }

    // 生成调试用JSON文件路径
    std::string debug_file = m_outPath + sheet.m_protoInfo.m_binFileName + "_debug.json";

    // 写入调试用JSON文件
    NFFileUtility::WriteFile(debug_file, excelJson);

    // 如果需要创建SQL，则将数据写入SQL
    if (sheet.m_createSql)
    {
        WriteToSql(sheet, pSheetProto);
    }
    return 0;
}

int ExcelToBin::WriteToSql(ExcelSheet& sheet, const google::protobuf::Message* pSheetProto)
{
    std::string full_name = sheet.m_protoInfo.m_protoPackage + "." + sheet.m_protoInfo.m_sheetMsgName;

    const google::protobuf::Descriptor* pDesc = pSheetProto->GetDescriptor();
    CHECK_EXPR(pDesc, -1, "GetDescriptor:{} Failed", full_name);
    const google::protobuf::Reflection* pReflect = pSheetProto->GetReflection();
    CHECK_EXPR(pReflect, -1, "pReflect:{} Failed", full_name);

    std::string proto_sheet_repeatedfieldname = sheet.m_protoInfo.m_protoMsgName + "_List";
    const google::protobuf::FieldDescriptor* pFieldDesc = pDesc->FindFieldByName(proto_sheet_repeatedfieldname);
    CHECK_EXPR(pReflect, -1, "{} Can't find field:{} Failed", full_name, proto_sheet_repeatedfieldname);
    CHECK_EXPR(pFieldDesc->is_repeated(), -1, "{} field:{} is not repeated", full_name, proto_sheet_repeatedfieldname);
    CHECK_EXPR(pFieldDesc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE, -1, "{} field:{} is not message", full_name,
               proto_sheet_repeatedfieldname);

    std::string content;
    std::map<std::string, DBTableColInfo> primaryKey;
    std::vector<std::pair<std::string, DBTableColInfo>> mapFields;
    int iRet = NFProtobufCommon::Instance()->GetDbFieldsInfoFromMessage(pFieldDesc->message_type(), primaryKey, mapFields);
    if (iRet != 0)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "GetDbFieldsInfoFromMessage Error, tableName:{}", pFieldDesc->full_name());
    }

    content += CreateTable(sheet.m_protoInfo.m_protoMsgName, primaryKey) + "\n\n";
    content += CreateAddCol(sheet.m_protoInfo.m_protoMsgName, primaryKey, mapFields) + "\n\n";

    ::google::protobuf::int32 repeatedSize = pReflect->FieldSize(*pSheetProto, pFieldDesc);
    for (int i = 0; i < (int)repeatedSize; i++)
    {
        const google::protobuf::Message& rowMessage = pReflect->GetRepeatedMessage(*pSheetProto, pFieldDesc, i);
        WriteToSql(content, sheet.m_protoInfo.m_protoMsgName, &rowMessage);
    }

    std::string bin_file = m_outPath + "CreateTable_" + sheet.m_protoInfo.m_binFileName + ".sql";
    NFFileUtility::WriteFile(bin_file, content);
    return 0;
}

std::string ExcelToBin::CreateTable(const std::string& tableName, const std::map<std::string, DBTableColInfo>& primaryKey)
{
    std::string sql;
    std::string colSql;
    std::string privateKey = "PRIMARY KEY(";
    std::string auto_increment;

    for (auto iter = primaryKey.begin(); iter != primaryKey.end(); iter++)
    {
        if (iter == primaryKey.begin())
        {
            privateKey += iter->first;
        }
        else
        {
            privateKey += "," + iter->first;
        }

        std::string col;
        std::string otherInfo;
        if (iter->second.m_notNull)
        {
            otherInfo += " NOT NULL ";
        }

        if (iter->second.m_autoIncrement)
        {
            otherInfo += " AUTO_INCREMENT ";
            auto_increment = "AUTO_INCREMENT = " + NFCommon::tostr(iter->second.m_autoIncrementValue);
        }

        if (iter->second.m_isDefaultValue)
        {
            otherInfo += " Default = " + NFCommon::tostr(iter->second.m_defaultValue);
        }

        if (iter->second.m_comment.size() > 0)
        {
            otherInfo += " COMMENT \"" + iter->second.m_comment + "\"";
        }
        NF_FORMAT_EXPR(col, " {} {} {},", iter->first, NFProtobufCommon::GetDBDataTypeFromPBDataType(iter->second.m_colType, iter->second.m_bufsize), otherInfo)
        colSql += col;
    }
    privateKey += ")";

    NF_FORMAT_EXPR(sql, "CREATE TABLE IF NOT EXISTS {} ({} {}) ENGINE=InnoDB {} DEFAULT CHARSET=utf8;", tableName, colSql, privateKey, auto_increment);

    return sql;
}

int ExcelToBin::WriteToSql(std::string& content, const std::string& tbName, const google::protobuf::Message* pRowMessage)
{
    std::vector<std::pair<std::string, std::string>> keyValueMap;

    NFProtobufCommon::GetDBFieldsFromMessage(*pRowMessage, keyValueMap);

    // update
    std::string sql;
    sql += "replace into " + tbName + "(";
    for (auto iter = keyValueMap.begin(); iter != keyValueMap.end(); ++iter)
    {
        if (iter == keyValueMap.begin())
        {
            sql += iter->first;
        }
        else
        {
            sql += "," + iter->first;
        }
    }

    sql += ") values (";
    for (auto iter = keyValueMap.begin(); iter != keyValueMap.end(); ++iter)
    {
        if (iter == keyValueMap.begin())
        {
            sql += +"\"" + iter->second + "\"";
        }
        else
        {
            sql += ",\"" + iter->second + "\"";
        }
    }
    sql += ");\n";
    content += sql;
    return 0;
}

std::string
ExcelToBin::CreateAddCol(const string& tableName, const map<std::string, DBTableColInfo>& primaryKey, const std::vector<std::pair<std::string, DBTableColInfo>>& mapFields)
{
    std::string content;
    std::multimap<uint32_t, std::string> needCreateColumn;
    for (auto iter = primaryKey.begin(); iter != primaryKey.end(); iter++)
    {
        if (!iter->second.m_primaryKey)
        {
            std::string otherInfo;
            if (iter->second.m_notNull)
            {
                otherInfo += " NOT NULL ";
            }

            if (iter->second.m_autoIncrement)
            {
                otherInfo += " AUTO_INCREMENT ";
            }

            if (iter->second.m_isDefaultValue)
            {
                otherInfo += " Default = " + NFCommon::tostr(iter->second.m_defaultValue);
            }

            if (iter->second.m_comment.size() > 0)
            {
                otherInfo += " COMMENT \"" + iter->second.m_comment + "\"";
            }

            std::string sql;
            NF_FORMAT_EXPR(sql, "alter table {} add column {} {} {};", tableName, iter->first,
                           NFProtobufCommon::GetDBDataTypeFromPBDataType(iter->second.m_colType, iter->second.m_bufsize), otherInfo);

            if (sql.size() > 0)
                needCreateColumn.emplace(iter->second.m_fieldIndex, sql);
        }
    }

    for (auto iter = mapFields.begin(); iter != mapFields.end(); iter++)
    {
        std::string otherInfo;
        if (iter->second.m_notNull)
        {
            otherInfo += " NOT NULL ";
        }

        if (iter->second.m_isDefaultValue)
        {
            otherInfo += " Default = " + NFCommon::tostr(iter->second.m_defaultValue);
        }

        if (iter->second.m_comment.size() > 0)
        {
            otherInfo += " COMMENT \"" + iter->second.m_comment + "\"";
        }

        std::string sql;
        NF_FORMAT_EXPR(sql, "alter table {} add column {} {} {};", tableName, iter->first,
                       NFProtobufCommon::GetDBDataTypeFromPBDataType(iter->second.m_colType, iter->second.m_bufsize), otherInfo);

        content += sql + "\n";

        if (iter->second.m_primaryKey)
        {
            sql.clear();
            NF_FORMAT_EXPR(sql, "alter table {} add PRIMARY KEY ({});", tableName, iter->first);
            content += sql + "\n";
        }

        if (iter->second.m_unionKey)
        {
            sql.clear();
            NF_FORMAT_EXPR(sql, "alter table {} add UNIQUE {} ({});", tableName, iter->first, iter->first);
            content += sql + "\n";
        }

        if (iter->second.m_indexKey)
        {
            sql.clear();
            NF_FORMAT_EXPR(sql, "alter table {} add INDEX {} ({});", tableName, iter->first, iter->first);
            content += sql + "\n";
        }
    }

    return content;
}


