#pragma once

#include <gtk/gtk.h>

namespace pcl {

/* ==========================================================================
 * 核心页面构件
 * ========================================================================== */

/** 左侧边栏: 模式栏 + 头像用户名 + 启动按钮 + 版本行 */
GtkWidget* build_sidebar();

/** 右侧资讯面板 + 浮动 FAB */
GtkWidget* build_news_with_fab();

/** 启动页: 左右两栏布局 */
GtkWidget* build_launch_page();

/* ==========================================================================
 * 通用两栏布局
 * ========================================================================== */

/** 左侧 300px + 分隔线 + 右侧 (hexpand) */
GtkWidget* build_two_panel_page(GtkWidget* left, GtkWidget* right);

/**
 * 导航列表项 — 匹配 PCL-CE MyListItem
 */
GtkWidget* build_nav_item(const char* label,
                          const char* icon_name,
                          bool active);

/**
 * 导航列表项 (带副标题, 无图标) — 匹配 PCL-CE MyListItem (tall)
 *
 * 比普通 nav_item 更高以容纳两行文字:
 *   标题 (semibold) + 副标题 (小字, 低透明度)
 */
GtkWidget* build_nav_item_subtitle(const char* label,
                                   const char* subtitle,
                                   bool active);

/* ==========================================================================
 * 通用控件构件 (匹配 PCL-CE 控件)
 * ========================================================================== */

/**
 * 提示条 — 匹配 PCL-CE MyHint
 * @param text       提示文字
 * @param show_close 是否显示关闭按钮
 */
GtkWidget* build_hint_bar(const char* text, bool show_close);

/**
 * 搜索框 — 匹配 PCL-CE MySearchBox
 * 包含搜索图标 + GtkEntry + 清除按钮
 */
GtkWidget* build_search_box();

/**
 * 增强版新闻卡片行 — 匹配 PCL-CE 卡片布局
 */
GtkWidget* build_news_card(const char* title,
                           const char* subtitle,
                           const char* icon_name);

/**
 * 右下角浮动按钮组 — 匹配 MyExtraButton 系列
 */
GtkWidget* build_extra_buttons();

/**
 * 加载动画构件 — 匹配 PCL-CE MyLoading
 */
GtkWidget* build_loading_widget(const char* label_text);

/**
 * 启动中页面 — 加载动画 + 进度条 + 状态信息 + 取消按钮
 */
GtkWidget* build_launching_page();

}  // namespace pcl
