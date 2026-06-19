#pragma once

#include "data/McVersion.hpp"

#include <functional>

namespace pcl::mc {

/** Callback type: void(VersionManifest) */
using ManifestCallback = std::function<void(VersionManifest)>;

/**
 * Asynchronously fetch the Minecraft version manifest.
 *
 * Tries BMCLAPI first; falls back to official Mojang API.
 * Once the response is parsed, `callback` is invoked on the
 * GTK main thread with the populated VersionManifest.
 */
void fetch_version_manifest(ManifestCallback callback);

/**
 * Load April Fools versions from the local lirpa_loof.json resource.
 *
 * The official version manifest API does not return `april_fool`-typed
 * versions, so we maintain our own list.  This function searches for
 * lirpa_loof.json relative to the binary (same resolution strategy as
 * IconHelper) and parses it into VersionEntry structs.
 *
 * @return A vector of VersionEntry, sorted by year descending (newest first).
 *         Returns an empty vector if the file cannot be found or parsed.
 */
std::vector<VersionEntry> load_lirpa_loof_versions();

}  // namespace pcl::mc
