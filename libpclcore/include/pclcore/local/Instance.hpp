#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace pclcore::local {

// ============================================================================
// Instance — 本地 Minecraft 实例
// ============================================================================

struct Instance {
    std::string name;          // "1.21.1", "BetterMinecraft"
    std::string path;          // "~/.minecraft/versions/1.21.1"
    std::string version_id;    // 从 version.json 解析的实际版本 ID（当前为空）
    std::string loader;        // "Vanilla", "Forge", "Fabric"...（当前为空）
    int64_t     last_played = 0;  // unix timestamp（当前为 0）
};

using InstanceList = std::vector<Instance>;

// ============================================================================
// InstanceProvider — 可插拔实例提供者
// ============================================================================

class InstanceProvider {
public:
    virtual ~InstanceProvider() = default;
    virtual InstanceList get_instances() = 0;
};

/** 当前返回硬编码数据，后续改为扫描 ~/.minecraft/versions/ */
InstanceProvider& get_instance_provider();
void set_instance_provider(InstanceProvider& p);

}  // namespace pclcore::local
