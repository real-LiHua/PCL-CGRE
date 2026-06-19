#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace pclcore::local {

// ============================================================================
// DmTask — 下载任务数据模型
// ============================================================================

struct DmTask {
    std::string name;          // 文件名
    std::string save_path;     // 保存路径
    int64_t     total      = 0;  // 总字节数
    int64_t     done       = 0;  // 已完成字节数
    int         status     = 0;  // 0=排队, 1=下载中, 2=暂停, 3=完成, 4=错误
    int         threads    = 0;  // 当前活动线程数
    int         max_threads = 0; // 最大线程数
    bool        checked    = false; // 复选框状态
    int64_t     speed      = 0;  // bytes/s, 仅 status==1
    int64_t     eta        = 0;  // 预计剩余秒数, 仅 status==1
};

// ============================================================================
// DownloadTaskProvider
// ============================================================================

class DownloadTaskProvider {
public:
    virtual ~DownloadTaskProvider() = default;

    /** 获取当前所有下载任务列表 */
    virtual std::vector<DmTask> get_tasks() = 0;

    /** 添加一个下载任务 */
    virtual void add_task(const DmTask& task) = 0;

    /** 移除指定位置的任务 */
    virtual void remove_task(size_t index) = 0;

    /** 清除所有已完成 (status==3) 的任务 */
    virtual void clear_completed() = 0;

    /** 持久化任务列表到磁盘（当前空实现，后续实现 JSON 序列化） */
    virtual void save() = 0;

    /** 从磁盘加载任务列表（当前空实现，后续实现 JSON 反序列化） */
    virtual void load() = 0;
};

DownloadTaskProvider& get_download_task_provider();
void set_download_task_provider(DownloadTaskProvider& p);

/** 格式化字节数为人类可读字符串 */
std::string format_file_size(int64_t bytes);

}  // namespace pclcore::local
