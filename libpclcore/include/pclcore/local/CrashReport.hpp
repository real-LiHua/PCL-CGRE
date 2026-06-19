#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace pclcore::local {

// ============================================================================
// CrashSource — 崩溃来源分类
// ============================================================================

enum class CrashSource {
    Minecraft,   // Minecraft 游戏本体崩溃
    Launcher,    // PCL 启动器自身崩溃
    Plugin,      // CGRE 插件崩溃
    JDK,         // Java 运行时崩溃 (hs_err_pid.log)
    System,      // 系统级崩溃 (驱动、内存等)
};

/** 返回 CrashSource 的中文标签 */
inline const char* crash_source_label(CrashSource src) {
    switch (src) {
        case CrashSource::Minecraft: return "Minecraft";
        case CrashSource::Launcher:  return "启动器";
        case CrashSource::Plugin:    return "CGRE 插件";
        case CrashSource::JDK:       return "JDK";
        case CrashSource::System:    return "系统";
    }
    return "未知";
}

// ============================================================================
// LogFile — 崩溃关联的日志文件描述
// ============================================================================

struct LogFile {
    std::string filename;    // 显示名，如 "latest.log"
    std::string description; // 简短说明，如 "游戏日志 (Minecraft 运行输出)"

    LogFile() = default;
    LogFile(std::string fn, std::string desc)
        : filename(std::move(fn)), description(std::move(desc)) {}
};

// ============================================================================
// CrashReport — 崩溃报告条目 (崩溃分析中心)
// ============================================================================

struct CrashReport {
    std::string instance_name;   // 关联实例名（JDK/启动器崩溃可为空）
    std::string instance_path;   // 关联实例路径（可为空）
    CrashSource source;          // 崩溃来源
    std::string title;           // 崩溃摘要，如 "NullPointerException: Ticking block entity"
    std::string suspect;         // 可疑模组/JAR/组件，如 "CrashMeIfYouCan.jar"
    std::string severity;        // "致命" / "警告" / "可恢复"
    std::string diagnosis;       // 诊断结论（非空则替换"嫌疑组件"卡片为诊断卡片）
    std::string advice;          // 诊断卡片的正文建议。若以 "TIPS:" 开头则随机展示一条 tip
    std::vector<LogFile> log_files; // 变长日志文件列表
    int64_t     timestamp = 0;   // 崩溃时间戳
};

using CrashReportList = std::vector<CrashReport>;

// ============================================================================
// CrashReportProvider — 可插拔崩溃报告提供者
// ============================================================================

class CrashReportProvider {
public:
    virtual ~CrashReportProvider() = default;
    virtual CrashReportList get_crash_reports() = 0;

    /** 获取指定崩溃报告的日志内容列表（用于详情页展开卡片）。
     *  返回的 vector 与对应 CrashReport::log_files 一一对应。
     *  @param crash_index 崩溃报告索引
     *  @return 日志文本数组，顺序与 log_files 相同 */
    virtual std::vector<std::string> get_log_contents(int crash_index) = 0;
};

CrashReportProvider& get_crash_provider();
void set_crash_provider(CrashReportProvider& p);

}  // namespace pclcore::local
