#pragma once

#include <gtk/gtk.h>

namespace pcl {

/**
 * 应用帧率限制到窗口。
 *
 * GTK4 移除了 gdk_frame_clock_set_fps(), 但可以通过
 * begin_updating() / end_updating() 脉冲控制帧时钟。
 *
 * 原理:
 *   - 帧时钟仅在 begin_updating → end_updating 区间内驱动渲染
 *   - 使用 g_timeout_add() 在目标 FPS 间隔触发短暂脉冲
 *   - 360 FPS 以上视为无限制 (不限制)
 *
 * @param window  GtkWindow
 * @param fps     目标帧率 (30–360, ≥360 = 无限制)
 */
void apply_fps_limit(GtkWindow* window, int fps);

}  // namespace pcl
