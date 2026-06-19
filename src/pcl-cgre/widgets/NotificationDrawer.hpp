#pragma once

#include <gtk/gtk.h>

namespace pcl {

/**
 * 构建通知抽屉 (左侧滑出面板)
 *
 * 返回外层 GtkBox，其中包含:
 *   - GtkRevealer (滑出动画)
 *   - 通知面板 (标题 + 清空按钮 + 关闭按钮 + 通知列表)
 *
 * 在 outer widget 上存储 "notif-list" (GtkListBox*),
 * 供 NotificationToast 结束后添加通知条目。
 */
GtkWidget* build_notification_drawer();

}  // namespace pcl
