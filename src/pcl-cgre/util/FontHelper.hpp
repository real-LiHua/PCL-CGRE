#pragma once

namespace pcl::font {

/**
 * Register custom TTF fonts from resources/fonts/ with Fontconfig.
 *
 * Searches for fonts in:
 *   1. $PCL_FONT_DIR  environment variable
 *   2. <binary_dir>/resources/fonts/     (build tree)
 *   3. <binary_dir>/../resources/fonts/  (dev tree)
 *
 * Call once at startup — after this, CSS font-family rules that reference
 * "HarmonyOS Sans SC" or "HarmonyOS Sans" will resolve correctly.
 */
void load_custom_fonts();

}  // namespace pcl::font
