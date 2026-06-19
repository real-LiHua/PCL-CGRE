#include "core/Styles.hpp"
#include "core/Colors.hpp"

#include <string>
#include <gtk/gtk.h>
#include <adwaita.h>

namespace pcl {

// ============================================================================
// 全局 CSS Provider — 支持运行时替换以实现主题实时切换
// ============================================================================

static GtkCssProvider* s_active_provider = nullptr;

/** 移除旧 provider 并加载新 provider */
static void apply_css(const std::string& css) {
    auto* display = gdk_display_get_default();
    if (!display) return;

    // 移除旧 provider
    if (s_active_provider) {
        gtk_style_context_remove_provider_for_display(
            display, GTK_STYLE_PROVIDER(s_active_provider));
        g_object_unref(s_active_provider);
        s_active_provider = nullptr;
    }

    // 加载新 provider
    auto* provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider, css.c_str());

    gtk_style_context_add_provider_for_display(
        display, GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);

    s_active_provider = provider; // 保持引用
}

// ============================================================================
// CSS 构建 — 根据主题亮度 + 强调色动态生成
// ============================================================================

/**
 * 构建完整 CSS 字符串。
 *
 * @param is_dark  true=深色主题, false=浅色主题
 * @param accent   用户选择的强调色 (#RRGGBB, 必须为蓝色系)
 */
