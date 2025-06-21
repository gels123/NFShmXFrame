// -------------------------------------------------------------------------
//    @FileName         :    NFLogMgr.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------
#pragma once

#include <NFComm/NFCore/NFStringUtility.h>

#include "NFILogModule.h"
#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFCore/NFSingleton.hpp"
#include "NFComm/NFCore/NFSnprintf.h"
#include "NFComm/NFCore/NFTime.h"
#include "NFComm/NFPluginModule/NFProtobufCommon.h"

namespace spdlog
{
    class logger;
}

template <typename T>
constexpr int NFCheckStringLiteral(T)
{
    return 0;
}

// ========================
// 阶段1：编译期字符串常量检查
// ========================
// 判断是否为字符串字面量的类型特征
template <typename T>
struct is_string_literal : std::false_type
{
};

template <size_t N>
struct is_string_literal<char[N]> : std::true_type
{
};

template <size_t N>
struct is_string_literal<const char[N]> : std::true_type
{
};

template <size_t N>
struct is_string_literal<volatile char[N]> : std::true_type
{
};

template <size_t N>
struct is_string_literal<const volatile char[N]> : std::true_type
{
};

// ========================
// 阶段2：格式类型识别（编译期/运行时）
// ========================
enum FormatType { C_Format, Fmt_Format, Invalid };

namespace nf_safe_format_detail
{
    // 编译期C格式验证
    constexpr bool is_digit(char c)
    {
        return c >= '0' && c <= '9';
    }

    constexpr bool is_msvc_length_prefix(char c)
    {
        return c == 'I'; // MSVC特有的I前缀
    }

    constexpr bool is_length_modifier(char c)
    {
        return c == 'h' || c == 'l' || c == 'L' || c == 'j' ||
            c == 'z' || c == 't' || c == 'q' ||
            is_msvc_length_prefix(c); // 合并MSVC支持
    }

    constexpr bool is_flag(char c)
    {
        return c == '-' || c == '+' || c == ' ' || c == '#' ||
            c == '0' || c == '\'';
    }

    constexpr bool validate_flags(const char* fmt, size_t pos,
                                  bool has_minus = false, bool has_zero = false,
                                  bool has_plus = false, bool has_space = false)
    {
        return !is_flag(fmt[pos])
                   ? true
                   :
                   // 冲突检测
                   (fmt[pos] == '-' && has_zero) ||
                   (fmt[pos] == '0' && has_minus) ||
                   (fmt[pos] == '+' && has_space) ||
                   (fmt[pos] == ' ' && has_plus)
                   ? false
                   :
                   // 递归处理
                   validate_flags(fmt, pos + 1,
                                  has_minus || (fmt[pos] == '-'),
                                  has_zero || (fmt[pos] == '0'),
                                  has_plus || (fmt[pos] == '+'),
                                  has_space || (fmt[pos] == ' '));
    }

    // 检查单个格式字符合法性（编译期计算）
    constexpr bool is_valid_specifier(char c)
    {
        return c == 'd' || c == 'i' || c == 'u' || c == 'o' || c == 'x' || c == 'X' ||
            c == 'f' || c == 'F' || c == 'e' || c == 'E' || c == 'g' || c == 'G' ||
            c == 'a' || c == 'A' || // 新增：十六进制浮点数 (C99)
            c == 'c' || c == 's' ||
            c == 'p' || // 指针
            c == 'n' || // 写入字符数
            c == '%'; // 转义 %
    }

    constexpr bool validate_format_impl(const char* fmt, size_t pos = 0, bool in_spec = false)
    {
        return fmt[pos] == '\0'
                   ? !in_spec
                   : in_spec
                   ? (
                       fmt[pos] == '%'
                           ? validate_format_impl(fmt, pos + 1, false)
                           : // %%转义
                           is_flag(fmt[pos])
                           ? validate_flags(fmt, pos) && validate_format_impl(fmt, pos + 1, true)
                           : fmt[pos] == '*' || is_digit(fmt[pos])
                           ? validate_format_impl(fmt, pos + 1, true)
                           : fmt[pos] == '.'
                           ? (fmt[pos + 1] == '*' || is_digit(fmt[pos + 1]) ? validate_format_impl(fmt, pos + 2, true) : false)
                           : is_msvc_length_prefix(fmt[pos])
                           ? ( // 处理I32/I64
                               is_digit(fmt[pos + 1]) && is_digit(fmt[pos + 2]) ? validate_format_impl(fmt, pos + 3, true) : false)
                           : is_length_modifier(fmt[pos])
                           ? validate_format_impl(fmt, pos + 1, true)
                           : is_valid_specifier(fmt[pos])
                           ? validate_format_impl(fmt, pos + 1, false)
                           : false
                   )
                   : (
                       fmt[pos] == '%' ? validate_format_impl(fmt, pos + 1, true) : validate_format_impl(fmt, pos + 1, false)
                   );
    }

    constexpr bool validate_format_string(const char* fmt)
    {
        return fmt && (fmt[0] != '\0') ? validate_format_impl(fmt) : false;
    }

    constexpr bool has_c_format_impl(const char* s, size_t pos = 0, bool pending = false)
    {
        return s[pos] == '\0'
                   ? pending
                   : s[pos] == '%'
                   ? (s[pos + 1] == '%' ? has_c_format_impl(s, pos + 2, pending) : has_c_format_impl(s, pos + 1, true))
                   : has_c_format_impl(s, pos + 1, pending);
    }

    constexpr bool has_c_format(const char* fmt)
    {
        return fmt && (fmt[0] != '\0') ? has_c_format_impl(fmt) : false;
    }

    constexpr bool is_digit_or_dot(char c)
    {
        return (c >= '0' && c <= '9') || c == '.'; // 检查数字或小数点‌:ml-citation{ref="5" data="citationList"}
    }

    constexpr bool scan_format(const char* fmt, size_t pos)
    {
        return fmt[pos] == '\0'
                   ? false
                   : // 终止条件
                   (fmt[pos] == '{')
                   ? (fmt[pos + 1] == '}'
                          ? true
                          : // 匹配{}
                          is_digit_or_dot(fmt[pos + 1])
                          ? // 匹配{N}或{N.M}
                          (fmt[pos + 2] == '}' ? true : scan_format(fmt, pos + 2))
                          : false)
                   : scan_format(fmt, pos + 1); // 继续扫描
    }

