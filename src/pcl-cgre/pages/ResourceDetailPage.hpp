#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <gtk/gtk.h>

namespace pcl {

/**
 * Navigate to the resource detail page.
 *
 * Switches the download page's internal stack to the detail view,
 * replaces the header bar tabs with a back-navigation bar,
 * hides the left sidebar, and populates the detail card.
 *
 * @param trigger_widget  The widget that triggered navigation (used to find the window).
 */
void navigate_to_resource_detail(GtkWidget*        trigger_widget,
                                 const std::string& name,
                                 const std::string& description,
                                 const std::string& source,
                                 const std::string& version_range,
                                 uint64_t           download_count,
                                 const std::string& date_modified,
                                 const std::string& icon_url,
                                 // ── PCL-CE aligned enriched params ──────
                                 const std::string& project_id  = "",
                                 const std::string& author      = "",
                                 const std::string& license_name = "",
                                 const std::string& project_url  = "",
                                 const std::string& wiki_url     = "",
                                 const std::string& source_url   = "",
                                 const std::vector<std::string>& categories   = {},
                                 const std::vector<std::string>& game_versions = {},
                                 uint64_t           followers    = 0);

/**
 * Build the resource detail page (scrolled-window containing the full layout).
 *
 * The page is added to the download stack as "detail".
 * Fields are populated at navigation time via g_object_set_data lookups.
 */
GtkWidget* build_resource_detail_page();

/**
 * Build the back-navigation bar for the header: [← icon] [resource name].
 */
GtkWidget* build_back_nav();

}  // namespace pcl
