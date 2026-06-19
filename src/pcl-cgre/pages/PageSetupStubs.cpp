/**
 * Settings sub-page builders — P1, P2, P3 implementations
 *
 * Each builder creates a GtkScrolledWindow with one or more cards.
 * All config is read/written via ConfigManager::instance().
 */

#include "pages/SettingsPage.hpp"
#include "pages/LaunchPage.hpp"
#include "core/ConfigManager.hpp"
#include "util/IconHelper.hpp"

#include <cstdio>
#include <gtk/gtk.h>

namespace pcl {

/* ═══════════════════════════════════════════════════════════════════════
 * 通用框架 — 创建带内容区域的滚动页面
 * ═══════════════════════════════════════════════════════════════════════ */

namespace {

/** 创建滚动页面框架，返回 (scrolled_window, content_box) 对 */
struct ScrolledPage {
    GtkWidget* sw;
    GtkWidget* content;
};

ScrolledPage scrolled_content() {
    ScrolledPage p;
    p.sw = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(p.sw, TRUE);
    gtk_widget_set_hexpand(p.sw, TRUE);

    p.content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_vexpand(p.content, TRUE);
    gtk_widget_set_hexpand(p.content, TRUE);
    gtk_widget_set_margin_start(p.content, 25);
    gtk_widget_set_margin_end(p.content, 25);
    gtk_widget_set_margin_top(p.content, 10);
    gtk_widget_set_margin_bottom(p.content, 25);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(p.sw), p.content);
    return p;
}

/* 卡片容器 */
GtkWidget* build_card(const char* title, GtkWidget* inner) {
    GtkWidget* card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(card, "card");
    gtk_widget_set_hexpand(card, TRUE);
    gtk_widget_set_margin_bottom(card, 16);

    GtkWidget* tl = gtk_label_new(title);
    gtk_widget_add_css_class(tl, "card-title");
    gtk_widget_set_halign(tl, GTK_ALIGN_START);
    gtk_widget_set_margin_start(tl, 20);
    gtk_widget_set_margin_end(tl, 20);
    gtk_widget_set_margin_top(tl, 16);
    gtk_box_append(GTK_BOX(card), tl);

    if (inner) {
        gtk_widget_set_margin_start(inner, 20);
        gtk_widget_set_margin_end(inner, 20);
        gtk_widget_set_margin_top(inner, 12);
        gtk_widget_set_margin_bottom(inner, 16);
        gtk_box_append(GTK_BOX(card), inner);
    }
    return card;
}

/* ── 控件构建 helpers ─────────────────────────────────────────────────── */

GtkWidget* make_dropdown(const char* const* items, int n, int sel, const char* cfg_path) {
    GtkStringList* model = gtk_string_list_new(nullptr);
    for (int i = 0; i < n; i++) gtk_string_list_append(model, items[i]);
    GtkWidget* dd = gtk_drop_down_new(G_LIST_MODEL(model), nullptr);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(dd), sel);
    gtk_widget_set_hexpand(dd, TRUE);
    gtk_widget_set_valign(dd, GTK_ALIGN_CENTER);

    std::string path(cfg_path);
    g_signal_connect(dd, "notify::selected",
        G_CALLBACK(+[](GObject* o, GParamSpec*, gpointer d) {
            auto* path = static_cast<std::string*>(d);
            guint idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(o));
            ConfigManager::instance().set(*path, (int)idx);
        }), new std::string(path));
    return dd;
}

/** 下拉框 — 选项对应字符串配置值 (非整数索引) */
GtkWidget* make_dropdown_str(const char* const* labels,
                              const char* const* values,
                              int n, int sel,
                              const char* cfg_path)
{
    GtkStringList* model = gtk_string_list_new(nullptr);
    for (int i = 0; i < n; i++) gtk_string_list_append(model, labels[i]);
    GtkWidget* dd = gtk_drop_down_new(G_LIST_MODEL(model), nullptr);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(dd), sel);
    gtk_widget_set_hexpand(dd, TRUE);
    gtk_widget_set_valign(dd, GTK_ALIGN_CENTER);

    // 复制 values 数组
    auto* vals = new std::vector<std::string>();
    for (int i = 0; i < n; i++) vals->push_back(values[i]);

    auto* path = new std::string(cfg_path);
    g_object_set_data(G_OBJECT(dd), "dd-vals", vals);
    g_object_set_data(G_OBJECT(dd), "dd-path", path);

    g_signal_connect(dd, "notify::selected",
        G_CALLBACK(+[](GObject* o, GParamSpec*, gpointer) {
            guint idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(o));
            auto* vals = static_cast<std::vector<std::string>*>(
                g_object_get_data(G_OBJECT(o), "dd-vals"));
            auto* path = static_cast<std::string*>(
                g_object_get_data(G_OBJECT(o), "dd-path"));
            if (vals && idx < vals->size() && path)
                ConfigManager::instance().set(*path, (*vals)[idx]);
        }), nullptr);
    return dd;
}

GtkWidget* make_check(const char* label, const char* cfg_path, bool def) {
    GtkWidget* cb = gtk_check_button_new_with_label(label);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(cb), def);
    gtk_widget_set_margin_bottom(cb, 6);

    std::string path(cfg_path);
    g_signal_connect(cb, "toggled",
        G_CALLBACK(+[](GtkCheckButton* btn, gpointer d) {
            auto* path = static_cast<std::string*>(d);
            ConfigManager::instance().set(*path,
                gtk_check_button_get_active(btn) ? true : false);
        }), new std::string(path));
    return cb;
}

GtkWidget* make_entry(const char* cfg_path, const char* placeholder = nullptr) {
    GtkWidget* e = gtk_entry_new();
    if (placeholder) gtk_entry_set_placeholder_text(GTK_ENTRY(e), placeholder);
    gtk_widget_set_hexpand(e, TRUE);

    std::string path(cfg_path);
    g_signal_connect(e, "notify::text",
        G_CALLBACK(+[](GObject* o, GParamSpec*, gpointer d) {
            auto* path = static_cast<std::string*>(d);
            ConfigManager::instance().set(*path,
                std::string(gtk_editable_get_text(GTK_EDITABLE(o))));
        }), new std::string(path));
    return e;
}

GtkWidget* labeled_row(const char* label, GtkWidget* widget) {
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_bottom(row, 7);

    GtkWidget* lbl = gtk_label_new(label);
    gtk_widget_set_valign(lbl, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_widget_set_size_request(lbl, 110, -1);
    gtk_box_append(GTK_BOX(row), lbl);

    if (widget) {
        gtk_widget_set_valign(widget, GTK_ALIGN_CENTER);
        gtk_box_append(GTK_BOX(row), widget);
    }
    return row;
}

GtkWidget* make_slider(const char* cfg_path, double min, double max,
                        double step, double def, const char* fmt = "%.0f") {
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_bottom(row, 7);

    GtkAdjustment* adj = gtk_adjustment_new(def, min, max, step, step, 0);
    GtkWidget* scale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, adj);
    gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
    gtk_widget_set_hexpand(scale, TRUE);
    gtk_box_append(GTK_BOX(row), scale);

    GtkWidget* val_lbl = gtk_label_new("");
    gtk_widget_set_size_request(val_lbl, 60, -1);
    gtk_widget_set_halign(val_lbl, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(row), val_lbl);

    // Store label + format on adjustment
    g_object_set_data(G_OBJECT(adj), "slider-label", val_lbl);
    g_object_set_data_full(G_OBJECT(adj), "slider-fmt", g_strdup(fmt), g_free);
    g_signal_connect(adj, "value-changed",
        G_CALLBACK(+[](GtkAdjustment* a, gpointer) {
            GtkLabel* l = GTK_LABEL(g_object_get_data(G_OBJECT(a), "slider-label"));
            const char* f = static_cast<const char*>(g_object_get_data(G_OBJECT(a), "slider-fmt"));
            if (!l || !f) return;
            char buf[32];
            std::snprintf(buf, sizeof(buf), f, gtk_adjustment_get_value(a));
            gtk_label_set_text(l, buf);
        }), nullptr);

    // Connect to config
    auto* path = new std::string(cfg_path);
    g_signal_connect(adj, "value-changed",
        G_CALLBACK(+[](GtkAdjustment* a, gpointer d) {
            auto* p = static_cast<std::string*>(d);
            ConfigManager::instance().set(*p, (int)gtk_adjustment_get_value(a));
        }), path);

    // Init display
    char buf[32];
    std::snprintf(buf, sizeof(buf), fmt, def);
    gtk_label_set_text(GTK_LABEL(val_lbl), buf);

    return row;
}

/** 带输入框的滑块 — 双向同步 (用户可手动输入数值) */
GtkWidget* make_slider_entry(const char* cfg_path, double min, double max,
                              double step, double def, const char* fmt = "%.0f")
{
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_bottom(row, 7);

    GtkAdjustment* adj = gtk_adjustment_new(def, min, max, step, step, 0);
    GtkWidget* scale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, adj);
    gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
    gtk_widget_set_hexpand(scale, TRUE);
    gtk_box_append(GTK_BOX(row), scale);

    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_input_purpose(GTK_ENTRY(entry), GTK_INPUT_PURPOSE_DIGITS);
    gtk_widget_set_size_request(entry, 72, -1);
    gtk_widget_set_valign(entry, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(row), entry);

    // Init entry text
    {
        char buf[32];
        std::snprintf(buf, sizeof(buf), fmt, def);
        gtk_editable_set_text(GTK_EDITABLE(entry), buf);
    }

    // Store on adjustment: entry + format + min/max
    g_object_set_data(G_OBJECT(adj), "se-entry", entry);
    g_object_set_data_full(G_OBJECT(adj), "se-fmt", g_strdup(fmt), g_free);
    auto* min_p = new double(min);
    auto* max_p = new double(max);
    g_object_set_data(G_OBJECT(adj), "se-min", min_p);
    g_object_set_data(G_OBJECT(adj), "se-max", max_p);

    // Scale → Entry
    g_signal_connect(adj, "value-changed",
        G_CALLBACK(+[](GtkAdjustment* a, gpointer) {
            GtkEditable* e = GTK_EDITABLE(g_object_get_data(G_OBJECT(a), "se-entry"));
            const char* f = static_cast<const char*>(g_object_get_data(G_OBJECT(a), "se-fmt"));
            if (!e || !f) return;
            char buf[32];
            std::snprintf(buf, sizeof(buf), f, gtk_adjustment_get_value(a));
            gtk_editable_set_text(e, buf);
        }), nullptr);

    // Scale → Config
    auto* path = new std::string(cfg_path);
    g_signal_connect(adj, "value-changed",
        G_CALLBACK(+[](GtkAdjustment* a, gpointer d) {
            auto* p = static_cast<std::string*>(d);
            ConfigManager::instance().set(*p, (int)gtk_adjustment_get_value(a));
        }), path);

    // Entry → Scale + Config (on Enter)
    auto entry_cb = +[](GtkEntry* e, gpointer data) {
        GtkAdjustment* a = GTK_ADJUSTMENT(data);
        const char* text = gtk_editable_get_text(GTK_EDITABLE(e));
        double val = g_strtod(text, nullptr);
        double min = *static_cast<double*>(g_object_get_data(G_OBJECT(a), "se-min"));
        double max = *static_cast<double*>(g_object_get_data(G_OBJECT(a), "se-max"));
        if (val < min) val = min;
        if (val > max) val = max;
        gtk_adjustment_set_value(a, val);
    };
    g_signal_connect(entry, "activate", G_CALLBACK(entry_cb), adj);

    return row;
}

}  // anonymous namespace


/* ============================================================================
 * P1 — 启动设置 (3 卡片, ~18 配置项)
 * ============================================================================ */

