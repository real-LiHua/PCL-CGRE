#include "pages/SettingsPage.hpp"
#include "pages/LaunchPage.hpp"
#include "core/Colors.hpp"
#include "core/ConfigManager.hpp"
#include "util/IconHelper.hpp"
#include "pclcore/pclcore.hpp"
#include "widgets/NotificationToast.hpp"

#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace pcl {

using namespace pcl::colors;

/* ── Blueprint 路径解析 (相对于二进制) ── */

/* ============================================================================
 * 内部辅助
 * ============================================================================ */

namespace {

/* ── 创建带 label 的 GtkDropDown ───────────────────────────────────────── */
[[maybe_unused]]
GtkWidget* build_labeled_dropdown(const char* label_text,
                                  const char* const* items,
                                  int n_items,
                                  int default_idx = 0)
{
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_bottom(row, 7);

    GtkWidget* lbl = gtk_label_new(label_text);
    gtk_widget_set_valign(lbl, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_widget_set_size_request(lbl, 100, -1);
    gtk_box_append(GTK_BOX(row), lbl);

    GtkStringList* model = gtk_string_list_new(nullptr);
    for (int i = 0; i < n_items; i++)
        gtk_string_list_append(model, items[i]);

    GtkWidget* dd = gtk_drop_down_new(G_LIST_MODEL(model), nullptr);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(dd), default_idx);
    gtk_widget_set_hexpand(dd, TRUE);
    gtk_widget_set_valign(dd, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(row), dd);

    return row;
}

/* ── 创建带 label 的 GtkCheckButton ────────────────────────────────────── */
[[maybe_unused]]
GtkWidget* build_labeled_check(const char* label_text)
{
    GtkWidget* cb = gtk_check_button_new_with_label(label_text);
    gtk_widget_set_margin_bottom(cb, 7);
    return cb;
}

/* ── 创建带 label 的 GtkEntry ──────────────────────────────────────────── */
[[maybe_unused]]
GtkWidget* build_labeled_entry(const char* label_text,
                               const char* placeholder = nullptr)
{
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_bottom(row, 7);

    GtkWidget* lbl = gtk_label_new(label_text);
    gtk_widget_set_valign(lbl, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_widget_set_size_request(lbl, 100, -1);
    gtk_box_append(GTK_BOX(row), lbl);

    GtkWidget* entry = gtk_entry_new();
    if (placeholder)
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), placeholder);
    gtk_widget_set_hexpand(entry, TRUE);
    gtk_widget_set_valign(entry, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(row), entry);

    return row;
}

/* ── 创建带 label 的 GtkScale + 数值标签 ───────────────────────────────── */
[[maybe_unused]]
GtkWidget* build_labeled_slider(const char* label_text,
                                double min, double max, double step,
                                double default_val)
{
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_bottom(row, 7);

    GtkWidget* lbl = gtk_label_new(label_text);
    gtk_widget_set_valign(lbl, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_widget_set_size_request(lbl, 100, -1);
    gtk_box_append(GTK_BOX(row), lbl);

    GtkAdjustment* adj = gtk_adjustment_new(default_val, min, max, step, step, 0);
    GtkWidget* scale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, adj);
    gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);
    gtk_widget_set_hexpand(scale, TRUE);
    gtk_box_append(GTK_BOX(row), scale);

    return row;
}

/* ── 创建 card 容器 — 带标题的圆角卡片 ────────────────────────────────── */
[[maybe_unused]]
GtkWidget* build_card(const char* title, GtkWidget* content)
{
    GtkWidget* card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(card, "card");
    gtk_widget_set_margin_bottom(card, 16);

    GtkWidget* title_lbl = gtk_label_new(title);
    gtk_widget_add_css_class(title_lbl, "card-title");
    gtk_widget_set_halign(title_lbl, GTK_ALIGN_START);
    gtk_widget_set_margin_start(title_lbl, 20);
    gtk_widget_set_margin_end(title_lbl, 20);
    gtk_widget_set_margin_top(title_lbl, 16);
    gtk_box_append(GTK_BOX(card), title_lbl);

    if (content) {
        gtk_widget_set_margin_start(content, 20);
        gtk_widget_set_margin_end(content, 20);
        gtk_widget_set_margin_top(content, 12);
        gtk_widget_set_margin_bottom(content, 16);
        gtk_box_append(GTK_BOX(card), content);
    }

    return card;
}

