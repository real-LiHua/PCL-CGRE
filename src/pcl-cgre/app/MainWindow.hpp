#pragma once

#include <gtk/gtk.h>
#include <adwaita.h>

namespace pcl {

/**
 * Create the main application window.
 *
 * Wires together the header bar (tabs + back-navigation), the top-level
 * AdwViewStack (launch / download / settings / more), and the initial
 * Minecraft-version fetch trigger.
 */
GtkWidget* create_main_window(GtkApplication* app);

}  // namespace pcl