GtkWidget* build_page_launch_set()
{
    using CM = ConfigManager;
    auto& cfg = CM::instance();

    auto sp = scrolled_content();
    GtkWidget* sw = sp.sw;
    GtkWidget* content = sp.content;

    /* ── Card 1: 启动选项 ──────────────────────────────────────────────── */
    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        /* 实例隔离 */
        {
            const char* items[] = {"关闭", "仅 Modded", "非发布版",
                                   "Modded + 非发布", "全部"};
            int sel = cfg.get_or<int>("game.launch.instance_isolation", 1);
            GtkWidget* cb = make_dropdown(items, 5, sel, "game.launch.instance_isolation");
            gtk_box_append(GTK_BOX(inner), labeled_row("实例隔离", cb));
        }

        /* 游戏窗口标题 */
        {
            std::string v = cfg.get_or<std::string>("game.window.title", "");
            GtkWidget* e = make_entry("game.window.title", "留空为默认");
            gtk_editable_set_text(GTK_EDITABLE(e), v.c_str());
            gtk_box_append(GTK_BOX(inner), labeled_row("窗口标题", e));
        }

        /* 自定义信息 */
        {
            std::string v = cfg.get_or<std::string>("game.launch.custom_info", "PCL");
            GtkWidget* e = make_entry("game.launch.custom_info");
            gtk_editable_set_text(GTK_EDITABLE(e), v.c_str());
            gtk_box_append(GTK_BOX(inner), labeled_row("自定义信息", e));
        }

        /* 启动器可见性 */
        {
            const char* items[] = {"关闭", "隐藏并关闭", "隐藏并保持",
                                   "最小化", "保留"};
            int sel = cfg.get_or<int>("game.window.visibility", 4);
            GtkWidget* cb = make_dropdown(items, 5, sel, "game.window.visibility");
            gtk_box_append(GTK_BOX(inner), labeled_row("启动器可见性", cb));
        }

        /* 进程优先级 */
        {
            const char* items[] = {"实时", "高", "高于标准", "标准", "低于标准"};
            int sel = cfg.get_or<int>("game.launch.priority", 3);
            GtkWidget* cb = make_dropdown(items, 5, sel, "game.launch.priority");
            gtk_box_append(GTK_BOX(inner), labeled_row("进程优先级", cb));
        }

        /* 窗口大小 (dropdown + 条件自定义 WxH) */
        {
            const char* items[] = {"全屏", "默认", "同启动器", "自定义", "最大化"};
            std::string mode = cfg.get_or<std::string>("game.window.size_mode", "default");
            int sel = 1;
            if (mode == "fullscreen") sel = 0;
            else if (mode == "default") sel = 1;
            else if (mode == "same_as_launcher") sel = 2;
            else if (mode == "custom") sel = 3;
            else if (mode == "maximized") sel = 4;

            GtkStringList* model = gtk_string_list_new(nullptr);
            for (int i = 0; i < 5; i++) gtk_string_list_append(model, items[i]);
            GtkWidget* dd = gtk_drop_down_new(G_LIST_MODEL(model), nullptr);
            gtk_drop_down_set_selected(GTK_DROP_DOWN(dd), sel);
            gtk_widget_set_hexpand(dd, TRUE);
            gtk_box_append(GTK_BOX(inner), labeled_row("窗口大小", dd));

            /* 自定义宽高 (GtkEntry 并排, 条件显示) */
            int cw = cfg.get_or<int>("game.window.custom_width", 854);
            int ch = cfg.get_or<int>("game.window.custom_height", 480);

            GtkWidget* size_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            gtk_widget_set_margin_bottom(size_row, 7);
            gtk_widget_set_margin_start(size_row, 122);

            GtkWidget* lbl_w = gtk_label_new("宽");
            gtk_box_append(GTK_BOX(size_row), lbl_w);
            GtkWidget* ew = gtk_entry_new();
            gtk_entry_set_input_purpose(GTK_ENTRY(ew), GTK_INPUT_PURPOSE_DIGITS);
            {
                char buf[16]; std::snprintf(buf, sizeof(buf), "%d", cw);
                gtk_editable_set_text(GTK_EDITABLE(ew), buf);
            }
            gtk_widget_set_size_request(ew, 72, -1);
            gtk_box_append(GTK_BOX(size_row), ew);

            GtkWidget* lbl_h = gtk_label_new("高");
            gtk_box_append(GTK_BOX(size_row), lbl_h);
            GtkWidget* eh = gtk_entry_new();
            gtk_entry_set_input_purpose(GTK_ENTRY(eh), GTK_INPUT_PURPOSE_DIGITS);
            {
                char buf[16]; std::snprintf(buf, sizeof(buf), "%d", ch);
                gtk_editable_set_text(GTK_EDITABLE(eh), buf);
            }
            gtk_widget_set_size_request(eh, 72, -1);
            gtk_box_append(GTK_BOX(size_row), eh);

            if (sel != 3)
                gtk_widget_set_visible(size_row, FALSE);
            gtk_box_append(GTK_BOX(inner), size_row);

            /* 连接 dropdown → 条件显示 + config */
            g_object_set_data(G_OBJECT(dd), "size-row", size_row);
            {
                auto cb = +[](GObject* o, GParamSpec*, gpointer) -> void {
                    guint idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(o));
                    GtkWidget* sr = GTK_WIDGET(g_object_get_data(G_OBJECT(o), "size-row"));
                    const char* modes[] = {"fullscreen", "default",
                        "same_as_launcher", "custom", "maximized"};
                    ConfigManager::instance().set("game.window.size_mode",
                        std::string(modes[idx]));
                    gtk_widget_set_visible(sr, idx == 3);
                };
                g_signal_connect_data(dd, "notify::selected",
                    (GCallback)cb, nullptr, nullptr, G_CONNECT_DEFAULT);
            }

            /* 宽高 entry 变化 → config */
            auto* pw = new std::string("game.window.custom_width");
            auto* ph = new std::string("game.window.custom_height");
            g_signal_connect(ew, "notify::text",
                G_CALLBACK(+[](GObject* o, GParamSpec*, gpointer d) {
                    auto* p = static_cast<std::string*>(d);
                    int v = std::atoi(gtk_editable_get_text(GTK_EDITABLE(o)));
                    ConfigManager::instance().set(*p, v);
                }), pw);
            g_signal_connect(eh, "notify::text",
                G_CALLBACK(+[](GObject* o, GParamSpec*, gpointer d) {
                    auto* p = static_cast<std::string*>(d);
                    int v = std::atoi(gtk_editable_get_text(GTK_EDITABLE(o)));
                    ConfigManager::instance().set(*p, v);
                }), ph);
        }

        /* 微软登录方式 */
        {
            const char* items[] = {"网页", "设备代码"};
            int sel = cfg.get_or<int>("account.microsoft.auth_method", 0);
            GtkWidget* cb = make_dropdown(items, 2, sel, "account.microsoft.auth_method");
            gtk_box_append(GTK_BOX(inner), labeled_row("微软登录方式", cb));
        }

        /* IP 协议栈 */
        {
            const char* items[] = {"IPv4", "Java 默认", "IPv6"};
            int sel = cfg.get_or<int>("game.launch.ip_stack", 0);
            GtkWidget* cb = make_dropdown(items, 3, sel, "game.launch.ip_stack");
            gtk_box_append(GTK_BOX(inner), labeled_row("IP 协议栈", cb));
        }

        gtk_box_append(GTK_BOX(content), build_card("启动选项", inner));
    }

    /* ── Card 2: 内存 ────────────────────────────────────────────────── */
    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        std::string mode = cfg.get_or<std::string>("game.java.memory.mode", "auto");
        int max_mb = cfg.get_or<int>("game.java.memory.max_mb", 2048);

        /* 模式切换按钮 (auto / custom) */
        const char* modes[] = {"自动分配", "自定义"};
        GtkWidget* toggle = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
        gtk_widget_add_css_class(toggle, "linked");
        gtk_widget_set_margin_bottom(toggle, 7);

        GtkWidget* btn_auto = gtk_toggle_button_new_with_label(modes[0]);
        GtkWidget* btn_custom = gtk_toggle_button_new_with_label(modes[1]);
        gtk_widget_set_size_request(btn_auto, 90, -1);
        gtk_widget_set_size_request(btn_custom, 90, -1);

        if (mode == "auto")
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn_auto), TRUE);
        else
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn_custom), TRUE);

        gtk_box_append(GTK_BOX(toggle), btn_auto);
        gtk_box_append(GTK_BOX(toggle), btn_custom);
        gtk_box_append(GTK_BOX(inner), labeled_row("内存模式", toggle));

        /* 内存滑块 (custom 模式下可见) + 输入框 */
        GtkWidget* scale_row = make_slider_entry("game.java.memory.max_mb", 512, 16384, 128, max_mb, "%.0f");
        gtk_widget_set_margin_start(scale_row, 122);

        if (mode == "auto")
            gtk_widget_set_visible(scale_row, FALSE);
        gtk_box_append(GTK_BOX(inner), scale_row);

        /* 内存可视化条 */
        GtkWidget* bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_widget_set_size_request(bar, -1, 20);
        gtk_widget_set_margin_start(bar, 122);
        gtk_widget_set_margin_bottom(bar, 7);

        GtkWidget* used_seg = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_widget_add_css_class(used_seg, "mem-used");
        gtk_widget_set_size_request(used_seg, 0, -1);
        gtk_box_append(GTK_BOX(bar), used_seg);

        GtkWidget* free_seg = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_widget_add_css_class(free_seg, "mem-free");
        gtk_widget_set_hexpand(free_seg, TRUE);
        gtk_box_append(GTK_BOX(bar), free_seg);
        gtk_box_append(GTK_BOX(inner), bar);

        /* 自定义 Java 警告 — 始终显示为提示 */
        GtkWidget* hint = gtk_label_new(
            "检测到自定义 Java 运行时\n"
            "可能无法正确分配超过 1024 MB 的内存");
        gtk_widget_set_margin_start(hint, 122);
        gtk_widget_set_margin_bottom(hint, 7);
        gtk_widget_set_opacity(hint, 0.55);
        gtk_widget_set_halign(hint, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(inner), hint);

        /* ── 信号连接 ── */

        // mode toggle → config + visibility
        auto toggle_cb = +[](GtkToggleButton* self, gpointer data) {
            if (!gtk_toggle_button_get_active(self)) return;
            GtkWidget* other = GTK_WIDGET(data);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(other), FALSE);
        };
        g_signal_connect(btn_auto, "toggled",
            G_CALLBACK(toggle_cb), btn_custom);
        g_signal_connect(btn_custom, "toggled",
            G_CALLBACK(toggle_cb), btn_auto);

        g_object_set_data(G_OBJECT(btn_auto), "scale-row", scale_row);
        g_object_set_data(G_OBJECT(btn_custom), "scale-row", scale_row);

        auto mode_cb = +[](GtkToggleButton* btn, gpointer) {
            bool active = gtk_toggle_button_get_active(btn);
            if (!active) return;
            bool is_auto = (std::string(gtk_button_get_label(GTK_BUTTON(btn))) == "自动分配");
            ConfigManager::instance().set("game.java.memory.mode",
                std::string(is_auto ? "auto" : "custom"));
            GtkWidget* sr = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "scale-row"));
            if (sr) gtk_widget_set_visible(sr, !is_auto);
        };
        g_signal_connect(btn_auto, "toggled", G_CALLBACK(mode_cb), nullptr);
        g_signal_connect(btn_custom, "toggled", G_CALLBACK(mode_cb), nullptr);

        gtk_box_append(GTK_BOX(content), build_card("内存", inner));
    }

    /* ── Card 3: 高级选项 (可折叠) ────────────────────────────────────── */
    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        /* 渲染器 */
        {
            const char* items[] = {"自动", "Software", "D3D12", "Vulkan"};
            int sel = cfg.get_or<int>("game.java.renderer", 0);
            GtkWidget* cb = make_dropdown(items, 4, sel, "game.java.renderer");
            gtk_box_append(GTK_BOX(inner), labeled_row("渲染器", cb));
        }

        /* JVM 参数 + [重置] */
        {
            GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            gtk_widget_set_margin_bottom(row, 7);
            GtkWidget* lbl = gtk_label_new("JVM 参数");
            gtk_widget_set_size_request(lbl, 110, -1);
            gtk_widget_set_halign(lbl, GTK_ALIGN_START);
            gtk_box_append(GTK_BOX(row), lbl);

            GtkWidget* e = gtk_entry_new();
            gtk_entry_set_placeholder_text(GTK_ENTRY(e), "-Xmx2048m -XX:+UseG1GC");
            gtk_editable_set_text(GTK_EDITABLE(e),
                cfg.get_or<std::string>("game.java.jvm_args", "").c_str());
            gtk_widget_set_hexpand(e, TRUE);
            gtk_box_append(GTK_BOX(row), e);

            GtkWidget* reset = gtk_button_new_with_label("重置");
            g_object_set_data(G_OBJECT(reset), "entry", e);
            g_signal_connect(reset, "clicked",
                G_CALLBACK(+[](GtkButton*, gpointer d) {
                    GtkEntry* en = GTK_ENTRY(d);
                    gtk_editable_set_text(GTK_EDITABLE(en), "");
                }), e);
            gtk_box_append(GTK_BOX(row), reset);

            std::string path = "game.java.jvm_args";
            auto* sp = new std::string(path);
            g_signal_connect(e, "notify::text",
                G_CALLBACK(+[](GObject* o, GParamSpec*, gpointer d) {
                    auto* p = static_cast<std::string*>(d);
                    ConfigManager::instance().set(*p,
                        std::string(gtk_editable_get_text(GTK_EDITABLE(o))));
                }), sp);

            gtk_box_append(GTK_BOX(inner), row);
        }

        /* 游戏追加参数 */
        {
            GtkWidget* e = gtk_entry_new();
            gtk_entry_set_placeholder_text(GTK_ENTRY(e), "--arg1 --arg2");
            gtk_editable_set_text(GTK_EDITABLE(e),
                cfg.get_or<std::string>("game.java.game_args", "").c_str());
            gtk_widget_set_hexpand(e, TRUE);

            auto* sp = new std::string("game.java.game_args");
            g_signal_connect(e, "notify::text",
                G_CALLBACK(+[](GObject* o, GParamSpec*, gpointer d) {
                    auto* p = static_cast<std::string*>(d);
                    ConfigManager::instance().set(*p,
                        std::string(gtk_editable_get_text(GTK_EDITABLE(o))));
                }), sp);

            gtk_box_append(GTK_BOX(inner), labeled_row("游戏参数", e));
        }

        /* 预启动命令 + 等待 */
        {
            GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            gtk_widget_set_margin_bottom(row, 7);
            GtkWidget* lbl = gtk_label_new("预启动命令");
            gtk_widget_set_size_request(lbl, 110, -1);
            gtk_widget_set_halign(lbl, GTK_ALIGN_START);
            gtk_box_append(GTK_BOX(row), lbl);

            GtkWidget* e = gtk_entry_new();
            gtk_editable_set_text(GTK_EDITABLE(e),
                cfg.get_or<std::string>("game.launch.pre_launch_cmd", "").c_str());
            gtk_widget_set_hexpand(e, TRUE);
            gtk_box_append(GTK_BOX(row), e);

            auto* sp = new std::string("game.launch.pre_launch_cmd");
            g_signal_connect(e, "notify::text",
                G_CALLBACK(+[](GObject* o, GParamSpec*, gpointer d) {
                    auto* p = static_cast<std::string*>(d);
                    ConfigManager::instance().set(*p,
                        std::string(gtk_editable_get_text(GTK_EDITABLE(o))));
                }), sp);
            gtk_box_append(GTK_BOX(inner), row);

            bool wait = cfg.get_or<bool>("game.launch.pre_launch_wait", false);
            gtk_box_append(GTK_BOX(inner),
                make_check("等待命令执行完成", "game.launch.pre_launch_wait", wait));
        }

        /* 5 个复选框 */
        gtk_box_append(GTK_BOX(inner),
            make_check("禁用 JavaLaunchWrapper",
                       "game.java.disable_jlw",
                       cfg.get_or<bool>("game.java.disable_jlw", false)));
        gtk_box_append(GTK_BOX(inner),
            make_check("禁用 LaunchFramework",
                       "game.java.disable_lf",
                       cfg.get_or<bool>("game.java.disable_lf", false)));
        gtk_box_append(GTK_BOX(inner),
            make_check("偏好独立 GPU",
                       "game.java.prefer_dedicated_gpu",
                       cfg.get_or<bool>("game.java.prefer_dedicated_gpu", false)));
        gtk_box_append(GTK_BOX(inner),
            make_check("禁用 LWJGL Unsafe Agent",
                       "game.java.disable_lwjgl_unsafe",
                       cfg.get_or<bool>("game.java.disable_lwjgl_unsafe", false)));

        GtkWidget* expander = gtk_expander_new("高级选项");
        gtk_expander_set_child(GTK_EXPANDER(expander), inner);
        gtk_box_append(GTK_BOX(content), build_card("高级选项", expander));
    }

    return sw;
}


