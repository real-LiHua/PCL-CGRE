#include "core/FrameLimiter.hpp"

#include <gtk/gtk.h>

namespace pcl {

void apply_fps_limit(GtkWindow* window, int fps)
{
    if (!window) return;

    GdkFrameClock* clock = gtk_widget_get_frame_clock(GTK_WIDGET(window));
    if (!clock) return;

    // 清理旧定时器
    guint old_id = GPOINTER_TO_UINT(
        g_object_get_data(G_OBJECT(window), "fps-pulse-id"));
    if (old_id) {
        g_source_remove(old_id);
        g_object_set_data(G_OBJECT(window), "fps-pulse-id", nullptr);
    }

    // 无限制: 保持帧时钟活跃
    if (fps >= 360 || fps <= 0) {
        gdk_frame_clock_begin_updating(clock);
        return;
    }

    // 脉冲机制: 每帧后暂停帧时钟, 定时器到期后恢复一帧
    guint interval_ms = 1000 / static_cast<guint>(fps);
    if (interval_ms < 5) interval_ms = 5;

    guint pulse_id = g_timeout_add(interval_ms, [](gpointer user) -> gboolean {
        auto* clk = static_cast<GdkFrameClock*>(user);
        gdk_frame_clock_begin_updating(clk);
        gdk_frame_clock_end_updating(clk);
        return G_SOURCE_CONTINUE;
    }, clock);

    g_object_set_data(G_OBJECT(window), "fps-pulse-id",
                      GUINT_TO_POINTER(pulse_id));

    // 初始启动
    gdk_frame_clock_begin_updating(clock);
}

}  // namespace pcl
