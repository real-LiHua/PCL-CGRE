#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace pcl::resource {

/* ═══════════════════════════════════════════════════════════════════════
 *  Project / Source / Sort / Loader enums
 * ═══════════════════════════════════════════════════════════════════════ */

enum class ProjectType { Mod = 0, Modpack, Datapack, ResourcePack, Shader, World };
enum class Source      { All, Modrinth, CurseForge };

/** Sort modes — mirrors PCL-CE CompSortType */
enum class SortType {
    Default   = 1,  // CurseForge: Popularity, Modrinth: relevance
    Relevance = 2,
    Downloads = 3,
    Follows   = 4,
    Newest    = 5,
    Updated   = 6
};

/** Mod loader types — mirrors PCL-CE CompLoaderType */
enum class CompLoaderType {
    Any        = 0,
    Forge      = 1,
    LiteLoader = 3,
    Fabric     = 4,
    Quilt      = 5,
    NeoForge   = 6,
    Vanilla    = 11
};

/* ═══════════════════════════════════════════════════════════════════════
 *  Normalised search result entry (both platforms unified)
 *  Enriched to match PCL-CE CompProject fields.
 * ═══════════════════════════════════════════════════════════════════════ */

struct ResourceEntry {
    std::string title;
    std::string description;
    std::string icon_url;        // direct URL (thumbnail / icon)
    std::vector<uint8_t> icon_bytes; // downloaded icon pixels (PNG/JPEG), empty = fallback
    std::string slug;            // URL-friendly identifier
    std::string project_id;      // Modrinth project_id or CurseForge numeric id
    std::string version_range;   // best-effort: e.g. "1.21.x"
    uint64_t    download_count = 0;
    uint64_t    followers      = 0;
    std::string date_modified;   // ISO 8601
    Source      source;

    // ── PCL-CE aligned enriched fields ──────────────────────────────────
    std::string author;          // primary author display name
    std::string license_name;    // e.g. "MIT", "All Rights Reserved"
    std::vector<std::string> categories;  // e.g. "科技", "魔法"
    std::vector<std::string> game_versions; // all supported MC versions
    std::vector<CompLoaderType> loaders;    // e.g. Forge, Fabric
    std::vector<std::string> screenshot_urls; // gallery thumbnails
    std::string wiki_url;
    std::string source_url;      // GitHub / source
    std::string discord_url;
    std::string project_url;     // direct link to CurseForge/Modrinth page
    std::string client_side;     // "required" / "optional" / "unsupported"
    std::string server_side;
    int         color = 0;       // Modrinth project color (RGB int)
};

/* ═══════════════════════════════════════════════════════════════════════
 *  CompFile — mirrors PCL-CE CompFile (individual file entry)
 * ═══════════════════════════════════════════════════════════════════════ */

struct CompFile {
    std::string file_name;
    std::string download_url;
    std::string game_version;    // primary MC version this file targets
    std::vector<std::string> game_versions;
    std::vector<CompLoaderType> mod_loaders;
    int64_t     file_size   = 0;
    int64_t     downloads   = 0;
    std::string date;
    std::string file_id;         // platform-specific ID
};

/* ═══════════════════════════════════════════════════════════════════════
 *  CompProject (full detail) — mirrors PCL-CE CompProject
 * ═══════════════════════════════════════════════════════════════════════ */

struct CompProject {
    ResourceEntry entry;
    std::vector<CompFile> files;
    bool files_loaded = false;
};

/* ═══════════════════════════════════════════════════════════════════════
 *  Search result container
 * ═══════════════════════════════════════════════════════════════════════ */

struct SearchResult {
    bool        success = false;
    std::string error;          // human-readable (Chinese) error
    std::vector<ResourceEntry> hits;
    uint64_t    total_hits = 0;
    int         curseforge_offset = 0;  // for cache tracking
    int         modrinth_offset   = 0;
    uint64_t    curseforge_total  = 0;
    uint64_t    modrinth_total    = 0;
};

/* ═══════════════════════════════════════════════════════════════════════
 *  CompProjectStorage — mirrors PCL-CE cache
 *
 *  Accumulates search results across pagination.  At any given time it
 *  holds all hits fetched so far plus the current offsets into each API.
 * ═══════════════════════════════════════════════════════════════════════ */

struct CompProjectStorage {
    std::vector<ResourceEntry> results;
    int  curseforge_offset  = 0;
    int  modrinth_offset    = 0;
    uint64_t curseforge_total = 0;
    uint64_t modrinth_total   = 0;
    std::string error_message;
    bool        loading = false;

    void clear()
    {
        results.clear();
        curseforge_offset = 0;
        modrinth_offset   = 0;
        curseforge_total  = 0;
        modrinth_total    = 0;
        error_message.clear();
        loading = false;
    }
};