/* ============================================================================
 * P1 — Java 管理 (工具栏 + 动态列表)
 * ============================================================================ */

namespace {

void java_refresh_list(GtkListBox* lb)
{
    /* clear */
    GtkWidget* row = gtk_widget_get_first_child(GTK_WIDGET(lb));
    while (row) {
        GtkWidget* next = gtk_widget_get_next_sibling(row);
        gtk_list_box_remove(lb, row);
        row = next;
    }

    auto& cfg = ConfigManager::instance();
    auto arr = cfg.get_json("java.runtimes");
    int def_idx = cfg.get_or<int>("java.default_runtime", 0);

    for (size_t i = 0; i < arr.size(); i++) {
        auto& rt = arr[i];
        std::string name = rt.value("name", "Unknown");
        std::string path = rt.value("path", "");

        GtkWidget* item_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_widget_set_margin_start(item_row, 8);
        gtk_widget_set_margin_end(item_row, 8);
        gtk_widget_set_margin_top(item_row, 6);
        gtk_widget_set_margin_bottom(item_row, 6);

        /* info box */
        GtkWidget* info = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        GtkWidget* name_lbl = gtk_label_new(name.c_str());
        gtk_widget_set_halign(name_lbl, GTK_ALIGN_START);
        gtk_widget_add_css_class(name_lbl, "card-title");
        gtk_box_append(GTK_BOX(info), name_lbl);

        GtkWidget* path_lbl = gtk_label_new(path.c_str());
        gtk_widget_set_opacity(path_lbl, 0.55);
        gtk_widget_set_halign(path_lbl, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(info), path_lbl);

        gtk_box_append(GTK_BOX(item_row), info);
        gtk_widget_set_hexpand(info, TRUE);

        /* [设为默认] */
        if ((int)i == def_idx) {
            GtkWidget* badge = gtk_label_new("默认");
            gtk_widget_add_css_class(badge, "resource-tag");
            gtk_widget_set_valign(badge, GTK_ALIGN_CENTER);
            gtk_box_append(GTK_BOX(item_row), badge);
        } else {
            GtkWidget* sel_btn = gtk_button_new_with_label("选用");
            gtk_widget_set_valign(sel_btn, GTK_ALIGN_CENTER);
            auto* idx_ptr = new int((int)i);
            g_object_set_data(G_OBJECT(sel_btn), "lb", lb);
            g_object_set_data(G_OBJECT(sel_btn), "idx", idx_ptr);
            g_signal_connect(sel_btn, "clicked",
                G_CALLBACK(+[](GtkButton* btn, gpointer) {
                    auto* idx_ptr = static_cast<int*>(g_object_get_data(G_OBJECT(btn), "idx"));
                    ConfigManager::instance().set("java.default_runtime", *idx_ptr);
                    GtkListBox* lb = GTK_LIST_BOX(g_object_get_data(G_OBJECT(btn), "lb"));
                    if (lb) java_refresh_list(lb);
                }), nullptr);
            gtk_box_append(GTK_BOX(item_row), sel_btn);
        }

        /* [移除] */
        GtkWidget* rm_btn = gtk_button_new_with_label("移除");
        gtk_widget_set_valign(rm_btn, GTK_ALIGN_CENTER);
        gtk_widget_add_css_class(rm_btn, "destructive-action");
        auto* rm_idx = new size_t(i);
        g_object_set_data(G_OBJECT(rm_btn), "lb", lb);
        g_object_set_data(G_OBJECT(rm_btn), "rm-idx", rm_idx);
        g_signal_connect(rm_btn, "clicked",
            G_CALLBACK(+[](GtkButton* btn, gpointer) {
                auto* rm_idx = static_cast<size_t*>(
                    g_object_get_data(G_OBJECT(btn), "rm-idx"));
                GtkListBox* lb = GTK_LIST_BOX(
                    g_object_get_data(G_OBJECT(btn), "lb"));
                auto& cfg = ConfigManager::instance();
                auto arr = cfg.get_json("java.runtimes");
                size_t idx = *rm_idx;
                if (idx < arr.size()) {
                    arr.erase(arr.begin() + (long)idx);
                    cfg.set_json("java.runtimes", arr);
                    int def = cfg.get_or<int>("java.default_runtime", 0);
                    if ((size_t)def >= arr.size())
                        cfg.set("java.default_runtime", 0);
                }
                if (lb) java_refresh_list(lb);
            }), nullptr);
        gtk_box_append(GTK_BOX(item_row), rm_btn);

        gtk_list_box_append(lb, item_row);
    }
}

}  // anonymous namespace

GtkWidget* build_page_java_mgmt()
{
    auto sp = scrolled_content();
    GtkWidget* sw = sp.sw;
    GtkWidget* content = sp.content;

    GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

    /* 工具栏 */
    GtkWidget* toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_bottom(toolbar, 12);

    GtkWidget* add_btn = gtk_button_new_with_label("+ 添加 Java 运行时");
    gtk_widget_add_css_class(add_btn, "suggested-action");
    gtk_box_append(GTK_BOX(toolbar), add_btn);

    GtkWidget* refresh_btn = gtk_button_new_with_label("刷新");
    gtk_box_append(GTK_BOX(toolbar), refresh_btn);
    gtk_box_append(GTK_BOX(inner), toolbar);

    /* Java 运行时列表 */
    GtkWidget* list_box = gtk_list_box_new();
    gtk_widget_add_css_class(list_box, "rich-list");
    gtk_box_append(GTK_BOX(inner), list_box);

    java_refresh_list(GTK_LIST_BOX(list_box));

    g_object_set_data(G_OBJECT(refresh_btn), "lb", list_box);
    g_signal_connect(refresh_btn, "clicked",
        G_CALLBACK(+[](GtkButton*, gpointer d) {
            java_refresh_list(GTK_LIST_BOX(d));
        }), list_box);

    /* [+] 添加 Java 运行时 */
    g_object_set_data(G_OBJECT(add_btn), "lb", list_box);
    g_signal_connect(add_btn, "clicked",
        G_CALLBACK(+[](GtkButton* btn, gpointer) {
            GtkListBox* lb = GTK_LIST_BOX(
                g_object_get_data(G_OBJECT(btn), "lb"));

            GtkFileDialog* dlg = gtk_file_dialog_new();
            gtk_file_dialog_set_title(dlg, "选择 Java 可执行文件");

            GtkFileFilter* filter = gtk_file_filter_new();
            gtk_file_filter_set_name(filter, "Java 可执行文件");
            gtk_file_filter_add_pattern(filter, "java");

            GListStore* filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
            g_list_store_append(filters, filter);
            gtk_file_dialog_set_filters(dlg, G_LIST_MODEL(filters));

            gtk_file_dialog_open(dlg,
                GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(btn))),
                nullptr,
                +[](GObject* src, GAsyncResult* res, gpointer data) {
                    GFile* file = gtk_file_dialog_open_finish(
                        GTK_FILE_DIALOG(src), res, nullptr);
                    if (!file) return;

                    char* path = g_file_get_path(file);
                    std::string java_path(path);
                    g_free(path);

                    // 检测 Java 版本
                    std::string version_name = "Java";
                    std::string cmd = "\"" + java_path + "\" --version 2>&1";
                    FILE* p = popen(cmd.c_str(), "r");
                    if (p) {
                        char buf[256];
                        if (fgets(buf, sizeof(buf), p)) {
                            std::string s(buf);
                            auto pos = s.find("\"");
                            if (pos != std::string::npos) {
                                auto end = s.find("\"", pos + 1);
                                if (end != std::string::npos)
                                    version_name = s.substr(pos + 1, end - pos - 1);
                            } else {
                                auto vpos = s.find("version ");
                                if (vpos != std::string::npos) {
                                    auto ver = s.substr(vpos + 8);
                                    auto sp = ver.find(' ');
                                    if (sp != std::string::npos)
                                        ver = ver.substr(0, sp);
                                    version_name = "Java " + ver;
                                }
                            }
                        }
                        pclose(p);
                    }

                    auto& cfg = ConfigManager::instance();
                    auto arr = cfg.get_json("java.runtimes");
                    nlohmann::json entry;
                    entry["name"] = version_name;
                    entry["path"] = java_path;
                    arr.push_back(entry);
                    cfg.set_json("java.runtimes", arr);

                    // Rebuild list
                    java_refresh_list(GTK_LIST_BOX(data));
                }, lb);

            g_object_unref(dlg);
        }), nullptr);

    gtk_box_append(GTK_BOX(content), build_card("Java 运行时管理", inner));

    return sw;
}


/* ============================================================================
 * P1 — 游戏管理 (3 卡片, 12 项)
 * ============================================================================ */

