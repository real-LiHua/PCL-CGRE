#include "pages/LaunchPage.hpp"
#include "core/Colors.hpp"
#include "util/IconHelper.hpp"
#include "widgets/NotificationToast.hpp"
#include "pclcore/pclcore.hpp"

#include <cstdint>
#include <string>
#include <adwaita.h>

namespace pcl {

using namespace pcl::colors;

/* ============================================================================
 *  PangoAttrList 缓存 — 避免重复创建销毁常用属性
 * ============================================================================ */

namespace {

/** Get or create a cached PangoAttrList with a single weight attribute. */
PangoAttrList* cached_weight_attr(PangoWeight weight)
{
    static PangoAttrList* bold   = []() { auto* a = pango_attr_list_new();
        pango_attr_list_insert(a, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
        return a; }();
    static PangoAttrList* semibold = []() { auto* a = pango_attr_list_new();
        pango_attr_list_insert(a, pango_attr_weight_new(PANGO_WEIGHT_SEMIBOLD));
        return a; }();
    static PangoAttrList* normal = []() { auto* a = pango_attr_list_new();
        pango_attr_list_insert(a, pango_attr_weight_new(PANGO_WEIGHT_NORMAL));
        return a; }();

    switch (weight) {
        case PANGO_WEIGHT_BOLD:     return bold;
        case PANGO_WEIGHT_SEMIBOLD: return semibold;
        default:                    return normal;
    }
}

/** Apply cached bold weight attr to a label. */
inline void set_label_bold(GtkWidget* label)
{
    gtk_label_set_attributes(GTK_LABEL(label), cached_weight_attr(PANGO_WEIGHT_BOLD));
}

/** Apply cached semibold weight attr to a label. */
inline void set_label_semibold(GtkWidget* label)
{
    gtk_label_set_attributes(GTK_LABEL(label), cached_weight_attr(PANGO_WEIGHT_SEMIBOLD));
}

/** Build a PangoAttrList with both weight and size in one shot. */
PangoAttrList* build_weight_size_attr(PangoWeight weight, int size_pt)
{
    auto* a = pango_attr_list_new();
    pango_attr_list_insert(a, pango_attr_weight_new(weight));
    pango_attr_list_insert(a, pango_attr_size_new(size_pt * PANGO_SCALE));
    return a;
}

/** Pre-built scale=0.9 attr for subtitle labels. */
static PangoAttrList* subtitle_scale_attr()
{
    static PangoAttrList* a = []() {
        auto* attr = pango_attr_list_new();
        pango_attr_list_insert(attr, pango_attr_scale_new(0.9));
        return attr;
    }();
    return a;
}

}  // anonymous namespace

/* ── Blueprint 路径解析 ── */

/* ============================================================================
 * 内部辅助 — 匿名命名空间
 * ============================================================================ */

namespace {
// (currently empty — all helpers are public builders)
}  // anonymous namespace

/* ============================================================================
 * build_hint_bar — 提示条 (匹配 PCL-CE MyHint)
 *   左侧 3px 红色边框 + 文字 + 可选关闭按钮
 * ============================================================================ */

GtkWidget* build_hint_bar(const char* text, bool show_close)
{
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(box, "hint-bar");

    /* 文字 */
    GtkWidget* label = gtk_label_new(text);
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_box_append(GTK_BOX(box), label);

    /* 关闭按钮 — lucide/x (matches PCL-CE MyHint) */
    if (show_close) {
        GtkWidget* close_btn = gtk_button_new();
        gtk_button_set_has_frame(GTK_BUTTON(close_btn), FALSE);
        gtk_button_set_child(GTK_BUTTON(close_btn), icon::load("x", 16));
        gtk_widget_add_css_class(close_btn, "flat");
        gtk_widget_set_valign(close_btn, GTK_ALIGN_CENTER);
        gtk_widget_set_size_request(close_btn, 24, 24);
        gtk_widget_set_tooltip_text(close_btn, "不再显示");
        gtk_widget_set_margin_start(close_btn, 8);
        gtk_box_append(GTK_BOX(box), close_btn);
    }

    return box;
}

/* ============================================================================
 * build_search_box — 搜索框 (匹配 PCL-CE MySearchBox)
 *   图标 + Entry + 清除按钮
 * ============================================================================ */

GtkWidget* build_search_box()
{
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_add_css_class(box, "search-box");

    /* 搜索图标 — lucide/search */
    GtkWidget* search_icon = icon::load("search", 18);
    gtk_widget_set_opacity(search_icon, 0.5);
    gtk_widget_set_valign(search_icon, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), search_icon);

    /* 输入框 */
    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "搜索…");
    gtk_entry_set_max_length(GTK_ENTRY(entry), 50);
    gtk_widget_set_hexpand(entry, TRUE);
    gtk_widget_set_valign(entry, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), entry);

    /* 清除按钮 — lucide/x (matches PCL-CE MySearchBox) */
    GtkWidget* clear_btn = gtk_button_new();
    gtk_button_set_has_frame(GTK_BUTTON(clear_btn), FALSE);
    gtk_button_set_child(GTK_BUTTON(clear_btn), icon::load("x", 14));
    gtk_widget_add_css_class(clear_btn, "flat");
    gtk_widget_set_size_request(clear_btn, 20, 20);
    gtk_widget_set_valign(clear_btn, GTK_ALIGN_CENTER);
    gtk_widget_set_opacity(clear_btn, 0.0);
    gtk_box_append(GTK_BOX(box), clear_btn);

    return box;
}

