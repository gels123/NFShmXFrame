//
// Created by gaoyi on 24-7-28.
//

#include "ExcelJsonParse.h"
#include "NFComm/NFCore/NFFileUtility.h"
#include "NFComm/NFPluginModule/NFCheck.h"

int ExcelJsonParse::Init(const std::string &jsonFile) {
    if (!NFFileUtility::IsFileExist(jsonFile))
    {
        NFLogError(NF_LOG_DEFAULT, 0, "jsonFile:{} not exist....", jsonFile);
        return -1;
    }

    std::string json;
    NFFileUtility::ReadFileContent(jsonFile, json);
    NFrame::ExcelParseInfo pbInfo;
    std::string error;
    bool ret = NFProtobufCommon::JsonToProtoMessage(json, &pbInfo, &error);
    if (!ret)
    {
        NFLogError(NF_LOG_DEFAULT, 0, "jsonFile:{} JsonToProtoMessage Failed, error:{}", jsonFile, error);
        return -1;
    }

    if (!m_excelInfo.FromPb(pbInfo))
    {
        NFLogError(NF_LOG_DEFAULT, 0, "jsonFile:{} FromPb Failed", jsonFile);
        return -1;
    }

    return 0;
}

ExcelSheetInfo *ExcelJsonParse::GetSheetInfo(const std::string &excelName, const std::string &sheetName) {
    auto pPbInfo = GetExcelInfo(excelName);
    if (pPbInfo)
    {
        auto iter = pPbInfo->sheet.find(sheetName);
        if (iter != pPbInfo->sheet.end())
        {
            return &iter->second;
        }
    }
    return nullptr;
}

ExcelPbInfo *ExcelJsonParse::GetExcelInfo(const std::string &excelName) {
    auto iter = m_excelInfo.excel.find(excelName);
    if (iter != m_excelInfo.excel.end())
    {
        return &iter->second;
    }
    return nullptr;
}