GtkWidget* build_page_game_manage()
{
    using CM = ConfigManager;
    auto& cfg = CM::instance();

    auto sp = scrolled_content();
    GtkWidget* sw = sp.sw;
    GtkWidget* content = sp.content;

    /* ── Card 1: 下载源 ──────────────────────────────────────────────── */
    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        {
            const char* labels[] = {"仅镜像源", "镜像源优先", "官方源优先", "仅官方源"};
            const char* vals[]   = {"mirror", "prefer_mirror", "prefer_official", "official"};
            std::string cur = cfg.get_or<std::string>("game.download.source", "prefer_official");
            int sel = 2; // default
            for (int i = 0; i < 4; i++) { if (cur == vals[i]) { sel = i; break; } }
            gtk_box_append(GTK_BOX(inner),
                labeled_row("游戏文件源", make_dropdown_str(labels, vals, 4, sel, "game.download.source")));
        }
        {
            const char* labels[] = {"仅镜像源", "镜像源优先", "官方源优先", "仅官方源"};
            const char* vals[]   = {"mirror", "prefer_mirror", "prefer_official", "official"};
            std::string cur = cfg.get_or<std::string>("game.download.version_source", "prefer_official");
            int sel = 2;
            for (int i = 0; i < 4; i++) { if (cur == vals[i]) { sel = i; break; } }
            gtk_box_append(GTK_BOX(inner),
                labeled_row("版本列表源", make_dropdown_str(labels, vals, 4, sel, "game.download.version_source")));
        }
        {
            int v = cfg.get_or<int>("game.download.threads", 64);
            GtkWidget* s = make_slider_entry("game.download.threads", 1, 256, 1, v, "%.0f");
            gtk_box_append(GTK_BOX(inner), labeled_row("下载线程数", s));
        }
        {
            int v = cfg.get_or<int>("game.download.speed_limit_kbps", 0);
            GtkWidget* s = make_slider_entry("game.download.speed_limit_kbps", 0, 100000, 100, v, "%.0f");
            gtk_box_append(GTK_BOX(inner), labeled_row("下载速度限制 (KB/s)", s));
        }
        gtk_box_append(GTK_BOX(inner),
            make_check("自动选择实例", "game.download.auto_select_instance",
                       cfg.get_or<bool>("game.download.auto_select_instance", true)));
        gtk_box_append(GTK_BOX(inner),
            make_check("下载后修复 Authlib", "game.download.fix_authlib",
                       cfg.get_or<bool>("game.download.fix_authlib", true)));

        gtk_box_append(GTK_BOX(content), build_card("下载源", inner));
    }

    /* ── Card 2: 社区源 ──────────────────────────────────────────────── */
    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        {
            const char* labels[] = {"仅 CurseForge", "CurseForge 优先", "Modrinth 优先", "仅 Modrinth"};
            const char* vals[]   = {"curseforge", "prefer_curseforge", "prefer_modrinth", "modrinth"};
            std::string cur = cfg.get_or<std::string>("game.community.mod_source", "prefer_curseforge");
            int sel = 1;
            for (int i = 0; i < 4; i++) { if (cur == vals[i]) { sel = i; break; } }
            gtk_box_append(GTK_BOX(inner),
                labeled_row("Mod 下载源", make_dropdown_str(labels, vals, 4, sel, "game.community.mod_source")));
        }
        {
            std::string fmt = cfg.get_or<std::string>("game.community.filename_format",
                                                      "${filename}");
            GtkWidget* e = gtk_entry_new();
            gtk_entry_set_placeholder_text(GTK_ENTRY(e), "${filename}");
            gtk_editable_set_text(GTK_EDITABLE(e), fmt.c_str());
            gtk_widget_set_hexpand(e, TRUE);
            gtk_widget_set_valign(e, GTK_ALIGN_CENTER);

            auto* sp = new std::string("game.community.filename_format");
            g_signal_connect(e, "notify::text",
                G_CALLBACK(+[](GObject* o, GParamSpec*, gpointer d) {
                    auto* p = static_cast<std::string*>(d);
                    ConfigManager::instance().set(*p,
                        std::string(gtk_editable_get_text(GTK_EDITABLE(o))));
                }), sp);
            gtk_box_append(GTK_BOX(inner), labeled_row("文件名格式", e));

            /* 提示条：可用变量 */
            GtkWidget* hint = gtk_label_new(
                "可用变量:\n"
                "${filename} — 原名\n"
                "${zh_name} — 中文名\n"
                "${en_name} — 英文名");
            gtk_widget_set_margin_start(hint, 122);
            gtk_widget_set_margin_bottom(hint, 4);
            gtk_widget_set_opacity(hint, 0.55);
            gtk_widget_set_halign(hint, GTK_ALIGN_START);
            gtk_box_append(GTK_BOX(inner), hint);
        }
        {
            const char* labels[] = {"翻译优先", "文件名优先"};
            const char* vals[]   = {"translation_first", "filename_first"};
            std::string cur = cfg.get_or<std::string>("game.community.mod_list_style", "translation_first");
            int sel = (cur == "filename_first") ? 1 : 0;
            gtk_box_append(GTK_BOX(inner),
                labeled_row("Mod 列表样式", make_dropdown_str(labels, vals, 2, sel, "game.community.mod_list_style")));
        }
        gtk_box_append(GTK_BOX(inner),
            make_check("隐藏 Quilt", "game.community.hide_quilt",
                       cfg.get_or<bool>("game.community.hide_quilt", false)));
        gtk_box_append(GTK_BOX(inner),
            make_check("自动安装依赖", "game.community.auto_install_deps",
                       cfg.get_or<bool>("game.community.auto_install_deps", true)));

        gtk_box_append(GTK_BOX(content), build_card("社区资源", inner));
    }

    /* ── Card 3: 无障碍 ──────────────────────────────────────────────── */
    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        gtk_box_append(GTK_BOX(inner),
            make_check("正式版更新通知", "game.accessibility.release_notice",
                       cfg.get_or<bool>("game.accessibility.release_notice", true)));
        gtk_box_append(GTK_BOX(inner),
            make_check("快照版更新通知", "game.accessibility.snapshot_notice",
                       cfg.get_or<bool>("game.accessibility.snapshot_notice", false)));
        gtk_box_append(GTK_BOX(inner),
            make_check("自动设置游戏语言", "game.accessibility.auto_lang",
                       cfg.get_or<bool>("game.accessibility.auto_lang", true)));
        gtk_box_append(GTK_BOX(inner),
            make_check("读取剪贴板下载链接", "game.accessibility.read_clipboard",
                       cfg.get_or<bool>("game.accessibility.read_clipboard", true)));

        gtk_box_append(GTK_BOX(content), build_card("无障碍", inner));
    }

    return sw;
}


/* ============================================================================
 * 通用 helpers (P2+)
 * ============================================================================ */

namespace {

/** 数组型复选框 — 用于功能隐藏等多项选择配置 */
GtkWidget* make_array_check(const char* label, const char* cfg_path, const char* value)
{
    auto& cfg = ConfigManager::instance();
    auto arr = cfg.get_json(cfg_path);
    bool active = false;
    if (arr.is_array()) {
        for (auto& v : arr)
            if (v.is_string() && v.get<std::string>() == value) { active = true; break; }
    }

    GtkWidget* cb = gtk_check_button_new_with_label(label);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(cb), active);
    gtk_widget_set_margin_bottom(cb, 4);

    auto* path = new std::string(cfg_path);
    auto* val  = new std::string(value);
    g_object_set_data(G_OBJECT(cb), "arr-path", path);
    g_object_set_data(G_OBJECT(cb), "arr-val",  val);

    g_signal_connect(cb, "toggled",
        G_CALLBACK(+[](GtkCheckButton* btn, gpointer) {
            auto* path = static_cast<std::string*>(g_object_get_data(G_OBJECT(btn), "arr-path"));
            auto* val  = static_cast<std::string*>(g_object_get_data(G_OBJECT(btn), "arr-val"));
            bool checked = gtk_check_button_get_active(btn);

            auto& cfg = ConfigManager::instance();
            auto arr = cfg.get_json(*path);
            if (!arr.is_array()) arr = nlohmann::json::array();

            if (checked) {
                bool found = false;
                for (auto& v : arr)
                    if (v.is_string() && v.get<std::string>() == *val) { found = true; break; }
                if (!found) arr.push_back(*val);
            } else {
                for (auto it = arr.begin(); it != arr.end(); ++it) {
                    if (it->is_string() && it->get<std::string>() == *val) {
                        arr.erase(it); break;
                    }
                }
            }
            cfg.set_json(*path, arr);
        }), nullptr);
    return cb;
}

/** 小型分组标签 (用于功能隐藏卡片内的子分类) */
GtkWidget* section_label(const char* text) {
    GtkWidget* lbl = gtk_label_new(text);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_widget_set_margin_top(lbl, 8);
    gtk_widget_set_margin_bottom(lbl, 2);
    gtk_widget_set_opacity(lbl, 0.60);
    return lbl;
}

/** 创建操作按钮行 */
GtkWidget* action_buttons_row(std::initializer_list<const char*> labels) {
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_top(row, 8);
    gtk_widget_set_margin_start(row, 122);
    for (auto lbl : labels) {
        GtkWidget* btn = gtk_button_new_with_label(lbl);
        gtk_box_append(GTK_BOX(row), btn);
    }
    return row;
}

}  // anonymous namespace


/* ============================================================================
 * P2 — build_page_ui: 界面设置 (6 卡片, ~40+ 项)
 * ============================================================================ */