/* ============================================================================
 * build_news_card — 增强版新闻卡片 (匹配 PCL-CE 卡片风格)
 *   卡片容器 + 图标 + 标题 + 副标题
 * ============================================================================ */

GtkWidget* build_news_card(const char* title,
                           const char* subtitle,
                           const char* icon_name)
{
    /* 卡片容器 */
    GtkWidget* card = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_add_css_class(card, "card");

    /* 图标 — Lucide SVG, 32px (matches GTK_ICON_SIZE_LARGE) */
    if (icon_name) {
        GtkWidget* icon = icon::load(icon_name, 32);
        gtk_widget_set_valign(icon, GTK_ALIGN_START);
        gtk_widget_set_margin_top(icon, 2);
        gtk_box_append(GTK_BOX(card), icon);
    }

    /* 文字区域 */
    GtkWidget* text_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_hexpand(text_box, TRUE);
    gtk_box_append(GTK_BOX(card), text_box);

    /* 标题 */
    GtkWidget* title_label = gtk_label_new(title);
    gtk_widget_add_css_class(title_label, "card-title");
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.0f);
    gtk_label_set_ellipsize(GTK_LABEL(title_label), PANGO_ELLIPSIZE_END);
    gtk_box_append(GTK_BOX(text_box), title_label);

    /* 副标题 */
    GtkWidget* subtitle_label = gtk_label_new(subtitle);
    gtk_label_set_xalign(GTK_LABEL(subtitle_label), 0.0f);
    gtk_label_set_ellipsize(GTK_LABEL(subtitle_label), PANGO_ELLIPSIZE_END);
    gtk_widget_set_opacity(subtitle_label, 0.6);
    gtk_label_set_attributes(GTK_LABEL(subtitle_label), subtitle_scale_attr());
    gtk_box_append(GTK_BOX(text_box), subtitle_label);

    return card;
}

/* ============================================================================
 * build_extra_buttons — 右下角浮动按钮组 (匹配 PCL-CE MyExtraButton)
 *   圆形 40×40, 蓝底白图标, 阴影
 * ============================================================================ */

GtkWidget* build_extra_buttons()
{
    GtkWidget* stack = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_halign(stack, GTK_ALIGN_END);
    gtk_widget_set_valign(stack, GTK_ALIGN_END);
    gtk_widget_set_margin_end(stack, 15);
    gtk_widget_set_margin_bottom(stack, 15);

    // Matches PCL-CE FormMain PanExtra buttons
    auto buttons = pclcore::local::get_tool_provider().get_extra_buttons();

    for (const auto& btn : buttons) {
        GtkWidget* eb = gtk_button_new();
        gtk_button_set_has_frame(GTK_BUTTON(eb), FALSE);
        std::string light_icon = btn.icon + "-light";
        gtk_button_set_child(GTK_BUTTON(eb), icon::load(light_icon.c_str(), 18));
        gtk_widget_add_css_class(eb, "extra-button");
        gtk_widget_set_size_request(eb, 40, 40);
        gtk_widget_set_tooltip_text(eb, btn.label.c_str());
        gtk_widget_set_halign(eb, GTK_ALIGN_END);
        gtk_box_append(GTK_BOX(stack), eb);
    }

    return stack;
}

/* ============================================================================
 * build_loading_widget — 加载动画构件 (匹配 PCL-CE MyLoading)
 *   旋转微调器 + 文字
 * ============================================================================ */

GtkWidget* build_loading_widget(const char* label_text)
{
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);

    /* 旋转加载动画 */
    GtkWidget* spinner = gtk_spinner_new();
    gtk_spinner_set_spinning(GTK_SPINNER(spinner), TRUE);
    gtk_widget_set_size_request(spinner, 50, 50);
    gtk_box_append(GTK_BOX(box), spinner);

    /* 文字标签 */
    GtkWidget* label = gtk_label_new(label_text);
    gtk_widget_add_css_class(label, "ph-title");
    gtk_box_append(GTK_BOX(box), label);

    return box;
}

/* ============================================================================
 * build_launching_page — 启动中页面
 *   加载动画 + 渐变进度条 + 状态信息 + 取消按钮
 * ============================================================================ */

