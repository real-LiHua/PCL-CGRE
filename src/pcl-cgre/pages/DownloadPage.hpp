#pragma once

#include <gtk/gtk.h>

namespace pcl {

/**
 * 构建"下载"页面 — 左侧导航 + GtkStack 右侧内容
 */
GtkWidget* build_download_page();

/**
 * 触发下载页 MC 版本列表拉取 (供 MainWindow 标签切换时自动调用)
 * @param download_page build_download_page() 返回的页面 widget
 */
void trigger_download_page_mc_fetch(GtkWidget* download_page);

}  // namespace pcl