GtkWidget* build_page_ui()
{
    using CM = ConfigManager;
    auto& cfg = CM::instance();

    auto sp = scrolled_content();
    GtkWidget* sw = sp.sw;
    GtkWidget* content = sp.content;

    /* ── Card 1: 基础外观 ──────────────────────────────────────────────── */
    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        /* 启动器不透明度 */
        {
            double v = cfg.get_or<double>("launcher.ui.opacity", 1.0);
            GtkAdjustment* adj = gtk_adjustment_new(v, 0.0, 1.0, 0.05, 0.1, 0);
            GtkWidget* scale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, adj);
            gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
            gtk_widget_set_hexpand(scale, TRUE);

            GtkWidget* val_lbl = gtk_label_new("");
            {
                char buf[16];
                std::snprintf(buf, sizeof(buf), "%.0f%%", v * 100);
                gtk_label_set_text(GTK_LABEL(val_lbl), buf);
            }
            gtk_widget_set_size_request(val_lbl, 48, -1);
            gtk_widget_set_halign(val_lbl, GTK_ALIGN_END);

            GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
            gtk_widget_set_margin_bottom(row, 7);
            gtk_box_append(GTK_BOX(row), scale);
            gtk_box_append(GTK_BOX(row), val_lbl);

            g_object_set_data(G_OBJECT(adj), "opacity-label", val_lbl);
            auto* sp = new std::string("launcher.ui.opacity");
            g_signal_connect(adj, "value-changed",
                G_CALLBACK(+[](GtkAdjustment* a, gpointer d) {
                    auto* p = static_cast<std::string*>(d);
                    double val = gtk_adjustment_get_value(a);
                    ConfigManager::instance().set(*p, val);
                    GtkLabel* l = GTK_LABEL(g_object_get_data(G_OBJECT(a), "opacity-label"));
                    char buf[16];
                    std::snprintf(buf, sizeof(buf), "%.0f%%", val * 100);
                    gtk_label_set_text(l, buf);
                }), sp);

            gtk_box_append(GTK_BOX(inner), labeled_row("不透明度", row));
        }

        /* 主题模式 */
        {
            const char* labels[] = {"跟随系统", "浅色", "深色"};
            const char* vals[]   = {"follow_system", "light", "dark"};
            std::string cur = cfg.get_or<std::string>("launcher.ui.theme", "follow_system");
            int sel = (cur == "light") ? 1 : (cur == "dark") ? 2 : 0;
            gtk_box_append(GTK_BOX(inner),
                labeled_row("主题模式", make_dropdown_str(labels, vals, 3, sel, "launcher.ui.theme")));
        }

        /* 浅色主题色 */
        {
            const char* labels[] = {"蓝", "绿", "橙", "紫", "红"};
            const char* vals[]   = {"blue", "green", "orange", "purple", "red"};
            std::string cur = cfg.get_or<std::string>("launcher.ui.light_accent", "blue");
            int sel = 0;
            for (int i = 0; i < 5; i++) if (cur == vals[i]) { sel = i; break; }
            gtk_box_append(GTK_BOX(inner),
                labeled_row("浅色主题色", make_dropdown_str(labels, vals, 5, sel, "launcher.ui.light_accent")));
        }

        /* 深色主题色 */
        {
            const char* labels[] = {"蓝", "绿", "橙", "紫", "红"};
            const char* vals[]   = {"blue", "green", "orange", "purple", "red"};
            std::string cur = cfg.get_or<std::string>("launcher.ui.dark_accent", "blue");
            int sel = 0;
            for (int i = 0; i < 5; i++) if (cur == vals[i]) { sel = i; break; }
            gtk_box_append(GTK_BOX(inner),
                labeled_row("深色主题色", make_dropdown_str(labels, vals, 5, sel, "launcher.ui.dark_accent")));
        }

        gtk_box_append(GTK_BOX(inner),
            make_check("显示启动器图标", "launcher.ui.show_logo",
                       cfg.get_or<bool>("launcher.ui.show_logo", true)));
        gtk_box_append(GTK_BOX(inner),
            make_check("锁定窗口大小", "launcher.ui.lock_window",
                       cfg.get_or<bool>("launcher.ui.lock_window", false)));
        gtk_box_append(GTK_BOX(inner),
            make_check("显示启动趣闻", "launcher.ui.show_trivia",
                       cfg.get_or<bool>("launcher.ui.show_trivia", true)));

        /* 模糊 — 复选框 + 条件子项 */
        {
            bool blur_on = cfg.get_or<bool>("launcher.ui.blur_enabled", false);
            GtkWidget* blur_cb = make_check("启用高级材质 / 模糊", "launcher.ui.blur_enabled", blur_on);
            gtk_box_append(GTK_BOX(inner), blur_cb);

            /* 模糊参数区域 (条件显示) */
            GtkWidget* blur_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
            gtk_widget_set_margin_start(blur_box, 20);

            /* 模糊半径 */
            {
                int v = cfg.get_or<int>("launcher.ui.blur_radius", 20);
                GtkWidget* sr = make_slider("launcher.ui.blur_radius", 0, 40, 1, v, "%.0f");
                gtk_box_append(GTK_BOX(blur_box), labeled_row("模糊半径", sr));
            }
            /* 模糊采样率 */
            {
                int v = cfg.get_or<int>("launcher.ui.blur_sampling", 50);
                GtkWidget* sr = make_slider("launcher.ui.blur_sampling", 0, 100, 1, v, "%.0f %%");
                gtk_box_append(GTK_BOX(blur_box), labeled_row("采样率", sr));
            }
            /* 模糊方式 */
            {
                const char* labels[] = {"高斯模糊", "方框模糊"};
                const char* vals[]   = {"gaussian", "box"};
                std::string cur = cfg.get_or<std::string>("launcher.ui.blur_method", "gaussian");
                int sel = (cur == "box") ? 1 : 0;
                gtk_box_append(GTK_BOX(blur_box),
                    labeled_row("模糊方式", make_dropdown_str(labels, vals, 2, sel, "launcher.ui.blur_method")));
            }

            if (!blur_on) gtk_widget_set_visible(blur_box, FALSE);
            gtk_box_append(GTK_BOX(inner), blur_box);

            /* blur checkbox → 条件显示 */
            g_object_set_data(G_OBJECT(blur_cb), "blur-box", blur_box);
            g_signal_connect(blur_cb, "toggled",
                G_CALLBACK(+[](GtkCheckButton* btn, gpointer) {
                    GtkWidget* bb = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "blur-box"));
                    if (bb) gtk_widget_set_visible(bb, gtk_check_button_get_active(btn));
                }), nullptr);
        }

        gtk_box_append(GTK_BOX(content), build_card("基础外观", inner));
    }

    /* ── Card 2: 字体 ──────────────────────────────────────────────────── */
    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        /* 全局字体 */
        {
            std::string fam = cfg.get_or<std::string>("launcher.ui.font_family", "");
            GtkWidget* e = gtk_entry_new();
            gtk_entry_set_placeholder_text(GTK_ENTRY(e), "HarmonyOS Sans SC");
            gtk_editable_set_text(GTK_EDITABLE(e), fam.c_str());
            gtk_widget_set_hexpand(e, TRUE);

            auto* sp = new std::string("launcher.ui.font_family");
            g_signal_connect(e, "notify::text",
                G_CALLBACK(+[](GObject* o, GParamSpec*, gpointer d) {
                    auto* p = static_cast<std::string*>(d);
                    ConfigManager::instance().set(*p,
                        std::string(gtk_editable_get_text(GTK_EDITABLE(o))));
                }), sp);
            gtk_box_append(GTK_BOX(inner), labeled_row("全局字体", e));
        }
        /* MOTD 字体 */
        {
            std::string fam = cfg.get_or<std::string>("launcher.ui.motd_font_family", "");
            GtkWidget* e = gtk_entry_new();
            gtk_entry_set_placeholder_text(GTK_ENTRY(e), "等宽字体");
            gtk_editable_set_text(GTK_EDITABLE(e), fam.c_str());
            gtk_widget_set_hexpand(e, TRUE);

            auto* sp = new std::string("launcher.ui.motd_font_family");
            g_signal_connect(e, "notify::text",
                G_CALLBACK(+[](GObject* o, GParamSpec*, gpointer d) {
                    auto* p = static_cast<std::string*>(d);
                    ConfigManager::instance().set(*p,
                        std::string(gtk_editable_get_text(GTK_EDITABLE(o))));
                }), sp);
            gtk_box_append(GTK_BOX(inner), labeled_row("MOTD 字体", e));
        }

        gtk_box_append(GTK_BOX(content), build_card("字体", inner));
    }

    /* ── Card 3: 背景图 ────────────────────────────────────────────────── */
    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        /* 内容适应 */
        {
            const char* labels[] = {"智能", "居中", "包含", "拉伸", "平铺",
                                     "左上角", "右上角", "左下角", "右下角"};
            const char* vals[]   = {"smart", "center", "contain", "stretch", "tile",
                                     "corner_left_up", "corner_right_up",
                                     "corner_left_down", "corner_right_down"};
            std::string cur = cfg.get_or<std::string>("launcher.ui.background.fit", "smart");
            int sel = 0;
            for (int i = 0; i < 9; i++) if (cur == vals[i]) { sel = i; break; }
            gtk_box_append(GTK_BOX(inner),
                labeled_row("内容适应", make_dropdown_str(labels, vals, 9, sel, "launcher.ui.background.fit")));
        }
        /* 背景不透明度 */
        {
            double v = cfg.get_or<double>("launcher.ui.background.opacity", 1.0);
            GtkWidget* sr = make_slider("launcher.ui.background.opacity", 0.0, 1.0, 0.05, v, "%.0f%%");
            gtk_box_append(GTK_BOX(inner), labeled_row("背景不透明度", sr));
        }
        /* 背景模糊 */
        {
            int v = cfg.get_or<int>("launcher.ui.background.blur", 0);
            GtkWidget* sr = make_slider("launcher.ui.background.blur", 0, 40, 1, v, "%.0f");
            gtk_box_append(GTK_BOX(inner), labeled_row("背景模糊", sr));
        }

        gtk_box_append(GTK_BOX(inner),
            make_check("失去焦点时暂停背景视频", "launcher.ui.background.pause_blur",
                       cfg.get_or<bool>("launcher.ui.background.pause_blur", true)));
        gtk_box_append(GTK_BOX(inner),
            make_check("彩色覆盖层", "launcher.ui.background.color_overlay",
                       cfg.get_or<bool>("launcher.ui.background.color_overlay", false)));

        /* 操作按钮 */
        gtk_box_append(GTK_BOX(inner), action_buttons_row({"打开文件夹", "刷新", "清除"}));

        gtk_box_append(GTK_BOX(content), build_card("背景图", inner));
    }

    /* ── Card 4: Logo ──────────────────────────────────────────────────── */
    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        std::string logo_type = cfg.get_or<std::string>("launcher.ui.logo.type", "default");

        /* Logo 类型 (互斥按钮组) */
        {
            const char* items[] = {"无", "默认", "文字", "图片"};
            GtkWidget* toggle = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
            gtk_widget_add_css_class(toggle, "linked");
            gtk_widget_set_margin_bottom(toggle, 10);

            GtkWidget* btns[4];
            for (int i = 0; i < 4; i++) {
                btns[i] = gtk_toggle_button_new_with_label(items[i]);
                gtk_widget_set_size_request(btns[i], 70, -1);
                gtk_box_append(GTK_BOX(toggle), btns[i]);
            }

            int active = (logo_type == "none") ? 0 : (logo_type == "text") ? 2 : (logo_type == "image") ? 3 : 1;
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btns[active]), TRUE);

            /* 互斥逻辑 */
            auto toggle_cb = +[](GtkToggleButton* self, gpointer data) {
                if (!gtk_toggle_button_get_active(self)) return;
                GtkWidget* group = GTK_WIDGET(data);
                for (GtkWidget* sib = gtk_widget_get_first_child(group); sib;
                     sib = gtk_widget_get_next_sibling(sib)) {
                    if (sib != GTK_WIDGET(self) && GTK_IS_TOGGLE_BUTTON(sib))
                        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sib), FALSE);
                }
            };
            for (int i = 0; i < 4; i++)
                g_signal_connect(btns[i], "toggled", G_CALLBACK(toggle_cb), toggle);

            gtk_box_append(GTK_BOX(inner), labeled_row("Logo 类型", toggle));

            /* 条件控件 */
            GtkWidget* text_entry = gtk_entry_new();
            gtk_entry_set_placeholder_text(GTK_ENTRY(text_entry), "自定义文字");
            gtk_editable_set_text(GTK_EDITABLE(text_entry),
                cfg.get_or<std::string>("launcher.ui.logo.text", "").c_str());
            gtk_widget_set_hexpand(text_entry, TRUE);

            GtkWidget* image_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            GtkWidget* img_entry = gtk_entry_new();
            gtk_entry_set_placeholder_text(GTK_ENTRY(img_entry), "选择图片路径...");
            gtk_editable_set_text(GTK_EDITABLE(img_entry),
                cfg.get_or<std::string>("launcher.ui.logo.image_path", "").c_str());
            gtk_widget_set_hexpand(img_entry, TRUE);
            gtk_box_append(GTK_BOX(image_row), img_entry);
            GtkWidget* browse_btn = gtk_button_new_with_label("浏览");
            gtk_box_append(GTK_BOX(image_row), browse_btn);

            gtk_box_append(GTK_BOX(inner), text_entry);
            gtk_box_append(GTK_BOX(inner), image_row);

            if (logo_type != "text")  gtk_widget_set_visible(text_entry, FALSE);
            if (logo_type != "image") gtk_widget_set_visible(image_row, FALSE);

            /* Config bindings */
            auto* tex_p = new std::string("launcher.ui.logo.text");
            auto* img_p = new std::string("launcher.ui.logo.image_path");

            const char* type_vals[] = {"none", "default", "text", "image"};

            for (int i = 0; i < 4; i++) {
                g_object_set_data(G_OBJECT(btns[i]), "logo-type", (void*)type_vals[i]);
                g_object_set_data(G_OBJECT(btns[i]), "logo-text-entry", text_entry);
                g_object_set_data(G_OBJECT(btns[i]), "logo-image-row", image_row);

                g_signal_connect(btns[i], "toggled",
                    G_CALLBACK(+[](GtkToggleButton* btn, gpointer) {
                        if (!gtk_toggle_button_get_active(btn)) return;
                        const char* t = static_cast<const char*>(
                            g_object_get_data(G_OBJECT(btn), "logo-type"));
                        ConfigManager::instance().set("launcher.ui.logo.type", std::string(t));
                        GtkWidget* te = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "logo-text-entry"));
                        GtkWidget* ir = GTK_WIDGET(g_object_get_data(G_OBJECT(btn), "logo-image-row"));
                        if (te) gtk_widget_set_visible(te, std::string(t) == "text");
                        if (ir) gtk_widget_set_visible(ir, std::string(t) == "image");
                    }), nullptr);
            }

            g_signal_connect(text_entry, "notify::text",
                G_CALLBACK(+[](GObject* o, GParamSpec*, gpointer d) {
                    auto* p = static_cast<std::string*>(d);
                    ConfigManager::instance().set(*p,
                        std::string(gtk_editable_get_text(GTK_EDITABLE(o))));
                }), tex_p);

            g_signal_connect(img_entry, "notify::text",
                G_CALLBACK(+[](GObject* o, GParamSpec*, gpointer d) {
                    auto* p = static_cast<std::string*>(d);
                    ConfigManager::instance().set(*p,
                        std::string(gtk_editable_get_text(GTK_EDITABLE(o))));
                }), img_p);
        }

        gtk_box_append(GTK_BOX(content), build_card("Logo", inner));
    }

    /* ── Card 5: 主页 ──────────────────────────────────────────────────── */
    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        std::string hp_type = cfg.get_or<std::string>("launcher.ui.homepage.type", "preset");

        /* 主页类型 */
        {
            const char* items[] = {"空白", "预设", "本地", "联网"};
            GtkWidget* toggle = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
            gtk_widget_add_css_class(toggle, "linked");
            gtk_widget_set_margin_bottom(toggle, 10);

            GtkWidget* btns[4];
            for (int i = 0; i < 4; i++) {
                btns[i] = gtk_toggle_button_new_with_label(items[i]);
                gtk_widget_set_size_request(btns[i], 70, -1);
                gtk_box_append(GTK_BOX(toggle), btns[i]);
            }

            int active = (hp_type == "blank") ? 0 : (hp_type == "local") ? 2 : (hp_type == "remote") ? 3 : 1;
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btns[active]), TRUE);

            auto toggle_cb = +[](GtkToggleButton* self, gpointer data) {
                if (!gtk_toggle_button_get_active(self)) return;
                GtkWidget* group = GTK_WIDGET(data);
                for (GtkWidget* sib = gtk_widget_get_first_child(group); sib;
                     sib = gtk_widget_get_next_sibling(sib)) {
                    if (sib != GTK_WIDGET(self) && GTK_IS_TOGGLE_BUTTON(sib))
                        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sib), FALSE);
                }
            };
            for (int i = 0; i < 4; i++)
                g_signal_connect(btns[i], "toggled", G_CALLBACK(toggle_cb), toggle);

            gtk_box_append(GTK_BOX(inner), labeled_row("主页内容", toggle));

            /* 条件: 预设下拉 */
            {
                const char* presets[] = {"Trivia", "McNews", "SimpleHomepage",
                    "DailyModpack", "McSkin", "OpenBmclapi",
                    "PclMarket", "PclNews", "PclManual",
                    "Magazine", "GitHub", "McUpdateSummary",
                    "PclAnnouncement", "McOfficialFeed", nullptr};
                int n = 14;
                std::string cur = cfg.get_or<std::string>("launcher.ui.homepage.preset", "Trivia");
                GtkStringList* model = gtk_string_list_new(nullptr);
                int sel = 0;
                for (int i = 0; i < n; i++) {
                    gtk_string_list_append(model, presets[i]);
                    if (cur == presets[i]) sel = i;
                }
                GtkWidget* dd = gtk_drop_down_new(G_LIST_MODEL(model), nullptr);
                gtk_drop_down_set_selected(GTK_DROP_DOWN(dd), sel);
                gtk_widget_set_hexpand(dd, TRUE);

                GtkWidget* row = labeled_row("预设页面", dd);
                gtk_box_append(GTK_BOX(inner), row);
                if (hp_type != "preset") gtk_widget_set_visible(row, FALSE);
                g_object_set_data(G_OBJECT(toggle), "hp-preset-row", row);

                g_object_set_data(G_OBJECT(dd), "dd-path",
                    new std::string("launcher.ui.homepage.preset"));
                g_object_set_data_full(G_OBJECT(dd), "dd-presets",
                    g_strdupv((char**)presets), (GDestroyNotify)g_strfreev);
                g_signal_connect(dd, "notify::selected",
                    G_CALLBACK(+[](GObject* o, GParamSpec*, gpointer) {
                        guint idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(o));
                        auto* path = static_cast<std::string*>(g_object_get_data(G_OBJECT(o), "dd-path"));
                        char** presets = (char**)g_object_get_data(G_OBJECT(o), "dd-presets");
                        if (path && idx < 14 && presets)
                            ConfigManager::instance().set(*path, std::string(presets[idx]));
                    }), nullptr);
            }

            /* 条件: 本地路径 */
            {
                std::string v = cfg.get_or<std::string>("launcher.ui.homepage.local_path", "");
                GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
                GtkWidget* e = gtk_entry_new();
                gtk_entry_set_placeholder_text(GTK_ENTRY(e), "HTML 文件路径");
                gtk_editable_set_text(GTK_EDITABLE(e), v.c_str());
                gtk_widget_set_hexpand(e, TRUE);
                gtk_box_append(GTK_BOX(row), e);
                gtk_box_append(GTK_BOX(row), gtk_button_new_with_label("刷新"));
                gtk_box_append(GTK_BOX(row), gtk_button_new_with_label("教程"));

                GtkWidget* lr = labeled_row("本地路径", row);
                gtk_box_append(GTK_BOX(inner), lr);
                if (hp_type != "local") gtk_widget_set_visible(lr, FALSE);
                g_object_set_data(G_OBJECT(toggle), "hp-local-row", lr);

                auto* sp = new std::string("launcher.ui.homepage.local_path");
                g_signal_connect(e, "notify::text",
                    G_CALLBACK(+[](GObject* o, GParamSpec*, gpointer d) {
                        auto* p = static_cast<std::string*>(d);
                        ConfigManager::instance().set(*p,
                            std::string(gtk_editable_get_text(GTK_EDITABLE(o))));
                    }), sp);
            }

            /* 条件: 联网 URL */
            {
                std::string v = cfg.get_or<std::string>("launcher.ui.homepage.remote_url", "");
                GtkWidget* e = gtk_entry_new();
                gtk_entry_set_placeholder_text(GTK_ENTRY(e), "https://...");
                gtk_editable_set_text(GTK_EDITABLE(e), v.c_str());
                gtk_widget_set_hexpand(e, TRUE);

                GtkWidget* lr = labeled_row("网页 URL", e);
                gtk_box_append(GTK_BOX(inner), lr);
                if (hp_type != "remote") gtk_widget_set_visible(lr, FALSE);
                g_object_set_data(G_OBJECT(toggle), "hp-remote-row", lr);

                auto* sp = new std::string("launcher.ui.homepage.remote_url");
                g_signal_connect(e, "notify::text",
                    G_CALLBACK(+[](GObject* o, GParamSpec*, gpointer d) {
                        auto* p = static_cast<std::string*>(d);
                        ConfigManager::instance().set(*p,
                            std::string(gtk_editable_get_text(GTK_EDITABLE(o))));
                    }), sp);
            }

            /* 类型切换 → config + 条件显示 */
            const char* hp_type_vals[] = {"blank", "preset", "local", "remote"};
            for (int i = 0; i < 4; i++) {
                g_object_set_data(G_OBJECT(btns[i]), "hp-type", (void*)hp_type_vals[i]);
                g_signal_connect(btns[i], "toggled",
                    G_CALLBACK(+[](GtkToggleButton* btn, gpointer) {
                        if (!gtk_toggle_button_get_active(btn)) return;
                        const char* t = static_cast<const char*>(
                            g_object_get_data(G_OBJECT(btn), "hp-type"));
                        ConfigManager::instance().set("launcher.ui.homepage.type", std::string(t));

                        GtkWidget* group = gtk_widget_get_parent(GTK_WIDGET(btn));
                        GtkWidget* preset_r = GTK_WIDGET(g_object_get_data(G_OBJECT(group), "hp-preset-row"));
                        GtkWidget* local_r  = GTK_WIDGET(g_object_get_data(G_OBJECT(group), "hp-local-row"));
                        GtkWidget* remote_r = GTK_WIDGET(g_object_get_data(G_OBJECT(group), "hp-remote-row"));
                        std::string ts(t);
                        if (preset_r) gtk_widget_set_visible(preset_r, ts == "preset");
                        if (local_r)  gtk_widget_set_visible(local_r,  ts == "local");
                        if (remote_r) gtk_widget_set_visible(remote_r, ts == "remote");
                    }), nullptr);
            }
        }

        gtk_box_append(GTK_BOX(content), build_card("主页", inner));
    }

    /* ── Card 6: 功能隐藏 ──────────────────────────────────────────────── */
    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

        /* 主页标签 */
        gtk_box_append(GTK_BOX(inner), section_label("主页标签"));
        {
            GtkWidget* grid = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
            GtkWidget* col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            gtk_box_append(GTK_BOX(col), make_array_check("下载",  "launcher.ui.hidden.pages", "download"));
            gtk_box_append(GTK_BOX(col), make_array_check("设置",  "launcher.ui.hidden.pages", "settings"));
            gtk_box_append(GTK_BOX(col), make_array_check("工具",  "launcher.ui.hidden.pages", "tools"));
            gtk_box_append(GTK_BOX(grid), col);
            gtk_box_append(GTK_BOX(inner), grid);
        }

        /* 设置子页面 */
        gtk_box_append(GTK_BOX(inner), section_label("设置子页面"));
        {
            GtkWidget* grid = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
            GtkWidget* c1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            GtkWidget* c2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            gtk_box_append(GTK_BOX(c1), make_array_check("启动",    "launcher.ui.hidden.setup", "launch"));
            gtk_box_append(GTK_BOX(c1), make_array_check("Java",   "launcher.ui.hidden.setup", "java"));
            gtk_box_append(GTK_BOX(c1), make_array_check("游戏管理","launcher.ui.hidden.setup", "game_manage"));
            gtk_box_append(GTK_BOX(c1), make_array_check("界面",    "launcher.ui.hidden.setup", "ui"));
            gtk_box_append(GTK_BOX(c1), make_array_check("语言",    "launcher.ui.hidden.setup", "language"));
            gtk_box_append(GTK_BOX(c2), make_array_check("杂项",    "launcher.ui.hidden.setup", "misc"));
            gtk_box_append(GTK_BOX(c2), make_array_check("关于",    "launcher.ui.hidden.setup", "about"));
            gtk_box_append(GTK_BOX(c2), make_array_check("反馈",    "launcher.ui.hidden.setup", "feedback"));
            gtk_box_append(GTK_BOX(c2), make_array_check("日志",    "launcher.ui.hidden.setup", "log"));
            gtk_widget_set_hexpand(c2, TRUE);
            gtk_box_append(GTK_BOX(grid), c1);
            gtk_box_append(GTK_BOX(grid), c2);
            gtk_box_append(GTK_BOX(inner), grid);
        }

        /* 工具子页面 */
        gtk_box_append(GTK_BOX(inner), section_label("工具子页面"));
        {
            GtkWidget* grid = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
            GtkWidget* col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            gtk_box_append(GTK_BOX(col), make_array_check("工具箱",   "launcher.ui.hidden.tools", "toolbox"));
            gtk_box_append(GTK_BOX(grid), col);
            gtk_box_append(GTK_BOX(inner), grid);
        }

        /* 实例功能 */
        gtk_box_append(GTK_BOX(inner), section_label("实例功能"));
        {
            GtkWidget* grid = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
            GtkWidget* c1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            GtkWidget* c2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            gtk_box_append(GTK_BOX(c1), make_array_check("编辑",    "launcher.ui.hidden.instance", "edit"));
            gtk_box_append(GTK_BOX(c1), make_array_check("导出",    "launcher.ui.hidden.instance", "export"));
            gtk_box_append(GTK_BOX(c1), make_array_check("存档",    "launcher.ui.hidden.instance", "save"));
            gtk_box_append(GTK_BOX(c1), make_array_check("截图",    "launcher.ui.hidden.instance", "screenshot"));
            gtk_box_append(GTK_BOX(c1), make_array_check("Mod",    "launcher.ui.hidden.instance", "mod"));
            gtk_box_append(GTK_BOX(c2), make_array_check("资源包",   "launcher.ui.hidden.instance", "resource_pack"));
            gtk_box_append(GTK_BOX(c2), make_array_check("光影",    "launcher.ui.hidden.instance", "shader"));
            gtk_box_append(GTK_BOX(c2), make_array_check("原理图",   "launcher.ui.hidden.instance", "schematic"));
            gtk_box_append(GTK_BOX(c2), make_array_check("服务器",   "launcher.ui.hidden.instance", "server"));
            gtk_widget_set_hexpand(c2, TRUE);
            gtk_box_append(GTK_BOX(grid), c1);
            gtk_box_append(GTK_BOX(grid), c2);
            gtk_box_append(GTK_BOX(inner), grid);
        }

        /* 特定功能 */
        gtk_box_append(GTK_BOX(inner), section_label("特定功能"));
        {
            GtkWidget* grid = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
            GtkWidget* col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            gtk_box_append(GTK_BOX(col), make_array_check("版本选择",    "launcher.ui.hidden.functions", "version_select"));
            gtk_box_append(GTK_BOX(col), make_array_check("Mod 更新",   "launcher.ui.hidden.functions", "mod_update"));
            gtk_box_append(GTK_BOX(col), make_array_check("功能隐藏本身", "launcher.ui.hidden.functions", "feature_hide"));
            gtk_box_append(GTK_BOX(grid), col);
            gtk_box_append(GTK_BOX(inner), grid);
        }

        gtk_box_append(GTK_BOX(content), build_card("功能隐藏", inner));
    }

    return sw;
}