GtkWidget* build_launching_page()
{
    GtkWidget* overlay = gtk_overlay_new();

    /* 主内容 */
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_valign(vbox, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(vbox, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_start(vbox, 80);
    gtk_widget_set_margin_end(vbox, 80);

    /* 标题 */
    GtkWidget* title = gtk_label_new("正在启动");
    {
        PangoAttrList* attrs = build_weight_size_attr(PANGO_WEIGHT_BOLD, 20);
        gtk_label_set_attributes(GTK_LABEL(title), attrs);
        pango_attr_list_unref(attrs);
    }
    gtk_label_set_xalign(GTK_LABEL(title), 0.5f);
    gtk_widget_set_margin_bottom(title, 8);
    gtk_box_append(GTK_BOX(vbox), title);

    /* 版本名称 */
    GtkWidget* version = gtk_label_new("1.21.5");
    gtk_widget_set_opacity(version, 0.7);
    gtk_widget_set_margin_bottom(version, 24);
    gtk_box_append(GTK_BOX(vbox), version);

    /* 加载动画 */
    GtkWidget* spinner = gtk_spinner_new();
    gtk_spinner_set_spinning(GTK_SPINNER(spinner), TRUE);
    gtk_widget_set_size_request(spinner, 50, 50);
    gtk_widget_set_margin_bottom(spinner, 24);
    gtk_box_append(GTK_BOX(vbox), spinner);

    /* 进度条 */
    GtkWidget* progress = gtk_progress_bar_new();
    gtk_widget_set_size_request(progress, 300, -1);
    gtk_widget_set_margin_bottom(progress, 24);
    gtk_box_append(GTK_BOX(vbox), progress);

    /* 状态信息网格 */
    GtkWidget* info_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(info_grid), 40);
    gtk_grid_set_row_spacing(GTK_GRID(info_grid), 6);
    gtk_widget_set_halign(info_grid, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_bottom(info_grid, 24);
    gtk_box_append(GTK_BOX(vbox), info_grid);

    auto add_info_row = [](GtkGrid* grid, int row,
                           const char* key, const char* value) {
        GtkWidget* key_label = gtk_label_new(key);
        gtk_widget_set_opacity(key_label, 0.5);
        gtk_widget_set_halign(key_label, GTK_ALIGN_END);
        gtk_grid_attach(grid, key_label, 0, row, 1, 1);

        GtkWidget* val_label = gtk_label_new(value);
        gtk_widget_set_halign(val_label, GTK_ALIGN_START);
        gtk_label_set_ellipsize(GTK_LABEL(val_label), PANGO_ELLIPSIZE_END);
        gtk_label_set_max_width_chars(GTK_LABEL(val_label), 30);
        gtk_grid_attach(grid, val_label, 1, row, 1, 1);
    };

    add_info_row(GTK_GRID(info_grid), 0, "当前步骤", "下载资源文件");
    add_info_row(GTK_GRID(info_grid), 1, "登录方式", "Microsoft 登录");
    add_info_row(GTK_GRID(info_grid), 2, "启动进度", "0%");
    add_info_row(GTK_GRID(info_grid), 3, "下载速度", "—");

    /* 取消按钮 */
    GtkWidget* cancel_btn = gtk_button_new_with_label("取消");
    gtk_widget_add_css_class(cancel_btn, "pill-button");
    gtk_widget_set_halign(cancel_btn, GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(cancel_btn, 120, 35);
    gtk_box_append(GTK_BOX(vbox), cancel_btn);

    gtk_overlay_set_child(GTK_OVERLAY(overlay), vbox);

    return overlay;
}

/* ============================================================================
 * build_sidebar — 左侧边栏
 *
 *   布局 (匹配 PCL-CE PageLaunchLeft, Width=300):
 *     ┌────────────────────┐
 *     │  [正版][离线][第三方] │  ← 模式栏 (居中)
 *     │                    │
 *     │       ⬆ spacer     │
 *     │       [头像]       │  ← AdwAvatar 64px
 *     │       Steve        │  ← 用户名
 *     │       ⬇ spacer     │
 *     │    [  启动游戏  ]   │  ← Highlight, Height 54
 *     │  [版本选择][版本设置] │  ← 版本行
 *     └────────────────────┘
 * ============================================================================ */

GtkWidget* build_sidebar()
{
    GtkBuilder* b = icon::load_ui("launch_sidebar.ui");
    GtkWidget* panel = GTK_WIDGET(gtk_builder_get_object(b, "launch_sidebar"));
    g_object_ref(panel);

    /* ── 模式按钮互斥 ── */
    GtkWidget* mode_bar = GTK_WIDGET(gtk_builder_get_object(b, "mode_bar"));
    for (GtkWidget* child = gtk_widget_get_first_child(mode_bar);
         child; child = gtk_widget_get_next_sibling(child))
    {
        g_signal_connect(child, "clicked", G_CALLBACK((+[](GtkWidget* btn, gpointer) {
            GtkWidget* bar = gtk_widget_get_parent(btn);
            for (GtkWidget* sib = gtk_widget_get_first_child(bar);
                 sib; sib = gtk_widget_get_next_sibling(sib))
            {
                if (sib == btn)
                    gtk_widget_add_css_class(sib, "mode-btn-active");
                else
                    gtk_widget_remove_css_class(sib, "mode-btn-active");
            }
        })), nullptr);
    }

    /* ── 启动按钮 Overlay（C++ 构建, 替代 Blueprint 占位符）── */
    GtkWidget* placeholder = GTK_WIDGET(gtk_builder_get_object(b, "launch_placeholder"));

    GtkWidget* overlay = gtk_overlay_new();
    gtk_widget_set_halign(overlay, GTK_ALIGN_FILL);
    gtk_widget_set_margin_start(overlay, 20);
    gtk_widget_set_margin_end(overlay, 20);
    gtk_widget_set_margin_bottom(overlay, 8);

    /* 主按钮 */
    GtkWidget* launch_btn = gtk_button_new_with_label("启动游戏");
    gtk_widget_add_css_class(launch_btn, "pill-button");
    gtk_widget_add_css_class(launch_btn, "suggested-action");
    gtk_widget_add_css_class(launch_btn, "launch-main");
    gtk_widget_set_size_request(launch_btn, -1, 54);
    gtk_widget_set_halign(launch_btn, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand(launch_btn, TRUE);
    gtk_widget_set_valign(launch_btn, GTK_ALIGN_FILL);
    gtk_overlay_set_child(GTK_OVERLAY(overlay), launch_btn);

    /* 版本号叠加标签 */
    GtkWidget* version_label = gtk_label_new("1.21.5");
    gtk_widget_add_css_class(version_label, "launch-version");
    gtk_widget_set_halign(version_label, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(version_label, GTK_ALIGN_END);
    gtk_widget_set_margin_bottom(version_label, 6);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), version_label);

    /* ── 版本选择按钮 → Popover (GitHub 分支选择风格) ── */
    GtkWidget* version_select_btn = GTK_WIDGET(
        gtk_builder_get_object(b, "version_select_btn"));

    /* 实例数据 (通过 libpclcore 接口获取) */
    auto instances = pclcore::local::get_instance_provider().get_instances();

    /* 构建 Popover */
    GtkWidget* popover = gtk_popover_new();
    gtk_popover_set_position(GTK_POPOVER(popover), GTK_POS_TOP);
    gtk_widget_set_size_request(popover, 280, 340);
    gtk_widget_add_css_class(popover, "version-popover");
    g_object_set_data(G_OBJECT(popover), "version-label", version_label);

    GtkWidget* pop_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* 搜索框 */
    GtkWidget* pop_search = gtk_search_entry_new();
    gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(pop_search), "筛选实例…");
    gtk_widget_add_css_class(pop_search, "popover-search");
    gtk_widget_set_margin_start(pop_search, 8);
    gtk_widget_set_margin_end(pop_search, 8);
    gtk_widget_set_margin_top(pop_search, 8);
    gtk_widget_set_margin_bottom(pop_search, 4);
    gtk_box_append(GTK_BOX(pop_box), pop_search);

    /* 实例列表 (GtkListBox) */
    GtkWidget* inst_list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(inst_list), GTK_SELECTION_SINGLE);
    gtk_widget_add_css_class(inst_list, "version-list");
    gtk_widget_set_vexpand(inst_list, TRUE);

    GtkWidget* inst_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(inst_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_max_content_height(GTK_SCROLLED_WINDOW(inst_scroll), 340);
    gtk_widget_set_vexpand(inst_scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(inst_scroll), inst_list);
    gtk_box_append(GTK_BOX(pop_box), inst_scroll);

    gtk_popover_set_child(GTK_POPOVER(popover), pop_box);
    gtk_widget_set_parent(popover, version_select_btn);

    /* Popover 关闭时, 解除按钮的"按下"状态 */
    g_signal_connect(popover, "closed",
        G_CALLBACK(+[](GtkWidget* p, gpointer) {
            GtkWidget* btn = gtk_widget_get_parent(p);
            gtk_widget_set_state_flags(btn, GTK_STATE_FLAG_NORMAL, TRUE);
        }), nullptr);

    /* 填充实例列表 & 点击回调 */
    for (const auto& inst : instances) {
        GtkWidget* row = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        gtk_widget_set_margin_start(row, 8);
        gtk_widget_set_margin_end(row, 8);
        gtk_widget_set_margin_top(row, 6);
        gtk_widget_set_margin_bottom(row, 6);

        GtkWidget* name_lbl = gtk_label_new(inst.name.c_str());
        gtk_label_set_xalign(GTK_LABEL(name_lbl), 0.0f);
        gtk_widget_add_css_class(name_lbl, "version-item-name");
        gtk_box_append(GTK_BOX(row), name_lbl);

        GtkWidget* path_lbl = gtk_label_new(inst.path.c_str());
        gtk_label_set_xalign(GTK_LABEL(path_lbl), 0.0f);
        gtk_widget_add_css_class(path_lbl, "version-item-path");
        gtk_widget_set_opacity(path_lbl, 0.55);
        gtk_box_append(GTK_BOX(row), path_lbl);

        gtk_list_box_append(GTK_LIST_BOX(inst_list), row);
    }

    /* 搜索过滤 */
    g_signal_connect(pop_search, "search-changed", G_CALLBACK(+[](GtkEntry* entry, gpointer data) {
        GtkListBox* list = GTK_LIST_BOX(data);
        const char* filter = gtk_editable_get_text(GTK_EDITABLE(entry));
        GtkWidget* row = gtk_widget_get_first_child(GTK_WIDGET(list));
        while (row) {
            GtkWidget* next = gtk_widget_get_next_sibling(row);
            GtkWidget* name_lbl = gtk_widget_get_first_child(row);
            const char* name = name_lbl ? gtk_label_get_text(GTK_LABEL(name_lbl)) : "";
            bool visible = !filter || !filter[0] ||
                strstr(name, filter) != nullptr;
            gtk_widget_set_visible(row, visible);
            row = next;
        }
    }), inst_list);

    /* 列表行激活 → 更新版本号并关闭 Popover */
    g_signal_connect(inst_list, "row-activated",
        G_CALLBACK(+[](GtkListBox*, GtkListBoxRow* row, gpointer data) {
            GtkWidget* p = GTK_WIDGET(data);
            GtkWidget* child = gtk_list_box_row_get_child(row);
            GtkWidget* name_lbl = child ? gtk_widget_get_first_child(child) : nullptr;
            GtkWidget* vlbl = static_cast<GtkWidget*>(
                g_object_get_data(G_OBJECT(p), "version-label"));
            if (vlbl && name_lbl)
                gtk_label_set_text(GTK_LABEL(vlbl),
                    gtk_label_get_text(GTK_LABEL(name_lbl)));
            gtk_popover_popdown(GTK_POPOVER(p));
        }), popover);

    /* 点击版本选择按钮 → 弹出 Popover */
    g_signal_connect(version_select_btn, "clicked",
        G_CALLBACK(+[](GtkWidget*, gpointer data) {
            gtk_popover_popup(GTK_POPOVER(data));
        }), popover);

    /* ── 版本设置按钮 → 跳转设置页"实例版本设置", 自动选中当前版本 ── */
    GtkWidget* version_settings_btn = GTK_WIDGET(
        gtk_builder_get_object(b, "version_settings_btn"));
    g_object_set_data(G_OBJECT(version_settings_btn), "version-label", version_label);
    g_signal_connect(version_settings_btn, "clicked",
        G_CALLBACK(+[](GtkWidget* btn, gpointer) {
            GObject* win = G_OBJECT(gtk_widget_get_root(btn));

            /* 1. 获取当前版本名称 */
            GtkWidget* vlbl = static_cast<GtkWidget*>(
                g_object_get_data(G_OBJECT(btn), "version-label"));
            const char* cur_ver = vlbl ? gtk_label_get_text(GTK_LABEL(vlbl)) : "";

            /* 2. 找到标题栏"设置"页签并点击 (切换主视图 + 更新页签样式) */
            GtkWidget* header_tabs = static_cast<GtkWidget*>(
                g_object_get_data(win, "header-tabs"));
            if (header_tabs) {
                for (GtkWidget* tab = gtk_widget_get_first_child(header_tabs);
                     tab; tab = gtk_widget_get_next_sibling(tab))
                {
                    const char* pn = static_cast<const char*>(
                        g_object_get_data(G_OBJECT(tab), "page-name"));
                    if (pn && g_strcmp0(pn, "settings") == 0) {
                        g_signal_emit_by_name(tab, "clicked");
                        break;
                    }
                }
            }

            /* 3. 导航设置页左栏 → "实例版本设置" */
            AdwViewStack* main_stk = ADW_VIEW_STACK(
                g_object_get_data(win, "main-stack"));
            if (!main_stk) return;
            GtkWidget* sp = adw_view_stack_get_child_by_name(main_stk, "settings");
            if (!sp) return;

            GtkWidget* left_nav = static_cast<GtkWidget*>(
                g_object_get_data(G_OBJECT(sp), "settings-left-nav"));
            GtkWidget* mid_stk = static_cast<GtkWidget*>(
                g_object_get_data(G_OBJECT(sp), "settings-mid-stack"));
            if (!left_nav || !mid_stk) return;

            gtk_stack_set_visible_child_name(GTK_STACK(mid_stk), "mid-instance");

            /* 左栏高亮 "实例版本设置" */
            for (GtkWidget* sib = gtk_widget_get_first_child(left_nav);
                 sib; sib = gtk_widget_get_next_sibling(sib))
            {
                if (!gtk_widget_has_css_class(sib, "nav-item")) continue;
                const char* mp = static_cast<const char*>(
                    g_object_get_data(G_OBJECT(sib), "mid-page"));
                if (mp && g_strcmp0(mp, "mid-instance") == 0)
                    gtk_widget_add_css_class(sib, "nav-item-active");
                else
                    gtk_widget_remove_css_class(sib, "nav-item-active");
            }

            /* 4. 中栏自动选中匹配当前版本的项 */
            GtkWidget* inst_nav = gtk_stack_get_child_by_name(
                GTK_STACK(mid_stk), "mid-instance");
            if (inst_nav && cur_ver && cur_ver[0]) {
                for (GtkWidget* row = gtk_widget_get_first_child(inst_nav);
                     row; row = gtk_widget_get_next_sibling(row))
                {
                    if (!gtk_widget_has_css_class(row, "nav-item")) continue;
                    /* build_nav_item_subtitle 结构: GtkBox(vertical) → GtkLabel(标题) */
                    GtkWidget* lbl = gtk_widget_get_first_child(row);
                    if (!GTK_IS_LABEL(lbl)) continue;
                    const char* row_text = gtk_label_get_text(GTK_LABEL(lbl));
                    if (row_text && g_strcmp0(row_text, cur_ver) == 0) {
                        /* 直接复现中栏点击逻辑 (信号绑定在 Gesture 上, 无法 emit) */
                        GtkWidget* rstk = static_cast<GtkWidget*>(
                            g_object_get_data(G_OBJECT(row), "right-stack"));
                        const char* page = static_cast<const char*>(
                            g_object_get_data(G_OBJECT(row), "right-page"));
                        GtkWidget* parent = static_cast<GtkWidget*>(
                            g_object_get_data(G_OBJECT(row), "mid-parent"));

                        if (rstk && page)
                            gtk_stack_set_visible_child_name(
                                GTK_STACK(rstk), page);

                        if (parent) {
                            for (GtkWidget* sib = gtk_widget_get_first_child(parent);
                                 sib; sib = gtk_widget_get_next_sibling(sib))
                            {
                                if (!gtk_widget_has_css_class(sib, "nav-item")) continue;
                                if (sib == row)
                                    gtk_widget_add_css_class(sib, "nav-item-active");
                                else
                                    gtk_widget_remove_css_class(sib, "nav-item-active");
                            }
                        }
                        break;
                    }
                }
            }
        }), nullptr);

    /* 替换占位符 */
    gtk_widget_insert_after(overlay, panel, placeholder);
    gtk_box_remove(GTK_BOX(panel), placeholder);

    g_object_unref(b);
    return panel;
}

/* ============================================================================
 * build_news_with_fab — 资讯面板 + 浮动 FAB + 额外按钮
 *
 *   布局:
 *     ┌──────────────────────────────┐
 *     │  🔍 搜索…                    │  ← 搜索框
 *     │  📰 资讯中心                  │  ← 标题
 *     │  ┌────────────────────────┐  │
 *     │  │ 📦 新闻卡片             │  │
 *     │  │ 📦 新闻卡片             │  │
 *     │  │ ...                     │  │
 *     │  └────────────────────────┘  │
 *     │                       ┌──┐   │  ← 右下 FAB
 *     │                       │▶ │   │
 *     │                       └──┘   │
 *     └──────────────────────────────┘
 * ============================================================================ */

GtkWidget* build_news_with_fab()
{
    GtkWidget* overlay = gtk_overlay_new();
    gtk_widget_set_hexpand(overlay, TRUE);

    /* ---- 可滚动内容 ---- */
    GtkWidget* scroll = gtk_scrolled_window_new();
    gtk_widget_add_css_class(scroll, "news-scroll");
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_overlay_set_child(GTK_OVERLAY(overlay), scroll);

    GtkWidget* news_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(news_box, "news-panel");
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), news_box);

    /* ---- 右下角圆形 FAB — lucide/play (matches PCL-CE) ---- */
    GtkWidget* fab = gtk_button_new();
    gtk_button_set_has_frame(GTK_BUTTON(fab), FALSE);
    gtk_button_set_child(GTK_BUTTON(fab), icon::load("play-light", 20));
    gtk_widget_set_size_request(fab, 48, 48);
    gtk_widget_add_css_class(fab, "fab");
    gtk_widget_add_css_class(fab, "fab-blue");
    gtk_widget_set_tooltip_text(fab, "启动游戏");

    gtk_widget_set_halign(fab, GTK_ALIGN_END);
    gtk_widget_set_valign(fab, GTK_ALIGN_END);
    gtk_widget_set_margin_end(fab, 24);
    gtk_widget_set_margin_bottom(fab, 24);

    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), fab);

    return overlay;
}