    constexpr size_t parse_index(const char* fmt, size_t pos, size_t value = 0)
    {
        return is_digit(fmt[pos]) ? parse_index(fmt, pos + 1, value * 10 + (fmt[pos] - '0')) : value;
    }

    constexpr size_t constexpr_max(size_t a, size_t b)
    {
        return a > b ? a : b;
    }

    constexpr size_t find_max_index_impl(const char* fmt, size_t pos,
                                         size_t explicit_max, size_t implicit_count)
    {
        return fmt[pos] == '\0'
                   ? (explicit_max > implicit_count ? explicit_max : implicit_count)
                   : fmt[pos] == '{' && fmt[pos + 1] == '}'
                   ? find_max_index_impl(fmt, pos + 2, explicit_max, implicit_count + 1)
                   : fmt[pos] == '{' && is_digit(fmt[pos + 1])
                   ? find_max_index_impl(fmt, pos + 1,
                                         constexpr_max(explicit_max, parse_index(fmt, pos + 1)),
                                         implicit_count)
                   : fmt[pos] == '}' && is_digit(fmt[pos - 1])
                   ? find_max_index_impl(fmt, pos + 1, explicit_max, implicit_count)
                   : find_max_index_impl(fmt, pos + 1, explicit_max, implicit_count);
    }

    constexpr size_t find_max_index(const char* fmt)
    {
        return fmt ? find_max_index_impl(fmt, 0, 0, 0) : 0;
    }


    // 增强版索引检测（跳过转义字符）
    constexpr bool contains_implicit_index(const char* fmt, size_t pos = 0)
    {
        return fmt[pos] == '\0'
                   ? false
                   : (fmt[pos] == '{' && fmt[pos + 1] == '}' && (pos == 0 || fmt[pos - 1] != '{')) || // 排除转义情况
                   contains_implicit_index(fmt, pos + 1);
    }


    // 检测是否包含显式索引 {N}（N为数字）
    constexpr bool contains_explicit_index(const char* fmt, size_t pos = 0)
    {
        return fmt[pos] == '\0'
                   ? false
                   : (fmt[pos] == '{' && is_digit(fmt[pos + 1])) ||
                   contains_explicit_index(fmt, pos + 1);
    }

    // 转义字符增强版格式校验
    constexpr bool is_valid_format(const char* fmt, size_t pos = 0, int brace_level = 0)
    {
        return fmt[pos] == '\0'
                   ? (brace_level == 0)
                   : (fmt[pos] == '{' && fmt[pos + 1] == '{')
                   ? // 转义左大括号
                   is_valid_format(fmt, pos + 2, brace_level)
                   : (fmt[pos] == '}' && fmt[pos + 1] == '}')
                   ? // 转义右大括号
                   is_valid_format(fmt, pos + 2, brace_level)
                   : fmt[pos] == '{'
                   ? (brace_level < 1 && is_valid_format(fmt, pos + 1, brace_level + 1))
                   : // 真实左大括号
                   fmt[pos] == '}'
                   ? (brace_level > 0 && is_valid_format(fmt, pos + 1, brace_level - 1))
                   : // 真实右大括号
                   is_valid_format(fmt, pos + 1, brace_level);
    }

    constexpr bool contains_mix_index(const char* fmt)
    {
        return (contains_explicit_index(fmt) && contains_implicit_index(fmt));
    }

    // 占位符计数逻辑
    constexpr size_t count_placeholders(const char* fmt, size_t pos = 0)
    {
        return fmt[pos] == '\0'
                   ? 0
                   : (fmt[pos] == '{' && fmt[pos + 1] == '}' && (pos == 0 || fmt[pos - 1] != '{'))
                   ? 1 + count_placeholders(fmt, pos + 2)
                   : count_placeholders(fmt, pos + 1);
    }

    // 增强版参数校验模板
    constexpr bool validate_format(const char* fmt, size_t argsNum)
    {
        return is_valid_format(fmt) && !contains_mix_index(fmt) && (contains_explicit_index(fmt) ? (argsNum > find_max_index(fmt)) : (argsNum >= count_placeholders(fmt)));
    }

    constexpr bool has_fmt_style_impl(const char* fmt, size_t pos = 0)
    {
        return fmt[pos] == '\0' ? false : (fmt[pos] == '{' ? true : has_fmt_style_impl(fmt, pos + 1));
    }

    constexpr bool has_fmt_style(const char* fmt)
    {
        return fmt && (fmt[0] != '\0') ? has_fmt_style_impl(fmt) : false;
    }

    // 运行时版本
    inline FormatType detect_format_runtime(const char* fmt)
    {
        bool has_curly = false;
        bool has_percent = false;

        for (size_t i = 0; fmt[i] != '\0'; ++i)
        {
            if (fmt[i] == '{')
            {
                has_curly = true;
            }
            else if (fmt[i] == '%' && fmt[i + 1] != '%')
            {
                has_percent = true;
                ++i; // Skip next character (specifier)
            }
        }

        if (has_curly && has_percent) return FormatType::Invalid;
        if (has_curly) return FormatType::Fmt_Format;
        if (has_percent) return FormatType::C_Format;
        return FormatType::Invalid;
    }

    inline FormatType detect_format_runtime(const std::string& fmt)
    {
        bool has_curly = false;
        bool has_percent = false;

        for (size_t i = 0; fmt[i] != '\0'; ++i)
        {
            if (fmt[i] == '{')
            {
                has_curly = true;
            }
            else if (fmt[i] == '%' && fmt[i + 1] != '%')
            {
                has_percent = true;
                ++i; // Skip next character (specifier)
            }
        }

        if (has_curly && has_percent) return FormatType::Invalid;
        if (has_curly) return FormatType::Fmt_Format;
        if (has_percent) return FormatType::C_Format;
        return FormatType::Invalid;
    }
}


class NFLogMgr : public NFSingleton<NFLogMgr>
{
public:
    NFLogMgr();

    virtual ~NFLogMgr();

public:
    bool Init(NFILogModule* pSpdlogModule = nullptr);

    void UnInit();

public:
    template <typename... Args>
    void LogFormat(NF_LOG_LEVEL logLevel, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, int moduleId, int retCode, const std::string& myFmt, const Args&... args)
    {
        LogFormat(logLevel, loc, logId, guid, moduleId, retCode, myFmt.c_str(), args...);
    }

    template <typename... Args>
    void LogSprintf(NF_LOG_LEVEL logLevel, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, int moduleId, int retCode, const std::string& myFmt, const Args&... args)
    {
        LogSprintf(logLevel, loc, logId, guid, moduleId, retCode, myFmt.c_str(), args...);
    }