/* ============================================================================
 * P2 — build_page_language: 语言 (1 卡片, 2 项)
 * ============================================================================ */

GtkWidget* build_page_language()
{
    using CM = ConfigManager;
    auto& cfg = CM::instance();

    auto sp = scrolled_content();
    GtkWidget* sw = sp.sw;
    GtkWidget* content = sp.content;

    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        /* 界面语言 */
        {
            const char* labels[] = {"简体中文", "English (US)", "繁體中文"};
            const char* vals[]   = {"zh_CN", "en_US", "zh_TW"};
            std::string cur = cfg.get_or<std::string>("launcher.locale.ui_lang", "zh_CN");
            int sel = 0;
            for (int i = 0; i < 3; i++) if (cur == vals[i]) { sel = i; break; }
            gtk_box_append(GTK_BOX(inner),
                labeled_row("界面语言", make_dropdown_str(labels, vals, 3, sel, "launcher.locale.ui_lang")));
        }

        /* 格式化文化 */
        {
            const char* labels[] = {"简体中文 (zh-CN)", "English (en-US)", "繁體中文 (zh-TW)"};
            const char* vals[]   = {"zh-CN", "en-US", "zh-TW"};
            std::string cur = cfg.get_or<std::string>("launcher.locale.format_culture", "zh-CN");
            int sel = 0;
            for (int i = 0; i < 3; i++) if (cur == vals[i]) { sel = i; break; }
            gtk_box_append(GTK_BOX(inner),
                labeled_row("格式化文化", make_dropdown_str(labels, vals, 3, sel, "launcher.locale.format_culture")));
        }

        /* 提示条 */
        {
            GtkWidget* hint = gtk_label_new("语言更改将在重启后完全生效");
            gtk_widget_set_margin_start(hint, 122);
            gtk_widget_set_margin_top(hint, 12);
            gtk_widget_set_opacity(hint, 0.55);
            gtk_widget_set_halign(hint, GTK_ALIGN_START);
            gtk_box_append(GTK_BOX(inner), hint);
        }

        gtk_box_append(GTK_BOX(content), build_card("语言与区域", inner));
    }

    return sw;
}


