#include "widgets/ResourceItem.hpp"
#include "pages/ResourceDetailPage.hpp"  // for navigate_to_resource_detail
#include "util/IconHelper.hpp"

namespace pcl {

/* ═══════════════════════════════════════════════════════════════════════
 * build_version_actions
 * ═══════════════════════════════════════════════════════════════════════ */

GtkWidget* build_version_actions()
{
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_widget_add_css_class(box, "version-actions");
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);

    struct { const char* icon; const char* tooltip; } actions[] = {
        {"save",        "将核心另存为……"},
        {"scroll-text", "更新日志"},
        {"server",      "下载服务端"},
    };

    for (auto& a : actions) {
        GtkWidget* btn = gtk_button_new();
        gtk_button_set_has_frame(GTK_BUTTON(btn), FALSE);
        gtk_widget_add_css_class(btn, "version-action-btn");
        gtk_widget_set_tooltip_text(btn, a.tooltip);
        GtkWidget* icn = icon::load(a.icon, 16);
        gtk_button_set_child(GTK_BUTTON(btn), icn);
        gtk_box_append(GTK_BOX(box), btn);
    }

    /* 拦截点击事件 — 防止冒泡触发父行导航 */
    GtkGesture* intercept = gtk_gesture_click_new();
    gtk_event_controller_set_propagation_phase(
        GTK_EVENT_CONTROLLER(intercept), GTK_PHASE_CAPTURE);
    g_signal_connect(intercept, "pressed",
        G_CALLBACK(+[](GtkGesture* g, int, double, double, gpointer) {
            gtk_gesture_set_state(g, GTK_EVENT_SEQUENCE_CLAIMED);
        }), nullptr);
    gtk_widget_add_controller(box, GTK_EVENT_CONTROLLER(intercept));

    return box;
}

/* ═══════════════════════════════════════════════════════════════════════
 * attach_row_hover
 * ═══════════════════════════════════════════════════════════════════════ */

void attach_row_hover(GtkWidget* row, GtkWidget* actions)
{
    g_object_set_data(G_OBJECT(row), "actions", actions);

    GtkEventController* motion = gtk_event_controller_motion_new();
    g_signal_connect(motion, "enter",
        G_CALLBACK(+[](GtkEventController* ctrl, double, double, gpointer) {
            GtkWidget* r = gtk_event_controller_get_widget(
                GTK_EVENT_CONTROLLER(ctrl));
            GtkWidget* a = static_cast<GtkWidget*>(
                g_object_get_data(G_OBJECT(r), "actions"));
            if (a) gtk_widget_add_css_class(a, "visible");
        }), nullptr);
    g_signal_connect(motion, "leave",
        G_CALLBACK(+[](GtkEventController* ctrl, gpointer) {
            GtkWidget* r = gtk_event_controller_get_widget(
                GTK_EVENT_CONTROLLER(ctrl));
            GtkWidget* a = static_cast<GtkWidget*>(
                g_object_get_data(G_OBJECT(r), "actions"));
            if (a) gtk_widget_remove_css_class(a, "visible");
        }), nullptr);
    gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(motion));
}

/* ═══════════════════════════════════════════════════════════════════════
 * build_resource_item
 * ═══════════════════════════════════════════════════════════════════════ */

