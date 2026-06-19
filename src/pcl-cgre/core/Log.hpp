#pragma once

#include <glib.h>
#include <cstdio>

/* ============================================================================
 * PCL-CGRE 日志系统
 *
 * 使用 GLib 的 g_message / g_warning 系列，自动输出到 stderr。
 * 用环境变量 G_MESSAGES_DEBUG=all 可以看到更多信息。
 *
 * 用法:
 *   LOG_DBG("x = %d", x);
 *   LOG_INFO("页面加载完成");
 *   LOG_ERR("请求失败: %s", err->message);
 *
 * LOG_TRACE 用于高频事件，默认不显示，需要设置 G_MESSAGES_DEBUG=pcl-cgre
 * ============================================================================ */

#define LOG_TRACE(fmt, ...) g_debug("pcl-cgre: " fmt, ##__VA_ARGS__)
#define LOG_DBG(fmt, ...)   g_message("pcl-cgre: " fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  g_message("pcl-cgre: " fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  g_warning("pcl-cgre: " fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)   g_critical("pcl-cgre: " fmt, ##__VA_ARGS__)

namespace pcl::log {

/** One-time initialisation: ensure debug output is enabled. */
inline void init()
{
    // Make glib debug output visible by default
    if (!g_getenv("G_MESSAGES_DEBUG")) {
        g_setenv("G_MESSAGES_DEBUG", "all", FALSE);
    }
}

}  // namespace pcl::log
