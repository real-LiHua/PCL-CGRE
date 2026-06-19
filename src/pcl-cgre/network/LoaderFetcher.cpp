#include "network/LoaderFetcher.hpp"
#include "network/HttpUtil.hpp"
#include "core/Log.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "pclcore/network/Dispatcher.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace pcl::mc {

/* ═══════════════════════════════════════════════════════════════════════
 *  内部: URL 模板 & 辅助函数
 * ═══════════════════════════════════════════════════════════════════════ */

namespace {

/* ── Helper: return string value if present and is a string, else "" ─── */
inline std::string jstr(const json& val) {
    return val.is_string() ? val.get<std::string>() : "";
}

/* ── Helper: return string member by key, or "" if missing/not a string ─ */
inline std::string jstr(const json& obj, const char* key) {
    auto it = obj.find(key);
    return (it != obj.end()) ? jstr(*it) : "";
}

/* ── Extract YYYY-MM-DD from ISO 8601 ─────────────────────────────────── */
std::string extract_date(const std::string& iso)
{
    if (iso.size() >= 10) return iso.substr(0, 10);
    return "";
}

/* ── Extract YYYY-MM-DD from timestamp (seconds since epoch) ─────────── */
std::string date_from_timestamp(double ts)
{
    if (ts <= 0) return "";
    time_t t = static_cast<time_t>(ts);
    struct tm* lt = localtime(&t);
    if (!lt) return "";
    char buf[16];
    strftime(buf, sizeof(buf), "%Y-%m-%d", lt);
    return buf;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Parser: OptiFine
 *
 *  API: https://bmclapi2.bangbang93.com/optifine/versionList
 *  Returns JSON array: [{mcversion, type, patch, filename, forge}]
 * ═══════════════════════════════════════════════════════════════════════ */
LoaderVersionList parse_optifine(const std::string& body,
                                 const std::string& mc_version)
{
    LoaderVersionList result;
    result.loader_name = "OptiFine";

    if (body.empty()) {
        result.error = "请求 OptiFine 版本列表时服务器无响应";
        return result;
    }

    json j;
    try {
        j = json::parse(body);
    } catch (const json::parse_error& e) {
        result.error = std::string("解析 OptiFine 数据失败: ") + e.what();
        return result;
    }

    if (!j.is_array()) {
        result.error = "OptiFine 返回数据格式异常";
        return result;
    }

    for (auto& obj : j) {
        std::string obj_mcver = jstr(obj, "mcversion");
        if (obj_mcver.empty() || mc_version != obj_mcver) continue;

        LoaderVersion lv;
        std::string type  = jstr(obj, "type");
        std::string patch = jstr(obj, "patch");
        std::string fname = jstr(obj, "filename");

        /* version = type + " " + patch, e.g. "HD_U J3" */
        lv.version = type + " " + patch;

        /* OptiFine API doesn't provide dates — leave empty */
        lv.date = "";

        /* Build download URL from filename */
        if (!fname.empty())
            lv.url = std::string("https://bmclapi2.bangbang93.com/optifine/")
                   + mc_version + "/" + fname;

        result.versions.push_back(std::move(lv));
    }

    /* Sort: newest first by patch string (reverse alphabetical ≈ newest) */
    std::sort(result.versions.begin(), result.versions.end(),
              [](const LoaderVersion& a, const LoaderVersion& b) {
                  return a.version > b.version;
              });

    result.loaded = true;
    return result;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Parser: Forge
 *
 *  API: https://bmclapi2.bangbang93.com/forge/minecraft/{MC_VERSION}
 *  Returns JSON array: [{version, branch, modified, files[{category,...}]}]
 * ═══════════════════════════════════════════════════════════════════════ */
LoaderVersionList parse_forge(const std::string& body)
{
    LoaderVersionList result;
    result.loader_name = "Forge";

    if (body.empty()) {
        result.error = "请求 Forge 版本列表时服务器无响应";
        return result;
    }

    json j;
    try {
        j = json::parse(body);
    } catch (const json::parse_error& e) {
        result.error = std::string("解析 Forge 数据失败: ") + e.what();
        return result;
    }

    if (!j.is_array()) {
        result.error = "Forge 返回数据格式异常";
        return result;
    }

    for (auto& obj : j) {
        LoaderVersion lv;

        /* version field */
        lv.version = jstr(obj, "version");

        /* branch: "recommended" / "latest" / etc. */
        lv.branch = jstr(obj, "branch");

        /* modified → date */
        lv.date = jstr(obj, "modified");
        if (lv.date.size() >= 10) lv.date = lv.date.substr(0, 10);

        /* Build download URL from files[0] (installer) */
        auto fit = obj.find("files");
        if (fit != obj.end() && fit->is_array()) {
            for (auto& fobj : *fit) {
                std::string cat = jstr(fobj, "category");
                if (cat == "installer") {
                    std::string fmt  = jstr(fobj, "format");
                    std::string hash = jstr(fobj, "hash");
                    if (!fmt.empty() && !hash.empty())
                        lv.url = std::string("https://bmclapi2.bangbang93.com/maven/net/minecraftforge/forge/")
                               + lv.version + "/forge-" + lv.version + "-installer." + fmt;
                    break;
                }
            }
        }

        result.versions.push_back(std::move(lv));
    }

    /* Sort: recommended first, then by version descending */
    std::sort(result.versions.begin(), result.versions.end(),
              [](const LoaderVersion& a, const LoaderVersion& b) {
                  bool a_rec = (a.branch == "recommended");
                  bool b_rec = (b.branch == "recommended");
                  if (a_rec != b_rec) return a_rec;
                  return a.version > b.version;
              });

    result.loaded = true;
    return result;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Parser: Fabric
 *
 *  API: https://bmclapi2.bangbang93.com/fabric-meta/v2/versions
 *  Uses the loader[] array (matching PCL-CE), NOT installer[].
 *  Entries: {version, stable, maven, build}
 * ═══════════════════════════════════════════════════════════════════════ */
LoaderVersionList parse_fabric(const std::string& body)
{
    LoaderVersionList result;
    result.loader_name = "Fabric";

    if (body.empty()) {
        result.error = "请求 Fabric 版本列表时服务器无响应";
        return result;
    }

    json j;
    try {
        j = json::parse(body);
    } catch (const json::parse_error& e) {
        result.error = std::string("解析 Fabric 数据失败: ") + e.what();
        return result;
    }

    if (!j.is_object()) {
        result.error = "Fabric 返回数据格式异常";
        return result;
    }

    auto ldr_it = j.find("loader");
    if (ldr_it == j.end()) {
        result.error = "Fabric 未收录该版本";
        return result;
    }

    for (auto& item : *ldr_it) {
        LoaderVersion lv;
        lv.version = jstr(item, "version");

        /* Build Maven download URL */
        std::string maven = jstr(item, "maven");
        if (!maven.empty()) {
            /* net.fabricmc:fabric-loader:0.19.3 → net/fabricmc/fabric-loader/0.19.3 */
            std::string path = maven;
            auto c1 = path.find(':');
            if (c1 != std::string::npos) path[c1] = '/';
            auto c2 = path.find(':', c1 + 1);
            if (c2 != std::string::npos) path[c2] = '/';
            lv.url = std::string("https://maven.fabricmc.net/") + path
                   + "/" + path.substr(path.rfind('/') + 1)
                   + "-" + lv.version + ".jar";
        }

        auto stable_it = item.find("stable");
        if (stable_it != item.end() && stable_it->is_boolean()) {
            lv.branch = stable_it->get<bool>() ? "stable" : "beta";
        }

        lv.date = "";
        result.versions.push_back(std::move(lv));
    }

    /* Sort: stable first, then version descending */
    std::sort(result.versions.begin(), result.versions.end(),
              [](const LoaderVersion& a, const LoaderVersion& b) {
                  bool a_stable = (a.branch == "stable");
                  bool b_stable = (b.branch == "stable");
                  if (a_stable != b_stable) return a_stable;
                  return a.version > b.version;
              });

    result.loaded = true;
    return result;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Parser: Quilt
 *
 *  API: https://meta.quiltmc.org/v3/versions
 *  Uses the loader[] array (matching PCL-CE), NOT installer[].
 *  Same structure as Fabric.
 * ═══════════════════════════════════════════════════════════════════════ */
LoaderVersionList parse_quilt(const std::string& body)
{
    LoaderVersionList result;
    result.loader_name = "Quilt";

    if (body.empty()) {
        result.error = "请求 Quilt 版本列表时服务器无响应";
        return result;
    }

    json j;
    try {
        j = json::parse(body);
    } catch (const json::parse_error& e) {
        result.error = std::string("解析 Quilt 数据失败: ") + e.what();
        return result;
    }

    if (!j.is_object()) {
        result.error = "Quilt 返回数据格式异常";
        return result;
    }

    auto ldr_it = j.find("loader");
    if (ldr_it == j.end()) {
        result.error = "Quilt 未收录该版本";
        return result;
    }

    for (auto& item : *ldr_it) {
        LoaderVersion lv;
        lv.version = jstr(item, "version");

        /* Build Maven download URL */
        std::string maven = jstr(item, "maven");
        if (!maven.empty()) {
            std::string path = maven;
            auto c1 = path.find(':');
            if (c1 != std::string::npos) path[c1] = '/';
            auto c2 = path.find(':', c1 + 1);
            if (c2 != std::string::npos) path[c2] = '/';
            lv.url = std::string("https://maven.quiltmc.org/repository/release/")
                   + path + "/" + path.substr(path.rfind('/') + 1)
                   + "-" + lv.version + ".jar";
        }

        auto stable_it = item.find("stable");
        if (stable_it != item.end() && stable_it->is_boolean()) {
            lv.branch = stable_it->get<bool>() ? "stable" : "beta";
        }

        lv.date = "";
        result.versions.push_back(std::move(lv));
    }

    std::sort(result.versions.begin(), result.versions.end(),
              [](const LoaderVersion& a, const LoaderVersion& b) {
                  bool a_stable = (a.branch == "stable");
                  bool b_stable = (b.branch == "stable");
                  if (a_stable != b_stable) return a_stable;
                  return a.version > b.version;
              });

    result.loaded = true;
    return result;
}

/* ════════════════════════════════════════════════════════════════════
 *  Parser: LiteLoader
 *
 *  API: https://dl.liteloader.com/versions/versions.json
 *  Structure: versions.{mcver}.snapshots."com.mumfrey:liteloader"
 *    ─ each child (including "latest") has {version, file, md5,
 *      timestamp, tweakClass, libraries, ...}
 * ════════════════════════════════════════════════════════════════════ */
LoaderVersionList parse_liteloader(const std::string& body,
                                   const std::string& mc_version)
{
    LoaderVersionList result;
    result.loader_name = "LiteLoader";

    if (body.empty()) {
        result.error = "请求 LiteLoader 版本列表时服务器无响应";
        return result;
    }

    json j;
    try {
        j = json::parse(body);
    } catch (const json::parse_error& e) {
        result.error = std::string("解析 LiteLoader 数据失败: ") + e.what();
        return result;
    }

    if (!j.is_object()) {
        result.error = "LiteLoader 返回数据格式异常";
        return result;
    }

    auto versions_it = j.find("versions");
    if (versions_it == j.end()) {
        result.error = "LiteLoader 未收录任何版本";
        return result;
    }

    /* Try exact MC version first, then major.minor, then major */
    std::string matched_ver;
    if (versions_it->contains(mc_version)) {
        matched_ver = mc_version;
    } else {
        std::string probe = mc_version;
        while (!probe.empty() && !versions_it->contains(probe)) {
            auto dot = probe.rfind('.');
            if (dot == std::string::npos) { probe.clear(); break; }
            probe = probe.substr(0, dot);
        }
        if (!probe.empty()) matched_ver = mc_version;  // use original as display
    }
    if (matched_ver.empty()) {
        result.error = std::string("LiteLoader 不支持 Minecraft ") + mc_version;
        return result;
    }

    /* Find the actual version object — try exact first, then shortest match */
    json mc_obj;
    std::string probe = mc_version;
    while (!probe.empty()) {
        auto it = versions_it->find(probe);
        if (it != versions_it->end()) {
            mc_obj = *it;
            break;
        }
        auto dot = probe.rfind('.');
        if (dot == std::string::npos) break;
        probe = probe.substr(0, dot);
    }
    if (mc_obj.is_null()) {
        result.error = std::string("LiteLoader 不支持 Minecraft ") + mc_version;
        return result;
    }

    /* Navigate: mc_obj → snapshots → "com.mumfrey:liteloader" */
    json loader_obj;
    auto snaps_it = mc_obj.find("snapshots");
    if (snaps_it != mc_obj.end()) {
        auto liteloader_it = snaps_it->find("com.mumfrey:liteloader");
        if (liteloader_it != snaps_it->end())
            loader_obj = *liteloader_it;
    }
    if (loader_obj.is_null()) {
        result.error = std::string("LiteLoader 未收录 Minecraft ") + mc_version + " 的快照版本";
        return result;
    }

    /* Iterate members — each key is either "latest" or a build hash */
    for (auto& [key, entry] : loader_obj.items()) {
        if (!entry.is_object()) continue;

        LoaderVersion lv;
        lv.version = jstr(entry, "version");
        if (lv.version.empty()) continue;  // skip entries without version

        auto ts_it = entry.find("timestamp");
        if (ts_it != entry.end() && ts_it->is_number()) {
            lv.date = date_from_timestamp(ts_it->get<double>());
        }

        auto file_it = entry.find("file");
        if (file_it != entry.end() && file_it->is_string())
            lv.url = std::string("https://dl.liteloader.com/versions/")
                   + mc_version + "/" + file_it->get<std::string>();
        else if (entry.contains("md5"))
            lv.url = std::string("https://dl.liteloader.com/versions/")
                   + mc_version + "/liteloader-installer-"
                   + mc_version + "-00-SNAPSHOT.jar";

        /* Mark the "latest" entry */
        if (key == "latest")
            lv.branch = "latest";

        result.versions.push_back(std::move(lv));
    }

    /* Sort: "latest" first, then by timestamp/version descending */
    std::sort(result.versions.begin(), result.versions.end(),
              [](const LoaderVersion& a, const LoaderVersion& b) {
                  if (a.branch == "latest") return true;
                  if (b.branch == "latest") return false;
                  return a.version > b.version;
              });

    result.loaded = true;
    return result;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Parser: Cleanroom
 *
 *  API: https://api.github.com/repos/CleanroomMC/Cleanroom/releases
 *  Returns GitHub API JSON array: [{tag_name, published_at, assets[...]}]
 * ═══════════════════════════════════════════════════════════════════════ */
LoaderVersionList parse_cleanroom(const std::string& body)
{
    LoaderVersionList result;
    result.loader_name = "Cleanroom";

    if (body.empty()) {
        result.error = "请求 Cleanroom 版本列表时服务器无响应";
        return result;
    }

    json j;
    try {
        j = json::parse(body);
    } catch (const json::parse_error& e) {
        result.error = std::string("解析 Cleanroom 数据失败: ") + e.what();
        return result;
    }

    if (!j.is_array()) {
        result.error = "Cleanroom 返回数据格式异常";
        return result;
    }

    for (auto& release : j) {
        LoaderVersion lv;
        lv.version = jstr(release, "tag_name");

        /* published_at → YYYY-MM-DD */
        lv.date = extract_date(jstr(release, "published_at"));

        /* First asset with .jar */
        auto assets_it = release.find("assets");
        if (assets_it != release.end() && assets_it->is_array()) {
            for (auto& asset : *assets_it) {
                std::string durl = jstr(asset, "browser_download_url");
                if (!durl.empty()) {
                    lv.url = durl;
                    break;
                }
            }
        }

        result.versions.push_back(std::move(lv));
    }

    /* Sort by version (tag_name) descending */
    std::sort(result.versions.begin(), result.versions.end(),
              [](const LoaderVersion& a, const LoaderVersion& b) {
                  return a.version > b.version;
              });

    result.loaded = true;
    return result;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Parser: NeoForge
 *
 *  Fetches both modern (neoforge) and legacy (forge) Maven listings,
 *  filters by MC version, deduplicates.
 * ═══════════════════════════════════════════════════════════════════════ */
LoaderVersionList parse_neoforge(const std::string& body_modern,
                                 const std::string& body_legacy)
{
    LoaderVersionList result;
    result.loader_name = "NeoForge";

    auto parse_one = [&](const std::string& body,
                         const std::string& pkg) {
        if (body.empty()) return;
        json j;
        try {
            j = json::parse(body);
        } catch (const json::parse_error&) {
            return;
        }
        if (!j.is_object()) return;

        auto files_it = j.find("files");
        if (files_it == j.end() || !files_it->is_array()) return;

        for (auto& f : *files_it) {
            std::string name = jstr(f, "name");
            if (name.empty()) continue;

            /* Skip metadata files */
            if (name.find("maven-metadata") != std::string::npos) continue;
            if (name.find(".xml") != std::string::npos) continue;
            if (name.find(".sha") != std::string::npos) continue;

            LoaderVersion lv;
            lv.version = name;

            /* Build download URL */
            lv.url = std::string("https://bmclapi2.bangbang93.com/maven/net/neoforged/")
                   + pkg + "/" + name + "/"
                   + pkg + "-" + name + "-installer.jar";

            if (name.find("-beta") != std::string::npos)
                lv.branch = "beta";
            else
                lv.branch = "stable";

            result.versions.push_back(std::move(lv));
        }
    };

    parse_one(body_modern, "neoforge");
    parse_one(body_legacy,  "forge");

    /* Deduplicate by version string */
    std::sort(result.versions.begin(), result.versions.end(),
              [](const LoaderVersion& a, const LoaderVersion& b) {
                  return a.version > b.version;
              });
    result.versions.erase(
        std::unique(result.versions.begin(), result.versions.end(),
                    [](const LoaderVersion& a, const LoaderVersion& b) {
                        return a.version == b.version;
                    }),
        result.versions.end());

    if (result.versions.empty()) {
        result.error = "NeoForge 暂无可选版本";
    } else {
        result.loaded = true;
    }
    return result;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  线程数据 + 工作线程
 * ═══════════════════════════════════════════════════════════════════════ */

struct FetchData {
    LoaderCallback callback;
    std::string    loader_name;
    std::string    mc_version;
};

void fetch_thread(FetchData* fd)
{
    LoaderVersionList result;
    result.loader_name = fd->loader_name;

    LOG_INFO("LoaderFetcher: fetching %s versions for MC %s",
             fd->loader_name.c_str(), fd->mc_version.c_str());

    std::string body;
    const auto& name = fd->loader_name;
    const auto& mc   = fd->mc_version;

    if (name == "optifine") {
        body = pcl::util::http_get_sync(
            "https://bmclapi2.bangbang93.com/optifine/versionList");
        result = parse_optifine(body, mc);
    } else if (name == "forge") {
        std::string url = "https://bmclapi2.bangbang93.com/forge/minecraft/" + mc;
        body = pcl::util::http_get_sync(url.c_str());
        result = parse_forge(body);
    } else if (name == "fabric") {
        body = pcl::util::http_get_sync(
            "https://bmclapi2.bangbang93.com/fabric-meta/v2/versions");
        result = parse_fabric(body);
    } else if (name == "quilt") {
        body = pcl::util::http_get_sync(
            "https://meta.quiltmc.org/v3/versions");
        result = parse_quilt(body);
    } else if (name == "liteloader") {
        /* BMCLAPI redirects to mirrors that don't always work —
         * use the official LiteLoader API directly. */
        body = pcl::util::http_get_sync(
            "https://dl.liteloader.com/versions/versions.json");
        result = parse_liteloader(body, mc);
    } else if (name == "neoforge") {
        std::string body_mod = pcl::util::http_get_sync(
            "https://bmclapi2.bangbang93.com/neoforge/meta/api/maven/details/releases/net/neoforged/neoforge");
        std::string body_leg = pcl::util::http_get_sync(
            "https://bmclapi2.bangbang93.com/neoforge/meta/api/maven/details/releases/net/neoforged/forge");
        result = parse_neoforge(body_mod, body_leg);
    } else if (name == "cleanroom") {
        body = pcl::util::http_get_sync(
            "https://api.github.com/repos/CleanroomMC/Cleanroom/releases");
        result = parse_cleanroom(body);
    } else {
        result.loaded = false;
        result.error = std::string("未知的加载器: ") + name;
    }

    /* 失败时设置 error */
    if (result.error.empty() && !result.loaded && result.versions.empty())
        result.error = "暂无可选版本";

    LOG_INFO("LoaderFetcher: %s → %zu versions (%s)",
             fd->loader_name.c_str(), result.versions.size(),
             result.loaded ? "ok" : result.error.c_str());

    /* Dispatch result via the configured dispatcher. */
    pclcore::network::get_dispatcher().dispatch(
        [cb = std::move(fd->callback), result = std::move(result)]() mutable {
            cb(std::move(result));
        });

    delete fd;
}

}  // anonymous namespace

/* ═══════════════════════════════════════════════════════════════════════
 *  Public API
 * ═══════════════════════════════════════════════════════════════════════ */

void fetch_loader_versions(const std::string& loader_name,
                           const std::string& mc_version,
                           LoaderCallback     callback)
{
    auto* fd = new FetchData{std::move(callback), loader_name, mc_version};
    try {
        std::thread(fetch_thread, fd).detach();
    } catch (const std::system_error& e) {
        LoaderVersionList err;
        err.loader_name = loader_name;
        err.loaded = false;
        err.error = "无法创建后台线程";
        fd->callback(std::move(err));
        delete fd;
    }
}

}  // namespace pcl::mc
