#pragma once

#include <gtk/gtk.h>

namespace pcl {

/**
 * 构建更多页面 — 左侧导航 + 右侧 GtkStack 子页面
 *
 * 侧边栏 2 项:
 *   帮助:   帮助 (静态帮助链接页面)
 *   奇妙小工具: 工具箱 (实用工具)
 */
GtkWidget* build_more_page();

}  // namespace pcl
