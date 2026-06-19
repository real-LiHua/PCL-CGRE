#pragma once

#include "data/LoaderVersion.hpp"

#include <functional>
#include <string>

namespace pcl::mc {

/** Callback type: void(LoaderVersionList) */
using LoaderCallback = std::function<void(LoaderVersionList)>;

/**
 * Asynchronously fetch available versions of a mod loader
 * for a given Minecraft version.
 *
 * Spawns a background thread; the callback is invoked on the
 * GTK main thread with the populated LoaderVersionList.
 *
 * @param loader_name  "optifine", "forge", "fabric", "quilt",
 *                     "liteloader", or "cleanroom"
 * @param mc_version   Target Minecraft version (e.g. "1.21.5")
 * @param callback     Invoked on main thread on completion
 */
void fetch_loader_versions(const std::string& loader_name,
                           const std::string& mc_version,
                           LoaderCallback     callback);

}  // namespace pcl::mc
