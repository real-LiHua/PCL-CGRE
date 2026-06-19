#include "app/MainWindow.hpp"
#include "pages/ResourceDetailPage.hpp"
#include "core/Styles.hpp"
#include "core/BackgroundManager.hpp"
#include "core/FrameLimiter.hpp"
#include "pages/LaunchPage.hpp"
#include "pages/SettingsPage.hpp"
#include "pages/MorePage.hpp"
#include "widgets/HeaderTabs.hpp"
#include "widgets/NotificationToast.hpp"
#include "widgets/NotificationDrawer.hpp"
#include "pages/DownloadPage.hpp"
#include "core/Colors.hpp"
#include "util/IconHelper.hpp"
#include "network/McVersionFetcher.hpp"
#include "network/ResourceFetcher.hpp"
#include "core/Log.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <adwaita.h>

namespace pcl {


GtkWidget* create_main_window(GtkApplication* app)
{
    /* ---- 窗口 ---- */
    GtkWidget* window = adw_application_window_new(app);
    gtk_widget_add_css_class(window, "pcl-app");
    gtk_window_set_title(GTK_WINDOW(window), "PCL-CGRE");
    gtk_window_set_default_size(GTK_WINDOW(window), 1050, 650);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

    /* ---- 应用帧率限制 (默认 60fps) ---- */
    apply_fps_limit(GTK_WINDOW(window), 60);

    /* ---- GtkOverlay: 主内容 + 抽屉覆盖层 ---- */
    GtkWidget* overlay = gtk_overlay_new();
    adw_application_window_set_content(ADW_APPLICATION_WINDOW(window), overlay);
    g_object_set_data(G_OBJECT(window), "main-overlay", overlay);

    /* ---- 内层 Overlay: 背景层(底) + UI 内容(上) 的 z-order 控制 ---- */
    GtkWidget* bg_overlay = gtk_overlay_new();
    gtk_widget_set_hexpand(bg_overlay, TRUE);
    gtk_widget_set_vexpand(bg_overlay, TRUE);
    gtk_overlay_set_child(GTK_OVERLAY(overlay), bg_overlay);
    g_object_set_data(G_OBJECT(window), "bg-overlay", bg_overlay);

    /* ---- ToolbarView 布局容器 (bg_overlay 上层) ---- */
    GtkWidget* toolbar_view = adw_toolbar_view_new();
    gtk_widget_set_hexpand(toolbar_view, TRUE);
    gtk_widget_set_vexpand(toolbar_view, TRUE);
    gtk_overlay_add_overlay(GTK_OVERLAY(bg_overlay), toolbar_view);

    /* ---- 应用背景设置 ---- */
    apply_background(bg_overlay);

    /* ---- Header bar (48px, 匹配 PCL-CE 标题栏) ---- */
    GtkWidget* header = adw_header_bar_new();
    adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar_view), header);

    /* 隐藏默认窗口控件, 改用自定义 Lucide 图标按钮 */
    adw_header_bar_set_show_end_title_buttons(ADW_HEADER_BAR(header), FALSE);

    /* 关闭按钮 — pack_end 先添加的在最右侧 */
    {
        GtkWidget* close_btn = gtk_button_new();
        gtk_button_set_has_frame(GTK_BUTTON(close_btn), FALSE);
        gtk_widget_add_css_class(close_btn, "window-ctrl-btn");
        gtk_widget_add_css_class(close_btn, "window-ctrl-close");
        gtk_button_set_child(GTK_BUTTON(close_btn),
                             icon::load("x-light", 18));
        gtk_widget_set_tooltip_text(close_btn, "关闭");
        g_signal_connect(close_btn, "clicked", G_CALLBACK(+[](GtkWidget* btn, gpointer) {
            gtk_window_close(GTK_WINDOW(gtk_widget_get_root(btn)));
        }), nullptr);
        adw_header_bar_pack_end(ADW_HEADER_BAR(header), close_btn);
    }

    /* 最小化按钮 */
    {
        GtkWidget* min_btn = gtk_button_new();
        gtk_button_set_has_frame(GTK_BUTTON(min_btn), FALSE);
        gtk_widget_add_css_class(min_btn, "window-ctrl-btn");
        gtk_button_set_child(GTK_BUTTON(min_btn),
                             icon::load("minus-light", 18));
        gtk_widget_set_tooltip_text(min_btn, "最小化");
        g_signal_connect(min_btn, "clicked", G_CALLBACK(+[](GtkWidget* btn, gpointer) {
            gtk_window_minimize(GTK_WINDOW(gtk_widget_get_root(btn)));
        }), nullptr);
        adw_header_bar_pack_end(ADW_HEADER_BAR(header), min_btn);
    }

    /* 应用标题按钮 (左侧, 点击打开通知抽屉)
     * 进入详情页时隐藏, 返回时恢复 */
    {
        GtkWidget* title_btn = gtk_button_new();
        gtk_button_set_has_frame(GTK_BUTTON(title_btn), FALSE);
        gtk_widget_add_css_class(title_btn, "app-title-btn");
        gtk_button_set_child(GTK_BUTTON(title_btn),
                             gtk_label_new("PCL-CGRE"));
        adw_header_bar_pack_start(ADW_HEADER_BAR(header), title_btn);
        g_object_set_data(G_OBJECT(window), "app-title", title_btn);

        /* 点击切换抽屉 */
        g_signal_connect(title_btn, "clicked", G_CALLBACK(+[](GtkWidget* btn, gpointer) {
            GtkWidget* win = GTK_WIDGET(gtk_widget_get_root(btn));
            GtkWidget* rev = static_cast<GtkWidget*>(
                g_object_get_data(G_OBJECT(win), "notif-revealer"));
            GtkWidget* bd = static_cast<GtkWidget*>(
                g_object_get_data(G_OBJECT(win), "notif-backdrop"));
            GtkWidget* outer = static_cast<GtkWidget*>(
                g_object_get_data(G_OBJECT(win), "notif-outer"));
            if (!rev) return;

            int open = GPOINTER_TO_INT(
                g_object_get_data(G_OBJECT(win), "notif-open"));
            if (open) {
                gtk_revealer_set_reveal_child(GTK_REVEALER(rev), FALSE);
                g_object_set_data(G_OBJECT(win), "notif-open", GINT_TO_POINTER(0));
                if (bd) gtk_widget_set_visible(bd, FALSE);
                if (outer) gtk_widget_set_can_target(outer, FALSE);
            } else {
                gtk_revealer_set_reveal_child(GTK_REVEALER(rev), TRUE);
                g_object_set_data(G_OBJECT(win), "notif-open", GINT_TO_POINTER(1));
                if (bd) gtk_widget_set_visible(bd, TRUE);
                if (outer) gtk_widget_set_can_target(outer, TRUE);
            }
        }), nullptr);
    }

    /* ---- View stack ---- */
    AdwViewStack* stack = ADW_VIEW_STACK(adw_view_stack_new());
    adw_view_stack_set_hhomogeneous(stack, FALSE);
    adw_view_stack_set_vhomogeneous(stack, FALSE);
    adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar_view), GTK_WIDGET(stack));

    /* ---- 自定义标题栏页签 (替代 AdwViewSwitcher) ---- */
    GtkWidget* header_tabs = build_header_tabs(stack);
    g_object_ref(header_tabs);  // keep alive when replaced by back_nav
    adw_header_bar_set_title_widget(ADW_HEADER_BAR(header), header_tabs);

    /* 详情页返回导航栏 — 初始隐藏, 进入资源详情时替换 header_tabs */
    GtkWidget* back_nav = build_back_nav();
    g_object_ref(back_nav);  // keep alive while detached
    g_object_set_data(G_OBJECT(window), "header-tabs", header_tabs);
    g_object_set_data(G_OBJECT(window), "back-nav", back_nav);
    g_object_set_data(G_OBJECT(window), "header-bar", header);
    g_object_set_data(G_OBJECT(window), "main-stack", stack);

    /* 返回按钮 → 退出详情页, 恢复标题栏页签 */
    {
        GtkWidget* back_btn = static_cast<GtkWidget*>(
            g_object_get_data(G_OBJECT(back_nav), "back-btn"));
        g_signal_connect(back_btn, "clicked", G_CALLBACK(+[](GtkWidget* btn, gpointer) {
            GObject* win = G_OBJECT(gtk_widget_get_root(btn));
            auto* hdr = static_cast<GtkWidget*>(
                g_object_get_data(win, "header-bar"));
            auto* tabs = static_cast<GtkWidget*>(
                g_object_get_data(win, "header-tabs"));
            auto* main_stk = static_cast<AdwViewStack*>(
                g_object_get_data(win, "main-stack"));

            /* 清除详情页状态: 移除左侧返回导航, 恢复居中页签 + "PCL-CGRE" */
            auto* bn = static_cast<GtkWidget*>(
                g_object_get_data(win, "back-nav"));
            if (bn && gtk_widget_get_parent(bn))
                adw_header_bar_remove(ADW_HEADER_BAR(hdr), bn);

            if (tabs)
                gtk_widget_set_visible(tabs, TRUE);

            GtkWidget* app_title = static_cast<GtkWidget*>(
                g_object_get_data(win, "app-title"));
            if (app_title && !gtk_widget_get_parent(app_title)) {
                adw_header_bar_pack_start(ADW_HEADER_BAR(hdr), app_title);
                g_object_unref(app_title);  // paired with g_object_ref in navigate_*
            }

            /* 切回下载页的内部 stack */
            GtkWidget* dl_page = adw_view_stack_get_child_by_name(main_stk, "download");
            if (dl_page) {
                /* 恢复左侧导航栏 + 分隔线 */
                GtkWidget* sidebar = static_cast<GtkWidget*>(
                    g_object_get_data(G_OBJECT(dl_page), "dl-sidebar"));
                GtkWidget* sep = static_cast<GtkWidget*>(
                    g_object_get_data(G_OBJECT(dl_page), "dl-sep"));
                if (sidebar)
                    gtk_widget_set_visible(sidebar, TRUE);
                if (sep)
                    gtk_widget_set_visible(sep, TRUE);

                auto* dl_stk = static_cast<GtkWidget*>(
                    g_object_get_data(G_OBJECT(dl_page), "dl-stack"));
                auto* prev_page = static_cast<const char*>(
                    g_object_get_data(G_OBJECT(dl_page), "prev-page"));
                if (dl_stk && prev_page)
                    gtk_stack_set_visible_child_name(GTK_STACK(dl_stk), prev_page);
            }
        }), nullptr);
    }

    /* ---- 页面 ---- */

    /* 启动页 — 两栏 PGTKDesign 布局 */
    GtkWidget* launch_page = build_launch_page();
    adw_view_stack_add_titled(stack, launch_page, "launch", "启动");

    /* 下载页 — 搜索框 + 卡片列表 */
    GtkWidget* download_page = build_download_page();
    adw_view_stack_add_titled(stack, download_page, "download", "下载");

    /* ★ 用户切换到下载页时自动触发版本列表拉取 */
    g_signal_connect(stack, "notify::visible-child-name",
        G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer) {
            AdwViewStack* stk = ADW_VIEW_STACK(obj);
            const char* name = adw_view_stack_get_visible_child_name(stk);
            if (!name || strcmp(name, "download") != 0) return;

            GtkWidget* page = adw_view_stack_get_child_by_name(stk, "download");
            if (!page) return;
            trigger_download_page_mc_fetch(page);
        }), nullptr);

    /* 设置页 — 完整设置界面 */
    GtkWidget* settings_page = build_settings_page();
    adw_view_stack_add_titled(stack, settings_page, "settings", "设置");

    /* 更多 — 帮助与工具箱 */
    GtkWidget* more_page = build_more_page();
    adw_view_stack_add_titled(stack, more_page, "more", "更多");

    /* ---- 通知抽屉 (左侧滑出) ---- */
    {
        /* 半透明背景遮罩 — 抽屉打开时显示, 点击关闭抽屉 */
        GtkWidget* backdrop = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_add_css_class(backdrop, "notif-backdrop");
        gtk_widget_set_visible(backdrop, FALSE);
        gtk_widget_set_can_target(backdrop, TRUE);
        gtk_widget_set_hexpand(backdrop, TRUE);
        gtk_widget_set_vexpand(backdrop, TRUE);
        gtk_overlay_add_overlay(GTK_OVERLAY(overlay), backdrop);
        g_object_set_data(G_OBJECT(window), "notif-backdrop", backdrop);

        /* 点击遮罩 → 关闭抽屉 */
        GtkGesture* bg_click = gtk_gesture_click_new();
        g_signal_connect(bg_click, "pressed", G_CALLBACK(+[](GtkGesture* g, int, double, double, gpointer) {
            GtkWidget* target = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(g));
            GtkWidget* win = GTK_WIDGET(gtk_widget_get_root(target));
            GtkWidget* rev = static_cast<GtkWidget*>(
                g_object_get_data(G_OBJECT(win), "notif-revealer"));
            GtkWidget* bd = static_cast<GtkWidget*>(
                g_object_get_data(G_OBJECT(win), "notif-backdrop"));
            GtkWidget* outer = static_cast<GtkWidget*>(
                g_object_get_data(G_OBJECT(win), "notif-outer"));
            if (rev) {
                gtk_revealer_set_reveal_child(GTK_REVEALER(rev), FALSE);
                g_object_set_data(G_OBJECT(win), "notif-open", GINT_TO_POINTER(0));
            }
            if (bd)
                gtk_widget_set_visible(bd, FALSE);
            if (outer)
                gtk_widget_set_can_target(outer, FALSE);
        }), nullptr);
        gtk_widget_add_controller(backdrop, GTK_EVENT_CONTROLLER(bg_click));

        /* 抽屉面板 */
        GtkWidget* drawer = build_notification_drawer();
        gtk_overlay_add_overlay(GTK_OVERLAY(overlay), drawer);

        /* 提取 revealer + outer 存储到 window */
        GtkWidget* revealer = gtk_widget_get_first_child(drawer);
        if (revealer)
            g_object_set_data(G_OBJECT(window), "notif-revealer", revealer);
        g_object_set_data(G_OBJECT(window), "notif-outer", drawer);

        /* 提取通知列表 (供 toast 结束后添加条目) */
        GtkWidget* notif_list = static_cast<GtkWidget*>(
            g_object_get_data(G_OBJECT(drawer), "notif-list"));
        if (notif_list)
            g_object_set_data(G_OBJECT(window), "notif-list", notif_list);
    }

    /* 注册 Toast 通知函数 (供各页面按钮调用)
     *   通过 uintptr_t 中转以规避 void* ↔ 函数指针转换问题 */
    /* ---- 加载全局 CSS (视觉预览模式) ---- */
    load_stylesheet_default();
    gtk_widget_queue_draw(window);

    return window;
}

}  // namespace pcl
