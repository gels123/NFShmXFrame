// -------------------------------------------------------------------------
//    @FileName         :    ExcelParse.h
//    @Author           :    gaoyi
//    @Date             :    23-8-26
//    @Email			:    445267987@qq.com
//    @Module           :    ExcelParse
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFPluginModule/NFCheck.h"
#include "Common.h"
#include "OpenXLSX/OpenXLSX.hpp"

using namespace OpenXLSX;

#define MAX_HEAD_ROWS 4
#define SHEET_LIST_TITLE_ROW 1
#define SHEET_LIST_SHEET_NAME 1

#define SHEET_TITLE_EN_NAME_ROW 1
#define SHEET_TITLE_CH_NAME_ROW 2
#define SHEET_TITLE_TYPE_ROW 3
#define SHEET_TITLE_CLIENTSERVER_ROW 4

#define SHEET_START_ROW 1
#define SHEET_CONTENT_START_ROW 5

#define SHEET_START_COL 1

enum NF_ENUM_COL_TYPE
{
    NF_ENUM_COL_TYPE_UNKNOWN = 0,
    NF_ENUM_COL_TYPE_INT8 = 1,
    NF_ENUM_COL_TYPE_UINT8 = 2,
    NF_ENUM_COL_TYPE_INT16 = 3,
    NF_ENUM_COL_TYPE_UINT16 = 4,
    NF_ENUM_COL_TYPE_INT32 = 5,
    NF_ENUM_COL_TYPE_UINT32 = 6,
    NF_ENUM_COL_TYPE_INT64 = 7,
    NF_ENUM_COL_TYPE_UINT64 = 8,
    NF_ENUM_COL_TYPE_BOOL = 9,
    NF_ENUM_COL_TYPE_FLOAT = 10,
    NF_ENUM_COL_TYPE_DOUBLE = 11,
    NF_ENUM_COL_TYPE_STRING = 12,
    NF_ENUM_COL_TYPE_MESSAGE = 13,
    NF_ENUM_COL_TYPE_ENUM = 14,
    NF_ENUM_COL_TYPE_DATE = 15,
};

class ExcelParseCheck;
class ExcelSheet;

class ExcelRelationDst
{
public:
    ExcelRelationDst()
    {
    }

    std::string m_excelName;
    std::string m_sheetName;
    std::string m_descName;
};

struct ExcelRelation
{
    ExcelRelation()
    {
    }

    std::string m_myColName;
    std::string m_releationStr;
    std::vector<ExcelRelationDst> m_dst;
    std::string m_noFindError;
    std::map<std::string, ExcelRelation*> m_relationMap;
};

struct ExcelColPbEnum
{
    ExcelColPbEnum()
    {
    }

    std::string m_myColName;
    std::string m_enumPbName;
};

struct ExcelColMessageType
{
    ExcelColMessageType()
    {
    }

    std::string m_myColName;
    std::string m_messagePbName;
};

struct ExcelSheetIndex
{
    ExcelSheetIndex()
    {
        m_unique = false;
        m_colIndex = -1;
    }

    std::string m_key;
    bool m_unique;
    int m_colIndex;
    std::string m_varName;
};

struct ExcelSheetComIndex
{
    ExcelSheetComIndex()
    {
        m_unique = false;
    }

    std::string m_key;
    bool m_unique;
    std::string m_varName;
    std::string m_structComName;
    std::vector<ExcelSheetIndex> m_index;
};

struct ExcelSheetColInfo
{
    ExcelSheetColInfo()
    {
        m_colIndex = 0;
        m_colTypeStrMaxSize = 32;
        m_maxSubNum = -1;
        m_strutNumFromZero = true;
        m_isArray = false;
        m_fieldParseType = FPT_DEFAULT;
    }

    uint32_t m_colIndex;
    std::string m_structEnName;
    std::string m_structCnName;
    std::string m_pbDescName;
    std::string m_pbDescTempName;
    std::string m_colFullName;
    std::string m_colType;
    NF_ENUM_COL_TYPE m_colCppType;
    std::string m_strParseRepeatedNum;
    std::string m_strParseMessage;
    FieldParseType m_fieldParseType;
    int32_t m_maxSubNum;
    bool m_isArray;
    bool m_strutNumFromZero;
    uint32_t m_colTypeStrMaxSize;
    std::vector<uint32_t> m_colIndexVec;

    std::map<std::string, ExcelSheetColInfo*> m_colInfoMap;
    std::map<int, ExcelSheetColInfo*> m_colInfoList;
};

/**
 * @brief --proto_package=proto_ff \
		--proto_sheet_msgname=Sheet_AchievementAchievement
		--excel_sheetname=achievement  --proto_msgname=E_AchievementAchievement  --start_row=4 --dst=${PROTOCGEN_FILE_PATH}/;
 */
