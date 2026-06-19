#pragma once

#include <gtk/gtk.h>

#include <string>

namespace pcl::icon {

/**
 * Resolve the directory containing the running binary via /proc/self/exe.
 * Returns an empty string on failure (non-Linux).
 */
std::string resolve_binary_dir();

/* ============================================================================
 * Lucide SVG icon loader — GTK icon-theme integration
 *
 * Registers the PCL-CGRE icon theme ("pcl-cgre") at startup so that
 * gtk_image_new_from_icon_name("pcl-<name>-symbolic") finds the
 * corresponding Lucide SVG.  Because the icons are registered as
 * "symbolic", GTK recolours them automatically from the CSS `color`
 * property — white on the header bar, dark in content areas, etc.
 *
 * Each icon is named "pcl-<lucide_name>-symbolic":
 *   pcl-play-symbolic, pcl-search-symbolic, pcl-download-symbolic, …
 * ============================================================================ */

/**
 * Register the PCL-CGRE icon theme with GTK.
 *
 * Searches for the "pcl-cgre" theme directory:
 *   1. $PCL_ICON_DIR  environment variable
 *   2. <binary_dir>/resources/icons/     (build tree)
 *   3. <binary_dir>/../resources/icons/  (dev tree)
 *   4. /usr/share/icons/                 (system install)
 *
 * Call once at startup before any icon is loaded.
 */
void init_icon_theme();

/**
 * Load a Lucide icon by its bare name (without "pcl-" prefix or "-symbolic").
 *
 *   auto* img = icon::load("play", 16);  // → "pcl-play-symbolic" @ 16 px
 *
 * The returned GtkImage will inherit CSS `color` for recolouring.
 *
 * @param name      Bare Lucide icon name (e.g. "search", "play")
 * @param px_size   Pixel size (square icons; 16 = GTK_ICON_SIZE_NORMAL-ish)
 * @return          GtkImage widget, never null
 */
GtkWidget* load(const char* name, int px_size);

/**
 * Load a PCL-CE block image (PNG) from Images/Blocks/ by its bare name.
 *
 *   auto* img = icon::load_block("Fabric", 24);
 *
 * These are used to represent mod loaders and version types.
 *
 * @param name      Bare file name without extension (e.g. "Fabric", "Minecraft")
 * @param px_size   Display pixel size; 0 = native size
 * @return          GtkImage widget, never null (empty image on failure)
 */
GtkWidget* load_block(const char* name, int px_size);

/**
 * Load a Blueprint .ui file, trying GResource first, then disk.
 *
 *   auto* b = icon::load_ui("launch_sidebar.ui");
 *
 * @param name  UI file name (e.g. "launch_sidebar.ui")
 * @return      GtkBuilder with loaded UI, never null
 */
GtkBuilder* load_ui(const char* name);

}  // namespace pcl::icon
