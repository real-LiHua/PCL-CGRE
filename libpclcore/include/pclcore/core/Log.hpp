#pragma once

#include <cstdio>
#include <string>

// ============================================================================
// PCL-Core 日志系统
//
// 轻量级日志宏，不依赖 GLib。
// 默认输出到 stderr，可通过 LOG_CORE_SET_CALLBACK 自定义。
//
// 用法:
//   LOG_CORE_INFO("页面加载完成");
//   LOG_CORE_ERR("请求失败: %s", path);
// ============================================================================

#define LOG_CORE_TRACE(fmt, ...) pclcore::log::_emit(pclcore::log::Level::Trace, "pclcore: " fmt, ##__VA_ARGS__)
#define LOG_CORE_DBG(fmt, ...)   pclcore::log::_emit(pclcore::log::Level::Debug, "pclcore: " fmt, ##__VA_ARGS__)
#define LOG_CORE_INFO(fmt, ...)  pclcore::log::_emit(pclcore::log::Level::Info,  "pclcore: " fmt, ##__VA_ARGS__)
#define LOG_CORE_WARN(fmt, ...)  pclcore::log::_emit(pclcore::log::Level::Warn,  "pclcore: " fmt, ##__VA_ARGS__)
#define LOG_CORE_ERR(fmt, ...)   pclcore::log::_emit(pclcore::log::Level::Error, "pclcore: " fmt, ##__VA_ARGS__)

namespace pclcore::log {

enum class Level { Trace = 0, Debug, Info, Warn, Error };

using LogCallback = void(*)(Level level, const char* message);

/** Set a custom log handler (e.g. forward to GLib in the GUI binary). */
void set_callback(LogCallback cb);

/** Internal: format + dispatch. */
void _emit(Level level, const char* fmt, ...);

}  // namespace pclcore::log
