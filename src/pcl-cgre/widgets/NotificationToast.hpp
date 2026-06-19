#pragma once

#include <gtk/gtk.h>
#include <string>

namespace pcl {

// ============================================================================
// ToastConfig — 通知配置结构体
// ============================================================================

struct ToastConfig {
    std::string type     = "info";   // "info" / "warn" / "error" / "fatal"
    std::string title;               // 通知标题
    std::string desc;                // 通知描述
    bool        add_to_center = true;  // 超时后是否加入通知中心
    bool        can_clear      = true; // 是否可被"清理通知"垃圾桶按钮清除
    int         duration_ms    = 3000; // 倒计时 (毫秒, 默认 3 秒)
};

/**
 * 右下角弹窗通知 — 统一入口
 *
 * 进度条走完后自动缩回:
 *   - add_to_center=true  → 放入通知中心
 *   - add_to_center=false → 直接销毁
 */
void show_toast(GtkWindow* win, const ToastConfig& cfg);

/**
 * 兼容旧式调用 (内部委托给 show_toast)
 */
inline void show_notification_toast(GtkWindow* win,
                                    const char* type,
                                    const char* title,
                                    const char* desc,
                                    bool add_to_center = true)
{
    show_toast(win, {type ? type : "", title ? title : "",
                     desc ? desc : "", add_to_center});
}

}  // namespace pcl
