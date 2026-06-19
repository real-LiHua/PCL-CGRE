#include "core/BackgroundManager.hpp"

#include <gtk/gtk.h>
#include <gsk/gsk.h>
#include <string>
#include <cstdio>

/* ══════════════════════════════════════════════════════════════════════════
 * BlurBox — 容器 widget, snapshot 时对子控件叠加 GSK 高斯模糊
 * ══════════════════════════════════════════════════════════════════════════ */

extern "C" {

typedef struct _BlurBox {
    GtkWidget parent;
    float blur_radius;
} BlurBox;

typedef struct _BlurBoxClass {
    GtkWidgetClass parent_class;
} BlurBoxClass;

G_DEFINE_TYPE(BlurBox, blur_box, GTK_TYPE_WIDGET)

static void blur_box_snapshot(GtkWidget* widget, GtkSnapshot* snapshot)
{
    BlurBox* self = (BlurBox*) widget;
    float r = self->blur_radius;

    if (r <= 0.0f) {
        GTK_WIDGET_CLASS(blur_box_parent_class)->snapshot(widget, snapshot);
        return;
    }

    gtk_snapshot_push_blur(snapshot, r);
    GTK_WIDGET_CLASS(blur_box_parent_class)->snapshot(widget, snapshot);
    gtk_snapshot_pop(snapshot);
}

static void blur_box_class_init(BlurBoxClass* klass)
{
    GTK_WIDGET_CLASS(klass)->snapshot = blur_box_snapshot;
    gtk_widget_class_set_layout_manager_type(GTK_WIDGET_CLASS(klass), GTK_TYPE_BIN_LAYOUT);
}

static void blur_box_init(BlurBox* self)
{
    self->blur_radius = 0.0f;
}

static GtkWidget* blur_box_new(void)
{
    return GTK_WIDGET(g_object_new(blur_box_get_type(), nullptr));
}

static void blur_box_set_radius(GtkWidget* w, float radius)
{
    BlurBox* self = (BlurBox*) w;
    if (self->blur_radius == radius) return;
    self->blur_radius = radius;
    gtk_widget_queue_draw(w);
}

}  // extern "C"

namespace pcl {

static constexpr const char* kBgCssName = "main-bg-layer";

/* ── 加载图片 ────────────────────────────────────────────────────────── */
static GdkPaintable* load_bg_paintable(const std::string& path, bool is_url)
{
    if (path.empty()) return nullptr;
    GFile* file = is_url
        ? g_file_new_for_uri(path.c_str())
        : g_file_new_for_path(path.c_str());
    GError* err = nullptr;
    GdkTexture* tex = gdk_texture_new_from_file(file, &err);
    g_object_unref(file);
    if (err) { g_clear_error(&err); return nullptr; }
    return tex ? GDK_PAINTABLE(tex) : nullptr;
}

/* ── 公共 API ────────────────────────────────────────────────────────── */

void apply_background(GtkWidget* bg_overlay, bool reapply)
{
    // 视觉预览模式: 不应用任何背景设置，保持默认
    (void) bg_overlay;
    (void) reapply;
}

}  // namespace pcl
