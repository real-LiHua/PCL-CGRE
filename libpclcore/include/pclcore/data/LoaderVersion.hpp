#pragma once

#include <string>
#include <vector>

namespace pclcore::mc {

/**
 * A single loader version displayed in a card.
 */
struct LoaderVersion {
    std::string version;    // Display string, e.g. "1.21.5-0.16.5"
    std::string date;       // ISO date (YYYY-MM-DD), or "" if unknown
    std::string branch;     // e.g. "stable" / "latest" (Forge only)
    std::string url;        // Download / installer URL
};

/**
 * Result of one loader version-list fetch.
 */
struct LoaderVersionList {
    std::string loader_name;         // e.g. "OptiFine", "Forge"
    bool        loaded = false;
    std::string error;
    std::vector<LoaderVersion> versions;
};

}  // namespace pclcore::mc
