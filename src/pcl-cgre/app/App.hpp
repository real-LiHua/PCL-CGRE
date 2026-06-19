#pragma once

#include <gtk/gtk.h>
#include <adwaita.h>

/**
 * PclApplication — GTK 应用包装类
 *
 * 封装 AdwApplication 的创建与运行，连接 activate 信号到
 * pcl::create_main_window()。
 */
class PclApplication {
public:
    PclApplication();
    ~PclApplication();

    int run(int argc, char* argv[]);

private:
    AdwApplication* m_app;

    static void on_activate(GtkApplication* app, gpointer user_data);
    static void on_startup(GtkApplication* app, gpointer user_data);
};