    template <typename... Args>
    void LogFormat(NF_LOG_LEVEL logLevel, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, int moduleId, int retCode, const char* myFmt, const Args&... args)
    {
        if (m_pLogModule)
        {
            if (!IsLogIdEnable(logLevel, logId))
                return;

            try
            {
                std::string str = fmt::format("[{}] ERR Ret:{}. ", moduleId, retCode);
                str += fmt::format(myFmt, args...);
                m_pLogModule->LogDefault(logLevel, loc, logId, guid, str);
            }
            catch (fmt::format_error& error)
            {
                std::string str = fmt::format("log format error------------{} error:{}", myFmt, error.what());
                m_pLogModule->LogDefault(NLL_ERROR_NORMAL, loc, logId, guid, str);
            }
        }
        else
        {
            CreateNoLog();
            try
            {
                std::string str = fmt::format("[{}] ERR Ret:{}. ", moduleId, retCode);
                str += fmt::format(myFmt, args...);
                NoLog(logLevel, loc, logId, guid, str);
            }
            catch (fmt::v5::format_error& error)
            {
                std::string str = fmt::format("log format error------------{} error:{}", myFmt, error.what());
                NoLog(NLL_ERROR_NORMAL, loc, logId, guid, str);
            }
        }
    }

    template <typename... Args>
    void LogSprintf(NF_LOG_LEVEL logLevel, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, int moduleId, int retCode, const char* myFmt, const Args&... args)
    {
        if (m_pLogModule)
        {
            if (!IsLogIdEnable(logLevel, logId))
                return;

            try
            {
                std::string str = fmt::format("[{}] ERR Ret:{}. ", moduleId, retCode);
                str += fmt::sprintf(myFmt, args...);
                m_pLogModule->LogDefault(logLevel, loc, logId, guid, str);
            }
            catch (fmt::format_error& error)
            {
                std::string str = fmt::format("log format error------------{} error:{}", myFmt, error.what());
                m_pLogModule->LogDefault(NLL_ERROR_NORMAL, loc, logId, guid, str);
            }
        }
        else
        {
            CreateNoLog();
            try
            {
                std::string str = fmt::format("[{}] ERR Ret:{}. ", moduleId, retCode);
                str += fmt::sprintf(myFmt, args...);
                NoLog(logLevel, loc, logId, guid, str);
            }
            catch (fmt::v5::format_error& error)
            {
                std::string str = fmt::format("log format error------------{} error:{}", myFmt, error.what());
                NoLog(NLL_ERROR_NORMAL, loc, logId, guid, str);
            }
        }
    }

    virtual void LogDefault(NF_LOG_LEVEL logLevel, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, const std::string& log)
    {
        if (m_pLogModule)
        {
            m_pLogModule->LogDefault(logLevel, loc, logId, guid, log);
        }
    }

    virtual void LogDefault(NF_LOG_LEVEL logLevel, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, uint32_t module, const std::string& log)
    {
        if (m_pLogModule)
        {
            m_pLogModule->LogDefault(logLevel, loc, logId, guid, module, log);
        }
    }

    virtual void LogBehaviour(NF_LOG_LEVEL logLevel, uint32_t logId, const std::string& log)
    {
        if (m_pLogModule)
        {
            m_pLogModule->LogBehaviour(logLevel, logId, log);
        }
    }

    virtual bool IsLogIdEnable(NF_LOG_LEVEL logLevel, uint32_t logId)
    {
        if (m_pLogModule)
        {
            return m_pLogModule->IsLogIdEnable(logLevel, logId);
        }
        return false;
    }

    void CreateNoLog();

    void NoLog(NF_LOG_LEVEL logLevel, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, const std::string& log);

protected:
    NFILogModule* m_pLogModule;
    std::shared_ptr<spdlog::logger> m_noLogger;
};

void NanoFromPbLogHandle(const char* format, ...);


template <bool IsStringLiteral, bool IsCStyle, bool IsFmtStyle, typename... Args>
struct NFSafeFormatImpl
{
    //编译期间检查字符串 同时带有C格式字符串和fmt格式，直接编译失败
    static_assert(!(IsCStyle && IsFmtStyle), "Mixed C-style and Fmt-style format specifiers detected");

    static void execute(NF_LOG_LEVEL logLevel, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, int module, int iRetCode, const char* fmt, Args&&... args)
    {
    }

    static void execute(NF_LOG_LEVEL logLevel, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, int module, int iRetCode, const std::string fmt, Args&&... args)
    {
    }

    static std::string GetString(const char* fmt, Args&&... args)
    {
        // 运行时路径
        FormatType type = nf_safe_format_detail::detect_format_runtime(fmt);
        if (type == FormatType::C_Format)
        {
            return NFSprintfFunc(fmt, std::forward<Args>(args)...);
        }
        else
        {
            return NFFormatFunc(fmt, std::forward<Args>(args)...);
        }
    }

    static std::string GetString(const std::string& fmt, Args&&... args)
    {
        // 运行时路径
        FormatType type = nf_safe_format_detail::detect_format_runtime(fmt);
        if (type == FormatType::C_Format)
        {
            return NFSprintfFunc(fmt, std::forward<Args>(args)...);
        }
        else
        {
            return NFFormatFunc(fmt, std::forward<Args>(args)...);
        }
    }
};

template <typename... Args>
struct NFSafeFormatImpl<true, true, false, Args...>
{
    static void execute(NF_LOG_LEVEL logLevel, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, int module, int iRetCode, const char* fmt, Args&&... args)
    {
        NFLogMgr::Instance()->LogSprintf(logLevel, loc, logId, guid, module, iRetCode, fmt, std::forward<Args>(args)...);
    }

    static void execute(NF_LOG_LEVEL logLevel, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, int module, int iRetCode, const std::string fmt, Args&&... args)
    {
        // 运行时路径
        FormatType type = nf_safe_format_detail::detect_format_runtime(fmt);
        if (type == FormatType::C_Format)
        {
            NFLogMgr::Instance()->LogSprintf(logLevel, loc, logId, guid, module, iRetCode, fmt, std::forward<Args>(args)...);
        }
        else
        {
            NFLogMgr::Instance()->LogFormat(logLevel, loc, logId, guid, module, iRetCode, fmt, std::forward<Args>(args)...);
        }
    }

    static std::string GetString(const char* fmt, Args&&... args)
    {
        return NFSprintfFunc(fmt, std::forward<Args>(args)...);
    }

