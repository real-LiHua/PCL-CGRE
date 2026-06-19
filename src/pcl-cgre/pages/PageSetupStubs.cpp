/**
 * PageSetupStubs.cpp — 占位 builder 实现 (待后续实现)
 *
 * 当对应子页面的完整实现完成后, 删除此文件中的对应函数即可。
 */

#include "pages/SettingsPage.hpp"
#include "util/IconHelper.hpp"

#include <gtk/gtk.h>

namespace pcl {

namespace {

/** 通用占位页面 (从 Blueprint 加载) */
GtkWidget* stub_page(const char* ui_name) {
    GtkBuilder* b = icon::load_ui(ui_name);
    if (!b) {
        auto* lbl = gtk_label_new("(Blueprint 加载失败)");
        gtk_widget_set_opacity(lbl, 0.45);
        return lbl;
    }
    GtkWidget* page = GTK_WIDGET(gtk_builder_get_object(b, "settings_subpage"));
    if (page)
        g_object_ref(page);
    else
        page = gtk_label_new("(页面加载失败)");
    g_object_unref(b);
    return page;
}

}  // anonymous namespace

// ── P1: 待实现 ──────────────────────────────────────────────────────────

GtkWidget* build_page_launch_set()     { return stub_page("page_setup_launch.ui"); }
GtkWidget* build_page_java_mgmt()      { return stub_page("page_setup_java.ui"); }
GtkWidget* build_page_game_manage()    { return stub_page("page_setup_game_manage.ui"); }

// ── P2: 待实现 ──────────────────────────────────────────────────────────

GtkWidget* build_page_ui()          { return stub_page("page_setup_ui.ui"); }
GtkWidget* build_page_language()    { return stub_page("page_setup_language.ui"); }
GtkWidget* build_page_misc()        { return stub_page("page_setup_misc.ui"); }

// ── P3: 待实现 ──────────────────────────────────────────────────────────

GtkWidget* build_page_about()   { return stub_page("page_setup_about.ui"); }
GtkWidget* build_page_update()  { return stub_page("page_setup_update.ui"); }

}  // namespace pcl
