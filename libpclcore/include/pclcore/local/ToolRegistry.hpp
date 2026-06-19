#pragma once

#include <string>
#include <vector>

namespace pclcore::local {

// ============================================================================
// ToolEntry — 工具箱 / 启动页按钮
// ============================================================================

struct ToolEntry {
    std::string label;       // "清理垃圾"
    std::string icon;        // 图标名，可能为空
    std::string action;      // action 标识符
    std::string css_class;   // CSS 样式类，可能为空（如 "destructive-action"）
};

// ============================================================================
// ToolProvider
// ============================================================================

class ToolProvider {
public:
    virtual ~ToolProvider() = default;

    /** 工具箱页面中的工具按钮列表 */
    virtual std::vector<ToolEntry> get_tools() = 0;

    /** 启动页右侧浮动按钮（ExtraButtons） */
    virtual std::vector<ToolEntry> get_extra_buttons() = 0;
};

ToolProvider& get_tool_provider();
void set_tool_provider(ToolProvider& p);

}  // namespace pclcore::local