/* ============================================================================
 * build_launch_page — 启动页完整布局
 *
 *   布局:
 *     ┌────────────┬────────────────────────┐
 *     │  左侧边栏   │  资讯面板 + 通知测试     │
 *     │  (300px)   │                    FAB │
 *     └────────────┴────────────────────────┘
 * ============================================================================ */

GtkWidget* build_launch_page()
{
    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    /* 左侧边栏 */
    GtkWidget* sidebar = build_sidebar();
    gtk_box_append(GTK_BOX(hbox), sidebar);

    /* 垂直分隔线 */
    GtkWidget* sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_append(GTK_BOX(hbox), sep);

    /* ── 右侧: 从 Blueprint 加载外壳, 动态填充内容 ── */
    GtkBuilder* rb = icon::load_ui("launch_right.ui");
    GtkWidget* overlay = GTK_WIDGET(gtk_builder_get_object(rb, "launch_right"));
    g_object_ref(overlay);

    GtkWidget* box = GTK_WIDGET(gtk_builder_get_object(rb, "news_box"));
    gtk_widget_set_margin_bottom(box, 80);  /* 防止 FAB 遮挡底部内容 */

    /* ── 资讯中心区域 (Provider 驱动) ── */
    {
        GtkWidget* title = gtk_label_new("资讯中心");
        gtk_widget_add_css_class(title, "section-title");
        gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
        gtk_widget_set_margin_bottom(title, 16);
        gtk_box_append(GTK_BOX(box), title);

        auto content = pclcore::local::get_launch_content_provider().get_content();
        for (size_t i = 0; i < content.news.size(); i++) {
            auto& n = content.news[i];
            GtkWidget* card = build_news_card(
                n.title.c_str(), n.description.c_str(), nullptr);
            bool last = (i == content.news.size() - 1);
            gtk_widget_set_margin_bottom(card, last ? 32 : 8);
            gtk_box_append(GTK_BOX(box), card);
        }
    }

    /* ── 通知测试区域 ── */
    {
        GtkWidget* title = gtk_label_new("通知测试");
        gtk_widget_add_css_class(title, "section-title");
        gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
        gtk_widget_set_margin_bottom(title, 8);
        gtk_box_append(GTK_BOX(box), title);

        GtkWidget* desc = gtk_label_new(
            "点击下方按钮，在右下角弹出通知并测试通知中心。");
        gtk_label_set_xalign(GTK_LABEL(desc), 0.0f);
        gtk_label_set_wrap(GTK_LABEL(desc), TRUE);
        gtk_widget_set_opacity(desc, 0.55);
        gtk_widget_set_margin_bottom(desc, 14);
        gtk_box_append(GTK_BOX(box), desc);

        GtkWidget* card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_widget_add_css_class(card, "card");
        gtk_box_append(GTK_BOX(box), card);

        struct BtnDef {
            const char* label;
            const char* type;
            const char* css;
        };
        const BtnDef btns[] = {
            {"显示提示",   "info",  "suggested-action"},
            {"显示警告",   "warn",  ""},
            {"显示错误",   "error", "destructive-action"},
            {"显示严重错误", "fatal", "destructive-action"},
        };

        for (auto& b : btns) {
            GtkWidget* btn = gtk_button_new_with_label(b.label);
            if (b.css[0])
                gtk_widget_add_css_class(btn, b.css);
            gtk_widget_set_halign(btn, GTK_ALIGN_FILL);
            gtk_widget_set_margin_start(btn, 8);
            gtk_widget_set_margin_end(btn, 8);

            using StrPair = std::pair<std::string, std::string>;
            auto* sp = new StrPair(b.type, b.label);
            g_object_set_data_full(G_OBJECT(btn), "toast-sp", sp,
                [](void* p) { delete static_cast<StrPair*>(p); });
            g_signal_connect(btn, "clicked",
                G_CALLBACK((+[](GtkWidget* w, gpointer) {
                    auto* pair = static_cast<StrPair*>(
                        g_object_get_data(G_OBJECT(w), "toast-sp"));
                    if (!pair) return;
                    GtkWindow* win = GTK_WINDOW(gtk_widget_get_root(w));
                    show_toast(win, {
                        pair->first,
                        pair->second,
                        "这是一条测试通知。",
                    });
                })), nullptr);

            gtk_box_append(GTK_BOX(card), btn);
        }
    }

    /* ── 安全模式测试开关 ── */
    {
        GtkWidget* title = gtk_label_new("安全模式测试");
        gtk_widget_add_css_class(title, "section-title");
        gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
        gtk_widget_set_margin_bottom(title, 8);
        gtk_box_append(GTK_BOX(box), title);

        GtkWidget* desc = gtk_label_new(
            "开启后将模拟 PCL-CGRE 本体崩溃，标题栏变红并触发安全模式通知。");
        gtk_label_set_xalign(GTK_LABEL(desc), 0.0f);
        gtk_label_set_wrap(GTK_LABEL(desc), TRUE);
        gtk_widget_set_opacity(desc, 0.55);
        gtk_widget_set_margin_bottom(desc, 14);
        gtk_box_append(GTK_BOX(box), desc);

        GtkWidget* card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_widget_add_css_class(card, "card");
        gtk_box_append(GTK_BOX(box), card);

        GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_box_append(GTK_BOX(card), row);

        GtkWidget* lbl = gtk_label_new("测试安全模式");
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
        gtk_widget_set_hexpand(lbl, TRUE);
        gtk_box_append(GTK_BOX(row), lbl);

        GtkWidget* sw = gtk_switch_new();
        gtk_widget_set_valign(sw, GTK_ALIGN_CENTER);
        gtk_box_append(GTK_BOX(row), sw);

        g_signal_connect(sw, "state-set",
            G_CALLBACK(+[](GtkSwitch*, gboolean state, gpointer data) -> gboolean {
                GtkWidget* sw_w = GTK_WIDGET(data);
                GtkWidget* win = GTK_WIDGET(gtk_widget_get_root(sw_w));
                if (!win) return FALSE;

                if (state) {
                    show_toast(GTK_WINDOW(win), {
                        "warn",
                        "已进入安全模式",
                        "所有插件禁用——进入 CrashSpy 查看详情",
                    });
                }
                return FALSE;
            }), sw);
    }

    /* ── 右下角圆形 FAB ── */
    GtkWidget* fab = gtk_button_new();
    gtk_button_set_has_frame(GTK_BUTTON(fab), FALSE);
    gtk_button_set_child(GTK_BUTTON(fab), icon::load("play-light", 20));
    gtk_widget_set_size_request(fab, 48, 48);
    gtk_widget_add_css_class(fab, "fab");
    gtk_widget_add_css_class(fab, "fab-blue");
    gtk_widget_set_tooltip_text(fab, "启动游戏");
    gtk_widget_set_halign(fab, GTK_ALIGN_END);
    gtk_widget_set_valign(fab, GTK_ALIGN_END);
    gtk_widget_set_margin_end(fab, 24);
    gtk_widget_set_margin_bottom(fab, 24);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), fab);

    g_object_unref(rb);
    gtk_box_append(GTK_BOX(hbox), overlay);

    return hbox;
}