    static std::string GetString(const std::string& fmt, Args&&... args)
    {
        // 运行时路径
        FormatType type = nf_safe_format_detail::detect_format_runtime(fmt);
        if (type == FormatType::C_Format)
        {
            return NFFormatFunc(fmt, std::forward<Args>(args)...);
        }
        else
        {
            return NFSprintfFunc(fmt, std::forward<Args>(args)...);
        }
    }
};

template <typename... Args>
struct NFSafeFormatImpl<true, false, true, Args...>
{
    static void execute(NF_LOG_LEVEL logLevel, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, int module, int iRetCode, const char* fmt, Args&&... args)
    {
        NFLogMgr::Instance()->LogFormat(logLevel, loc, logId, guid, module, iRetCode, fmt, std::forward<Args>(args)...);
    }

    static void execute(NF_LOG_LEVEL logLevel, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, int module, int iRetCode, const std::string fmt, Args&&... args)
    {
        // 运行时路径
        FormatType type = nf_safe_format_detail::detect_format_runtime(fmt);
        if (type == FormatType::C_Format)
        {
            NFLogMgr::Instance()->LogSprintf(logLevel, loc, logId, guid, module, iRetCode, fmt, std::forward<Args>(args)...);
        }
        else
        {
            NFLogMgr::Instance()->LogFormat(logLevel, loc, logId, guid, module, iRetCode, fmt, std::forward<Args>(args)...);
        }
    }

    static std::string GetString(const char* fmt, Args&&... args)
    {
        return NFFormatFunc(fmt, std::forward<Args>(args)...);
    }

    static std::string GetString(const std::string& fmt, Args&&... args)
    {
        // 运行时路径
        FormatType type = nf_safe_format_detail::detect_format_runtime(fmt);
        if (type == FormatType::C_Format)
        {
            return NFFormatFunc(fmt, std::forward<Args>(args)...);
        }
        else
        {
            return NFSprintfFunc(fmt, std::forward<Args>(args)...);
        }
    }
};

template <typename... Args>
struct NFSafeFormatImpl<true, false, false, Args...>
{
    static void execute(NF_LOG_LEVEL logLevel, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, int module, int iRetCode, const char* fmt, Args&&... args)
    {
        NFLogMgr::Instance()->LogFormat(logLevel, loc, logId, guid, module, iRetCode, fmt, std::forward<Args>(args)...);
    }

    static void execute(NF_LOG_LEVEL logLevel, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, int module, int iRetCode, const std::string fmt, Args&&... args)
    {
        // 运行时路径
        FormatType type = nf_safe_format_detail::detect_format_runtime(fmt);
        if (type == FormatType::C_Format)
        {
            NFLogMgr::Instance()->LogSprintf(logLevel, loc, logId, guid, module, iRetCode, fmt, std::forward<Args>(args)...);
        }
        else
        {
            NFLogMgr::Instance()->LogFormat(logLevel, loc, logId, guid, module, iRetCode, fmt, std::forward<Args>(args)...);
        }
    }

    static std::string GetString(const char* fmt, Args&&... args)
    {
        return NFFormatFunc(fmt, std::forward<Args>(args)...);
    }

    static std::string GetString(const std::string& fmt, Args&&... args)
    {
        // 运行时路径
        FormatType type = nf_safe_format_detail::detect_format_runtime(fmt);
        if (type == FormatType::C_Format)
        {
            return NFFormatFunc(fmt, std::forward<Args>(args)...);
        }
        else
        {
            return NFSprintfFunc(fmt, std::forward<Args>(args)...);
        }
    }
};


template <typename... Args>
struct NFSafeFormatImpl<false, false, false, Args...>
{
    static void execute(NF_LOG_LEVEL logLevel, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, int module, int iRetCode, const char* fmt, Args&&... args)
    {
        // 运行时路径
        FormatType type = nf_safe_format_detail::detect_format_runtime(fmt);
        if (type == FormatType::C_Format)
        {
            NFLogMgr::Instance()->LogSprintf(logLevel, loc, logId, guid, module, iRetCode, fmt, std::forward<Args>(args)...);
        }
        else
        {
            NFLogMgr::Instance()->LogFormat(logLevel, loc, logId, guid, module, iRetCode, fmt, std::forward<Args>(args)...);
        }
    }

    static void execute(NF_LOG_LEVEL logLevel, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, int module, int iRetCode, const std::string fmt, Args&&... args)
    {
        // 运行时路径
        FormatType type = nf_safe_format_detail::detect_format_runtime(fmt);
        if (type == FormatType::C_Format)
        {
            NFLogMgr::Instance()->LogSprintf(logLevel, loc, logId, guid, module, iRetCode, fmt, std::forward<Args>(args)...);
        }
        else
        {
            NFLogMgr::Instance()->LogFormat(logLevel, loc, logId, guid, module, iRetCode, fmt, std::forward<Args>(args)...);
        }
    }

    static std::string GetString(const char* fmt, Args&&... args)
    {
        // 运行时路径
        FormatType type = nf_safe_format_detail::detect_format_runtime(fmt);
        if (type == FormatType::C_Format)
        {
            return NFSprintfFunc(fmt, std::forward<Args>(args)...);
        }
        else
        {
            return NFFormatFunc(fmt, std::forward<Args>(args)...);
        }
    }

    static std::string GetString(const std::string& fmt, Args&&... args)
    {
        // 运行时路径
        FormatType type = nf_safe_format_detail::detect_format_runtime(fmt);
        if (type == FormatType::C_Format)
        {
            return NFSprintfFunc(fmt, std::forward<Args>(args)...);
        }
        else
        {
            return NFFormatFunc(fmt, std::forward<Args>(args)...);
        }
    }
};

class NFHasCFormat
{
public:
    // 通过 SFINAE 分离路径 编译期间检查字符串是否带有C格式字符串
    template <typename T>
    static constexpr auto execute(T&& fmt) -> typename std::enable_if<is_string_literal<typename std::remove_reference<T>::type>::value, bool>::type
    {
        return nf_safe_format_detail::has_c_format(fmt);
    }

    template <typename T>
    static constexpr auto execute(T&& fmt) -> typename std::enable_if<!is_string_literal<typename std::remove_reference<T>::type>::value, bool>::type
    {
        return false;
    }
};

class NFHasFmtStyle
{
public:
    // 通过 SFINAE 分离路径 编译期间检查字符串是否带有fmt格式字符串
    template <typename T>
    static constexpr auto execute(T&& fmt) -> typename std::enable_if<is_string_literal<typename std::remove_reference<T>::type>::value, bool>::type
    {
        return nf_safe_format_detail::has_fmt_style(fmt);
    }