GtkWidget* build_resource_item(const char*         fallback_icon,
                               const char*         title,
                               const char*         description,
                               const char*         version,
                               const char*         downloads,
                               const char*         update_time,
                               const char*         source,
                               ResourceItemData*   item_data,
                               const char*         subtitle,
                               const std::vector<const char*>& tags)
{
    (void) tags;  // tags no longer displayed per user preference
    using namespace pcl::icon;

    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_add_css_class(row, "resource-item");
    gtk_widget_set_margin_start(row, 6);
    gtk_widget_set_margin_end(row, 6);
    gtk_widget_set_margin_top(row, 3);
    gtk_widget_set_margin_bottom(row, 3);

    /* ── Logo: 50×50, 渲染到 50px 避免留白 ── */
    GtkWidget* logo = load(fallback_icon, 50);
    gtk_widget_set_valign(logo, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(logo, GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(logo, 50, 50);
    gtk_widget_set_hexpand(logo, FALSE);
    gtk_widget_set_vexpand(logo, FALSE);
    gtk_widget_add_css_class(logo, "resource-logo");
    gtk_box_append(GTK_BOX(row), logo);
    g_object_set_data(G_OBJECT(row), "logo", logo);

    /* ── 中间: 标题 + 描述 + 信息栏 ── */
    GtkWidget* mid = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    gtk_widget_set_hexpand(mid, TRUE);
    gtk_widget_set_valign(mid, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(row), mid);

    /* 标题行: [资源名 (semibold)] + [by 作者 (small, faded)] */
    {
        GtkWidget* title_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        gtk_widget_set_valign(title_row, GTK_ALIGN_CENTER);

        /* 资源名 */
        GtkWidget* title_lbl = gtk_label_new(title);
        gtk_label_set_xalign(GTK_LABEL(title_lbl), 0.0f);
        gtk_label_set_ellipsize(GTK_LABEL(title_lbl), PANGO_ELLIPSIZE_END);
        gtk_box_append(GTK_BOX(title_row), title_lbl);
        {
            PangoAttrList* attrs = pango_attr_list_new();
            pango_attr_list_insert(attrs,
                pango_attr_weight_new(PANGO_WEIGHT_SEMIBOLD));
            pango_attr_list_insert(attrs,
                pango_attr_size_new(12 * PANGO_SCALE));
            gtk_label_set_attributes(GTK_LABEL(title_lbl), attrs);
            pango_attr_list_unref(attrs);
        }

        /* by 作者 — 小字、低透明度，紧跟在标题后 */
        if (item_data && !item_data->author.empty()) {
            std::string byline = "by ";
            byline += item_data->author;
            GtkWidget* author_lbl = gtk_label_new(byline.c_str());
            gtk_label_set_xalign(GTK_LABEL(author_lbl), 0.0f);
            gtk_label_set_ellipsize(GTK_LABEL(author_lbl), PANGO_ELLIPSIZE_END);
            gtk_widget_set_opacity(author_lbl, 0.45);
            gtk_widget_set_valign(author_lbl, GTK_ALIGN_CENTER);
            gtk_box_append(GTK_BOX(title_row), author_lbl);
            {
                PangoAttrList* attrs = pango_attr_list_new();
                pango_attr_list_insert(attrs,
                    pango_attr_size_new(10 * PANGO_SCALE));
                gtk_label_set_attributes(GTK_LABEL(author_lbl), attrs);
                pango_attr_list_unref(attrs);
            }
        }

        gtk_box_append(GTK_BOX(mid), title_row);
    }

    /* 副标题 (原始英文名) — 仅当调用者显式传入 subtitle 时显示 */
    if (subtitle && *subtitle) {
        GtkWidget* sub_lbl = gtk_label_new(subtitle);
        gtk_label_set_xalign(GTK_LABEL(sub_lbl), 0.0f);
        gtk_label_set_ellipsize(GTK_LABEL(sub_lbl), PANGO_ELLIPSIZE_END);
        gtk_widget_set_opacity(sub_lbl, 0.40);
        gtk_box_append(GTK_BOX(mid), sub_lbl);
        {
            PangoAttrList* attrs = pango_attr_list_new();
            pango_attr_list_insert(attrs,
                pango_attr_size_new(10 * PANGO_SCALE));
            gtk_label_set_attributes(GTK_LABEL(sub_lbl), attrs);
            pango_attr_list_unref(attrs);
        }
    }

    /* 描述 */
    GtkWidget* desc_lbl = gtk_label_new(description);
    gtk_label_set_xalign(GTK_LABEL(desc_lbl), 0.0f);
    gtk_label_set_ellipsize(GTK_LABEL(desc_lbl), PANGO_ELLIPSIZE_END);
    gtk_widget_set_opacity(desc_lbl, 0.55);
    gtk_box_append(GTK_BOX(mid), desc_lbl);
    {
        PangoAttrList* attrs = pango_attr_list_new();
        pango_attr_list_insert(attrs,
            pango_attr_size_new(10 * PANGO_SCALE));
        gtk_label_set_attributes(GTK_LABEL(desc_lbl), attrs);
        pango_attr_list_unref(attrs);
    }

    /* 信息栏 */
    {
        GtkWidget* info = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_widget_set_valign(info, GTK_ALIGN_CENTER);

        struct InfoCol { const char* icon; const char* text; };
        const InfoCol cols[] = {
            {"layout-list",         version},
            {"download",            downloads},
            {"arrow-up-from-line",  update_time},
            {"globe",               source},
        };
        for (auto& c : cols) {
            GtkWidget* pair = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
            GtkWidget* icn = load(c.icon, 11);
            gtk_widget_set_opacity(icn, 0.45);
            gtk_widget_set_valign(icn, GTK_ALIGN_CENTER);
            gtk_box_append(GTK_BOX(pair), icn);

            GtkWidget* txt = gtk_label_new(c.text);
            gtk_widget_set_opacity(txt, 0.50);
            gtk_widget_set_valign(txt, GTK_ALIGN_CENTER);
            gtk_box_append(GTK_BOX(pair), txt);
            {
                PangoAttrList* attrs = pango_attr_list_new();
                pango_attr_list_insert(attrs,
                    pango_attr_size_new(8 * PANGO_SCALE));
                gtk_label_set_attributes(GTK_LABEL(txt), attrs);
                pango_attr_list_unref(attrs);
            }
            gtk_box_append(GTK_BOX(info), pair);
        }
        gtk_box_append(GTK_BOX(mid), info);
    }

    /* ── 右侧操作按钮 (hover 显示) ── */
    {
        GtkWidget* actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
        gtk_widget_add_css_class(actions, "version-actions");
        gtk_widget_set_valign(actions, GTK_ALIGN_CENTER);

        GtkWidget* fav_btn = gtk_button_new();
        gtk_button_set_has_frame(GTK_BUTTON(fav_btn), FALSE);
        gtk_widget_add_css_class(fav_btn, "version-action-btn");
        gtk_widget_set_tooltip_text(fav_btn, "收藏");
        gtk_button_set_child(GTK_BUTTON(fav_btn), load("heart", 15));
        gtk_box_append(GTK_BOX(actions), fav_btn);

        GtkWidget* dl_btn = gtk_button_new();
        gtk_button_set_has_frame(GTK_BUTTON(dl_btn), FALSE);
        gtk_widget_add_css_class(dl_btn, "version-action-btn");
        gtk_widget_set_tooltip_text(dl_btn, "下载");
        gtk_button_set_child(GTK_BUTTON(dl_btn), load("download", 15));
        gtk_box_append(GTK_BOX(actions), dl_btn);

        gtk_box_append(GTK_BOX(row), actions);
        attach_row_hover(row, actions);

        /* 拦截点击事件 — 防止冒泡触发父行导航 */
        GtkGesture* intercept2 = gtk_gesture_click_new();
        gtk_event_controller_set_propagation_phase(
            GTK_EVENT_CONTROLLER(intercept2), GTK_PHASE_CAPTURE);
        g_signal_connect(intercept2, "pressed",
            G_CALLBACK(+[](GtkGesture* g, int, double, double, gpointer) {
                gtk_gesture_set_state(g, GTK_EVENT_SEQUENCE_CLAIMED);
            }), nullptr);
        gtk_widget_add_controller(actions, GTK_EVENT_CONTROLLER(intercept2));
    }

    /* ── 存储完整数据 + 点击跳转详情页 ── */
    if (item_data) {
        auto* data = new ResourceItemData(std::move(*item_data));
        g_object_set_data_full(G_OBJECT(row), "item-data", data,
                               [](void* p) { delete static_cast<ResourceItemData*>(p); });

        GtkGesture* click = gtk_gesture_click_new();
        g_signal_connect(click, "pressed",
            G_CALLBACK(+[](GtkGesture* g, int, double, double, gpointer) {
                GtkWidget* target = gtk_event_controller_get_widget(
                    GTK_EVENT_CONTROLLER(g));
                auto* d = static_cast<ResourceItemData*>(
                    g_object_get_data(G_OBJECT(target), "item-data"));
                if (!d) return;
                pcl::navigate_to_resource_detail(
                    target,
                    d->title,
                    d->description,
                    d->source,
                    d->version_range,
                    d->download_count,
                    d->date_modified,
                    d->icon_url,
                    d->project_id,
                    d->author,
                    d->license_name,
                    d->project_url,
                    d->wiki_url,
                    d->source_url,
                    d->categories,
                    d->game_versions,
                    d->followers);
            }), nullptr);
        gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(click));
    }

    return row;
}

}  // namespace pcl
