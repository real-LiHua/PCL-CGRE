#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace pclcore::local {

// ============================================================================
// Account — Minecraft 账号信息
// ============================================================================

struct Account {
    std::string username;       // "Steve"
    std::string account_type;   // "Microsoft", "AuthLib"
    std::string uuid;           // 玩家 UUID（当前为空）
    std::string avatar_url;     // 头像 URL（当前为空）
    int64_t     last_login = 0; // unix timestamp（当前为 0）
    bool        logged_in  = false;  // 当前是否已登录

    // 后续扩展字段:
    // std::string access_token;
    // std::string refresh_token;
    // int64_t     token_expiry = 0;
};

using AccountList = std::vector<Account>;

// ============================================================================
// AccountProvider
// ============================================================================

class AccountProvider {
public:
    virtual ~AccountProvider() = default;
    virtual AccountList get_accounts() = 0;
};

AccountProvider& get_account_provider();
void set_account_provider(AccountProvider& p);

}  // namespace pclcore::local
