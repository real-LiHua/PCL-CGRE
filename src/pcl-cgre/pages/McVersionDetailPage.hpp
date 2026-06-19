#pragma once

#include <string>

#include <gtk/gtk.h>

namespace pcl {

/**
 * Navigate to the Minecraft version detail / install page.
 *
 * Switches the download page's internal stack to the detail view,
 * replaces the header bar tabs with a back-navigation bar,
 * hides the left sidebar, and populates the version install UI.
 *
 * @param trigger_widget  The widget that triggered navigation (used to find the window).
 * @param version_id      The Minecraft version string (e.g. "1.21.5", "25w22a").
 * @param version_type    Mojang type string ("release","snapshot","old_alpha","old_beta","april_fool")
 */
void navigate_to_mc_version_detail(GtkWidget*        trigger_widget,
                                   const std::string& version_id,
                                   const std::string& version_type = "");

/**
 * Build the Minecraft version detail / install page.
 *
 * The page is added to the download stack as "mc-detail".
 * Fields are populated at navigation time.
 */
GtkWidget* build_mc_version_detail_page();

}  // namespace pcl
