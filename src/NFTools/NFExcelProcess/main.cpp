// -------------------------------------------------------------------------
//    @FileName         :    main.cpp
//    @Author           :    gaoyi
//    @Date             :    23-8-19
//    @Email			:    445267987@qq.com
//    @Module           :    main
//
// -------------------------------------------------------------------------

#include "Common.h"
#include "ExcelToMakefile.h"
#include "ExcelToProto.h"
#include "ExcelToBin.h"
#include "ExcelJsonParse.h"

#include <type_traits>
#include <string>

int main(int argc, char* argv[])
{
    // 根据不同操作系统平台，执行特定的控制台设置操作
#if NF_PLATFORM == NF_PLATFORM_WIN
    // 设置控制台输出代码页为UTF-8
    SetConsoleOutputCP(CP_UTF8);
    // 获得控制台句柄
    HANDLE HOUT = GetStdHandle(STD_OUTPUT_HANDLE);
    // 获得控制台最大坐标，坐标以字符数为单位
    COORD NewSize = GetLargestConsoleWindowSize(HOUT);
    // 调整控制台尺寸，留出边框空间
    NewSize.X -= 1;
    NewSize.Y -= 1; // 可以不加，不加时显示滚动条
    // 设置控制台缓冲区大小
    SetConsoleScreenBufferSize(HOUT, NewSize);

    // 定义显示区域
    SMALL_RECT DisplayArea = {0, 0, 0, 0};
    DisplayArea.Right = NewSize.X;
    DisplayArea.Bottom = NewSize.Y;
    // 设置控制台大小
    SetConsoleWindowInfo(HOUT, TRUE, &DisplayArea);

    // 控制台已经最大化，但是初始位置不在屏幕左上角，添加如下代码
    HWND hwnd = GetConsoleWindow();
    // 窗体最大化
    ShowWindow(hwnd, SW_MAXIMIZE);
#elif NF_PLATFORM == NF_PLATFORM_LINUX
    // Linux平台下暂无特殊设置
#endif

    try
    {
        // 创建命令行解析器实例
        NFCmdLine::NFParser cmdParser;

        // 添加命令行参数
        cmdParser.Add<std::string>("work", 'w', "work", false, "work");
        cmdParser.Add<std::string>("src", 's', "src excel", false, "achievement.xlsx");
        cmdParser.Add<std::string>("dst", 'd', "dst dir path", false, "dir");
        cmdParser.Add<std::string>("proto_ds", 'o', "proto_ds file", false, "achievement.proto.ds");
        cmdParser.Add<std::string>("excel_json", 'j', "excel json file", false, "excel.json");

        // 打印命令行使用说明
        cmdParser.Usage();

        // 解析命令行参数并进行有效性检查
        cmdParser.ParseCheck(argc, argv);

        // 获取"work"参数值
        std::string work = cmdParser.Get<std::string>("work");

        // 根据"work"参数值，执行不同的操作
        if (work == "exceltomakefile")
        {
            // 获取源文件和目标路径参数
            std::string src_file = cmdParser.Get<std::string>("src");
            std::string dst_path = cmdParser.Get<std::string>("dst");
            // 去除参数值前后空格
            NFStringUtility::Trim(src_file);
            NFStringUtility::Trim(dst_path);

            // 分解源文件路径
            std::vector<std::string> vecFile;
            NFStringUtility::Split(src_file, " ", &vecFile);
            // 调用函数处理Excel文件生成Makefile
            ExcelToMakeFile(vecFile, dst_path);
        }
        else if (work == "exceltoproto")
        {
            // 获取源文件、目标路径和其他参数
            std::string excel = cmdParser.Get<std::string>("src");
            std::string out_path = cmdParser.Get<std::string>("dst");
            std::string excelJson = cmdParser.Get<std::string>("excel_json");
            // 去除参数值前后空格
            NFStringUtility::Trim(excel);
            NFStringUtility::Trim(out_path);
            NFStringUtility::Trim(excelJson);

            // 初始化Excel JSON解析器
            ExcelJsonParse::Instance()->Init(excelJson);

            // 初始化Excel到Proto的转换器
            int ret = ExcelToProto::Instance()->Init(excel, out_path);
            if (ret != 0)
            {
                // 如果初始化失败，记录错误日志并退出程序
                NFLogError(NF_LOG_DEFAULT, 0, "ExcelToProto Init Failed");
                NFSLEEP(1000);
                exit(-1);
            }

            // 处理Excel文件转换为Proto格式
            ret = ExcelToProto::Instance()->HandleExcel();
            if (ret != 0)
            {
                // 如果处理失败，记录错误日志并退出程序
                NFLogError(NF_LOG_DEFAULT, 0, "ExcelToProto HandleExcel Failed");
                NFSLEEP(1000);
                exit(-1);
            }
            // 释放Excel到Proto转换器实例
            ExcelToProto::ReleaseInstance();
        }
        else if (work == "exceltobin")
        {
            // 获取源文件、目标路径、Proto_DS文件和Excel JSON文件参数
            std::string excel = cmdParser.Get<std::string>("src");
            std::string out_path = cmdParser.Get<std::string>("dst");
            std::string proto_ds = cmdParser.Get<std::string>("proto_ds");
            std::string excelJson = cmdParser.Get<std::string>("excel_json");
            // 去除参数值前后空格
            NFStringUtility::Trim(excel);
            NFStringUtility::Trim(out_path);
            NFStringUtility::Trim(proto_ds);
            NFStringUtility::Trim(excelJson);

            // 初始化Excel JSON解析器
            ExcelJsonParse::Instance()->Init(excelJson);

            // 初始化Excel到Bin的转换器
            int ret = ExcelToBin::Instance()->Init(excel, out_path, proto_ds);
            if (ret != 0)
            {
                // 如果初始化失败，记录错误日志并退出程序
                NFLogError(NF_LOG_DEFAULT, 0, "ExcelToBin Init Failed");
                NFSLEEP(1000);
                exit(-1);
            }

            // 处理Excel文件转换为Bin格式
            ret = ExcelToBin::Instance()->HandleExcel();
            if (ret != 0)
            {
                // 如果处理失败，记录错误日志并退出程序
                NFLogError(NF_LOG_DEFAULT, 0, "ExcelToBin HandleExcel Failed");
                NFSLEEP(1000);
                exit(-1);
            }
            // 释放Excel到Bin转换器实例
            ExcelToBin::ReleaseInstance();
        }
        else if (work == "filecopy")
        {
            // 获取源文件和目标路径参数
            std::string files = cmdParser.Get<std::string>("src");
            NFStringUtility::Trim(files);
            std::string out_path = cmdParser.Get<std::string>("dst");
            NFStringUtility::Trim(out_path);
            // 分解源文件路径
            std::vector<std::string> vecFiles;
            NFStringUtility::Split(files, " ", &vecFiles);
            NFStringUtility::Trim(vecFiles);

            // 复制文件到目标目录，无论文件是否存在
            for (int i = 0; i < (int)vecFiles.size(); i++)
            {
                if (!vecFiles[i].empty())
                {
                    if (NFFileUtility::CopyFile(vecFiles[i], out_path, true, true))
                    {
                        // 如果复制成功，记录日志信息
                        NFLogInfo(NF_LOG_DEFAULT, 0, "copy file:{} to dir:{} success", vecFiles[i], out_path);
                    }
                }
            }
        }
        else if (work == "filecopy_notexist")
        {
            // 获取源文件和目标路径参数
            std::string files = cmdParser.Get<std::string>("src");
            NFStringUtility::Trim(files);
            std::string out_path = cmdParser.Get<std::string>("dst");
            NFStringUtility::Trim(out_path);
            // 分解源文件路径
            std::vector<std::string> vecFiles;
            NFStringUtility::Split(files, " ", &vecFiles);
            NFStringUtility::Trim(vecFiles);

            // 复制文件到目标目录，只有当目标目录中不存在该文件时才复制
            for (int i = 0; i < (int)vecFiles.size(); i++)
            {
                if (!vecFiles[i].empty())
                {
                    if (NFFileUtility::CopyFile(vecFiles[i], out_path, false, true))
                    {
                        // 如果复制成功，记录日志信息
                        NFLogInfo(NF_LOG_DEFAULT, 0, "copy file:{} to dir:{} success", vecFiles[i], out_path);
                    }
                }
            }
        }
    }
    catch (NFCmdLine::NFCmdLine_Error& e)
    {
        // 捕获命令行解析异常，并输出错误信息
        std::cout << e.what() << std::endl;
        NFSLEEP(1000);
        exit(-1);
    }
}
