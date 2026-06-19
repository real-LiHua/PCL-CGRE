#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <gtk/gtk.h>

namespace pcl {

/** Data carried by each resource list item — used for click-to-detail navigation. */
struct ResourceItemData {
    std::string title;
    std::string description;
    std::string source;         // "CurseForge" / "Modrinth"
    std::string version_range;
    std::string date_modified;  // ISO 8601
    std::string icon_url;
    std::string project_id;     // for favorites & detail navigation
    uint64_t    download_count = 0;
    // ── PCL-CE aligned enrichment ─────────────────────────────────────
    std::string author;
    std::string license_name;
    std::string project_url;
    std::string wiki_url;
    std::string source_url;
    std::vector<std::string> categories;  // e.g. "科技", "魔法"
    std::vector<std::string> game_versions;
    uint64_t    followers = 0;
};

/**
 * Build a resource list item row (对标 PCL-CE MyCompItem, 64px high).
 *
 * @param fallback_icon  Lucide icon name for the placeholder logo.
 * @param title           Primary title.
 * @param description     Short description.
 * @param version         Version range string.
 * @param downloads       Formatted download count.
 * @param update_time     Formatted date.
 * @param source          "CurseForge" or "Modrinth".
 * @param item_data       If non-null, the struct is copied to heap and attached
 *                        to the returned row via g_object_set_data("item-data").
 *                        A GtkGestureClick is also installed — on press it calls
 *                        navigate_to_resource_detail().
 * @param subtitle        Optional. Secondary title (e.g. original English name).
 *                        Empty string = hidden.
 * @param tags            Optional. Category tags to display under the title.
 *                        Empty vector = hidden.
 */
GtkWidget* build_resource_item(const char*          fallback_icon,
                               const char*          title,
                               const char*          description,
                               const char*          version,
                               const char*          downloads,
                               const char*          update_time,
                               const char*          source,
                               ResourceItemData*    item_data = nullptr,
                               const char*          subtitle  = nullptr,
                               const std::vector<const char*>& tags = {});

/** Three-icon hover action bar (save / changelog / server). */
GtkWidget* build_version_actions();

/** Attach mouse-enter/leave to show/hide @a actions on @a row. */
void attach_row_hover(GtkWidget* row, GtkWidget* actions);

}  // namespace pcl
