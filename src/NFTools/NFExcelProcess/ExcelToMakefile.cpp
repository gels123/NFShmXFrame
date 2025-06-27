// -------------------------------------------------------------------------
//    @FileName         :    ExcelToMakefile.cpp
//    @Author           :    gaoyi
//    @Date             :    23-8-19
//    @Email			:    445267987@qq.com
//    @Module           :    ExcelToMakefile
//
// -------------------------------------------------------------------------

#include "ExcelToMakefile.h"

/**
 * 生成Makefile中处理Excel和资源元数据的部分
 *
 * @param excelmmo_gen 用于生成Excel处理部分的字符串
 * @param resmetas_gen 用于生成资源元数据处理部分的字符串
 * @param file 文件名，用于指定要处理的Excel文件和生成的元数据文件
 */
void WriteMakeFile(std::string& excelmmo_gen, std::string& resmetas_gen, const std::string& file)
{
    // 添加Excel处理的规则到excelmmo_gen
    excelmmo_gen += "${PROTOCGEN_FILE_PATH}/" + file +"_xlsx_finish:${RESDB_EXCELMMO_PATH}/" + file + ".xlsx\n";
    excelmmo_gen += "\tmkdir -p ${PROTOCGEN_FILE_PATH}\n";
    excelmmo_gen += "\tmkdir -p ${RESDB_META_PATH}\n";
    excelmmo_gen += "\trm -rf ${PROTOCGEN_FILE_PATH}/" + file + "_xlsx_finish\n";
    excelmmo_gen += "\t${NFEXCELPROCESS} --work=\"exceltoproto\" --excel_json=${RESDB_EXCEL_JSON} --src=$^ --dst=${PROTOCGEN_FILE_PATH}/\n";
    excelmmo_gen += "\t${FILE_COPY_EXE} --work=\"filecopy\" --src=\"${PROTOCGEN_FILE_PATH}/E_" + NFStringUtility::FirstUpper(file) + ".proto ${PROTOCGEN_FILE_PATH}/" + file + "_gen.makefile\" --dst=${RESDB_META_PATH}/\n";
    excelmmo_gen += "\ttouch ${PROTOCGEN_FILE_PATH}/" + file + "_xlsx_finish\n";
    excelmmo_gen += "\n\n";

    // 添加资源元数据处理的规则到resmetas_gen
    resmetas_gen += "${PROTOCGEN_FILE_PATH}/" + file+"_proto_finish:${RESDB_META_PATH}/E_" + NFStringUtility::FirstUpper(file) + ".proto\n";
    resmetas_gen += "\tmkdir -p ${PROTOCGEN_FILE_PATH}\n";
    resmetas_gen += "\tmkdir -p ${NEW_RESMETAS_FILE_PATH}\n";
    resmetas_gen += "\trm -rf ${PROTOCGEN_FILE_PATH}/" + file + "_proto_finish\n";
    resmetas_gen += "\t${PROTOC} $^ --plugin=protoc-gen-nanopb=${NANOPB_PLUGIN} -I${NANOPB_PLUGIN_PROTO} -I${THIRD_PARTY_INC_PATH} -I${PROTOCOL_PATH} -I${RESDB_META_PATH} -I${PROTOCOL_SS_LOGIC_PATH} ${ALL_COMMON_LOGIC_META_PATH} --cpp_out=${PROTOCGEN_FILE_PATH} --nanopb_out=-e.nanopb:${PROTOCGEN_FILE_PATH} --include_imports --descriptor_set_out=${PROTOCGEN_FILE_PATH}/" + file + ".proto.ds\n";
    resmetas_gen += "\t${FILE_COPY_EXE} --work=\"filecopy\" --src=\"${PROTOCGEN_FILE_PATH}/E_" + NFStringUtility::FirstUpper(file) + ".pb.h ${PROTOCGEN_FILE_PATH}/E_" + NFStringUtility::FirstUpper(file) + ".pb.cc ${PROTOCGEN_FILE_PATH}/E_" + NFStringUtility::FirstUpper(file) + ".nanopb.h ${PROTOCGEN_FILE_PATH}/E_" + NFStringUtility::FirstUpper(file) + ".nanopb.cc \" --dst=${NEW_RESMETAS_FILE_PATH}/\n";
    resmetas_gen += "\ttouch ${PROTOCGEN_FILE_PATH}/" + file + "_proto_finish\n";
    resmetas_gen += "\n\n";
}

/**
 * 将Excel文件列表转换为Makefile格式的字符串并写入指定目录
 *
 * @param vecStr 包含Excel文件名的向量
 * @param dst 目标目录，Makefile将写入该目录
 */
void ExcelToMakeFile(const std::vector<std::string>& vecStr, const std::string& dst)
{
    // 日志记录，包括源文件列表和目标目录
    NFLogInfo(NF_LOG_DEFAULT, 0, "src={}, dst={}", NFCommon::tostr(vecStr), dst);
    std::string excelmmo_gen;
    std::string resmetas_gen;

    // 初始化Excel处理部分的Makefile通用设置
    excelmmo_gen += "include ./define.makefile\n\n";
    excelmmo_gen += ".PHONY:all\n\n";
    excelmmo_gen += "all:";
    for(int i = 0; i < (int)vecStr.size(); i++)
    {
        if (vecStr[i].empty()) continue;

       excelmmo_gen += "${PROTOCGEN_FILE_PATH}/" + vecStr[i] + "_xlsx_finish ";
    }
    excelmmo_gen += "\n\n";

    // 初始化资源元数据处理部分的Makefile通用设置
    resmetas_gen += "include ./define.makefile\n\n";
    resmetas_gen += ".PHONY:all\n\n";
    resmetas_gen += "all:";
    for(int i = 0; i < (int)vecStr.size(); i++)
    {
        if (vecStr[i].empty()) continue;

       resmetas_gen += "${PROTOCGEN_FILE_PATH}/" + vecStr[i] + "_proto_finish ";
    }
    resmetas_gen += "\n\n";

    // 为每个Excel文件名生成特定的处理规则
    for(int i = 0; i < (int)vecStr.size(); i++)
    {
        if (vecStr[i].empty()) continue;
        WriteMakeFile(excelmmo_gen, resmetas_gen, vecStr[i]);
    }

    // 如果目标是一个目录，将生成的Makefile写入该目录
    if (NFFileUtility::IsDir(dst))
    {
        std::string dir = NFFileUtility::NormalizePath(dst);
        NFFileUtility::WriteFile(dir + "excel_gen.makefile", excelmmo_gen);
        NFFileUtility::WriteFile(dir + "resmetas_gen.makefile", resmetas_gen);
        NFLogInfo(NF_LOG_DEFAULT, 0, "create excel_gen.makefile success");
        NFLogInfo(NF_LOG_DEFAULT, 0, "create resmetas_gen.makefile success");
    }
    else {
        NFLogInfo(NF_LOG_DEFAULT, 0, "create makefile fail");
    }
}
