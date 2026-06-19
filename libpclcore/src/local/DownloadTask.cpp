#include "pclcore/local/DownloadTask.hpp"
#include "pclcore/core/Log.hpp"
#include <algorithm>

namespace pclcore::local {

// ============================================================================
// format_file_size
// ============================================================================

std::string format_file_size(int64_t bytes) {
    if (bytes >= 1000000000LL) {
        char b[32];
        std::snprintf(b, sizeof(b), "%.1f GB", bytes / 1e9);
        return b;
    }
    if (bytes >= 1000000) {
        char b[32];
        std::snprintf(b, sizeof(b), "%.1f MB", bytes / 1e6);
        return b;
    }
    if (bytes >= 1000) {
        char b[32];
        std::snprintf(b, sizeof(b), "%.1f KB", bytes / 1e3);
        return b;
    }
    char b[32];
    std::snprintf(b, sizeof(b), "%ld B", (long)bytes);
    return b;
}

// ============================================================================
// HardcodedDownloadTaskProvider — 预填充示例任务
// ============================================================================

class HardcodedDownloadTaskProvider final : public DownloadTaskProvider {
public:
    HardcodedDownloadTaskProvider() {
        load();
    }

    std::vector<DmTask> get_tasks() override {
        return m_tasks;
    }

    void add_task(const DmTask& task) override {
        m_tasks.push_back(task);
    }

    void remove_task(size_t index) override {
        if (index < m_tasks.size())
            m_tasks.erase(m_tasks.begin() + (ptrdiff_t)index);
    }

    void clear_completed() override {
        m_tasks.erase(
            std::remove_if(m_tasks.begin(), m_tasks.end(),
                           [](const DmTask& t) { return t.status == 3; }),
            m_tasks.end());
    }

    void save() override {
        // TODO: JSON 序列化到磁盘
        LOG_CORE_DBG("DownloadTaskProvider::save() — not yet implemented");
    }

    void load() override {
        // TODO: 从磁盘 JSON 反序列化
        // 当前预填充示例任务
        m_tasks = {
            {"1.21.1.jar",
             "~/.minecraft/versions/1.21.1",
             25600000, 25600000, 3, 0, 0},
            {"[苹果皮] appleskin-fabric-mc1.21-3.0.6.jar",
             "~/.minecraft/versions/1.21.1-Fabric0.19.3/mods",
             1057000, 258000, 1, 8, 16, false,
             2000000, 0},
            {"Win11_25H2_Chinese_Simplified_x64_v2.iso",
             "~/Downloads",
             8000000000LL, 0, 0, 0, 0},
        };
    }

private:
    std::vector<DmTask> m_tasks;
};

// ============================================================================
// 全局提供者实例
// ============================================================================

static HardcodedDownloadTaskProvider s_default_download_task_provider;
static DownloadTaskProvider* s_download_task_provider = &s_default_download_task_provider;

DownloadTaskProvider& get_download_task_provider() {
    return *s_download_task_provider;
}
void set_download_task_provider(DownloadTaskProvider& p) {
    s_download_task_provider = &p;
}

}  // namespace pclcore::local
