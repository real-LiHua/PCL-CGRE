#pragma once

#include <gtk/gtk.h>
#include <adwaita.h>

namespace pcl {

/**
 * 构建标题栏页签组 — 4 个互斥按钮 (启动/下载/设置/更多)
 * @param stack 目标 AdwViewStack
 * @return GtkBox 包含 4 个互斥 GtkButton 页签
 */
GtkWidget* build_header_tabs(AdwViewStack* stack);

}  // namespace pcl