    template <typename T>
    static constexpr auto execute(T&& fmt) -> typename std::enable_if<!is_string_literal<typename std::remove_reference<T>::type>::value, bool>::type
    {
        return false;
    }
};

template <bool IsStringLiteral, bool IsCStyle, bool IsFmtStyle, typename... Args>
void NFSafeFormat(NF_LOG_LEVEL logLevel, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, int module, int iRetCode, const char* fmt, Args&&... args)
{
    NFSafeFormatImpl<IsStringLiteral, IsCStyle, IsFmtStyle, Args...>::execute(logLevel, loc, logId, guid, module, iRetCode, fmt, std::forward<Args>(args)...);
}

template <bool IsStringLiteral, bool IsCStyle, bool IsFmtStyle, typename... Args>
void NFSafeFormat(NF_LOG_LEVEL logLevel, const NFSourceLoc& loc, uint32_t logId, uint64_t guid, int module, int iRetCode, const std::string& fmt, Args&&... args)
{
    NFSafeFormatImpl<IsStringLiteral, IsCStyle, IsFmtStyle, Args...>::execute(logLevel, loc, logId, guid, module, iRetCode, fmt, std::forward<Args>(args)...);
}

template <bool IsStringLiteral, bool IsCStyle, bool IsFmtStyle, typename... Args>
std::string NFSafeFormatGetString(const char* fmt, Args&&... args)
{
    return NFSafeFormatImpl<IsStringLiteral, IsCStyle, IsFmtStyle, Args...>::GetString(fmt, std::forward<Args>(args)...);
}

template <bool IsStringLiteral, bool IsCStyle, bool IsFmtStyle, typename... Args>
std::string NFSafeFormatGetString(const std::string& fmt, Args&&... args)
{
    return NFSafeFormatImpl<IsStringLiteral, IsCStyle, IsFmtStyle, Args...>::GetString(fmt, std::forward<Args>(args)...);
}

template <bool IsStringLiteral, bool IsCStyle>
struct NFSafeFormatCheckCFormat
{
    template <typename T>
    static constexpr bool execute(T&& fmt)
    {
        return true;
    }

    template <typename T, typename... Args>
    static bool check(T&& fmt, Args&&... args)
    {
        return true;
    }
};

template <>
struct NFSafeFormatCheckCFormat<true, true>
{
    template <typename T>
    static constexpr auto execute(T&& fmt) -> typename std::enable_if<is_string_literal<typename std::remove_reference<T>::type>::value, bool>::type
    {
        return nf_safe_format_detail::validate_format_string(fmt);
    }

    template <typename T>
    static constexpr auto execute(T&& fmt) -> typename std::enable_if<!is_string_literal<typename std::remove_reference<T>::type>::value, bool>::type
    {
        return true;
    }


    PRINTF_FORMAT_ATTR
    static bool check(const char* fmt, ...)
    {
        return true;
    }
};

template <bool IsStringLiteral, bool IsCStyle>
struct NFSafeFormatCheckFmtStyle
{
    template <typename T>
    static constexpr auto execute(T&& fmt) -> typename std::enable_if<is_string_literal<typename std::remove_reference<T>::type>::value, bool>::type
    {
        return true;
    }

    template <typename T>
    static constexpr auto execute(T&& fmt) -> typename std::enable_if<!is_string_literal<typename std::remove_reference<T>::type>::value, bool>::type
    {
        return true;
    }

    template <typename T>
    static constexpr bool check(T&& fmt, size_t argsNum)
    {
        return true;
    }
};

template <>
struct NFSafeFormatCheckFmtStyle<true, true>
{
    template <typename T>
    static constexpr auto execute(T&& fmt) -> typename std::enable_if<is_string_literal<typename std::remove_reference<T>::type>::value, bool>::type
    {
        return nf_safe_format_detail::scan_format(fmt, 0);
    }

    template <typename T>
    static constexpr auto execute(T&& fmt) -> typename std::enable_if<!is_string_literal<typename std::remove_reference<T>::type>::value, bool>::type
    {
        return true;
    }

    static constexpr bool check(const char* fmt, size_t argsNum)
    {
        return nf_safe_format_detail::validate_format(fmt, argsNum);
    }
};

#ifdef NF_DEBUG_MODE

#ifdef COMPILE_CHECK_STRING_FORMAT
#define NF_GET_STRING(format, ...) NFSafeFormatGetString<noexcept(NFCheckStringLiteral(format)), NFHasCFormat::execute(format), NFHasFmtStyle::execute(format)>(format, ##__VA_ARGS__);