/* ── 占位设置页面 — 从 Blueprint 加载, 动态设置标题 ───────────────── */
static GtkWidget* build_placeholder(const char* title)
{
    GtkBuilder* b = icon::load_ui("settings_placeholder.ui");
    GtkWidget* page = GTK_WIDGET(gtk_builder_get_object(b, "settings_subpage"));
    g_object_ref(page);

    GtkWidget* title_lbl = GTK_WIDGET(gtk_builder_get_object(b, "card_title"));
    if (title_lbl)
        gtk_label_set_text(GTK_LABEL(title_lbl), title);

    g_object_unref(b);
    return page;
}

}  // anonymous namespace

/* ============================================================================
 * build_settings_page — 设置页面主入口 (三栏层级布局)
 *
 *   左栏: 5 个一级分类
 *   中栏: 二级子项导航 (随左栏切换), nav-sidebar 风格
 *   右栏: 卡片内容 (随中栏切换)
 * ============================================================================ */
GtkWidget* build_settings_page()
{
    /* ═══════════════════════════════════════════════════════════════════
     *  右栏 GtkStack — 先创建, 供中栏子项引用
     * ═══════════════════════════════════════════════════════════════════ */
    GtkWidget* right_stack = gtk_stack_new();
    gtk_widget_add_css_class(right_stack, "settings-right");
    gtk_stack_set_transition_type(GTK_STACK(right_stack),
                                  GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(GTK_STACK(right_stack), 150);
    gtk_widget_set_valign(right_stack, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand(right_stack, TRUE);

    /* 将各个子页面 builder 包装后注册到右栏 stack
     * 同时保存 builder 引用到 map, 供回滚时重建页面
     *
     * 每页外层包一个 GtkBox wrapper: GtkStack 的子页是 wrapper,
     * 回滚时只需替换 wrapper 内容, 永不触碰 GtkStack 子页列表,
     * 从而彻底避免 gtk_stack_remove 引发的自动切页 / 导航损坏 */
    auto page_builders = std::make_shared<std::unordered_map<std::string, std::function<GtkWidget*()>>>();
    auto reg_page = [&](const char* name, std::function<GtkWidget*()> builder) {
        (*page_builders)[name] = builder;
        GtkWidget* wrapper = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_set_vexpand(wrapper, TRUE);
        gtk_widget_set_hexpand(wrapper, TRUE);
        GtkWidget* page = builder();
        gtk_box_append(GTK_BOX(wrapper), page);
        gtk_stack_add_named(GTK_STACK(right_stack), wrapper, name);
    };

    reg_page("page-launch-set",   build_page_launch_set);     // P1: ✓ 已实现
    reg_page("page-java",         build_page_java_mgmt);      // P1: ✓ 已实现
    reg_page("page-game-manage",  build_page_game_manage);    // P1: ✓ 已实现
    reg_page("page-ui",           build_page_ui);           // P2: ✓ 已实现
    reg_page("page-language",     build_page_language);     // P2: ✓ 已实现
    reg_page("page-misc",         build_page_misc);         // P2: ✓ 已实现
    reg_page("page-about",        build_page_about);        // P3: ✓ 已实现
    reg_page("page-update",       build_page_update);       // P3: ✓ 已实现
    reg_page("page-feedback",     build_page_feedback);     // P3: ✓ 已实现
    reg_page("page-log",          build_page_log);          // P3: ✓ 已实现

    /* 实例 & 账号占位页面 — 动态注册, 匹配 libpclcore 数据 */
    {
        auto insts = pclcore::local::get_instance_provider().get_instances();
        for (size_t i = 0; i < insts.size(); i++) {
            std::string page_name = "page-instance-" + std::to_string(i + 1);
            std::string title = insts[i].name;
            reg_page(page_name.c_str(), [title]() { return build_placeholder(title.c_str()); });
        }
    }
    {
        auto accts = pclcore::local::get_account_provider().get_accounts();
        for (size_t i = 0; i < accts.size(); i++) {
            std::string page_name = "page-account-" + std::to_string(i + 1);
            std::string title = accts[i].username + " (" + accts[i].account_type + ")";
            reg_page(page_name.c_str(), [title]() { return build_placeholder(title.c_str()); });
        }
    }

    /* 默认空白占位 */
    {
        GtkWidget* ph = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_set_halign(ph, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(ph, GTK_ALIGN_CENTER);
        gtk_widget_set_vexpand(ph, TRUE);
        GtkWidget* lbl = gtk_label_new("选择左侧子项以查看详情");
        gtk_widget_set_opacity(lbl, 0.30);
        gtk_box_append(GTK_BOX(ph), lbl);
        gtk_stack_add_named(GTK_STACK(right_stack), ph, "blank");
    }
    gtk_stack_set_visible_child_name(GTK_STACK(right_stack), "blank");

    /* ═══════════════════════════════════════════════════════════════════
     *  中栏 GtkBox — nav-sidebar 风格, 承载二级子项导航
     * ═══════════════════════════════════════════════════════════════════ */
    GtkWidget* mid_box_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(mid_box_content, "settings-mid");
    gtk_widget_set_valign(mid_box_content, GTK_ALIGN_FILL);

    /* 中栏用 GtkStack 切换不同组的子项列表 */
    GtkWidget* mid_stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(mid_stack),
                                  GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(GTK_STACK(mid_stack), 120);
    gtk_widget_set_vexpand(mid_stack, TRUE);
    gtk_box_append(GTK_BOX(mid_box_content), mid_stack);

    /* 包裹滚动窗口 — 内容过多时可滚动 */
    GtkWidget* mid_box = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(mid_box),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(mid_box), mid_box_content);
    gtk_widget_set_valign(mid_box, GTK_ALIGN_FILL);

    /* ── 二级子项定义 ── */
    struct SubItem {
        const char* label;
        const char* icon;
        const char* right_page;  // right_stack child name
        const char* subtitle;    // optional: if set, use build_nav_item_subtitle (no icon)
    };

    /* 全局游戏设置 */
    const SubItem sub_global[] = {
        {"启动设置",   "rocket",       "page-launch-set",  nullptr},
        {"Java 管理",  "coffee",       "page-java",        nullptr},
        {"游戏管理",   "book-marked",  "page-game-manage", nullptr},
    };
    /* 实例设置 — 通过 libpclcore 接口获取, 预构建字符串防止悬空指针 */
    auto instances = pclcore::local::get_instance_provider().get_instances();
    std::vector<std::string> inst_page_names;
    for (size_t i = 0; i < instances.size(); i++)
        inst_page_names.push_back("page-instance-" + std::to_string(i + 1));
    std::vector<SubItem> sub_instance;
    for (size_t i = 0; i < instances.size(); i++) {
        sub_instance.push_back({instances[i].name.c_str(), nullptr,
                                inst_page_names[i].c_str(),
                                instances[i].path.c_str()});
    }
    /* 账号设置 — 通过 libpclcore 接口获取, 预构建字符串防止悬空指针 */
    auto accounts = pclcore::local::get_account_provider().get_accounts();
    std::vector<std::string> acct_page_names, acct_subtitles;
    for (size_t i = 0; i < accounts.size(); i++) {
        acct_page_names.push_back("page-account-" + std::to_string(i + 1));
        acct_subtitles.push_back(accounts[i].account_type + " 账号");
    }
    std::vector<SubItem> sub_account;
    for (size_t i = 0; i < accounts.size(); i++) {
        sub_account.push_back({accounts[i].username.c_str(), nullptr,
                               acct_page_names[i].c_str(),
                               acct_subtitles[i].c_str()});
    }
    /* 启动器设置 */
    const SubItem sub_launcher[] = {
        {"界面设置",   "palette",      "page-ui",      nullptr},
        {"语言",       "earth",        "page-language", nullptr},
        {"杂项",       "monitor-cog",  "page-misc",     nullptr},
    };
    /* 关于此软件 */
    const SubItem sub_about[] = {
        {"关于",       "info",           "page-about",    nullptr},
        {"更新",       "refresh-cw",     "page-update",   nullptr},
        {"反馈",       "message-circle", "page-feedback", nullptr},
        {"日志",       "scroll-text",    "page-log",      nullptr},
    };

    /* 辅助: 为给定子项数组构建一个 GtkBox (nav-sidebar 风格) */
    auto build_sub_nav = [&](const SubItem* items, int count) -> GtkWidget* {
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        gtk_widget_set_valign(box, GTK_ALIGN_FILL);

        for (int i = 0; i < count; i++) {
            GtkWidget* item;
            if (items[i].subtitle) {
                item = build_nav_item_subtitle(items[i].label,
                                               items[i].subtitle, i == 0);
            } else {
                item = build_nav_item(items[i].label, items[i].icon, i == 0);
            }
            gtk_box_append(GTK_BOX(box), item);

            /* 点击切换右栏页面 */
            g_object_set_data(G_OBJECT(item), "right-stack", right_stack);
            g_object_set_data_full(G_OBJECT(item), "right-page",
                                   g_strdup(items[i].right_page), g_free);
            g_object_set_data(G_OBJECT(item), "mid-parent", box);

            GtkGesture* click = gtk_gesture_click_new();
            g_signal_connect(click, "pressed",
                G_CALLBACK(+[](GtkGesture* g, int, double, double, gpointer) {
                    GtkWidget* target = gtk_event_controller_get_widget(
                        GTK_EVENT_CONTROLLER(g));
                    GtkWidget* rstk = static_cast<GtkWidget*>(
                        g_object_get_data(G_OBJECT(target), "right-stack"));
                    const char* page = static_cast<const char*>(
                        g_object_get_data(G_OBJECT(target), "right-page"));
                    GtkWidget* parent = static_cast<GtkWidget*>(
                        g_object_get_data(G_OBJECT(target), "mid-parent"));

                    gtk_stack_set_visible_child_name(GTK_STACK(rstk), page);

                    /* 互斥高亮 */
                    for (GtkWidget* sib = gtk_widget_get_first_child(parent);
                         sib; sib = gtk_widget_get_next_sibling(sib))
                    {
                        if (!gtk_widget_has_css_class(sib, "nav-item")) continue;
                        if (sib == target)
                            gtk_widget_add_css_class(sib, "nav-item-active");
                        else
                            gtk_widget_remove_css_class(sib, "nav-item-active");
                    }
                }), nullptr);
            gtk_widget_add_controller(item, GTK_EVENT_CONTROLLER(click));
        }
        return box;
    };

    gtk_stack_add_named(GTK_STACK(mid_stack),
        build_sub_nav(sub_global,   3), "mid-global");
    /* mid-instance: 子项列表 + 添加按钮 */
    {
        GtkWidget* wrap = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_box_append(GTK_BOX(wrap),
            build_sub_nav(sub_instance.data(), (int)sub_instance.size()));
        GtkWidget* btn = gtk_button_new_with_label("添加Minecraft目录");
        gtk_widget_add_css_class(btn, "suggested-action");
        gtk_widget_set_margin_start(btn, 8);
        gtk_widget_set_margin_end(btn, 8);
        gtk_widget_set_margin_top(btn, 8);
        gtk_box_append(GTK_BOX(wrap), btn);
        gtk_stack_add_named(GTK_STACK(mid_stack), wrap, "mid-instance");
    }
    /* mid-account: 子项列表 + 添加按钮 */
    {
        GtkWidget* wrap = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_box_append(GTK_BOX(wrap),
            build_sub_nav(sub_account.data(), (int)sub_account.size()));
        GtkWidget* btn = gtk_button_new_with_label("添加");
        gtk_widget_add_css_class(btn, "suggested-action");
        gtk_widget_set_margin_start(btn, 8);
        gtk_widget_set_margin_end(btn, 8);
        gtk_widget_set_margin_top(btn, 8);
        gtk_box_append(GTK_BOX(wrap), btn);
        gtk_stack_add_named(GTK_STACK(mid_stack), wrap, "mid-account");
    }
    gtk_stack_add_named(GTK_STACK(mid_stack),
        build_sub_nav(sub_launcher, 3), "mid-launcher");
    gtk_stack_add_named(GTK_STACK(mid_stack),
        build_sub_nav(sub_about,    4), "mid-about");

    gtk_stack_set_visible_child_name(GTK_STACK(mid_stack), "mid-global");

    /* ═══════════════════════════════════════════════════════════════════
     *  左栏 — 一级分类
     * ═══════════════════════════════════════════════════════════════════ */
    GtkWidget* left_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_add_css_class(left_box, "nav-sidebar");
    gtk_widget_set_valign(left_box, GTK_ALIGN_FILL);

    struct NavGroup {
        const char* label;
        const char* icon;
        const char* mid_page;
    };
    const NavGroup groups[] = {
        {"全局游戏设置",  "gamepad",     "mid-global"},
        {"实例版本设置",  "layers",      "mid-instance"},
        {"账号设置",      "contact",     "mid-account"},
        {"启动器设置",    "settings",     "mid-launcher"},
        {"关于此软件",    "info",        "mid-about"},
    };
    constexpr int N_GROUPS = 5;

    for (int i = 0; i < N_GROUPS; i++) {
        GtkWidget* item = build_nav_item(groups[i].label, groups[i].icon, i == 0);
        gtk_box_append(GTK_BOX(left_box), item);

        /* 左栏点击 → 切换中栏子项列表 */
        g_object_set_data(G_OBJECT(item), "mid-stack", mid_stack);
        g_object_set_data_full(G_OBJECT(item), "mid-page",
                               g_strdup(groups[i].mid_page), g_free);
        g_object_set_data(G_OBJECT(item), "left-nav", left_box);

        GtkGesture* click = gtk_gesture_click_new();
        g_signal_connect(click, "pressed",
            G_CALLBACK(+[](GtkGesture* g, int, double, double, gpointer) {
                GtkWidget* target = gtk_event_controller_get_widget(
                    GTK_EVENT_CONTROLLER(g));
                GtkWidget* mstk = static_cast<GtkWidget*>(
                    g_object_get_data(G_OBJECT(target), "mid-stack"));
                const char* name = static_cast<const char*>(
                    g_object_get_data(G_OBJECT(target), "mid-page"));
                GtkWidget* nav = static_cast<GtkWidget*>(
                    g_object_get_data(G_OBJECT(target), "left-nav"));

                gtk_stack_set_visible_child_name(GTK_STACK(mstk), name);

                /* 互斥高亮 */
                for (GtkWidget* sib = gtk_widget_get_first_child(nav);
                     sib; sib = gtk_widget_get_next_sibling(sib))
                {
                    if (!gtk_widget_has_css_class(sib, "nav-item")) continue;
                    if (sib == target)
                        gtk_widget_add_css_class(sib, "nav-item-active");
                    else
                        gtk_widget_remove_css_class(sib, "nav-item-active");
                }
            }), nullptr);
        gtk_widget_add_controller(item, GTK_EVENT_CONTROLLER(click));
    }

    /* 包裹滚动窗口 — 内容过多时可滚动，默认不显示滚动条 */
    GtkWidget* left = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(left),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(left), left_box);
    gtk_widget_set_valign(left, GTK_ALIGN_FILL);

    /* ═══════════════════════════════════════════════════════════════════
     *  三栏可拖动布局 — 使用两层 GtkPaned 支持用户自由调整宽度
     * ═══════════════════════════════════════════════════════════════════ */

    /* 第一层 paned: 左栏 (一级导航) | (中栏 + 右栏) */
    GtkWidget* paned1 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_hexpand(paned1, TRUE);
    gtk_widget_set_vexpand(paned1, TRUE);

    /* 设置最小宽度防止拖得太窄 */
    gtk_widget_set_size_request(left, 120, -1);
    gtk_paned_set_start_child(GTK_PANED(paned1), left);
    gtk_paned_set_position(GTK_PANED(paned1), 190);  // 初始位置 190px

    /* 第二层 paned: 中栏 (二级导航) | 右栏 (内容) */
    GtkWidget* paned2 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_hexpand(paned2, TRUE);
    gtk_widget_set_vexpand(paned2, TRUE);

    /* 设置最小宽度防止拖得太窄 */
    gtk_widget_set_size_request(mid_box, 140, -1);
    gtk_paned_set_start_child(GTK_PANED(paned2), mid_box);
    gtk_widget_set_size_request(right_stack, 280, -1);
    gtk_paned_set_end_child(GTK_PANED(paned2), right_stack);
    gtk_paned_set_position(GTK_PANED(paned2), 210);  // 初始位置 210px

    /* 子页面切换 → 静默保存 (不弹 Toast) */
    g_signal_connect(right_stack, "notify::visible-child-name",
        G_CALLBACK(+[](GObject*, GParamSpec*, gpointer) {
            ConfigManager::instance().save_silent();
        }), nullptr);

    /* ConfigManager 保存回调 → 展示自定义 Toast */
    ConfigManager::instance().set_save_callback(
        [paned = paned1](bool show) {
            if (!show) return;
            auto* win = GTK_WINDOW(gtk_widget_get_root(paned));
            pcl::ToastConfig cfg;
            cfg.type         = "info";
            cfg.title        = "设置已自动保存";
            cfg.desc         = "点击此通知回滚";
            cfg.add_to_center = false;
            cfg.can_clear    = false;
            cfg.duration_ms  = 1600;
            cfg.on_click     = []() { ConfigManager::instance().rollback(); };
            show_toast(win, cfg);
        });

    /* ConfigManager 回滚回调 → 替换当前页内容 (不触碰 GtkStack 子页列表) */
    ConfigManager::instance().set_rollback_callback(
        [paned = paned1, right_stack, page_builders]() {
            const char* cur = gtk_stack_get_visible_child_name(GTK_STACK(right_stack));
            if (!cur || !page_builders->count(cur)) return;
            auto& builder = (*page_builders)[cur];

            /* wrapper 是 GtkStack 的直接子页, 永不增删;
             * 只替换其内部内容, 避免 gtk_stack_remove 引发的:
             *   - GTK 自动切到其他子页 → notify::visible-child-name → save_silent()
             *   - 导航状态损坏 → 切走后无法切回 */
            GtkWidget* wrapper = gtk_stack_get_child_by_name(GTK_STACK(right_stack), cur);
            if (!GTK_IS_BOX(wrapper)) return;

            GtkWidget* old_page = gtk_widget_get_first_child(wrapper);
            if (old_page) gtk_box_remove(GTK_BOX(wrapper), old_page);

            GtkWidget* new_page = builder();
            gtk_box_append(GTK_BOX(wrapper), new_page);

            /* 重建时控件初始化触发的 set() / set_json() 等值调用
             * 已被 dedup 拦截; clear_backup() 清理残余 */
            ConfigManager::instance().clear_backup();

            auto* win = GTK_WINDOW(gtk_widget_get_root(paned));
            pcl::ToastConfig cfg;
            cfg.type         = "info";
            cfg.title        = "设置已回滚";
            cfg.add_to_center = false;
            cfg.can_clear    = false;
            cfg.duration_ms  = 1600;
            show_toast(win, cfg);
        });
    gtk_paned_set_end_child(GTK_PANED(paned1), paned2);

    /* 离开设置页时立即保存 (切换到其他顶层标签页 / 窗口失焦) */
    g_signal_connect(paned1, "unmap",
        G_CALLBACK(+[](GtkWidget*, gpointer) {
            ConfigManager::instance().save_silent();
        }), nullptr);

    /* 存储关键控件, 供外部 (如启动页) 导航到特定子项 */
    g_object_set_data(G_OBJECT(paned1), "settings-left-nav", left);
    g_object_set_data(G_OBJECT(paned1), "settings-mid-stack", mid_stack);

    /* 默认选中第一个中栏项, 展开对应右侧面板 */
    {
        GtkWidget* first_mid = gtk_stack_get_child_by_name(
            GTK_STACK(mid_stack), "mid-global");
        if (first_mid) {
            GtkWidget* row = gtk_widget_get_first_child(first_mid);
            while (row) {
                if (gtk_widget_has_css_class(row, "nav-item")) {
                    /* 复现中栏点击逻辑 */
                    GtkWidget* rstk = static_cast<GtkWidget*>(
                        g_object_get_data(G_OBJECT(row), "right-stack"));
                    const char* page = static_cast<const char*>(
                        g_object_get_data(G_OBJECT(row), "right-page"));

                    if (rstk && page)
                        gtk_stack_set_visible_child_name(
                            GTK_STACK(rstk), page);

                    gtk_widget_add_css_class(row, "nav-item-active");
                    break;
                }
                row = gtk_widget_get_next_sibling(row);
            }
        }
    }

    return paned1;
}

}  // namespace pcl