/* ═══════════════════════════════════════════════════════════════════════
 *  CompFavorites — mirrors PCL-CE CompFavorites
 *
 *  Each FavData holds a named collection of project IDs.
 * ═══════════════════════════════════════════════════════════════════════ */

struct FavData {
    std::string name;
    std::string id;               // UUID
    std::vector<std::string> favs; // project IDs
    std::unordered_map<std::string, std::string> notes; // project_id → note
};

class CompFavorites {
public:
    /** Load favorites from disk (JSON file in config dir). */
    static void load();
    /** Persist to disk. */
    static void save();

    /** Check whether @a project_id is favorited in ANY collection. */
    static bool is_favorited(const std::string& project_id);

    /** Toggle @a project_id in the first collection.
     *  Returns true if it was added, false if removed. */
    static bool toggle(const std::string& project_id);

    /** Get all collection names. */
    static const std::vector<FavData>& collections();

private:
    static std::vector<FavData> s_favorites;
    static std::mutex           s_mutex;
    static std::string          s_file_path;
    static bool                 s_loaded;
};

/* ═══════════════════════════════════════════════════════════════════════
 *  Callback types & public API
 * ═══════════════════════════════════════════════════════════════════════ */

using SearchCallback = std::function<void(SearchResult)>;
using FilesCallback  = std::function<void(std::vector<CompFile>)>;

/** Launch a background thread to search resources.
 *
 *  @param query          Search text (may be empty for browse-all)
 *  @param type           Resource category (Mod, Modpack, …)
 *  @param source         Which platform(s) to query
 *  @param version_filter Game version from dropdown (e.g. "1.21.x"), empty = all
 *  @param offset         Pagination offset
 *  @param limit          Max results per source
 *  @param sort           Sort order
 *  @param loader_filter  Mod loader filter (Any = no filter)
 *  @param cb             Callback invoked on GTK main thread via g_idle_add
 */
void search_resources(const std::string&   query,
                      ProjectType          type,
                      Source               source,
                      const std::string&   version_filter,
                      int                  offset,
                      int                  limit,
                      SortType             sort,
                      CompLoaderType       loader_filter,
                      SearchCallback       cb);

/** Legacy overload (SortType::Default, CompLoaderType::Any). */
inline void search_resources(const std::string& query,
                             ProjectType        type,
                             Source             source,
                             const std::string& version_filter,
                             int                offset,
                             int                limit,
                             SearchCallback     cb)
{
    search_resources(query, type, source, version_filter,
                     offset, limit,
                     SortType::Default, CompLoaderType::Any,
                     std::move(cb));
}

/** Fetch the full file list for a project.
 *
 *  @param project_id   Platform-specific project ID
 *  @param from_cf      true = CurseForge, false = Modrinth
 *  @param cb           Callback on main thread with file list
 */
void fetch_project_files(const std::string& project_id,
                         bool               from_cf,
                         FilesCallback      cb);

/* ═══════════════════════════════════════════════════════════════════════
 *  Async icon loader
 *
 *  Downloads icon PNGs in background threads with disk caching.
 *  @param urls    One URL per result item (empty strings = no download).
 *  @param cb      Called on main thread for each completed download:
 *                 (index, raw PNG bytes).  Bytes empty on failure.
 * ═══════════════════════════════════════════════════════════════════════ */

using IconCallback = std::function<void(int index, std::vector<uint8_t> data)>;

void load_icons_async(std::vector<std::string> urls,
                      IconCallback             cb);

/* ═══════════════════════════════════════════════════════════════════════
 *  Inline helpers
 * ═══════════════════════════════════════════════════════════════════════ */

/** Modrinth facet string for the given project type. */
inline const char* project_type_to_modrinth_facet(ProjectType t)
{
    switch (t) {
        case ProjectType::Mod:          return "mod";
        case ProjectType::Modpack:      return "modpack";
        case ProjectType::Datapack:     return "datapack";
        case ProjectType::ResourcePack: return "resourcepack";
        case ProjectType::Shader:       return "shader";
        default: return nullptr;  // World — no Modrinth equivalent
    }
}

/** CurseForge classId for the given project type (gameId=432 = Minecraft). */
inline int project_type_to_curseforge_class(ProjectType t)
{
    switch (t) {
        case ProjectType::Mod:          return 6;
        case ProjectType::Modpack:      return 4471;
        case ProjectType::ResourcePack: return 4546;
        case ProjectType::Datapack:     return 4548;
        case ProjectType::Shader:       return 6552;
        case ProjectType::World:        return 4552;
        default: return 0;
    }
}

/** Format a download count into a human-readable string: "120.3M", "85K", … */
std::string format_download_count(uint64_t count);

/** Extract YYYY-MM-DD from an ISO 8601 date string. */
std::string format_date(const char* iso8601);

}  // namespace pcl::resource
