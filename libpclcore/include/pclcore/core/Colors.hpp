#pragma once

namespace pclcore::data {
enum class ThemeBrightness {
    kLight,
    kDark
};
}

namespace pclcore::colors {

// ============================================================================
// PCL-CE 完整配色常量 (来源: Application.xaml)
// ============================================================================

// ── 主蓝色系 (ColorBrush1–8) — 浅色主题 ──────────────────────────────────

constexpr const char* BLUE_1 = "#343d4a";  // ColorBrush1  主文字色
constexpr const char* BLUE_2 = "#0b5bcb";  // ColorBrush2  Highlight 按钮常态
constexpr const char* BLUE_3 = "#1370f3";  // ColorBrush3  标题栏背景、高亮
constexpr const char* BLUE_4 = "#4890f5";  // ColorBrush4  IconButton Color 常态
constexpr const char* BLUE_5 = "#96c0f9";  // ColorBrush5  IconButton Color 悬停/Bg0
constexpr const char* BLUE_6 = "#d5e6fd";  // ColorBrush6  右侧资讯面板
constexpr const char* BLUE_7 = "#e0eafd";  // ColorBrush7  Button 悬停背景
constexpr const char* BLUE_8 = "#eaf2fe";  // ColorBrush8  左侧边栏背景

// ── 背景色 ────────────────────────────────────────────────────────────────

constexpr const char* BG_MAIN         = "#fbfbfb";  // ColorBrushBackground
constexpr const char* BG_TRANSPARENT  = "#d2fbfbfb";  // ColorBrushTransparentBackground

// ── 灰度系 (ColorBrushGray1–8) ──────────────────────────────────────────

constexpr const char* GRAY_1 = "#404040";
constexpr const char* GRAY_2 = "#737373";
constexpr const char* GRAY_3 = "#8c8c8c";
constexpr const char* GRAY_4 = "#a6a6a6";
constexpr const char* GRAY_5 = "#cccccc";
constexpr const char* GRAY_6 = "#ebebeb";
constexpr const char* GRAY_7 = "#f0f0f0";
constexpr const char* GRAY_8 = "#f5f5f5";

// ── 红色系 ────────────────────────────────────────────────────────────────

constexpr const char* RED_LIGHT = "#ff4c4c";   // ColorBrushRedLight
constexpr const char* RED_DARK  = "#ce2111";   // ColorBrushRedDark
constexpr const char* RED_BACK  = "#80fbdddd"; // ColorBrushRedBack  (半透明)

// ── 状态色 ────────────────────────────────────────────────────────────────

constexpr const char* STATUS_ERROR = "#e74c3c";  // ColorBrushError
constexpr const char* STATUS_WARN  = "#f39c12";  // ColorBrushWarn
constexpr const char* STATUS_FATAL = "#c23616";  // ColorBrushFatal

// ── 透明 / 半透明变体 ────────────────────────────────────────────────────

constexpr const char* HALF_WHITE        = "#55ffffff";  // ColorBrushHalfWhite
constexpr const char* SEMI_WHITE        = "#bbffffff";  // ColorBrushSemiWhite
constexpr const char* SEMI_TRANSPARENT  = "#01eaf2fe";  // ColorBrushSemiTransparent
constexpr const char* PURE_TRANSPARENT  = "#00ffffff";  // ColorBrushTransparent

// ── 纯色 ──────────────────────────────────────────────────────────────────

constexpr const char* WHITE = "#ffffff";
constexpr const char* BLACK = "#000000";

// ── 工具提示 / 阴影 ──────────────────────────────────────────────────────

constexpr const char* TOOLTIP_BG    = "#e5ffffff";  // ColorBrushToolTip
constexpr const char* MSGBOX_SHADOW = "#3c3c3c";    // ColorObjectMsgBoxShadow

// ── 启动按钮 (介于 BLUE_2 和 BLUE_3 之间, 比标题栏略深) ──────────────

constexpr const char* LAUNCH_BTN_BG = "#1066e6";  // 启动游戏按钮底色

// ── 提示条 ────────────────────────────────────────────────────────────────

constexpr const char* HINT_BORDER = "#99FF4444";  // MyHint 左边框红色

// ── 快捷别名 (保持与旧代码兼容) ──────────────────────────────────────────

constexpr const char* HEADER_BLUE   = BLUE_3;
constexpr const char* SIDEBAR_BG    = BLUE_8;
constexpr const char* NEWS_PANEL_BG = BLUE_6;

// ── RGBA 函数色值 (用于 CSS rgba()) ──────────────────────────────────────

constexpr const char* HEADER_BTN_HOVER = "rgba(255, 255, 255, 0.15)";
constexpr const char* TAB_INACTIVE = "rgba(255, 255, 255, 0.7)";
constexpr const char* TAB_HOVER = "rgba(255, 255, 255, 0.12)";
constexpr const char* CARD_BG = "rgba(255, 255, 255, 0.65)";
constexpr const char* BTN_BG_NORMAL = "rgba(255, 255, 255, 0.33)";
constexpr const char* CHECKBOX_BG = "rgba(255, 255, 255, 0.33)";

// ============================================================================
// 深色主题蓝色系 (Dark Blue)
// ============================================================================

namespace dark {

constexpr const char* BLUE_1 = "#c8d6e5";
constexpr const char* BLUE_2 = "#5b9cf5";
constexpr const char* BLUE_3 = "#1a3a6b";
constexpr const char* BLUE_4 = "#3a7de0";
constexpr const char* BLUE_5 = "#2a5a9e";
constexpr const char* BLUE_6 = "#152238";
constexpr const char* BLUE_7 = "#1a2d4a";
constexpr const char* BLUE_8 = "#131d2e";

constexpr const char* BG_MAIN         = "#1a1f2e";
constexpr const char* BG_TRANSPARENT  = "#d21a1f2e";

constexpr const char* GRAY_1 = "#c0c0c0";
constexpr const char* GRAY_2 = "#a0a0a0";
constexpr const char* GRAY_3 = "#808080";
constexpr const char* GRAY_4 = "#6a6a6a";
constexpr const char* GRAY_5 = "#505050";
constexpr const char* GRAY_6 = "#353535";
constexpr const char* GRAY_7 = "#2a2a2a";
constexpr const char* GRAY_8 = "#202020";

constexpr const char* RED_LIGHT = "#ff6b6b";
constexpr const char* RED_DARK  = "#c0392b";
constexpr const char* RED_BACK  = "#80fbdddd";

constexpr const char* STATUS_ERROR = "#e74c3c";
constexpr const char* STATUS_WARN  = "#f39c12";
constexpr const char* STATUS_FATAL = "#c23616";

constexpr const char* HALF_WHITE        = "#551a1f2e";
constexpr const char* SEMI_WHITE        = "#bb1a1f2e";
constexpr const char* SEMI_TRANSPARENT  = "#01152038";
constexpr const char* PURE_TRANSPARENT  = "#00000000";

constexpr const char* WHITE = "#e8e8e8";
constexpr const char* BLACK = "#000000";

constexpr const char* TOOLTIP_BG    = "#e52a2a3a";
constexpr const char* MSGBOX_SHADOW = "#1a1a1a";

constexpr const char* LAUNCH_BTN_BG = "#2a6ed4";

constexpr const char* HINT_BORDER = "#99FF4444";

constexpr const char* HEADER_BLUE   = BLUE_3;
constexpr const char* SIDEBAR_BG    = BLUE_8;
constexpr const char* NEWS_PANEL_BG = BLUE_6;

constexpr const char* HEADER_BTN_HOVER = "rgba(255, 255, 255, 0.10)";
constexpr const char* TAB_INACTIVE     = "rgba(255, 255, 255, 0.65)";
constexpr const char* TAB_HOVER        = "rgba(255, 255, 255, 0.08)";
constexpr const char* CARD_BG          = "rgba(30, 40, 60, 0.65)";
constexpr const char* BTN_BG_NORMAL    = "rgba(30, 40, 60, 0.40)";
constexpr const char* CHECKBOX_BG      = "rgba(30, 40, 60, 0.40)";

}  // namespace dark

// ============================================================================
// 主题切换辅助
// ============================================================================

template<const char* const& Light, const char* const& Dark>
constexpr const char* theme_color(data::ThemeBrightness b) {
    return (b == data::ThemeBrightness::kDark) ? Dark : Light;
}

}  // namespace pclcore::colors