static std::string build_css_impl(bool is_dark, const std::string& accent) {
    using namespace pcl::colors;
    using std::string;

    // 颜色选择 lambda: light or dark
    auto c = [is_dark](const char* light, const char* darkk) -> const char* {
        return is_dark ? darkk : light;
    };

    auto s = [](const char* str) { return string(str); };

    // HEX → RGBA 半透明: "#d5e6fd" → "rgba(213,230,253,0.80)"
    auto hex_to_rgba = [](const char* hex, double alpha) -> string {
        if (!hex || hex[0] != '#') return string(hex);
        unsigned int r=0, g=0, b=0;
        std::sscanf(hex+1, "%2x%2x%2x", &r, &g, &b);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "rgba(%u,%u,%u,%.2f)", r, g, b, alpha);
        return string(buf);
    };

    const string ACCENT = accent.empty()
        ? (is_dark ? string(dark::BLUE_3) : string(BLUE_3))
        : accent;

    return
        s("/* PCL-CGRE Stylesheet — Theme: ") + (is_dark ? "Dark" : "Light") +
        " | Accent: " + ACCENT + " */\n"
        "\n"
        /* ── 全局 (仅作用于 .pcl-app 窗口, 系统对话框不受影响) ── */
        ".pcl-app * { font-family: 'HarmonyOS Sans SC', 'HarmonyOS Sans', system-ui, sans-serif; }\n"
        ".pcl-app image { color: " + s(is_dark ? "white" : BLUE_1) + "; }\n"
        "\n"
        /* ── Header bar ── */
        "headerbar, headerbar:backdrop { background-color: " + ACCENT + "; min-height: 48px; }\n"
        "headerbar label, headerbar label:backdrop, headerbar image,\n"
        "headerbar button:not(.suggested-action):not(.destructive-action),\n"
        "headerbar button:backdrop:not(.suggested-action):not(.destructive-action) { color: white; }\n"
        "headerbar button:not(.suggested-action):not(.destructive-action):hover {\n"
        "  background-color: " + s(c(HEADER_BTN_HOVER, dark::HEADER_BTN_HOVER)) + "; }\n"
        "\n"
        /* 窗口控件 */
        "headerbar button.window-ctrl-btn {\n"
        "  all: unset; min-width: 30px; min-height: 30px; padding: 4px; border-radius: 50%;\n"
        "  color: white; background-color: transparent; background-image: none;\n"
        "  border: none; outline: none; box-shadow: none; -gtk-icon-source: none;\n"
        "  transition: background-color 120ms ease-out; }\n"
        "headerbar button.window-ctrl-btn:hover {\n"
        "  color: white; background-color: " + s(c(HEADER_BTN_HOVER, dark::HEADER_BTN_HOVER)) +
        "; background-image: none; }\n"
        "headerbar button.window-ctrl-btn image { color: white; }\n"
        "headerbar button.window-ctrl-close:hover { color: white; background-color: #c42b1c; background-image: none; }\n"
        "headerbar button.window-ctrl-close:hover image { color: white; }\n"
        "\n"
        "headerbar .app-title { font-size: 12pt; font-weight: 500; color: white; margin-left: 8px; }\n"
        "headerbar:backdrop .app-title { color: white; }\n"
        "\n"
        /* ── 标题栏页签 ── */
        "headerbar button.header-tab {\n"
        "  all: unset; color: white; background-color: transparent; background-image: none;\n"
        "  border-radius: 9999px; min-height: 27px; padding: 2px 12px; font-size: 10.5pt;\n"
        "  border: none; box-shadow: none; outline: none; -gtk-icon-source: none;\n"
        "  transition: background-color 150ms ease-out; }\n"
        "headerbar button.header-tab:hover {\n"
        "  color: white; background-color: " + s(c(TAB_HOVER, dark::TAB_HOVER)) +
        "; transition: background-color 90ms ease-out; }\n"
        "headerbar button.header-tab-active {\n"
        "  color: " + ACCENT + "; background-color: white; background-image: none;\n"
        "  font-weight: 600; border: none; box-shadow: none; }\n"
        "headerbar button.header-tab-active:hover { color: " + ACCENT + "; background-color: white; background-image: none; }\n"
        "headerbar button.header-tab image, headerbar button.header-tab label { color: white; }\n"
        "headerbar button.header-tab-active image, headerbar button.header-tab-active label { color: " + ACCENT + "; }\n"
        "\n"
        /* ── 普通按钮 (仅作用于 .pcl-app 窗口) ── */
        ".pcl-app button:not(.suggested-action):not(.destructive-action):not(.toggle):not(.flat):not(.fab):not(.extra-button) {\n"
        "  border: 1px solid " + s(c(BLUE_1, dark::BLUE_1)) + "; border-radius: 3px;\n"
        "  background-color: " + s(c(BTN_BG_NORMAL, dark::BTN_BG_NORMAL)) + ";\n"
        "  color: " + s(c(BLUE_1, dark::BLUE_1)) + "; font-size: 10pt; padding: 6px 16px; transition: all 200ms ease-out; }\n"
        "button:not(.suggested-action):not(.destructive-action):not(.toggle):not(.flat):not(.fab):not(.extra-button):hover {\n"
        "  border-color: " + ACCENT + "; background-color: " + s(c(BLUE_7, dark::BLUE_7)) +
        "; color: " + ACCENT + "; transition: all 100ms ease-out; }\n"
        "button:not(.suggested-action):not(.destructive-action):not(.toggle):not(.flat):not(.fab):not(.extra-button):active {\n"
        "  background-color: " + s(c(BLUE_6, dark::BLUE_6)) + "; }\n"
        "\n"
        /* ── suggested-action (仅作用于 .pcl-app 窗口) ── */
        ".pcl-app button.suggested-action {\n"
        "  border: 1px solid " + ACCENT + "; border-radius: 3px; background-color: " + ACCENT +
        "; color: white; font-weight: 600; transition: all 200ms ease-out; }\n"
        ".pcl-app button.suggested-action:hover {\n"
        "  border-color: " + ACCENT + "; background-color: " + ACCENT + "; transition: all 100ms ease-out; }\n"
        "\n"
        /* ── destructive (仅作用于 .pcl-app 窗口) ── */
        ".pcl-app button.destructive-action {\n"
        "  border: 1px solid " + s(c(RED_DARK, dark::RED_DARK)) + "; border-radius: 3px;\n"
        "  background-color: " + s(c(RED_DARK, dark::RED_DARK)) + "; color: white; font-weight: 600;\n"
        "  transition: all 200ms ease-out; }\n"
        ".pcl-app button.destructive-action:hover {\n"
        "  border-color: " + s(c(RED_LIGHT, dark::RED_LIGHT)) + ";\n"
        "  background-color: " + s(c(RED_LIGHT, dark::RED_LIGHT)) + "; transition: all 100ms ease-out; }\n"
        "\n"
        /* ── 切换按钮 (仅作用于 .pcl-app 窗口) ── */
        ".pcl-app button.toggle {\n"
        "  border: 1px solid " + s(c(BLUE_5, dark::BLUE_5)) + "; border-radius: 3px;\n"
        "  background-color: " + s(c(BTN_BG_NORMAL, dark::BTN_BG_NORMAL)) + ";\n"
        "  color: " + s(c(BLUE_1, dark::BLUE_1)) + "; font-size: 10pt; padding: 6px 16px;\n"
        "  transition: all 200ms ease-out; }\n"
        ".pcl-app button.toggle:hover {\n"
        "  border-color: " + ACCENT + "; background-color: " + s(c(BLUE_7, dark::BLUE_7)) +
        "; color: " + ACCENT + "; transition: all 100ms ease-out; }\n"
        ".pcl-app button.toggle:checked, .pcl-app button.toggle:active {\n"
        "  border-color: " + ACCENT + "; background-color: " + ACCENT + "; color: white;\n"
        "  font-weight: 600; }\n"
        ".pcl-app button.toggle:checked:hover {\n"
        "  border-color: " + ACCENT + "; background-color: " + ACCENT + ";\n"
        "  opacity: 0.88; }\n"
        "\n"
        /* ── linked 容器内切换按钮处理 ── */
        ".pcl-app .linked button.toggle { border-radius: 0; margin: 0; }\n"
        ".pcl-app .linked button.toggle:first-child { border-radius: 3px 0 0 3px; }\n"
        ".pcl-app .linked button.toggle:last-child { border-radius: 0 3px 3px 0; }\n"
        ".pcl-app .linked button.toggle + button.toggle { border-left: none; }\n"
        "\n"
        /* ── 进度条自定义 ── */
        ".pcl-app progressbar trough {\n"
        "  border-radius: 2px; min-height: 4px;\n"
        "  background-color: " + s(c(BLUE_6, dark::BLUE_6)) + "; }\n"
        ".pcl-app progressbar progress {\n"
        "  border-radius: 2px;\n"
        "  background: linear-gradient(to right, " + s(c(BLUE_4, dark::BLUE_4)) + ", " + ACCENT + "); }\n"
        "\n"
        /* ── Scale/Slider 自定义 ── */
        ".pcl-app scale trough {\n"
        "  border: 1px solid " + s(c(BLUE_5, dark::BLUE_5)) + "; border-radius: 3px;\n"
        "  min-height: 6px; background-color: " + s(c(BTN_BG_NORMAL, dark::BTN_BG_NORMAL)) + "; }\n"
        ".pcl-app scale trough highlight {\n"
        "  border-radius: 2px; background-color: " + ACCENT + "; }\n"
        ".pcl-app scale trough slider {\n"
        "  border: 1px solid " + ACCENT + "; border-radius: 9999px;\n"
        "  min-width: 14px; min-height: 14px;\n"
        "  background-color: white; box-shadow: 0 1px 3px rgba(0,0,0,0.18); }\n"
        "\n"
        /* ── 胶囊按钮 ── */
        ".pill-button { border-radius: 9999px; padding: 12px 28px; font-weight: 600; }\n"
        ".pill-button.suggested-action { border-radius: 3px; }\n"
        ".launch-main { padding-bottom: 18px; }\n"
        ".launch-version { font-size: 8pt; color: rgba(255, 255, 255, 0.55); }\n"
        "\n"
        /* ── fab ── */
        ".fab { border-radius: 9999px; min-width: 48px; min-height: 48px; padding: 0; }\n"
        ".fab-blue, .fab-blue:backdrop {\n"
        "  background-color: " + ACCENT + "; color: white; box-shadow: 0 4px 14px rgba(19, 112, 243, 0.45); }\n"
        ".fab-blue:hover { background-color: shade(" + ACCENT + ", 1.15); box-shadow: 0 6px 20px rgba(19, 112, 243, 0.55); }\n"
        ".fab-blue image { color: white; }\n"
        "\n"
        /* ── extra-button ── */
        ".extra-button { border-radius: 9999px; min-width: 40px; min-height: 40px; padding: 8px;\n"
        "  background-color: " + ACCENT + "; color: white; }\n"
        ".extra-button image { color: white; }\n"
        ".extra-button:hover { background-color: shade(" + ACCENT + ", 1.15); }\n"
        "\n"
        ".launch-nav { background-color: transparent; }\n"
        ".launch-nav row { background-color: transparent; min-height: 0px; padding: 0; }\n"
        "\n"
        /* ── nav-item ── */
        ".nav-item { border-radius: 6px; color: " + s(c(BLUE_1, dark::BLUE_1)) + "; font-size: 10pt; }\n"
        ".nav-item:hover { background-color: " + s(c(BLUE_6, dark::BLUE_6)) + "; }\n"
        ".nav-item-active { background-color: " + s(c(BLUE_6, dark::BLUE_6)) + "; font-weight: 600; }\n"
        ".nav-sidebar { background-color: " + hex_to_rgba(c(SIDEBAR_BG, dark::SIDEBAR_BG), is_dark?0.85:0.80) + "; min-width: 160px; padding: 8px 8px; }\n"
        ".settings-mid { background-color: " + hex_to_rgba(c(BLUE_7, dark::BLUE_7), is_dark?0.85:0.80) + "; padding: 8px 0; }\n"
        ".settings-right { background-color: " + hex_to_rgba(c(NEWS_PANEL_BG, dark::NEWS_PANEL_BG), is_dark?0.85:0.80) + "; padding: 16px; }\n"
        ".nav-item-wrapper { border-radius: 6px; }\n"
        ".nav-item-wrapper:hover { background-color: " + s(c(BLUE_6, dark::BLUE_6)) + "; }\n"
        ".nav-item-wrapper .nav-item:hover { background-color: transparent; }\n"
        ".nav-item-wrapper-active { background-color: " + s(c(BLUE_6, dark::BLUE_6)) + "; }\n"
        ".nav-item-wrapper-active .nav-item { font-weight: 600; }\n"
        ".nav-item-wrapper-active .nav-item image { opacity: 1; }\n"
        ".nav-refresh-btn { all: unset; margin-right: 6px; opacity: 0; border-radius: 4px; padding: 4px;\n"
        "  color: " + s(c(BLUE_1, dark::BLUE_1)) + "; transition: opacity 120ms ease-out; }\n"
        ".nav-item-wrapper:hover .nav-refresh-btn { opacity: 0.55; }\n"
        ".nav-item:hover .nav-refresh-btn { opacity: 0.55; }\n"
        ".nav-refresh-btn:hover { opacity: 1 !important; background-color: " + s(c(BLUE_5, dark::BLUE_5)) + "; }\n"
        "\n"
        /* ── sidebar ── */
        ".sidebar, .sidebar:backdrop { background-color: " + hex_to_rgba(c(SIDEBAR_BG, dark::SIDEBAR_BG), is_dark?0.85:0.80) +
        "; min-width: 300px; padding: 24px 24px; }\n"
        ".sidebar .avatar-box { margin-bottom: 8px; }\n"
        ".sidebar .username { font-size: 14pt; font-weight: 600; color: " + s(c(BLUE_1, dark::BLUE_1)) + "; }\n"
        "\n"
        /* ── mode-bar ── */
        ".mode-bar { margin-bottom: 24px; }\n"
        ".mode-btn, button.flat.mode-btn {\n"
        "  border-radius: 9999px; padding: 3px 14px; font-size: 10pt; min-height: 24px;\n"
        "  color: " + s(c(GRAY_4, dark::GRAY_4)) + "; background-color: transparent; background-image: none;\n"
        "  border: none; outline: none; box-shadow: none; -gtk-icon-source: none;\n"
        "  transition: background-color 150ms ease-out, color 150ms ease-out; }\n"
        ".mode-btn:hover, button.flat.mode-btn:hover {\n"
        "  color: " + ACCENT + "; background-color: " + s(c(BLUE_6, dark::BLUE_6)) + "; background-image: none; }\n"
        ".mode-btn label, button.flat.mode-btn label { color: " + s(c(GRAY_4, dark::GRAY_4)) + "; }\n"
        ".mode-btn:hover label, button.flat.mode-btn:hover label { color: " + ACCENT + "; }\n"
        ".mode-btn-active, .mode-btn-active:hover, button.flat.mode-btn-active, button.flat.mode-btn-active:hover {\n"
        "  color: white; font-weight: 600; background-color: " + ACCENT + "; background-image: none; }\n"
        ".mode-btn-active label, .mode-btn-active:hover label,\n"
        "button.flat.mode-btn-active label, button.flat.mode-btn-active:hover label { color: white; }\n"
        "\n"
        /* ── version-row ── */
        ".version-row { margin-top: 0px; }\n"
        ".version-row button { border-radius: 3px; padding: 8px 0; font-size: 10pt; font-weight: normal; }\n"
        ".loader-ver-row { border-radius: 5px; transition: background-color 120ms ease-out; }\n"
        ".loader-ver-row:hover { background-color: " + s(c(BLUE_6, dark::BLUE_6)) + "; }\n"
        ".loader-ver-row-selected { background-color: " + s(c(BLUE_6, dark::BLUE_6)) +
        "; box-shadow: inset 0 0 0 1px " + s(c(BLUE_5, dark::BLUE_5)) + "; }\n"
        "\n"
        /* ── content-area (半透明, 让背景图透过) ── */
        ".content-area, .content-area:backdrop { background-color: " + hex_to_rgba(c(NEWS_PANEL_BG, dark::NEWS_PANEL_BG), is_dark?0.85:0.80) + "; }\n"
        ".content-area viewport, .content-area viewport:backdrop { background-color: " +
        hex_to_rgba(c(NEWS_PANEL_BG, dark::NEWS_PANEL_BG), is_dark?0.85:0.80) + "; }\n"
        "\n"
        /* ── 背景穿透: ViewStack / ScrolledWindow 透明, 让背景图可见 ── */
        /* GTK4 CSS node 名称 (经验证):
           AdwViewStack → stack, AdwViewStackPage → stack, AdwToolbarView → toolbarview */
        "stack { background-color: transparent; }\n"
        "stack viewport { background-color: transparent; }\n"
        "toolbarview { background-color: transparent; }\n"
        "\n"
        /* dm */
        ".dm-scroll viewport, .dm-scroll viewport:backdrop { background-color: " +
        hex_to_rgba(c(NEWS_PANEL_BG, dark::NEWS_PANEL_BG), is_dark?0.85:0.80) + "; }\n"
        ".dm-scroll list, .dm-scroll list:backdrop { background-color: transparent; }\n"
        ".dm-scroll list row, .dm-scroll list row:backdrop { background-color: transparent; }\n"
        ".dm-scroll .card { overflow: hidden; }\n"
        "\n"
        ".news-scroll, .news-scroll:backdrop { background-color: " + hex_to_rgba(c(NEWS_PANEL_BG, dark::NEWS_PANEL_BG), is_dark?0.85:0.80) + "; }\n"
        ".news-panel, .news-panel:backdrop { background-color: " + hex_to_rgba(c(NEWS_PANEL_BG, dark::NEWS_PANEL_BG), is_dark?0.85:0.80) +
        "; padding: 24px; }\n"
        ".news-panel .section-title { font-size: 14pt; font-weight: 600; margin-bottom: 16px;\n"
        "  color: " + s(c(BLUE_1, dark::BLUE_1)) + "; }\n"
        "\n"
        /* ── boxed-list / card ── */
        ".news-panel .boxed-list { background-color: " + s(c(CARD_BG, dark::CARD_BG)) + "; border-radius: 3px; }\n"
        ".news-panel row { background-color: transparent; }\n"
        ".card { background-color: " + s(c(CARD_BG, dark::CARD_BG)) + "; border-radius: 5px;\n"
        "  border: 1px solid " + s(c(BLUE_6, dark::BLUE_6)) + "; padding: 10px;\n"
        "  box-shadow: 0 2px 5px 0 rgba(0,0,0,0.06); }\n"
        ".card .card-title { font-size: 11pt; font-weight: 600; color: " + s(c(BLUE_1, dark::BLUE_1)) +
        "; margin-bottom: 8px; }\n"
        ".card-header-btn { padding: 0; margin: -4px 0; }\n"
        "\n"
        /* ── hint-bar ── */
        ".hint-bar { border-left: 3px solid " + s(c(HINT_BORDER, dark::HINT_BORDER)) + "; border-radius: 2px;\n"
        "  background-color: " + s(c(CARD_BG, dark::CARD_BG)) + "; padding: 9px 12px; font-size: 10pt;\n"
        "  color: " + s(c(BLUE_1, dark::BLUE_1)) + "; }\n"
        "\n"
        /* ── search-box ── */
        ".search-box { background-color: " + s(c(BTN_BG_NORMAL, dark::BTN_BG_NORMAL)) +
        "; border: 1px solid " + s(c(BLUE_5, dark::BLUE_5)) +
        "; border-radius: 3px; min-height: 24px; padding: 2px 6px; }\n"
        ".search-box entry { background-color: transparent; border: none; font-size: 10pt;\n"
        "  color: " + s(c(BLUE_1, dark::BLUE_1)) + "; padding: 2px 6px; }\n"
        ".search-box entry:focus { box-shadow: none; }\n"
        "\n"
        ".search-card dropdown, .search-card combobox { font-size: 8.5pt; min-height: 24px;\n"
        "  border: 1px solid " + s(c(BLUE_5, dark::BLUE_5)) + "; border-radius: 3px;\n"
        "  background-color: " + s(c(BTN_BG_NORMAL, dark::BTN_BG_NORMAL)) + "; }\n"
        ".search-card dropdown button { padding: 1px 5px; min-height: 22px; background-color: transparent;\n"
        "  border: none; transition: background-color 100ms ease-out; }\n"
        ".search-card dropdown button:active, .search-card dropdown button:checked {\n"
        "  background-color: " + s(c(BLUE_6, dark::BLUE_6)) + "; }\n"
        ".search-card dropdown popover row { min-height: 24px; padding: 2px 8px; font-size: 8.5pt; }\n"
        "\n"
        /* ── version popover ── */
        ".version-popover { padding: 0; }\n"
        ".version-popover .popover-search { border: 1px solid " + s(c(BLUE_5, dark::BLUE_5)) +
        "; border-radius: 6px; background-color: " + s(c(BTN_BG_NORMAL, dark::BTN_BG_NORMAL)) +
        "; font-size: 9.5pt; padding: 4px 8px; min-height: 0; }\n"
        ".version-popover .popover-search:focus { border-color: " + ACCENT + "; box-shadow: none; }\n"
        ".version-list { background-color: transparent; }\n"
        ".version-list row { padding: 0; margin: 0; }\n"
        ".version-list row:hover { background-color: " + s(c(BLUE_6, dark::BLUE_6)) + "; }\n"
        ".version-item-name { font-size: 10pt; font-weight: 600; color: " + s(c(BLUE_1, dark::BLUE_1)) + "; }\n"
        ".version-item-path { font-size: 8pt; color: " + s(c(BLUE_1, dark::BLUE_1)) + "; }\n"
        "\n"
        /* ── entry (仅作用于 .pcl-app 窗口) ── */
        ".pcl-app entry { border: 1px solid " + s(c(BLUE_5, dark::BLUE_5)) + "; border-radius: 3px;\n"
        "  background-color: " + s(c(BTN_BG_NORMAL, dark::BTN_BG_NORMAL)) + ";\n"
        "  color: " + s(c(BLUE_1, dark::BLUE_1)) + "; font-size: 10pt; padding: 6px 10px;\n"
        "  transition: all 150ms ease-out; box-shadow: none; }\n"
        ".pcl-app entry:focus { border-color: " + ACCENT + "; background-color: " + s(c(BLUE_7, dark::BLUE_7)) +
        "; box-shadow: none; }\n"
        ".pcl-app entry:hover { border-color: " + s(c(BLUE_4, dark::BLUE_4)) + "; background-color: " +
        s(c(BLUE_7, dark::BLUE_7)) + "; }\n"
        "\n"
        /* ── checkbox (仅作用于 .pcl-app 窗口) ── */
        ".pcl-app checkbutton check { min-width: 18px; min-height: 18px; border: 1px solid " +
        s(c(BLUE_1, dark::BLUE_1)) + "; border-radius: 3px; background-color: " +
        s(c(CHECKBOX_BG, dark::CHECKBOX_BG)) + "; }\n"
        ".pcl-app checkbutton check:checked { background-color: " + ACCENT + "; border-color: " + ACCENT +
        "; color: white; }\n"
        "\n"
        /* ── scrollbar (仅作用于 .pcl-app 窗口) ── */
        ".pcl-app scrolledwindow undershoot, .pcl-app scrolledwindow overshoot { background: none; }\n"
        ".pcl-app scrollbar slider { min-width: 6px; min-height: 30px; margin: 3px; border-radius: 9999px;\n"
        "  background-color: " + s(c(BLUE_4, dark::BLUE_4)) + "; }\n"
        "\n"
        /* ── tooltip ── */
        ".pcl-app tooltip { border: 1px solid " + s(c(BLUE_1, dark::BLUE_1)) + "; border-radius: 4px;\n"
        "  background-color: " + s(c(TOOLTIP_BG, dark::TOOLTIP_BG)) + "; color: " +
        s(c(BLUE_1, dark::BLUE_1)) + "; font-size: 9pt; padding: 5px 7px;\n"
        "  box-shadow: 0 2px 4px rgba(0,0,0,0.3); }\n"
        "\n"
        /* ── row (仅作用于 .pcl-app 窗口) ── */
        ".pcl-app row { min-height: 36px; font-size: 10pt; color: " + s(c(BLUE_1, dark::BLUE_1)) + "; }\n"
        "\n"
        /* ── placeholder ── */
        ".placeholder { padding: 48px; }\n"
        ".placeholder .ph-title { font-size: 24pt; font-weight: 300; opacity: 0.55;\n"
        "  color: " + s(c(BLUE_1, dark::BLUE_1)) + "; }\n"
        "\n"
        /* ── version-actions ── */
        ".version-actions { opacity: 0; transition: opacity 120ms ease-out; }\n"
        ".version-actions.visible { opacity: 1; }\n"
        ".version-action-btn { all: unset; padding: 4px; border-radius: 4px;\n"
        "  color: " + s(c(BLUE_1, dark::BLUE_1)) + "; transition: background-color 100ms ease-out; }\n"
        ".version-action-btn:hover { background-color: " + s(c(BLUE_6, dark::BLUE_6)) + "; }\n"
        ".version-action-btn image { color: " + s(c(BLUE_1, dark::BLUE_1)) + "; }\n"
        "\n"
        /* ── resource ── */
        ".resource-card row { background: none; background-color: transparent; }\n"
        ".resource-item { min-height: 64px; border: 1px solid transparent; border-radius: 3px;\n"
        "  transition: border-color 100ms ease-out; }\n"
        ".resource-item:hover { border-color: " + s(c(BLUE_5, dark::BLUE_5)) + "; }\n"
        ".resource-logo { border-radius: 6px; background-color: " + s(c(BLUE_6, dark::BLUE_6)) + "; }\n"
        ".resource-logo-detail { border-radius: 6px; background-color: " + s(c(BLUE_6, dark::BLUE_6)) +
        "; min-width: 64px; min-height: 64px; }\n"
        ".resource-tag { background-color: " + s(c(BLUE_6, dark::BLUE_6)) + "; color: " +
        s(c(BLUE_2, dark::BLUE_2)) + "; border-radius: 3px; padding: 0px 4px; font-size: 8pt; }\n"
        "\n"
        /* ── resource detail page ── */
        ".detail-left { background-color: " + hex_to_rgba(c(NEWS_PANEL_BG, dark::NEWS_PANEL_BG), is_dark?0.85:0.80) +
        "; border-radius: 8px; padding: 20px 16px; }\n"
        ".detail-right { background-color: " + hex_to_rgba(c(NEWS_PANEL_BG, dark::NEWS_PANEL_BG), is_dark?0.70:0.55) +
        "; border-radius: 8px; padding: 16px 20px; }\n"
        ".detail-icon { border-radius: 12px; min-width: 96px; min-height: 96px; }\n"
        ".detail-loading { padding: 48px 24px; }\n"
        "\n"
        /* ── version-tab ── */
        ".version-tab-bar { margin-top: 8px; min-height: 28px; padding-bottom: 0; }\n"
        ".version-tab { background: transparent; border: 1px solid " + s(c(BLUE_5, dark::BLUE_5)) +
        "; border-radius: 9999px; padding: 2px 10px; font-size: 9pt; color: " + s(c(BLUE_2, dark::BLUE_2)) +
        "; transition: all 150ms ease-out; }\n"
        ".version-tab:hover { border-color: " + ACCENT + "; color: " + ACCENT + "; }\n"
        ".version-tab-active { color: white; background-color: " + ACCENT + "; border-color: " + ACCENT +
        "; font-weight: 600; }\n"
        "\n"
        ".ver-tab-scroll > undershoot, .ver-tab-scroll > overshoot { background: none; }\n"
        ".ver-tab-scroll scrollbar.horizontal { min-height: 4px; margin: 0; padding: 0; }\n"
        ".ver-tab-scroll scrollbar.horizontal trough { min-height: 4px; border: none; background-color: " +
        s(c(BLUE_6, dark::BLUE_6)) + "; }\n"
        ".ver-tab-scroll scrollbar.horizontal slider { min-height: 4px; min-width: 20px; border-radius: 2px;\n"
        "  background-color: " + ACCENT + "; border: none; }\n"
        "\n"
        /* ── settings-category ── */
        ".settings-category-header { font-size: 12px; opacity: 0.6; color: " +
        s(c(BLUE_1, dark::BLUE_1)) + "; margin: 13px 5px 5px 3px; }\n"
        "\n"
        /* ── settings expander ── */
        ".settings-expander { margin-top: 4px; }\n"
        ".settings-expander > expander > box { background-color: transparent; }\n"
        "\n"
        /* ── memory bar ── */
        ".settings-memory-bar { min-height: 22px; border-radius: 4px; overflow: hidden; }\n"
        ".mem-used { background-color: " + ACCENT + "; border-radius: 4px 0 0 4px; }\n"
        ".mem-free { background-color: " + hex_to_rgba(c(BLUE_5, dark::BLUE_5), 0.30) +
        "; border-radius: 0 4px 4px 0; }\n"
        "\n"
        /* ── ack-avatar ── */
        ".ack-avatar { border-radius: 9999px; background-color: " + s(c(BLUE_6, dark::BLUE_6)) +
        "; min-width: 34px; min-height: 34px; }\n"
        "\n"
        /* ── notif ── */
        ".notif-backdrop { background-color: rgba(0,0,0,0.25); }\n"
        ".notif-panel { background-color: " + s(c(BG_MAIN, dark::BG_MAIN)) + "; border-right: 1px solid " +
        s(c(BLUE_5, dark::BLUE_5)) + "; box-shadow: 4px 0 20px rgba(0,0,0,0.12); }\n"
        ".notif-header { padding: 14px 16px; min-height: 48px; }\n"
        ".notif-title { font-size: 13pt; font-weight: 600; color: " + s(c(BLUE_1, dark::BLUE_1)) + "; }\n"
        ".notif-list { background-color: transparent; }\n"
        "\n"
        ".notif-scroll scrollbar { background-color: transparent; opacity: 0; transition: opacity 300ms ease-out; }\n"
        ".notif-scroll scrollbar slider { min-width: 5px; margin: 2px; background-color: rgba(128,128,128,0.25);\n"
        "  border-radius: 3px; transition: background-color 200ms ease-out; }\n"
        ".notif-scroll scrollbar slider:hover { background-color: rgba(128,128,128,0.45); }\n"
        ".notif-scroll:hover scrollbar { opacity: 1; }\n"
        "\n"
        ".dm-check check { min-width: 14px; min-height: 14px; }\n"
        ".dm-danger-btn { all: unset; padding: 4px; border-radius: 4px; color: " +
        s(c(BLUE_1, dark::BLUE_1)) + "; transition: background-color 120ms ease-out; }\n"
        ".dm-danger-btn:hover { background-color: rgba(244,67,54,0.12); color: " +
        s(c(STATUS_ERROR, dark::STATUS_ERROR)) + "; }\n"
        "\n"
        ".dm-progress { margin: 0; padding: 0; }\n"
        ".dm-progress trough { min-height: 2px; border-radius: 0; border: none; background-color: #e8e8e8; padding: 0; }\n"
        ".dm-progress progress { min-height: 2px; border-radius: 0; background-color: " + ACCENT +
        "; border: none; }\n"
        "\n"
        ".notif-list row { background-color: transparent; min-height: 0px; padding: 0; }\n"
        ".notif-item { padding: 10px 12px; }\n"
        ".notif-item:hover { background-image: linear-gradient(rgba(0,0,0,0.04),rgba(0,0,0,0.04)); }\n"
        ".notif-item:active { background-image: linear-gradient(rgba(0,0,0,0.09),rgba(0,0,0,0.09)); }\n"
        "\n"
        ".notif-del-btn { opacity: 0; transition: opacity 120ms ease-out; }\n"
        ".notif-item:hover .notif-del-btn { opacity: 0.45; }\n"
        ".notif-del-btn:hover { opacity: 1 !important; }\n"
        "\n"
        ".notif-info { color: " + ACCENT + "; }\n"
        ".notif-warn { color: " + s(c(STATUS_WARN, dark::STATUS_WARN)) + "; }\n"
        ".notif-error { color: " + s(c(STATUS_ERROR, dark::STATUS_ERROR)) + "; }\n"
        ".notif-fatal { color: " + s(c(STATUS_FATAL, dark::STATUS_FATAL)) + "; }\n"
        "\n"
        ".notif-bg-info  { background-color: rgba(76,175,80,0.07); }\n"
        ".notif-bg-warn  { background-color: rgba(33,150,243,0.07); }\n"
        ".notif-bg-error { background-color: rgba(244,67,54,0.07); }\n"
        ".notif-bg-fatal { background-color: rgba(244,67,54,0.11); }\n"
        "\n"
        "button.app-title-btn { all: unset; font-size: 12pt; font-weight: 500; color: white;\n"
        "  margin-left: 8px; padding: 2px 8px; border-radius: 6px; cursor: pointer; }\n"
        "button.app-title-btn:hover { background-color: " + s(c(HEADER_BTN_HOVER, dark::HEADER_BTN_HOVER)) + "; }\n"
        "button.app-title-btn:backdrop { color: white; }\n"
        "\n"
        /* ── toast ── */
        ".toast-box { background-color: " + s(c(BG_MAIN, dark::BG_MAIN)) +
        s("; border-radius: 8px; box-shadow: 0 4px 20px rgba(0,0,0,0.15); }\n") +
        s(".toast-progress progress { background-color: ") + s(c(BLUE_5, dark::BLUE_5)) +
        s("; color: ") + ACCENT + s("; min-height: 3px; border-radius: 2px; }\n") +
        s("/* toast 类型背景 — 在 .toast-box 之后定义, 覆盖默认背景色 */\n") +
        s(".toast-bg-info  { background-color: #e8f5e9; }\n") +
        s(".toast-bg-warn  { background-color: #e3f2fd; }\n") +
        s(".toast-bg-error { background-color: #ffebee; }\n") +
        s(".toast-bg-fatal { background-color: #ffcdd2; }\n") +
        s("\n") +
        /* ── background layer ── */
        s(".main-bg-layer { background-size: cover; background-position: center; background-repeat: no-repeat; }\n") +
        s(".bg-suit-smart       { background-size: cover; background-position: center; }\n") +
        s(".bg-suit-center      { background-size: auto; background-position: center; background-repeat: no-repeat; }\n") +
        s(".bg-suit-fit         { background-size: contain; background-position: center; background-repeat: no-repeat; }\n") +
        s(".bg-suit-stretch     { background-size: 100% 100%; background-position: center; }\n") +
        s(".bg-suit-tile        { background-size: auto; background-position: 0 0; background-repeat: repeat; }\n") +
        s(".bg-suit-top-left    { background-size: auto; background-position: top left; background-repeat: no-repeat; }\n") +
        s(".bg-suit-top-right   { background-size: auto; background-position: top right; background-repeat: no-repeat; }\n") +
        s(".bg-suit-bottom-left { background-size: auto; background-position: bottom left; background-repeat: no-repeat; }\n") +
        s(".bg-suit-bottom-right{ background-size: auto; background-position: bottom right; background-repeat: no-repeat; }\n")
    ;
}

// ============================================================================
// 公共 API (视觉预览模式)
// ============================================================================

void load_stylesheet_default()
{
    // 跟随系统颜色方案
    bool is_dark = (adw_style_manager_get_color_scheme(
        adw_style_manager_get_default()) == ADW_COLOR_SCHEME_FORCE_DARK);
    auto css = build_css_impl(is_dark, "");
    apply_css(css);
}

}  // namespace pcl
