#include "widgets/HeaderTabs.hpp"
#include "util/IconHelper.hpp"

#include <string>
#include <adwaita.h>

namespace pcl {

namespace {

/**
 * 构建单个标题栏页签按钮 — 匹配 PCL-CE MyRadioButton (White 主题)
 */
GtkWidget* build_header_tab(const char* label,
                            const char* lucide,
                            const char* page_name,
                            AdwViewStack* stack,
                            bool active)
{
    GtkWidget* btn = gtk_button_new();
    gtk_button_set_has_frame(GTK_BUTTON(btn), FALSE);
    gtk_widget_add_css_class(btn, "header-tab");
    if (active)
        gtk_widget_add_css_class(btn, "header-tab-active");

    /* 内容: 图标 + 文字 */
    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_halign(hbox, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(hbox, GTK_ALIGN_CENTER);

    if (active)
        gtk_box_append(GTK_BOX(hbox), icon::load(lucide, 16));
    else
        gtk_box_append(GTK_BOX(hbox),
                       icon::load((std::string(lucide) + "-light").c_str(), 16));

    GtkWidget* lbl = gtk_label_new(label);
    gtk_box_append(GTK_BOX(hbox), lbl);
    gtk_button_set_child(GTK_BUTTON(btn), hbox);

    /* 储存数据供回调读取 — use g_intern_string for fast key lookup */
    g_object_set_data_full(G_OBJECT(btn), "page-name",
                           g_strdup(page_name), g_free);
    g_object_set_data(G_OBJECT(btn), "view-stack", stack);
    g_object_set_data_full(G_OBJECT(btn), "lucide-icon",
                           g_strdup(lucide), g_free);

    /* 储存 GtkBox 引用，避免 widget tree 遍历 */
    g_object_set_data(G_OBJECT(btn), "icon-box", hbox);

    /* 点击: 互斥切换 + 切页 + 仅更新前后两个活跃页签的图标 */
    g_signal_connect(btn, "clicked", G_CALLBACK(+[](GtkWidget* clicked, gpointer) {
        GtkWidget* bar = gtk_widget_get_parent(clicked);

        /* 找到之前活跃的页签 */
        GtkWidget* prev_active = nullptr;
        for (GtkWidget* sib = gtk_widget_get_first_child(bar);
             sib != nullptr;
             sib = gtk_widget_get_next_sibling(sib))
        {
            if (sib != clicked && gtk_widget_has_css_class(sib, "header-tab-active")) {
                prev_active = sib;
                break;
            }
        }

        /* β 快速路径: 如果点击的是已活跃的页签，不处理 */
        if (prev_active == nullptr && gtk_widget_has_css_class(clicked, "header-tab-active"))
            return;

        /* 1. 恢复前一个活跃页签的图标为 light 变体 */
        if (prev_active) {
            gtk_widget_remove_css_class(prev_active, "header-tab-active");
            const char* prev_lucide = static_cast<const char*>(
                g_object_get_data(G_OBJECT(prev_active), "lucide-icon"));
            if (prev_lucide) {
                GtkWidget* icon_box = static_cast<GtkWidget*>(
                    g_object_get_data(G_OBJECT(prev_active), "icon-box"));
                if (icon_box) {
                    GtkWidget* old_icon = gtk_widget_get_first_child(icon_box);
                    if (old_icon) {
                        std::string var(prev_lucide);
                        var += "-light";
                        GtkWidget* new_icon = icon::load(var.c_str(), 16);
                        gtk_widget_insert_after(new_icon, icon_box, nullptr);
                        gtk_box_remove(GTK_BOX(icon_box), old_icon);
                    }
                }
            }
        }

        /* 2. 设置点击的页签为活跃态，恢复实色图标 */
        gtk_widget_add_css_class(clicked, "header-tab-active");
        {
            const char* clicked_lucide = static_cast<const char*>(
                g_object_get_data(G_OBJECT(clicked), "lucide-icon"));
            if (clicked_lucide) {
                GtkWidget* icon_box = static_cast<GtkWidget*>(
                    g_object_get_data(G_OBJECT(clicked), "icon-box"));
                if (icon_box) {
                    GtkWidget* old_icon = gtk_widget_get_first_child(icon_box);
                    if (old_icon) {
                        GtkWidget* new_icon = icon::load(clicked_lucide, 16);
                        gtk_widget_insert_after(new_icon, icon_box, nullptr);
                        gtk_box_remove(GTK_BOX(icon_box), old_icon);
                    }
                }
            }
        }

        /* 3. 切换视图 */
        auto* stk = static_cast<AdwViewStack*>(
            g_object_get_data(G_OBJECT(clicked), "view-stack"));
        const char* name = static_cast<const char*>(
            g_object_get_data(G_OBJECT(clicked), "page-name"));
        adw_view_stack_set_visible_child_name(stk, name);
    }), nullptr);

    return btn;
}

}  // anonymous namespace

GtkWidget* build_header_tabs(AdwViewStack* stack)
{
    GtkWidget* bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_valign(bar, GTK_ALIGN_CENTER);

    struct TabInfo { const char* label; const char* icon; const char* page; };
    const TabInfo tabs[] = {
        {"启动", "play",     "launch"},
        {"下载", "download", "download"},
        {"设置", "settings", "settings"},
        {"更多", "wrench",   "more"},
    };

    for (int i = 0; i < 4; i++) {
        GtkWidget* tab = build_header_tab(
            tabs[i].label, tabs[i].icon, tabs[i].page, stack, i == 0);
        gtk_box_append(GTK_BOX(bar), tab);
    }

    return bar;
}

}  // namespace pcl
