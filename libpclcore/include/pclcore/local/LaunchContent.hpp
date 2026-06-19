#pragma once

#include <string>
#include <vector>

namespace pclcore::local {

// ============================================================================
// NewsEntry — 启动页资讯卡片
// ============================================================================

struct NewsEntry {
    std::string title;        // "PCL-CGRE 开发预览版发布"
    std::string description;  // "欢迎体验全新的 GTK4 原生启动器！"
    std::string action_url;   // 可选: 点击"了解更多"的目标 URL (空字符串 = 无操作)
};

// ============================================================================
// LaunchContent — 启动页右侧内容
// ============================================================================

struct LaunchContent {
    std::vector<NewsEntry> news;
};

// ============================================================================
// LaunchContentProvider
// ============================================================================

class LaunchContentProvider {
public:
    virtual ~LaunchContentProvider() = default;
    virtual LaunchContent get_content() = 0;
};

LaunchContentProvider& get_launch_content_provider();
void set_launch_content_provider(LaunchContentProvider& p);

}  // namespace pclcore::local
