#pragma once

#include <gtk/gtk.h>

namespace pcl {

/**
 * 将背景设置应用到主窗口的 GtkOverlay。
 *
 * 在 overlay 最底层插入 GtkPicture 作为背景层，根据 LauncherSettings 中的
 * bg_image_type / bg_image_path / bg_image_url / bg_paint_color 选择图片来源。
 *
 * @param overlay  主窗口的 GtkOverlay (来自 create_main_window)
 * @param reapply  如果已有背景层则重新应用 (true), 否则创建新的 (false)
 */
void apply_background(GtkWidget* overlay, bool reapply = false);

}  // namespace pcl
