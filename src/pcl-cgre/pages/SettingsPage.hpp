#pragma once

#include <gtk/gtk.h>

namespace pcl {

/**
 * 构建设置页面 — 三栏层级布局 (左/中/右) + 互斥切换
 *
 * 左栏: 5 个一级分类 (全局游戏设置 / 实例设置 / 账号设置 / 启动器设置 / 关于此软件)
 * 中栏: 二级子项导航 (随左栏切换)
 * 右栏: 卡片内容 (随中栏切换, 目前均为占位)
 *
 * 所有配置项已清空, 供用户自行实现各子页面。
 */
GtkWidget* build_settings_page();

/* ── 各设置子页面 builder ───────────────────────────────────────────── */

/** 启动设置 (§4.2.1) */
GtkWidget* build_page_launch_set();

/** Java 管理 (§4.2.2) */
GtkWidget* build_page_java_mgmt();

/** 游戏管理 (§4.2.3) */
GtkWidget* build_page_game_manage();

/** 个性化 (§4.2.6) */
GtkWidget* build_page_ui();

/** 语言 (§4.2.7) */
GtkWidget* build_page_language();

/** 杂项 (§4.2.8) */
GtkWidget* build_page_misc();

/** 关于 (§4.2.9) */
GtkWidget* build_page_about();

/** 更新 (§4.2.9) */
GtkWidget* build_page_update();

}  // namespace pcl
