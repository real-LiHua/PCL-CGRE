#pragma once

#include <string>
#include <vector>
#include <ctime>

namespace pcl::mc {

/** A single Minecraft version entry from the manifest. */
struct VersionEntry {
    std::string id;           // e.g. "1.21.5", "25w22a"
    std::string type;         // "release", "snapshot", "old_alpha", "old_beta"
    std::string url;          // per-version JSON URL
    std::string release_time; // ISO 8601
    time_t      timestamp;    // parsed from release_time (0 = unknown)
};

/** Group of versions sharing the same category. */
struct VersionGroup {
    std::string label;            // "正式版", "预览版", …
    std::string block_icon;       // "Minecraft", "Command", …
    std::string type_filter;      // Mojang type string
    std::vector<VersionEntry> versions;
};

/** Complete parsed manifest. */
struct VersionManifest {
    std::string latest_release;
    std::string latest_snapshot;
    std::vector<VersionGroup> groups;
    bool        loaded = false;
    std::string source_name;  // "BMCLAPI" or "Mojang"
    std::string error;        // set when loaded == false
};

}  // namespace pcl::mc