/* ============================================================================
 * P2 — build_page_misc: 杂项 (3 卡片, 11 项)
 * ============================================================================ */

GtkWidget* build_page_misc()
{
    using CM = ConfigManager;
    auto& cfg = CM::instance();

    auto sp = scrolled_content();
    GtkWidget* sw = sp.sw;
    GtkWidget* content = sp.content;

    /* ── Card 1: 系统 ──────────────────────────────────────────────────── */
    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        /* 公告显示 */
        {
            const char* labels[] = {"全部公告", "仅重要", "关闭"};
            const char* vals[]   = {"all", "important", "off"};
            std::string cur = cfg.get_or<std::string>("launcher.system.announcements", "all");
            int sel = (cur == "important") ? 1 : (cur == "off") ? 2 : 0;
            gtk_box_append(GTK_BOX(inner),
                labeled_row("公告显示", make_dropdown_str(labels, vals, 3, sel, "launcher.system.announcements")));
        }

        /* 最大帧率 */
        {
            int v = cfg.get_or<int>("launcher.system.max_fps", 60);
            GtkWidget* sr = make_slider_entry("launcher.system.max_fps", 1, 240, 1, v, "%.0f");
            gtk_box_append(GTK_BOX(inner), labeled_row("最大帧率", sr));
        }

        /* 最大日志行数 */
        {
            int v = cfg.get_or<int>("launcher.system.max_log_lines", 10000);
            GtkWidget* sr = make_slider_entry("launcher.system.max_log_lines", 1000, 100000, 1000, v, "%.0f");
            gtk_box_append(GTK_BOX(inner), labeled_row("最大日志行数", sr));
        }

        gtk_box_append(GTK_BOX(inner),
            make_check("禁用硬件加速", "launcher.system.disable_hw_accel",
                       cfg.get_or<bool>("launcher.system.disable_hw_accel", false)));

        /* [导出设置] [导入设置] */
        gtk_box_append(GTK_BOX(inner), action_buttons_row({"导出设置", "导入设置"}));

        gtk_box_append(GTK_BOX(content), build_card("系统", inner));
    }

    /* ── Card 2: 网络 ──────────────────────────────────────────────────── */
    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        gtk_box_append(GTK_BOX(inner),
            make_check("DNS over HTTPS", "network.doh_enabled",
                       cfg.get_or<bool>("network.doh_enabled", false)));

        /* HTTP 代理 */
        {
            const char* items[] = {"无代理", "系统代理", "自定义"};
            const char* vals[]  = {"none", "system", "custom"};
            std::string cur = cfg.get_or<std::string>("network.http_proxy.type", "none");
            int sel = (cur == "system") ? 1 : (cur == "custom") ? 2 : 0;

            GtkWidget* toggle = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
            gtk_widget_add_css_class(toggle, "linked");
            gtk_widget_set_margin_bottom(toggle, 6);

            GtkWidget* btns[3];
            for (int i = 0; i < 3; i++) {
                btns[i] = gtk_toggle_button_new_with_label(items[i]);
                gtk_widget_set_size_request(btns[i], 80, -1);
                gtk_box_append(GTK_BOX(toggle), btns[i]);
            }
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btns[sel]), TRUE);

            auto toggle_cb = +[](GtkToggleButton* self, gpointer data) {
                if (!gtk_toggle_button_get_active(self)) return;
                GtkWidget* group = GTK_WIDGET(data);
                for (GtkWidget* sib = gtk_widget_get_first_child(group); sib;
                     sib = gtk_widget_get_next_sibling(sib)) {
                    if (sib != GTK_WIDGET(self) && GTK_IS_TOGGLE_BUTTON(sib))
                        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sib), FALSE);
                }
            };
            for (int i = 0; i < 3; i++)
                g_signal_connect(btns[i], "toggled", G_CALLBACK(toggle_cb), toggle);

            gtk_box_append(GTK_BOX(inner), labeled_row("HTTP 代理", toggle));

            /* 自定义代理 URL */
            {
                std::string url = cfg.get_or<std::string>("network.http_proxy.url", "");
                GtkWidget* e = gtk_entry_new();
                gtk_entry_set_placeholder_text(GTK_ENTRY(e), "http://host:port");
                gtk_editable_set_text(GTK_EDITABLE(e), url.c_str());
                gtk_widget_set_hexpand(e, TRUE);
                GtkWidget* lr = labeled_row("代理 URL", e);
                if (cur != "custom") gtk_widget_set_visible(lr, FALSE);
                gtk_box_append(GTK_BOX(inner), lr);

                auto* sp = new std::string("network.http_proxy.url");
                g_signal_connect(e, "notify::text",
                    G_CALLBACK(+[](GObject* o, GParamSpec*, gpointer d) {
                        auto* p = static_cast<std::string*>(d);
                        ConfigManager::instance().set(*p,
                            std::string(gtk_editable_get_text(GTK_EDITABLE(o))));
                    }), sp);

                g_object_set_data(G_OBJECT(toggle), "proxy-url-row", lr);
            }

            /* 类型切换 → config + 条件显示 */
            for (int i = 0; i < 3; i++) {
                g_object_set_data(G_OBJECT(btns[i]), "proxy-type", (void*)vals[i]);
                g_signal_connect(btns[i], "toggled",
                    G_CALLBACK(+[](GtkToggleButton* btn, gpointer) {
                        if (!gtk_toggle_button_get_active(btn)) return;
                        const char* t = static_cast<const char*>(
                            g_object_get_data(G_OBJECT(btn), "proxy-type"));
                        ConfigManager::instance().set("network.http_proxy.type", std::string(t));
                        GtkWidget* group = gtk_widget_get_parent(GTK_WIDGET(btn));
                        GtkWidget* url_row = GTK_WIDGET(g_object_get_data(G_OBJECT(group), "proxy-url-row"));
                        if (url_row) gtk_widget_set_visible(url_row, std::string(t) == "custom");
                    }), nullptr);
            }
        }

        gtk_box_append(GTK_BOX(content), build_card("网络", inner));
    }

    /* ── Card 3: 调试 (折叠) ────────────────────────────────────────────── */
    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        /* 动画速度 */
        {
            double v = cfg.get_or<double>("launcher.debug.anim_speed", 1.0);
            GtkWidget* sr = make_slider_entry("launcher.debug.anim_speed", 0.1, 5.0, 0.1, v, "%.1f");
            gtk_box_append(GTK_BOX(inner), labeled_row("动画速度", sr));
        }

        gtk_box_append(GTK_BOX(inner),
            make_check("跳过文件拷贝", "launcher.debug.skip_copy",
                       cfg.get_or<bool>("launcher.debug.skip_copy", false)));
        gtk_box_append(GTK_BOX(inner),
            make_check("调试模式", "launcher.debug.mode",
                       cfg.get_or<bool>("launcher.debug.mode", false)));
        gtk_box_append(GTK_BOX(inner),
            make_check("调试延迟", "launcher.debug.delay",
                       cfg.get_or<bool>("launcher.debug.delay", false)));

        GtkWidget* expander = gtk_expander_new("调试选项");
        gtk_expander_set_expanded(GTK_EXPANDER(expander), FALSE);
        gtk_expander_set_child(GTK_EXPANDER(expander), inner);
        gtk_box_append(GTK_BOX(content), build_card("调试选项", expander));
    }

    return sw;
}


/* ============================================================================
 * P3 — build_page_about: 关于 (6 卡片, 静态内容)
 * ============================================================================ */

