// -------------------------------------------------------------------------
//    @FileName         :    ExcelToBin.h
//    @Author           :    gaoyi
//    @Date             :    23-8-26
//    @Email			:    445267987@qq.com
//    @Module           :    ExcelToBin
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "ExcelParse.h"
#include "NFComm/NFPluginModule/NFProtobufCommon.h"

class ExcelToBin : public ExcelParse, public NFSingleton<ExcelToBin>
{
public:
    ExcelToBin();

    virtual ~ExcelToBin();

    // 初始化函数，加载Excel文件并设置输出路径和协议数据结构
    // 参数:
    //   excel: Excel文件路径
    //   outPath: 输出文件路径
    //   proto_ds: 协议数据结构描述
    // 返回值:
    //   成功返回0，失败返回非0
    virtual int Init(const std::string& excel, const std::string& outPath, const std::string& proto_ds);

    // 处理Excel数据的函数
    // 返回值:
    //   成功返回0，失败返回非0
    virtual int HandleExcel();

    // 处理其他信息的函数，具体功能未知
    // 返回值:
    //   成功返回0，失败返回非0
    int OnHandleOtherInfo();

    // 处理其他信息的函数，具体功能未知，参数为Excel工作表
    // 参数:
    //   sheet: Excel工作表对象
    // 返回值:
    //   成功返回0，失败返回非0
    int OnHandleOtherInfo(ExcelSheet& sheet);

    // 将数据写入二进制文件的函数
    // 返回值:
    //   成功返回0，失败返回非0
    int WriteToBin();

    // 将特定工作表的数据写入二进制文件的函数
    // 参数:
    //   sheet: Excel工作表对象
    // 返回值:
    //   成功返回0，失败返回非0
    int WriteToBin(ExcelSheet& sheet);

    // 将数据写入SQL数据库的函数，针对特定工作表和协议消息
    // 参数:
    //   sheet: Excel工作表对象
    //   pSheetProto: 协议消息指针
    // 返回值:
    //   成功返回0，失败返回非0
    int WriteToSql(ExcelSheet& sheet, const google::protobuf::Message* pSheetProto);

    // 创建数据库表的函数
    // 参数:
    //   tbName: 表名
    //   primaryKey: 主键信息映射
    // 返回值:
    //   返回创建表的SQL语句字符串
    std::string CreateTable(const std::string& tbName, const std::map<std::string, DBTableColInfo>& primaryKey);

    // 创建添加列的SQL语句的函数
    // 参数:
    //   tbName: 表名
    //   primaryKey: 主键信息映射
    //   mapFields: 列信息对的向量，包含列名和列信息
    // 返回值:
    //   返回添加列的SQL语句字符串
    std::string CreateAddCol(const std::string& tbName, const std::map<std::string, DBTableColInfo>& primaryKey, const std::vector<std::pair<std::string, DBTableColInfo>>& mapFields);

    // 将数据写入SQL数据库的函数，参数为内容字符串和协议消息
    // 参数:
    //   content: SQL内容字符串
    //   tbName: 表名
    //   pSheetProto: 协议消息指针
    // 返回值:
    //   成功返回0，失败返回非0
    int WriteToSql(std::string& content, const std::string& tbName, const google::protobuf::Message* pSheetProto);

public:
    std::string m_proto_ds;
};
