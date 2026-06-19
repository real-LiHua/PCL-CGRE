#pragma once

#include <string>
#include <vector>

namespace pclcore::local {

// ============================================================================
// HelpEntry / FaqEntry — 帮助与 FAQ 内容
// ============================================================================

struct HelpEntry {
    std::string title;         // "用户手册"
    std::string description;   // "查看 PCL 的完整使用文档..."
    std::string action_label;  // "查看"
    std::string url;           // "https://github.com/..."
};

struct FaqEntry {
    std::string question;      // "Q: 启动器无法下载 Minecraft？"
    std::string answer;        // "A: 请检查网络连接..."
};

// ============================================================================
// HelpContentProvider
// ============================================================================

class HelpContentProvider {
public:
    virtual ~HelpContentProvider() = default;
    virtual std::vector<HelpEntry> get_help_entries() = 0;
    virtual std::vector<FaqEntry>  get_faq_entries()  = 0;
};

HelpContentProvider& get_help_content_provider();
void set_help_content_provider(HelpContentProvider& p);

}  // namespace pclcore::local
