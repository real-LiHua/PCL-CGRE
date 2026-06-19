#include "app/App.hpp"
#include "app/MainWindow.hpp"
#include "util/IconHelper.hpp"
#include "util/FontHelper.hpp"
#include "core/Log.hpp"

/* ============================================================================
 * PclApplication — GTK 应用包装类
 * ============================================================================ */

PclApplication::PclApplication()
    : m_app(adw_application_new("com.example.pcl-cgre",
                                 G_APPLICATION_DEFAULT_FLAGS))
{
    g_signal_connect(m_app, "activate", G_CALLBACK(on_activate), this);
    g_signal_connect(m_app, "startup",  G_CALLBACK(on_startup),  this);
}

PclApplication::~PclApplication()
{
    g_object_unref(m_app);
}

int PclApplication::run(int argc, char* argv[])
{
    int status = g_application_run(G_APPLICATION(m_app), argc, argv);
    return status;
}

/* static */
void PclApplication::on_startup(GtkApplication* app, gpointer user_data)
{
    (void) app;
    (void) user_data;

    // Initialize logging early
    pcl::log::init();
    LOG_INFO("PCL-CGRE starting up...");

    // Load embedded GResource (icons / fonts / UI / JSON)
    std::string res_path = pcl::icon::resolve_binary_dir() + "/pcl-cgre.gresource";
    GError* err = nullptr;
    GResource* res = g_resource_load(res_path.c_str(), &err);
    if (res) {
        g_resources_register(res);
        LOG_INFO("GResource loaded: %s", res_path.c_str());
    } else {
        LOG_WARN("GResource not found: %s — %s",
                 res_path.c_str(), err ? err->message : "unknown");
        g_clear_error(&err);
    }

    // Register custom fonts with Fontconfig.
    pcl::font::load_custom_fonts();

    // Register the PCL-CGRE icon theme with GTK before any widgets
    // are created.  Must be called before on_activate.
    pcl::icon::init_icon_theme();

    // 跟随系统颜色方案
    adw_style_manager_set_color_scheme(
        adw_style_manager_get_default(),
        ADW_COLOR_SCHEME_DEFAULT);

    LOG_INFO("Startup complete");
}

/* static */
void PclApplication::on_activate(GtkApplication* app, gpointer user_data)
{
    (void) user_data;

    GtkWidget* window = pcl::create_main_window(app);
    gtk_window_present(GTK_WINDOW(window));
}