#if NF_PLATFORM == NF_PLATFORM_WIN
#define CHECK_LOG(format, ...)\
    constexpr bool LogIsHasCFormat = NFHasCFormat::execute(format);\
    constexpr bool LogIsHasFmtStyle = NFHasFmtStyle::execute(format);\
    static_assert(!(LogIsHasCFormat && LogIsHasFmtStyle), "Mixed C-style and Fmt-style format specifiers detected:"#format);\
    constexpr bool IsStringLiteral = noexcept(NFCheckStringLiteral(format));\
    static_assert(NFSafeFormatCheckCFormat<IsStringLiteral, LogIsHasCFormat>::execute(format), "not good c format:"#format);\
    NFSafeFormatCheckCFormat<IsStringLiteral, LogIsHasCFormat>::check(format, ##__VA_ARGS__);\
    static_assert(NFSafeFormatCheckFmtStyle<IsStringLiteral, LogIsHasFmtStyle>::execute(format), "not good fmt format:"#format);\

#else
#define CHECK_LOG(format, ...)\
    constexpr bool LogIsHasCFormat = NFHasCFormat::execute(format);\
    constexpr bool LogIsHasFmtStyle = NFHasFmtStyle::execute(format);\
    static_assert(!(LogIsHasCFormat && LogIsHasFmtStyle), "Mixed C-style and Fmt-style format specifiers detected:"#format);\
    constexpr bool IsStringLiteral = noexcept(NFCheckStringLiteral(format));\
    NFSafeFormatCheckCFormat<IsStringLiteral, LogIsHasCFormat>::check(format, ##__VA_ARGS__);\
    constexpr size_t argsNum = COUNT_ARGS(__VA_ARGS__);\
    static_assert(NFSafeFormatCheckFmtStyle<IsStringLiteral, LogIsHasFmtStyle>::check(format, argsNum), "not good fmt format:"#format);\

#endif

#else
#define NF_GET_STRING(format, ...) NFSafeFormatGetString<false, false, false>(format, ##__VA_ARGS__);

#define CHECK_LOG(format, ...)\
    constexpr bool LogIsHasCFormat = false;\
    constexpr bool LogIsHasFmtStyle = false;\
    constexpr bool IsStringLiteral = false;\

#endif

#else
#define NF_GET_STRING(format, ...) NFSafeFormatGetString<false, false, false>(format, ##__VA_ARGS__);

#define CHECK_LOG(format, ...)\
    constexpr bool LogIsHasCFormat = false;\
    constexpr bool LogIsHasFmtStyle = false;\
    constexpr bool IsStringLiteral = false;

#endif

#define COUNT_ARGS(...) FL_INTERNAL_ARG_COUNT_PRIVATE(0, ##__VA_ARGS__,\
64, 63, 62, 61, 60, \
59, 58, 57, 56, 55, 54, 53, 52, 51, 50, \
49, 48, 47, 46, 45, 44, 43, 42, 41, 40, \
39, 38, 37, 36, 35, 34, 33, 32, 31, 30, \
29, 28, 27, 26, 25, 24, 23, 22, 21, 20, \
19, 18, 17, 16, 15, 14, 13, 12, 11, 10, \
9,  8,  7,  6,  5,  4,  3,  2,  1,  0)

#define FL_INTERNAL_ARG_COUNT_PRIVATE(\
    _0,  _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, \
    _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, \
    _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, \
    _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, \
    _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, \
    _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, \
    _60, _61, _62, _63, _64, N, ...) N

#if NF_PLATFORM == NF_PLATFORM_WIN
#define NFLogTrace(logID, guid, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_TRACE_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, logID, guid, 0, 0, format, ##__VA_ARGS__);\
    } while (false);
#define NFLogDebug(logID, guid, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_DEBUG_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, logID, guid, 0, 0, format, ##__VA_ARGS__);\
    } while (false);
#define NFLogInfo(logID, guid, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_INFO_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, logID, guid, 0, 0, format, ##__VA_ARGS__);\
    } while (false);
#define NFLogWarning(logID, guid, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_WARING_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, logID, guid, 0, 0, format, ##__VA_ARGS__);\
    } while (false);
#define NFLogError(logID, guid, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_ERROR_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, logID, guid, 0, 0, format, ##__VA_ARGS__);\
    } while (false);
#define NFLogFatal(logID, guid, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_CRITICAL_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, logID, guid, 0, 0, format, ##__VA_ARGS__);\
    } while (false);
#define NFLogTraceIf(CONDITION, logID, guid, format, ...) 		if(CONDITION) NFLogTrace(logID, guid, format, ##__VA_ARGS__)
#define NFLogDebugIf(CONDITION, logID, guid, format, ...) 		if(CONDITION) NFLogDebug(logID, guid, format, ##__VA_ARGS__)
#define NFLogInfoIf(CONDITION, logID, guid, format, ...) 		if(CONDITION) NFLogInfo(logID, guid, format, ##__VA_ARGS__)
#define NFLogWarningIf(CONDITION, logID, guid, format, ...) 	if(CONDITION) NFLogWarning(logID, guid, format, ##__VA_ARGS__)
#define NFLogErrorIf(CONDITION, logID, guid, format, ...) 		if(CONDITION) NFLogError(logID, guid, format, ##__VA_ARGS__)
#define NFLogFatalIf(CONDITION, logID, guid, format, ...) 		if(CONDITION) NFLogFatal(logID, guid, format, ##__VA_ARGS__)

#define LOG_TRACE(guid, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_TRACE_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NF_LOG_DEFAULT, guid, 0, 0, format, ##__VA_ARGS__);\
    } while (false);
#define LOG_DEBUG(guid, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_DEBUG_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NF_LOG_DEFAULT, guid, 0, 0, format, ##__VA_ARGS__);\
    } while (false);
#define LOG_INFO(guid, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_INFO_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NF_LOG_DEFAULT, guid, 0, 0, format, ##__VA_ARGS__);\
    } while (false);
#define LOG_WARN(guid, retCode,  format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_WARING_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NF_LOG_DEFAULT, guid, 0, retCode, format, ##__VA_ARGS__);\
    } while (false);
#define LOG_ERR(guid, retCode,  format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_ERROR_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NF_LOG_DEFAULT, guid, 0, retCode, format, ##__VA_ARGS__);\
    } while (false);
#define LOG_FATAL(guid, retCode,  format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_CRITICAL_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NF_LOG_DEFAULT, guid, 0, retCode, format, ##__VA_ARGS__);\
    } while (false);
#define LOG_TRACE_M(guid, module, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_TRACE_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NF_LOG_DEFAULT, guid, module, 0, format, ##__VA_ARGS__);\
    } while (false);
#define LOG_DEBUG_M(guid, module, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_DEBUG_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NF_LOG_DEFAULT, guid, module, 0, format, ##__VA_ARGS__);\
    } while (false);
#define LOG_INFO_M(guid, module, format, ...) \
    do {\
        constexpr bool LogIsHasCFormat = NFHasCFormat::execute(format);\
        constexpr bool LogIsHasFmtStyle = NFHasFmtStyle::execute(format);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_INFO_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NLL_TRACE_NORMAL, guid, module, 0, format, ##__VA_ARGS__);\
        constexpr bool IsStringLiteral = noexcept(NFCheckStringLiteral(format));\
        static_assert(NFSafeFormatCheckCFormat<IsStringLiteral, LogIsHasCFormat>::execute(format), "not good c format:"#format);\
        NFSafeFormatCheckCFormat<IsStringLiteral, LogIsHasCFormat>::check(format, ##__VA_ARGS__);\
        static_assert(NFSafeFormatCheckFmtStyle<IsStringLiteral, LogIsHasFmtStyle>::execute(format), "not good fmt format:"#format);\
        static_assert(!(LogIsHasCFormat && LogIsHasFmtStyle), "Mixed C-style and Fmt-style format specifiers detected:"#format);\
    } while (false);
#define LOG_WARN_M(guid, module, retCode,  format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_WARING_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NLL_TRACE_NORMAL, guid, module, retCode, format, ##__VA_ARGS__);\
    } while (false);
#define LOG_ERR_M(guid, module, retCode,  format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_ERROR_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NLL_TRACE_NORMAL, guid, module, retCode, format, ##__VA_ARGS__);\
    } while (false);
#define LOG_FATAL_M(guid, module, retCode,  format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_CRITICAL_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NLL_TRACE_NORMAL, guid, module, retCode, format, ##__VA_ARGS__);\
    } while (false);
#define LOG_ANY_M(guid, module, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_TRACE_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NLL_TRACE_NORMAL, guid, module, 0, format, ##__VA_ARGS__);\
    } while (false);