struct ExcelSheetProtoInfo
{
    ExcelSheetProtoInfo()
    {
        m_protoPackage = "proto_ff";
        m_startRow = 5;
        m_endRow = 5;
        m_startCol = 0;
        m_endCol = 0;
    }

    std::string m_protoPackage;
    std::string m_sheetMsgName;
    std::string m_sheetName;
    std::string m_protoMsgName;
    std::string m_binFileName;
    uint32_t m_startRow;
    uint32_t m_endRow;
    uint32_t m_startCol;
    uint32_t m_endCol;
};

struct ExcelSheetColIndex
{
    ExcelSheetColIndex()
    {
        m_pColInfo = NULL;
    }

    ExcelSheetColInfo* m_pColInfo;
    uint32_t m_colIndex;
    std::string m_fullEnName;
    std::string m_colEnName;
};

struct ExcelSheet
{
    ExcelSheet()
    {
        m_rows = 0;
        m_createSql = false;
        m_maxRows = 0;
        m_maxCols = 0;
    }

    ~ExcelSheet()
    {
        for (auto iter = m_colInfoMap.begin(); iter != m_colInfoMap.end(); iter++)
        {
            delete iter->second;
        }
    }

    std::string m_excelName;
    std::string m_sheetName;
    uint32_t m_rows;
    uint32_t m_maxRows;
    uint32_t m_maxCols;
    std::string m_uniqueIndexStr;
    std::string m_multiIndexStr;
    std::string m_relationStr;
    std::string m_enumStr;
    std::string m_otherName;
    std::string m_sheetMsgNameStr;
    std::string m_protoMsgNameStr;
    std::string m_colMessageTypeStr;
    std::string m_define_max_rows;
    std::map<uint32_t, ExcelSheetColInfo*> m_colInfoVec;
    std::map<std::string, ExcelSheetColInfo*> m_colInfoMap;
    std::map<uint32_t, ExcelSheetColIndex> m_allColInfoList;
    std::set<std::string> m_allFullEnName;

    std::map<std::string, ExcelSheetIndex> m_indexMap;
    std::map<std::string, ExcelSheetComIndex> m_comIndexMap;
    std::map<std::string, ExcelRelation> m_colRelationMap;
    std::map<std::string, ExcelColPbEnum> m_colEnumMap;
    std::map<std::string, ExcelColMessageType> m_colMessageTypeMap;
    std::unordered_set<std::string> m_protoFileMap;
    std::vector<std::string> m_addFileVec;
    bool m_createSql;

    ExcelSheetProtoInfo m_protoInfo;
    std::map<std::string, int> m_firstColKeyMap;

    bool IsExist(const std::string& firstColKey)
    {
        return m_firstColKeyMap.find(firstColKey) != m_firstColKeyMap.end();
    }

    int ParseUniqueIndex(const std::string& indexStr);
    int ParseMultiIndex(const std::string& indexStr);
    int ParseIndex(const std::string& indexStr, bool unique);
    int ParseRelation(const std::string& relation);
    int ParseEnum(const std::string& enumStr);
    int ParseColMessageType(const std::string& colMessageType);
    int ParseImportProto(const std::string& proto);
    int ParseAddField(const std::string& proto);
};

class ExcelParse
{
public:
    ExcelParse();

    virtual ~ExcelParse();

public:
    // 初始化函数，加载指定的Excel文件并设置输出路径
    // 参数:
    //   excel: 要处理的Excel文件路径
    //   outPath: 处理结果的输出路径
    //   all: 是否处理所有工作表的标志，默认为true
    // 返回值:
    //   成功返回0，否则返回非0错误码
    virtual int Init(const std::string& excel, const std::string& outPath, bool all = true);

    // 处理Excel文件中的所有工作表
    // 返回值:
    //   成功返回0，否则返回非0错误码
    virtual int HandleExcel();

    // 处理工作表列表，用于枚举所有工作表
    // 返回值:
    //   成功返回0，否则返回非0错误码
    virtual int HandleSheetList();

    // 处理特定工作表的列表项
    // 参数:
    //   sheet: 要处理的工作表对象
    // 返回值:
    //   成功返回0，否则返回非0错误码
    virtual int HandleSheetList(const XLWorksheet sheet);

    // 处理工作表中的数据
    // 返回值:
    //   成功返回0，否则返回非0错误码
    virtual int HandleSheetWork();

    // 处理特定工作表中的数据
    // 参数:
    //   sheet: 要处理的工作表对象
    // 返回值:
    //   成功返回0，否则返回非0错误码
    virtual int HandleSheetWork(const XLWorksheet sheet);

    // 处理工作表索引
    // 返回值:
    //   成功返回0，否则返回非0错误码
    virtual int HandleSheetIndex();

    // 处理工作表的协议信息
public:
    void OnHandleSheetProtoInfo();