/* ============================================================================
 * build_two_panel_page — 通用两栏布局
 *
 *   左侧 300px 固定 + 分隔线 + 右侧 hexpand
 * ============================================================================ */

GtkWidget* build_two_panel_page(GtkWidget* left, GtkWidget* right)
{
    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    gtk_widget_set_hexpand(left, FALSE);
    gtk_widget_set_valign(left, GTK_ALIGN_FILL);
    gtk_box_append(GTK_BOX(hbox), left);

    GtkWidget* sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_append(GTK_BOX(hbox), sep);

    gtk_widget_set_hexpand(right, TRUE);
    gtk_widget_add_css_class(right, "content-area");
    gtk_box_append(GTK_BOX(hbox), right);

    return hbox;
}

/* ============================================================================
 * build_nav_item — 导航列表项 (匹配 PCL-CE MyListItem)
 *   36px 高, 图标 + 文字, 可选 active 态
 * ============================================================================ */

GtkWidget* build_nav_item(const char* label,
                          const char* icon_name,
                          bool active)
{
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_add_css_class(row, "nav-item");
    gtk_widget_set_size_request(row, -1, 36);
    gtk_widget_set_margin_start(row, 8);
    gtk_widget_set_margin_end(row, 8);

    if (active)
        gtk_widget_add_css_class(row, "nav-item-active");

    /* 图标 — Lucide SVG, 16px (可选) */
    if (icon_name && icon_name[0]) {
        GtkWidget* icon = icon::load(icon_name, 16);
        gtk_widget_set_opacity(icon, active ? 1.0 : 0.55);
        gtk_widget_set_valign(icon, GTK_ALIGN_CENTER);
        gtk_box_append(GTK_BOX(row), icon);
    }

    /* 文字 */
    GtkWidget* lbl = gtk_label_new(label);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_widget_set_valign(lbl, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(row), lbl);

    return row;
}