#else
#define NFLogTrace(logID, guid, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_TRACE_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, logID, guid, 0, 0, format, ##__VA_ARGS__);\
    } while (false);
#define NFLogDebug(logID, guid, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_DEBUG_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, logID, guid, 0, 0, format, ##__VA_ARGS__);\
    } while (false);
#define NFLogInfo(logID, guid, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_INFO_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, logID, guid, 0, 0, format, ##__VA_ARGS__);\
    } while (false);
#define NFLogWarning(logID, guid, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_WARING_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, logID, guid, 0, 0, format, ##__VA_ARGS__);\
    } while (false);
#define NFLogError(logID, guid, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_ERROR_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, logID, guid, 0, 0, format, ##__VA_ARGS__);\
    } while (false);
#define NFLogFatal(logID, guid, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_CRITICAL_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, logID, guid, 0, 0, format, ##__VA_ARGS__);\
    } while (false);
#define NFLogTraceIf(CONDITION, logID, guid, format, ...) 		if(CONDITION) NFLogTrace(logID, guid, format, ##__VA_ARGS__)
#define NFLogDebugIf(CONDITION, logID, guid, format, ...) 		if(CONDITION) NFLogDebug(logID, guid, format, ##__VA_ARGS__)
#define NFLogInfoIf(CONDITION, logID, guid, format, ...) 		if(CONDITION) NFLogInfo(logID, guid, format, ##__VA_ARGS__)
#define NFLogWarningIf(CONDITION, logID, guid, format, ...) 	if(CONDITION) NFLogWarning(logID, guid, format, ##__VA_ARGS__)
#define NFLogErrorIf(CONDITION, logID, guid, format, ...) 		if(CONDITION) NFLogError(logID, guid, format, ##__VA_ARGS__)
#define NFLogFatalIf(CONDITION, logID, guid, format, ...) 		if(CONDITION) NFLogFatal(logID, guid, format, ##__VA_ARGS__)

#define LOG_TRACE(guid, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_TRACE_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NF_LOG_DEFAULT, guid, 0, 0, format, ##__VA_ARGS__);\
    } while (false);
#define LOG_DEBUG(guid, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_DEBUG_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NF_LOG_DEFAULT, guid, 0, 0, format, ##__VA_ARGS__);\
    } while (false);
#define LOG_INFO(guid, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_INFO_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NF_LOG_DEFAULT, guid, 0, 0, format, ##__VA_ARGS__);\
    } while (false);
#define LOG_WARN(guid, retCode,  format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_WARING_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NF_LOG_DEFAULT, guid, 0, retCode, format, ##__VA_ARGS__);\
    } while (false);
#define LOG_ERR(guid, retCode,  format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_ERROR_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NF_LOG_DEFAULT, guid, 0, retCode, format, ##__VA_ARGS__);\
    } while (false);
#define LOG_FATAL(guid, retCode,  format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_CRITICAL_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NF_LOG_DEFAULT, guid, 0, retCode, format, ##__VA_ARGS__);\
    } while (false);
#define LOG_TRACE_M(guid, module, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_TRACE_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NF_LOG_DEFAULT, guid, module, 0, format, ##__VA_ARGS__);\
    } while (false);
#define LOG_DEBUG_M(guid, module, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_DEBUG_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NF_LOG_DEFAULT, guid, module, 0, format, ##__VA_ARGS__);\
    } while (false);
#define LOG_INFO_M(guid, module, format, ...) \
    do {\
        constexpr bool LogIsHasCFormat = NFHasCFormat::execute(format);\
        constexpr bool LogIsHasFmtStyle = NFHasFmtStyle::execute(format);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_INFO_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NLL_TRACE_NORMAL, guid, module, 0, format, ##__VA_ARGS__);\
        constexpr bool IsStringLiteral = noexcept(NFCheckStringLiteral(format));\
        NFSafeFormatCheckCFormat<IsStringLiteral, LogIsHasCFormat>::check(format, ##__VA_ARGS__);\
        constexpr size_t argsNum = COUNT_ARGS(__VA_ARGS__);\
        static_assert(NFSafeFormatCheckFmtStyle<IsStringLiteral, LogIsHasFmtStyle>::check(format, argsNum), "not good fmt format:"#format);\
        static_assert(!(LogIsHasCFormat && LogIsHasFmtStyle), "Mixed C-style and Fmt-style format specifiers detected:"#format);\
    } while (false);
#define LOG_WARN_M(guid, module, retCode,  format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_WARING_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NLL_TRACE_NORMAL, guid, module, retCode, format, ##__VA_ARGS__);\
    } while (false);
#define LOG_ERR_M(guid, module, retCode,  format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_ERROR_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NLL_TRACE_NORMAL, guid, module, retCode, format, ##__VA_ARGS__);\
    } while (false);
#define LOG_FATAL_M(guid, module, retCode,  format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_CRITICAL_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NLL_TRACE_NORMAL, guid, module, retCode, format, ##__VA_ARGS__);\
    } while (false);