    // 处理特定工作表的协议信息
    // 参数:
    //   sheet: 要处理的工作表对象
public:
    void OnHandleSheetProtoInfo(ExcelSheet& sheet);

    // 检查工作表中的关系数据
    // 参数:
    //   sheet: 要检查的工作表对象
    //   pLastRelation: 上一个关系的指针，用于链接关系数据
    //   pLastColInfo: 上一列信息的指针，用于参考列数据
    // 返回值:
    //   成功返回0，否则返回非0错误码
public:
    int CheckRelation(ExcelSheet& sheet, ExcelRelation* pLastRelation, ExcelSheetColInfo* pLastColInfo);

    // 处理列类型字符串，解析并确定最大尺寸
    // 参数:
    //   colType: 列类型的字符串表示，可能包含尺寸信息
    //   maxSize: 解析后的最大尺寸
public:
    void HandleColTypeString(std::string& colType, uint32_t& maxSize);

    // 处理新列的消息，用于创建或更新列数据结构
    // 参数:
    //   pSheet: 工作表对象的指针
    //   pColInfo: 列信息的指针
    //   col_index: 列索引
    //   col_type: 列类型
    //   vecEnName: 英文名称的向量
    //   vecCnName: 中文名称的向量
    //   vecStructNum: 结构数量的向量
    // 返回值:
    //   成功返回0，否则返回非0错误码
public:
    virtual int HandleNewColMsg(ExcelSheet* pSheet, ExcelSheetColInfo* pColInfo, int col_index, const std::string& col_type, const std::vector<std::string>& vecEnName, const std::vector<std::string>& vecCnName, const std::vector<int>& vecStructNum);

    // 获取最大数值的工具函数
    // 参数:
    //   num: 输入的数值
    // 返回值:
    //   最大数值
public:
    static int get_max_num(int num);

    // 获取字符串定义的工具函数
    // 参数:
    //   num: 输入的数值
    // 返回值:
    //   对应的字符串定义
public:
    static std::string get_string_define(int num);

    // 获取工作表中特定单元格的值
    // 参数:
    //   sheet: 工作表对象
    //   row: 行索引
    //   col: 列索引
    // 返回值:
    //   单元格的值作为字符串
public:
    static std::string GetSheetValue(const XLWorksheet sheet, int row, int col);

    // 获取单元格的值
    // 参数:
    //   cell: 单元格对象
    // 返回值:
    //   单元格的值作为字符串
public:
    static std::string GetSheetValue(const XLCellValue& cell);

    // 分解列头信息，将字符串分割为多个部分
    // 参数:
    //   enName: 英文名称字符串
    //   cnName: 中文名称字符串
    //   vecEnName: 存储分解后的英文名称的向量
    //   vecCnName: 存储分解后的中文名称的向量
    //   vecStructNum: 存储结构数量的向量
public:
    static void SplitColHead(const std::string& enName, const std::string& cnName, std::vector<std::string>& vecEnName, std::vector<std::string>& vecCnName, std::vector<int>& vecStructNum);

    // 分解列头信息，将字符串分割为多个部分
    // 参数:
    //   str: 要分解的字符串
    //   result: 存储分解后的字符串向量的指针
public:
    static void SplitColHead(const std::string& str, std::vector<std::string>* result);

    // 获取协议缓冲区（protobuf）字段名
    // 参数:
    //   fieldName: 原始字段名称
    // 返回值:
    //   格式化后的字段名称，用于protobuf
public:
    static std::string GetPbName(const std::string& filedName);

    // 获取列类型
    // 参数:
    //   colType: 列类型的字符串表示
    //   pStrJsonRepeatedNum: 用于存储重复次数的字符串指针
    //   pStrJsonMessag: 用于存储消息类型的字符串指针
    //   pFieldParseType: 用于存储字段解析类型的指针
    // 返回值:
    //   列类型的枚举值
public:
    static NF_ENUM_COL_TYPE GetColType(const std::string& colType, std::string* pStrJsonRepeatedNum, std::string* pStrJsonMessag, FieldParseType* pFieldParseType);

    // 检查列值的有效性
    // 参数:
    //   pColInfo: 列信息的指针
    //   colValue: 列的值
    // 返回值:
    //   true表示列值有效，false表示无效
public:
    static bool CheckValue(ExcelSheetColInfo* pColInfo, const std::string& colValue);

    // 获取Excel工作表对象
    // 参数:
    //   sheetName: 工作表的名称
    // 返回值:
    //   成功返回ExcelSheet对象的指针，否则返回nullptr
    ExcelSheet* GetExcelSheet(const std::string& sheetName);

public:
    std::string m_outPath;
    std::string m_excel;
    std::string m_excelName;
    std::map<std::string, ExcelSheet> m_sheets;
    XLDocument m_excelReader;
};