/* ============================================================================
 * build_nav_item_subtitle — 带副标题的导航项 (无图标, 更高)
 *   52px 高, 标题 (semibold) + 副标题 (小字, 低透明度)
 * ============================================================================ */

GtkWidget* build_nav_item_subtitle(const char* label,
                                   const char* subtitle,
                                   bool active)
{
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    gtk_widget_add_css_class(row, "nav-item");
    gtk_widget_set_size_request(row, -1, 52);
    gtk_widget_set_margin_start(row, 8);
    gtk_widget_set_margin_end(row, 8);

    if (active)
        gtk_widget_add_css_class(row, "nav-item-active");

    /* 标题 */
    GtkWidget* lbl = gtk_label_new(label);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_widget_set_valign(lbl, GTK_ALIGN_END);
    gtk_widget_set_margin_top(lbl, 5);
    {
        PangoAttrList* attrs = pango_attr_list_new();
        pango_attr_list_insert(attrs,
            pango_attr_weight_new(PANGO_WEIGHT_SEMIBOLD));
        gtk_label_set_attributes(GTK_LABEL(lbl), attrs);
        pango_attr_list_unref(attrs);
    }
    gtk_box_append(GTK_BOX(row), lbl);

    /* 副标题 */
    if (subtitle && *subtitle) {
        GtkWidget* sub = gtk_label_new(subtitle);
        gtk_widget_set_halign(sub, GTK_ALIGN_START);
        gtk_widget_set_valign(sub, GTK_ALIGN_START);
        gtk_widget_set_opacity(sub, 0.45);
        gtk_label_set_ellipsize(GTK_LABEL(sub), PANGO_ELLIPSIZE_MIDDLE);
        gtk_widget_set_margin_bottom(sub, 5);
        {
            PangoAttrList* attrs = pango_attr_list_new();
            pango_attr_list_insert(attrs,
                pango_attr_size_new(9 * PANGO_SCALE));
            gtk_label_set_attributes(GTK_LABEL(sub), attrs);
            pango_attr_list_unref(attrs);
        }
        gtk_box_append(GTK_BOX(row), sub);
    }

    return row;
}

}  // namespace pcl