GtkWidget* build_page_about()
{
    auto sp = scrolled_content();
    GtkWidget* sw = sp.sw;
    GtkWidget* content = sp.content;

    /* ── Card 1: 作者信息 ──────────────────────────────────────────────── */
    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

        struct Author {
            const char* avatar_label;
            const char* name;
            const char* role;
            const char* btn_label;
            const char* url;
        };
        const Author authors[] = {
            {"🖼", "LTCat",       "原作者",      "赞助原作者", "https://afdian.com/a/LTCat"},
            {"🖼", "PCL-Community","社区维护",    "GitHub 主页","https://github.com/PCL-Community"},
            {"🖼", "X-LeeHe",     "PCL-CGRE 作者","GitHub 主页","https://github.com/X-LeeHe"},
            {"🖼", "PCL-CGRE",    "v0.1.0-dev",  "源代码",     "https://github.com/X-LeeHe/PCL-CGRE"},
        };

        for (auto& a : authors) {
            GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
            gtk_widget_set_margin_bottom(row, 8);

            /* avatar placeholder */
            GtkWidget* av = gtk_label_new(a.avatar_label);
            gtk_widget_add_css_class(av, "ack-avatar");
            gtk_widget_set_halign(av, GTK_ALIGN_CENTER);
            gtk_box_append(GTK_BOX(row), av);

            /* name + role */
            GtkWidget* info = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            GtkWidget* name_lbl = gtk_label_new(a.name);
            gtk_widget_set_halign(name_lbl, GTK_ALIGN_START);
            {
                char buf[128];
                std::snprintf(buf, sizeof(buf),
                    "<span weight='bold'>%s</span>", a.name);
                gtk_label_set_markup(GTK_LABEL(name_lbl), buf);
            }
            gtk_box_append(GTK_BOX(info), name_lbl);

            GtkWidget* role_lbl = gtk_label_new(a.role);
            gtk_widget_set_halign(role_lbl, GTK_ALIGN_START);
            gtk_widget_set_opacity(role_lbl, 0.55);
            gtk_box_append(GTK_BOX(info), role_lbl);

            gtk_box_append(GTK_BOX(row), info);
            gtk_widget_set_hexpand(info, TRUE);

            /* link button */
            GtkWidget* link_btn = gtk_button_new_with_label(a.btn_label);
            gtk_widget_set_valign(link_btn, GTK_ALIGN_CENTER);
            gtk_box_append(GTK_BOX(row), link_btn);

            gtk_box_append(GTK_BOX(inner), row);
        }

        gtk_box_append(GTK_BOX(content), build_card("关于 PCL-CGRE", inner));
    }

    /* ── Card 2: 特别感谢 ──────────────────────────────────────────────── */
    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

        struct Thanks {
            const char* name;
            const char* role;
            const char* btn_label;
        };
        const Thanks thanks[] = {
            {"bangbang93", "BMCLAPI 维护者", "赞助"},
            {"MCMOD",       "MC 百科",        "打开"},
        };

        for (auto& t : thanks) {
            GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
            gtk_widget_set_margin_bottom(row, 8);

            GtkWidget* av = gtk_label_new("🖼");
            gtk_widget_add_css_class(av, "ack-avatar");
            gtk_widget_set_halign(av, GTK_ALIGN_CENTER);
            gtk_box_append(GTK_BOX(row), av);

            GtkWidget* info = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            GtkWidget* name_lbl = gtk_label_new(t.name);
            {
                char buf[128];
                std::snprintf(buf, sizeof(buf),
                    "<span weight='bold'>%s</span>", t.name);
                gtk_label_set_markup(GTK_LABEL(name_lbl), buf);
            }
            gtk_widget_set_halign(name_lbl, GTK_ALIGN_START);
            gtk_box_append(GTK_BOX(info), name_lbl);

            GtkWidget* role_lbl = gtk_label_new(t.role);
            gtk_widget_set_opacity(role_lbl, 0.55);
            gtk_widget_set_halign(role_lbl, GTK_ALIGN_START);
            gtk_box_append(GTK_BOX(info), role_lbl);

            gtk_box_append(GTK_BOX(row), info);
            gtk_widget_set_hexpand(info, TRUE);

            GtkWidget* btn = gtk_button_new_with_label(t.btn_label);
            gtk_widget_set_valign(btn, GTK_ALIGN_CENTER);
            gtk_box_append(GTK_BOX(row), btn);

            gtk_box_append(GTK_BOX(inner), row);
        }

        gtk_box_append(GTK_BOX(content), build_card("特别感谢", inner));
    }

    /* ── Card 3: 贡献者 ────────────────────────────────────────────────── */
    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

        /* 占位头像网格 */
        GtkWidget* grid = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_widget_set_margin_bottom(grid, 8);
        for (int i = 0; i < 8; i++) {
            GtkWidget* av = gtk_label_new("🖼");
            gtk_widget_add_css_class(av, "ack-avatar");
            gtk_widget_set_halign(av, GTK_ALIGN_CENTER);
            gtk_box_append(GTK_BOX(grid), av);
        }
        gtk_box_append(GTK_BOX(inner), grid);

        GtkWidget* more_btn = gtk_button_new_with_label("查看所有贡献者 →");
        gtk_widget_set_halign(more_btn, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(inner), more_btn);

        gtk_box_append(GTK_BOX(content), build_card("贡献者", inner));
    }

    /* ── Card 4: 法律信息 (折叠) ──────────────────────────────────────── */
    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        GtkWidget* privacy = gtk_label_new(
            "PCL-CGRE 为开源项目，不收集任何个人数据。\n"
            "设置文件仅存储在您的本地设备。");
        gtk_widget_set_halign(privacy, GTK_ALIGN_START);
        gtk_widget_set_margin_bottom(privacy, 8);
        gtk_box_append(GTK_BOX(inner), privacy);

        GtkWidget* copyright_lbl = gtk_label_new(
            "Copyright © 2024-2026 X-LeeHe & PCL-Community\n"
            "基于 PCL-CE (Community Edition) 构建");
        gtk_widget_set_halign(copyright_lbl, GTK_ALIGN_START);
        gtk_widget_set_opacity(copyright_lbl, 0.55);
        gtk_widget_set_margin_bottom(copyright_lbl, 8);
        gtk_box_append(GTK_BOX(inner), copyright_lbl);

        GtkWidget* src_btn = gtk_button_new_with_label("社区源代码");
        gtk_widget_set_halign(src_btn, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(inner), src_btn);

        GtkWidget* expander = gtk_expander_new("法律信息");
        gtk_expander_set_child(GTK_EXPANDER(expander), inner);
        gtk_box_append(GTK_BOX(content), build_card("法律信息", expander));
    }

    /* ── Card 5: 上游法律信息 (折叠) ──────────────────────────────────── */
    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        GtkWidget* pcl2_lbl = gtk_label_new(
            "PCL2 (Plain Craft Launcher 2) 由 LTCat 开发。\n"
            "PCL-CGRE 是基于 PCL-CE 的重写版本，使用 GTK4/libadwaita。");
        gtk_widget_set_halign(pcl2_lbl, GTK_ALIGN_START);
        gtk_widget_set_margin_bottom(pcl2_lbl, 8);
        gtk_box_append(GTK_BOX(inner), pcl2_lbl);

        GtkWidget* btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_box_append(GTK_BOX(btn_row), gtk_button_new_with_label("条款与免责"));
        gtk_box_append(GTK_BOX(btn_row), gtk_button_new_with_label("上游源码 (PCL2)"));
        gtk_box_append(GTK_BOX(btn_row), gtk_button_new_with_label("上游源码 (PCL-CE)"));
        gtk_box_append(GTK_BOX(inner), btn_row);

        GtkWidget* expander = gtk_expander_new("上游法律信息");
        gtk_expander_set_child(GTK_EXPANDER(expander), inner);
        gtk_box_append(GTK_BOX(content), build_card("上游法律信息", expander));
    }

    /* ── Card 6: 许可证列表 (折叠) ────────────────────────────────────── */
    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        struct License {
            const char* lib;
            const char* license;
            const char* url;
        };
        const License libs[] = {
            {"GTK 4",       "LGPL 2.1+", "https://www.gtk.org"},
            {"libadwaita",   "LGPL 2.1+", "https://gnome.pages.gitlab.gnome.org/libadwaita"},
            {"nlohmann/json","MIT",       "https://github.com/nlohmann/json"},
            {"Pango",        "LGPL 2+",   "https://pango.gnome.org"},
            {"Cairo",        "LGPL 2.1 / MPL 1.1", "https://www.cairographics.org"},
            {"OpenSSL",      "Apache 2.0", "https://www.openssl.org"},
        };

        for (auto& l : libs) {
            GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
            gtk_widget_set_margin_bottom(row, 4);

            GtkWidget* name_lbl = gtk_label_new(l.lib);
            gtk_widget_set_halign(name_lbl, GTK_ALIGN_START);
            gtk_widget_set_size_request(name_lbl, 120, -1);
            gtk_box_append(GTK_BOX(row), name_lbl);

            GtkWidget* lic_lbl = gtk_label_new(l.license);
            gtk_widget_set_halign(lic_lbl, GTK_ALIGN_START);
            gtk_widget_set_opacity(lic_lbl, 0.55);
            gtk_widget_set_size_request(lic_lbl, 110, -1);
            gtk_box_append(GTK_BOX(row), lic_lbl);

            GtkWidget* site_btn = gtk_button_new_with_label("网站");
            gtk_widget_set_valign(site_btn, GTK_ALIGN_CENTER);
            gtk_box_append(GTK_BOX(row), site_btn);

            GtkWidget* lic_btn = gtk_button_new_with_label("许可");
            gtk_widget_set_valign(lic_btn, GTK_ALIGN_CENTER);
            gtk_box_append(GTK_BOX(row), lic_btn);

            gtk_box_append(GTK_BOX(inner), row);
        }

        GtkWidget* expander = gtk_expander_new("第三方许可证");
        gtk_expander_set_child(GTK_EXPANDER(expander), inner);
        gtk_box_append(GTK_BOX(content), build_card("第三方许可证", expander));
    }

    return sw;
}


/* ============================================================================
 * P3 — build_page_update: 更新 (版本信息 + 检查)
 * ============================================================================ */

GtkWidget* build_page_update()
{
    auto sp = scrolled_content();
    GtkWidget* sw = sp.sw;
    GtkWidget* content = sp.content;

    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        GtkWidget* ver_lbl = gtk_label_new("当前版本: PCL-CGRE v0.1.0-dev (Linux)");
        gtk_widget_set_halign(ver_lbl, GTK_ALIGN_START);
        gtk_widget_set_margin_bottom(ver_lbl, 12);
        gtk_box_append(GTK_BOX(inner), ver_lbl);

        GtkWidget* status_lbl = gtk_label_new("更新检查尚未实现");
        gtk_widget_set_halign(status_lbl, GTK_ALIGN_START);
        gtk_widget_set_opacity(status_lbl, 0.55);
        gtk_widget_set_margin_bottom(status_lbl, 12);
        gtk_box_append(GTK_BOX(inner), status_lbl);

        GtkWidget* check_btn = gtk_button_new_with_label("检查更新");
        gtk_widget_add_css_class(check_btn, "suggested-action");
        gtk_widget_set_halign(check_btn, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(inner), check_btn);

        gtk_box_append(GTK_BOX(content), build_card("更新", inner));
    }

    return sw;
}


/* ============================================================================
 * P3 — build_page_feedback: 反馈 (GitHub Issues)
 * ============================================================================ */

GtkWidget* build_page_feedback()
{
    auto sp = scrolled_content();
    GtkWidget* sw = sp.sw;
    GtkWidget* content = sp.content;

    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        GtkWidget* desc = gtk_label_new(
            "遇到问题或有建议？欢迎提交 GitHub Issue。\n"
            "请尽量提供详细的复现步骤和日志信息。");
        gtk_widget_set_halign(desc, GTK_ALIGN_START);
        gtk_widget_set_margin_bottom(desc, 12);
        gtk_box_append(GTK_BOX(inner), desc);

        GtkWidget* btn = gtk_button_new_with_label("提交新反馈");
        gtk_widget_add_css_class(btn, "suggested-action");
        gtk_widget_set_halign(btn, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(inner), btn);

        gtk_box_append(GTK_BOX(content), build_card("提交反馈", inner));
    }

    return sw;
}


/* ============================================================================
 * P3 — build_page_log: 日志 (GtkTextView)
 * ============================================================================ */

GtkWidget* build_page_log()
{
    auto sp = scrolled_content();
    GtkWidget* sw = sp.sw;
    GtkWidget* content = sp.content;

    {
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

        /* 工具栏 */
        GtkWidget* toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_widget_set_margin_bottom(toolbar, 8);

        GtkWidget* clear_btn = gtk_button_new_with_label("清除");
        GtkWidget* copy_btn  = gtk_button_new_with_label("复制全部");
        GtkWidget* scroll_btn = gtk_check_button_new_with_label("自动滚动");
        gtk_check_button_set_active(GTK_CHECK_BUTTON(scroll_btn), TRUE);
        gtk_box_append(GTK_BOX(toolbar), clear_btn);
        gtk_box_append(GTK_BOX(toolbar), copy_btn);
        gtk_box_append(GTK_BOX(toolbar), scroll_btn);
        gtk_box_append(GTK_BOX(inner), toolbar);

        /* 文本视图 */
        GtkWidget* tv = gtk_text_view_new();
        gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
        gtk_text_view_set_monospace(GTK_TEXT_VIEW(tv), TRUE);
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tv), GTK_WRAP_WORD_CHAR);
        gtk_widget_set_vexpand(tv, TRUE);
        gtk_widget_set_hexpand(tv, TRUE);
        gtk_widget_add_css_class(tv, "card");

        GtkWidget* tv_scroll = gtk_scrolled_window_new();
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(tv_scroll), tv);
        gtk_widget_set_vexpand(tv_scroll, TRUE);
        gtk_widget_set_size_request(tv_scroll, -1, 400);
        gtk_box_append(GTK_BOX(inner), tv_scroll);

        /* 初始日志文本 */
        GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
        gtk_text_buffer_set_text(buf, "PCL-CGRE 日志输出...\n", -1);

        /* 清除按钮 */
        {
            auto cb = +[](GtkButton*, gpointer d) {
                GtkTextBuffer* b = GTK_TEXT_BUFFER(d);
                gtk_text_buffer_set_text(b, "", -1);
            };
            g_signal_connect_data(clear_btn, "clicked", (GCallback)cb, buf, nullptr, G_CONNECT_DEFAULT);
        }

        /* 复制按钮 */
        g_object_set_data(G_OBJECT(copy_btn), "log-buf", buf);
        {
            auto cb = +[](GtkButton* btn, gpointer) {
                GtkTextBuffer* b = GTK_TEXT_BUFFER(
                    g_object_get_data(G_OBJECT(btn), "log-buf"));
                GtkTextIter start, end;
                gtk_text_buffer_get_bounds(b, &start, &end);
                char* text = gtk_text_buffer_get_text(b, &start, &end, FALSE);
                GdkClipboard* clip = gdk_display_get_clipboard(
                    gdk_display_get_default());
                gdk_clipboard_set_text(clip, text);
                g_free(text);
            };
            g_signal_connect_data(copy_btn, "clicked", (GCallback)cb, nullptr, nullptr, G_CONNECT_DEFAULT);
        }

        /* 自动滚动 — 滚动到底部 */
        g_object_set_data(G_OBJECT(scroll_btn), "log-scroll", tv_scroll);
        g_object_set_data(G_OBJECT(scroll_btn), "log-tv", tv);

        gtk_box_append(GTK_BOX(content), build_card("运行日志", inner));
    }

    return sw;
}

}  // namespace pcl