#define LOG_ANY_M(guid, module, format, ...) \
    do {\
        CHECK_LOG(format, ##__VA_ARGS__);\
        NFSafeFormat<IsStringLiteral, LogIsHasCFormat, LogIsHasFmtStyle>(NLL_TRACE_NORMAL, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NLL_TRACE_NORMAL, guid, module, 0, format, ##__VA_ARGS__);\
    } while (false);
#endif

#define LOG_TRACE_IF(CONDITION, guid, format, ...)              if (CONDITION)  LOG_TRACE(guid, format, ##__VA_ARGS__)
#define LOG_DEBUG_IF(CONDITION, guid,  format, ...)             if (CONDITION)  LOG_DEBUG(guid, format, ##__VA_ARGS__)
#define LOG_INFO_IF(CONDITION, guid,  format, ...)              if (CONDITION)  LOG_INFO(guid, format, ##__VA_ARGS__)
#define LOG_WARN_IF(CONDITION, guid, retCode,  format, ...)     if (CONDITION)  LOG_WARN(guid, retCode, format, ##__VA_ARGS__)
#define LOG_ERR_IF(CONDITION, guid, retCode,  format, ...)      if (CONDITION)  LOG_ERR(guid, retCode, format, ##__VA_ARGS__)
#define LOG_FATAL_IF(CONDITION, guid, retCode,  format, ...)    if (CONDITION)  LOG_FATAL(guid, retCode, format, ##__VA_ARGS__)

#define LOG_TRACE_M_IF(CONDITION, RoleID, module, format, ...)                  if (CONDITION) LOG_TRACE_M(RoleID, module, format, ##__VA_ARGS__)
#define LOG_DEBUG_M_IF(CONDITION, RoleID, module,  format, ...)                 if (CONDITION) LOG_DEBUG_M(RoleID, module, format, ##__VA_ARGS__)
#define LOG_INFO_M_IF(CONDITION, RoleID, module, format, ...)                   if (CONDITION) LOG_INFO_M(RoleID, module, format, ##__VA_ARGS__)
#define LOG_WARN_M_IF(CONDITION, RoleID, module, iRetCode,  format, ...)        if (CONDITION) LOG_WARN_M(RoleID, module, iRetCode,  format, ##__VA_ARGS__)
#define LOG_ERR_M_IF(CONDITION, RoleID, module, iRetCode,  format, ...)         if (CONDITION) LOG_ERR_M(RoleID, module, iRetCode,  format, ##__VA_ARGS__)
#define LOG_FATAL_M_IF(CONDITION, RoleID, module, iRetCode,  format, ...)       if (CONDITION) LOG_FATAL_M(RoleID, module, iRetCode,  format, ##__VA_ARGS__)
#define LOG_ANY_M_IF(CONDITION, RoleID, module, format, ...)                    if (CONDITION) LOG_ANY_M(RoleID, module, format, ##__VA_ARGS__)

//统计日志
#define LOG_STATISTIC(format, ...) do {\
		NFLogInfo(NF_LOG_STATISTIC, 0, format, ##__VA_ARGS__);\
	}while(false)

#define LOG_BEHAVIOUR(message) do {\
		std::string temp_LogStr;\
		int iRetCode = NFProtobufCommon::MessageToLogStr(temp_LogStr, message);\
		CHECK_BREAK(iRetCode == 0);\
		std::string temp_log = message.GetTypeName()+temp_LogStr;\
		LOG_INFO(0, temp_log);\
		NFLogMgr::Instance()->LogBehaviour(NLL_INFO_NORMAL, NF_LOG_BEHAVIOUR, temp_log);\
	}while(false)

#define SVR_LOG_COMM(logger_level, logEvent)\
{\
	std::stringstream ss;\
	ss << logEvent;\
	NFLogMgr::Instance()->LogFormat(logger_level, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NF_LOG_DEFAULT, 0, 0, 0, "{}", ss.str());\
}

#define PLAYER_LOG_COMM(logger_level, id, logEvent)\
{\
	std::stringstream ss;\
	ss << logEvent;\
	NFLogMgr::Instance()->LogFormat(logger_level, NFSourceLoc{NFLOG_FILE_BASENAME(__FILE__), __LINE__, NF_MACRO_FUNCTION}, NF_LOG_DEFAULT, id, 0, 0, "{}", ss.str());\
}

#define LOGSVR_FATAL(logEvent)  SVR_LOG_COMM(NLL_CRITICAL_NORMAL, logEvent)
#define LOGSVR_ERROR(logEvent)  SVR_LOG_COMM(NLL_ERROR_NORMAL, logEvent)
#define LOGSVR_WARN(logEvent)   SVR_LOG_COMM(NLL_WARING_NORMAL,  logEvent)
#define LOGSVR_INFO(logEvent)   SVR_LOG_COMM(NLL_INFO_NORMAL,  logEvent)
#define LOGSVR_DEBUG(logEvent)  SVR_LOG_COMM(NLL_DEBUG_NORMAL, logEvent)
#define LOGSVR_TRACE(logEvent)  SVR_LOG_COMM(NLL_TRACE_NORMAL, logEvent)

#define LOGSVR_FATAL_IF(CONDITION, log_event) if(CONDITION) LOGSVR_FATAL(log_event)
#define LOGSVR_ERROR_IF(CONDITION, log_event) if(CONDITION) LOGSVR_ERROR(log_event)
#define LOGSVR_WARN_IF(CONDITION, log_event)  if(CONDITION) LOGSVR_WARN(log_event)
#define LOGSVR_INFO_IF(CONDITION, log_event)  if(CONDITION) LOGSVR_INFO(log_event)
#define LOGSVR_DEBUG_IF(CONDITION, log_event) if(CONDITION) LOGSVR_DEBUG(log_event)
#define LOGSVR_TRACE_IF(CONDITION, log_event) if(CONDITION) LOGSVR_TRACE(log_event)

#define LOGPLAYER_FATAL(id, logEvent) PLAYER_LOG_COMM(NLL_CRITICAL_NORMAL, id, logEvent)
#define LOGPLAYER_ERROR(id, logEvent) PLAYER_LOG_COMM(NLL_ERROR_NORMAL, id, logEvent)
#define LOGPLAYER_WARN(id, logEvent)  PLAYER_LOG_COMM(NLL_WARING_NORMAL,  id, logEvent)
#define LOGPLAYER_INFO(id, logEvent)  PLAYER_LOG_COMM(NLL_INFO_NORMAL,  id, logEvent)
#define LOGPLAYER_DEBUG(id, logEvent) PLAYER_LOG_COMM(NLL_DEBUG_NORMAL, id, logEvent)
#define LOGPLAYER_TRACE(id, logEvent) PLAYER_LOG_COMM(NLL_TRACE_NORMAL, id, logEvent)

#define LOGPLAYER_FATAL_IF(CONDITION, id, log_event) if(CONDITION) LOGPLAYER_FATAL(id, log_event)
#define LOGPLAYER_ERROR_IF(CONDITION, id, log_event) if(CONDITION) LOGPLAYER_ERROR(id, log_event)
#define LOGPLAYER_WARN_IF(CONDITION, id, log_event)  if(CONDITION) LOGPLAYER_WARN(id, log_event )
#define LOGPLAYER_INFO_IF(CONDITION, id, log_event)  if(CONDITION) LOGPLAYER_INFO(id, log_event)
#define LOGPLAYER_DEBUG_IF(CONDITION, id, log_event) if(CONDITION) LOGPLAYER_DEBUG(id, log_eve
