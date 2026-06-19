#include "pclcore/local/Instance.hpp"
#include "pclcore/local/CrashReport.hpp"
#include "pclcore/local/HelpContent.hpp"
#include "pclcore/local/ToolRegistry.hpp"
#include "pclcore/local/Account.hpp"
#include "pclcore/local/LaunchContent.hpp"

namespace pclcore::local {

// ============================================================================
// HardcodedInstanceProvider — 临时硬编码，后续替换为磁盘扫描
// ============================================================================

class HardcodedInstanceProvider final : public InstanceProvider {
public:
    InstanceList get_instances() override {
        return {
            {"1.21.1",         "~/.minecraft/versions/1.21.1",
             "1.21.1", "Fabric",  0},
            {"1.21.5",         "~/.minecraft/versions/1.21.5",
             "1.21.5", "Forge",   0},
            {"BetterMinecraft", "~/.local/share/.minecraft",
             "1.21.1", "Fabric",  0},
        };
    }
};

// ============================================================================
// HardcodedCrashReportProvider — 临时硬编码 + mock 日志内容
// ============================================================================

class HardcodedCrashReportProvider final : public CrashReportProvider {
public:
    CrashReportList get_crash_reports() override {
        return {
            /* ── Minecraft 崩溃 × 3 ── */
            {
                /* instance  */ "1.21.1", "~/.minecraft/versions/1.21.1",
                /* source    */ CrashSource::Minecraft,
                /* title     */ "NullPointerException: Ticking block entity",
                /* suspect   */ "CrashMeIfYouCan.jar",
                /* severity  */ "致命",
                /* diagnosis */ "",
                /* advice    */ "",
                /* logs      */ {
                    {"latest.log",  "游戏日志 (Minecraft 运行输出)"},
                    {"Crash-20260614-153042-client.txt", "崩溃报告 (Minecraft Crash Report)"},
                    {"launcher.log","启动器日志 (PCL-CGRE 输出)"},
                },
            },
            {
                /* instance  */ "1.21.5", "~/.minecraft/versions/1.21.5",
                /* source    */ CrashSource::Minecraft,
                /* title     */ "OutOfMemoryError: Java heap space",
                /* suspect   */ "",
                /* severity  */ "致命",
                /* diagnosis */ "该实例内存不足",
                /* advice    */ "请清理你的系统 RAM 空间，关闭一些不必要的软件；"
                               "或考虑升级你的电脑硬件。",
                /* logs      */ {
                    {"latest.log",  "游戏日志 (Minecraft 运行输出)"},
                    {"Crash-20260613-221518-server.txt", "崩溃报告 (Minecraft Crash Report)"},
                    {"launcher.log","启动器日志 (PCL-CGRE 输出)"},
                },
            },
            {
                /* instance  */ "BetterMinecraft", "~/.local/share/.minecraft",
                /* source    */ CrashSource::Minecraft,
                /* title     */ "Manually triggered debug crash",
                /* suspect   */ "",
                /* severity  */ "警告",
                /* diagnosis */ "玩家手动触发崩溃",
                /* advice    */ "TIPS:\n"
                               "烧烤，好吃\n"
                               "你真的没有什么重要的事要做吗？\n"
                               "核电，轻而易举啊\n"
                               "你今天很幸运，真的\n"
                               "这个错误本来可以避免的\n"
                               "你似乎很闲啊\n"
                               "为什么会变成这样呢\n"
                               "不要随意修改你不了解的文件\n"
                               "你终于发现我了！\n"
                               "你好，世界。",
                /* logs      */ {
                    {"latest.log",  "游戏日志 (Minecraft 运行输出)"},
                    {"Crash-20260612-084221-client.txt", "崩溃报告 (Minecraft Crash Report)"},
                    {"launcher.log","启动器日志 (PCL-CGRE 输出)"},
                },
            },

            /* ── CGRE 启动器插件崩溃 ── */
            {
                /* instance  */ "", "",
                /* source    */ CrashSource::Plugin,
                /* title     */ "插件 'DownloadPlus' 发生异常, 已被自动禁用",
                /* suspect   */ "~/.local/share/PCL-CGRE/plugins/DownloadPlus",
                /* severity  */ "警告",
                /* diagnosis */ "PCL-CGRE 未能定位到问题",
                /* advice    */ "你可以重新启用这个插件或重置插件状态，自行排查原因；"
                               "或复制详情以供开发者定位错误。",
                /* logs      */ {
                    {"launcher.log", "启动器日志 (PCL-CGRE 输出)"},
                    {"plugin-downloadplus.log", "插件日志 (DownloadPlus 输出)"},
                },
            },

            /* ── JDK 崩溃 ── */
            {
                /* instance  */ "", "",
                /* source    */ CrashSource::JDK,
                /* title     */ "SIGSEGV in JVM compiled code",
                /* suspect   */ "libjvm.so",
                /* severity  */ "致命",
                /* diagnosis */ "PCL-CGRE 未能定位到问题",
                /* advice    */ "请检查 Java 版本与启动器兼容性，尝试更新显卡驱动和 JDK；"
                               "或复制详情以供开发者定位错误。",
                /* logs      */ {
                    {"hs_err_pid31451.log", "JVM 致命错误日志 (HotSpot Error)"},
                    {"launcher.log",        "启动器日志 (PCL-CGRE 输出)"},
                },
            },

            /* ── PCL-CGRE 本体崩溃 ── */
            {
                /* instance  */ "", "",
                /* source    */ CrashSource::Launcher,
                /* title     */ "Config_ReadOnly",
                /* suspect   */ "",
                /* severity  */ "致命",
                /* diagnosis */ "进入安全模式",
                /* advice    */ "PCL-CGRE 已尝试使用安全模式启动；"
                               "该模式下，所有插件都处于禁用状态。"
                               "排查你的插件列表，或检查是否有更新。"
                               "如需进一步帮助，请复制详情，并进入 GitHub 提出 Issue。",
                /* logs      */ {
                    {"launcher.log", "启动器日志 (PCL-CGRE 输出)"},
                },
            },
        };
    }

    std::vector<std::string> get_log_contents(int crash_index) override {
        // Mock 日志内容 — 后续改为读取实际文件
        static const std::vector<std::vector<std::string>> mock_logs = {
            {
                // crash 0: 1.21.1
                "[12:34:56] [main/INFO]: Starting Minecraft 1.21.1...\n"
                "[12:34:57] [main/INFO]: Loading mods...\n"
                "[12:34:58] [main/WARN]: Mod 'CrashMeIfYouCan' may be unstable\n"
                "[12:35:02] [main/ERROR]: java.lang.NullPointerException\n"
                "[12:35:02] [main/INFO]: Game crashed! See crash report for details.",

                "---- Minecraft Crash Report ----\n"
                "// Sorry for the inconvenience.\n\n"
                "Time: 2026-06-14 15:30:42\n"
                "Description: Unexpected error\n\n"
                "java.lang.NullPointerException: Ticking block entity\n"
                "\tat com.example.badmod.BadBlockEntity.tick(BadBlockEntity.java:42)\n"
                "\tat net.minecraft.server.MinecraftServer.lambda$tick$3(MinecraftServer.java:156)\n"
                "\tat java.util.ArrayList.forEach(ArrayList.java:1512)\n\n"
                "-- System Details --\n"
                "Minecraft Version: 1.21.1\n"
                "Java Version: 21.0.4\n"
                "Memory: 2048 MB / 4096 MB",

                "[12:34:55] [PCL-CGRE/INFO]: Launching Minecraft 1.21.1\n"
                "[12:34:56] [PCL-CGRE/INFO]: Java: /usr/lib/jvm/java-21-openjdk/bin/java\n"
                "[12:35:02] [PCL-CGRE/WARN]: Process exited with code -1\n"
                "[12:35:02] [PCL-CGRE/INFO]: Crash detected! Opening CrashSpy...\n"
                "[12:35:02] [PCL-CGRE/INFO]: Crash report saved to crash-reports/",
            },
            {
                // crash 1: 1.21.5
                "[08:15:32] [main/INFO]: Starting Minecraft 1.21.5...\n"
                "[08:15:33] [main/INFO]: Loading world 'Server Survival'\n"
                "[08:15:35] [main/ERROR]: java.lang.OutOfMemoryError: Java heap space\n"
                "[08:15:35] [main/INFO]: Game crashed! See crash report for details.",

                "---- Minecraft Crash Report ----\n"
                "// There are four lights!\n\n"
                "Time: 2026-06-13 22:15:18\n"
                "Description: Out of memory\n\n"
                "java.lang.OutOfMemoryError: Java heap space\n"
                "\tat net.minecraft.world.level.chunk.ChunkRegionLoader.loadEntities(ChunkRegionLoader.java:95)\n"
                "\tat net.minecraft.server.MinecraftServer.tick(MinecraftServer.java:120)\n\n"
                "-- System Details --\n"
                "Minecraft Version: 1.21.5\n"
                "Java Version: 17.0.12\n"
                "Memory: 4096 MB / 4096 MB (100%)",

                "[08:15:31] [PCL-CGRE/INFO]: Launching Minecraft 1.21.5\n"
                "[08:15:32] [PCL-CGRE/INFO]: Java: /usr/lib/jvm/java-17-openjdk/bin/java\n"
                "[08:15:32] [PCL-CGRE/WARN]: JVM max heap: 4096M\n"
                "[08:15:35] [PCL-CGRE/WARN]: Process exited with code -1\n"
                "[08:15:35] [PCL-CGRE/INFO]: Crash detected! Opening CrashSpy...",
            },
            {
                // crash 2: BetterMinecraft
                "[22:45:18] [main/INFO]: Starting Minecraft with BetterMinecraft...\n"
                "[22:45:19] [main/INFO]: Loading 127 mods...\n"
                "[22:45:22] [main/ERROR]: java.lang.NoClassDefFoundError: com/example/ModCompat\n"
                "[22:45:22] [main/INFO]: Game crashed! See crash report for details.",

                "---- Minecraft Crash Report ----\n"
                "// My bad.\n\n"
                "Time: 2026-06-12 08:42:21\n"
                "Description: Mod compatibility error\n\n"
                "java.lang.NoClassDefFoundError: com/example/ModCompat\n"
                "\tat com.example.bigmod.BigMod.init(BigMod.java:17)\n\n"
                "-- System Details --\n"
                "Minecraft Version: 1.21.1\n"
                "Modpack: BetterMinecraft v2.5.0\n"
                "Java Version: 21.0.4",

                "[22:45:17] [PCL-CGRE/INFO]: Launching BetterMinecraft (127 mods)\n"
                "[22:45:18] [PCL-CGRE/INFO]: Java: /usr/lib/jvm/java-21-openjdk/bin/java\n"
                "[22:45:22] [PCL-CGRE/WARN]: Process exited with code -1\n"
                "[22:45:22] [PCL-CGRE/INFO]: Crash detected! Opening CrashSpy...",
            },
            /* ── crash 3: CGRE Plugin 'DownloadPlus' ── */
            {
                "[16:42:11] [PCL-CGRE/INFO]: Launcher v0.2.0 starting...\n"
                "[16:42:11] [PCL-CGRE/INFO]: Discovering plugins in ~/.local/share/PCL-CGRE/plugins...\n"
                "[16:42:12] [PCL-CGRE/INFO]: Loading plugin: DownloadPlus v2.1.0\n"
                "[16:42:12] [PCL-CGRE/INFO]: Plugin 'DownloadPlus' loaded (type: download-hook)\n"
                "[16:42:15] [PCL-CGRE/WARN]: Plugin 'DownloadPlus' callback on_download_start threw exception:\n"
                "[16:42:15] [PCL-CGRE/WARN]:   std::runtime_error: Failed to resolve mirror host\n"
                "[16:42:15] [PCL-CGRE/ERROR]: Plugin 'DownloadPlus' has been automatically disabled.\n"
                "[16:42:15] [PCL-CGRE/INFO]: 请在 设置 → 插件管理 中重新启用此插件。\n"
                "[16:42:16] [PCL-CGRE/INFO]: Continuing with remaining plugins...",

                "[DownloadPlus v2.1.0] 16:42:12 : Initializing download hooks...\n"
                "[DownloadPlus v2.1.0] 16:42:12 : Registered handler: on_download_start\n"
                "[DownloadPlus v2.1.0] 16:42:12 : Registered handler: on_progress\n"
                "[DownloadPlus v2.1.0] 16:42:12 : Registered handler: on_download_complete\n"
                "[DownloadPlus v2.1.0] 16:42:12 : Mirror list loaded (3 sources)\n"
                "[DownloadPlus v2.1.0] 16:42:15 : on_download_start fired\n"
                "[DownloadPlus v2.1.0] 16:42:15 : Resolving fastest mirror...\n"
                "[DownloadPlus v2.1.0] 16:42:15 : ERROR: All mirrors unreachable (timeout)\n"
                "[DownloadPlus v2.1.0] 16:42:15 : ── Exception ──\n"
                "[DownloadPlus v2.1.0] 16:42:15 : std::runtime_error at mirror_resolver.cpp:142\n"
                "[DownloadPlus v2.1.0] 16:42:15 :   Failed to resolve mirror host\n"
                "[DownloadPlus v2.1.0] 16:42:15 :   → Plugin disabled by launcher\n"
                "[DownloadPlus v2.1.0] 16:42:15 : ── End of Report ──",
            },
            /* ── crash 4: JDK SIGSEGV ── */
            {
                "#\n"
                "# A fatal error has been detected by the Java Runtime Environment:\n"
                "#\n"
                "#  SIGSEGV (0xb) at pc=0x00007f8b3c2a1f80, pid=31451, tid=31458\n"
                "#\n"
                "# JRE version: OpenJDK Runtime Environment (21.0.4+7) (build 21.0.4+7-LTS)\n"
                "# Java VM: OpenJDK 64-Bit Server VM (21.0.4+7-LTS, mixed mode, sharing,\n"
                "#           tiered, compressed oops, compressed class ptrs, g1 gc, linux-amd64)\n"
                "# Problematic frame:\n"
                "# V  [libjvm.so+0x8a1f80]  AccessInternal::PostRuntimeDispatch<...>\n"
                "#\n"
                "# No core dump will be written. Core dumps have been disabled.\n"
                "#\n"
                "---------------  S U M M A R Y ------------\n"
                "Command Line: -Xmx4096M -Xms1024M -jar pcl-cgre.jar\n"
                "Host: Intel(R) Core(TM) i7-12700H, 16 cores, 31.1G, Arch Linux\n"
                "Time: Sun Jun 15 14:22:07 2026 CST elapsed time: 1347.42 seconds\n"
                "---------------  T H R E A D  ---------------\n"
                "Current thread (0x00007f8b1c025800):  JavaThread \"C2 CompilerThread0\"\n"
                "daemon [_thread_in_native, id=31458,\n"
                "stack(0x00007f8b0c000000,0x00007f8b0c100000)]\n"
                "siginfo: si_signo: 11 (SIGSEGV), si_code: 1 (SEGV_MAPERR),\n"
                "         si_addr: 0x0000000000000008\n"
                "Register to memory mapping:\n"
                "RAX=0x0000000000000000 is an unknown value\n"
                "RBX=0x00007f8b1c026800 points into unknown readable memory: 0x00007f8b34001200\n"
                "Stack: [0x00007f8b0c000000,0x00007f8b0c100000], sp=0x00007f8b0c0ff3d0\n"
                "Instructions: (pc=0x00007f8b3c2a1f80)\n"
                "  0x00007f8b3c2a1f60:   48 8b 43 08 48 8b 13 48 8b 48 10 48 8b 43 18\n"
                "  0x00007f8b3c2a1f70:   ff e0 66 66 2e 0f 1f 84 00 00 00 00 00 90 48\n"
                "  0x00007f8b3c2a1f80:   8b 40 08 c3 66 66 2e 0f 1f 84 00 00 00 00 00",

                "[14:21:55] [PCL-CGRE/INFO]: Launcher v0.2.0 starting (Java 21.0.4)\n"
                "[14:21:55] [PCL-CGRE/INFO]: JVM: -Xmx4096M -Xms1024M\n"
                "[14:21:56] [PCL-CGRE/INFO]: Loading 3 plugins...\n"
                "[14:21:57] [PCL-CGRE/INFO]: All plugins loaded, starting GUI...\n"
                "[14:22:07] [PCL-CGRE/ERROR]: JVM crashed with SIGSEGV!\n"
                "[14:22:07] [PCL-CGRE/ERROR]: See hs_err_pid31451.log for details.\n"
                "[14:22:07] [PCL-CGRE/WARN]: The launcher will now exit.\n"
                "[14:22:07] [PCL-CGRE/INFO]: Crash log written to logs/hs_err_pid31451.log",
            },
            /* ── crash 5: PCL-CGRE 本体 Config_ReadOnly ── */
            {
                "[18:33:02] [PCL-CGRE/INFO]: PCL-CGRE v0.2.0 starting...\n"
                "[18:33:02] [PCL-CGRE/INFO]: Loading config from ~/.config/PCL-CGRE/config.json\n"
                "[18:33:02] [PCL-CGRE/ERROR]: Failed to write config: Read-only file system\n"
                "[18:33:02] [PCL-CGRE/ERROR]: Config_ReadOnly — cannot persist settings\n"
                "[18:33:02] [PCL-CGRE/WARN]: Entering safe mode — all plugins disabled\n"
                "[18:33:03] [PCL-CGRE/INFO]: Safe mode active. 0 plugins loaded.\n"
                "[18:33:03] [PCL-CGRE/INFO]: Launcher ready (safe mode, limited functionality)\n"
                "[18:33:03] [PCL-CGRE/WARN]: Please check file permissions for config directory\n"
                "[18:33:03] [PCL-CGRE/WARN]: Or verify no other PCL-CGRE instance is running\n"
                "[18:33:03] [PCL-CGRE/INFO]: Crash report saved to crash-reports/",
            },
        };

        if (crash_index >= 0 && crash_index < (int)mock_logs.size())
            return mock_logs[crash_index];
        return {};
    }
};

// ============================================================================
// HardcodedHelpContentProvider
// ============================================================================

class HardcodedHelpContentProvider final : public HelpContentProvider {
public:
    std::vector<HelpEntry> get_help_entries() override {
        return {
            {"用户手册", "查看 PCL 的完整使用文档和常见问题解答。",
             "查看", "https://github.com/PCL-Community/PCL-CGRE/wiki"},
            {"报告问题", "遇到 Bug 或崩溃？请前往 GitHub Issues 提交反馈。",
             "前往 GitHub", "https://github.com/PCL-Community/PCL-CGRE/issues"},
            {"加入社区", "与其它用户交流，获取最新消息和帮助。",
             "加入", "https://github.com/PCL-Community"},
        };
    }

    std::vector<FaqEntry> get_faq_entries() override {
        return {
            {"Q: 启动器无法下载 Minecraft？",
             "A: 请检查网络连接，尝试切换下载源为 BMCLAPI 镜像。"},
            {"Q: 游戏启动后闪退？",
             "A: 请检查 Java 版本是否匹配，尝试更新显卡驱动。可前往"
             "CrashSpy 查看崩溃报告以获取更多信息。"},
            {"Q: Mod 加载失败？",
             "A: 确认 Mod 版本与游戏版本、加载器版本兼容，检查是否缺少前置 Mod。"},
        };
    }
};

// ============================================================================
// HardcodedToolProvider
// ============================================================================

class HardcodedToolProvider final : public ToolProvider {
public:
    std::vector<ToolEntry> get_tools() override {
        return {
            {"清理垃圾",   "",    "cleanup",        ""},
            {"今日玄学",   "",    "fortune",        ""},
            {"崩溃测试",   "",    "crash-test",     "destructive-action"},
            {"创建快捷方式", "",   "create-shortcut", ""},
            {"启动统计",   "",    "launch-stats",    ""},
        };
    }

    std::vector<ToolEntry> get_extra_buttons() override {
        return {
            {"检查更新",    "refresh-cw",       "check-update",    ""},
            {"回到顶部",    "arrow-up-to-line",  "scroll-top",      ""},
            {"任务管理",    "download",          "task-mgr",        ""},
            {"放弃当前任务", "flag",              "cancel-task",     ""},
            {"关机",        "power",             "shutdown",        ""},
            {"日志",        "scroll-text",       "show-log",        ""},
            {"音乐",        "music",             "music",           ""},
        };
    }
};

// ============================================================================
// 全局提供者实例（默认 = 硬编码）
// ============================================================================

static HardcodedInstanceProvider     s_default_instance_provider;
static HardcodedCrashReportProvider  s_default_crash_provider;
static HardcodedHelpContentProvider  s_default_help_provider;
static HardcodedToolProvider         s_default_tool_provider;

static InstanceProvider*     s_instance_provider = &s_default_instance_provider;
static CrashReportProvider*  s_crash_provider    = &s_default_crash_provider;
static HelpContentProvider*  s_help_provider     = &s_default_help_provider;
static ToolProvider*         s_tool_provider     = &s_default_tool_provider;

InstanceProvider& get_instance_provider()      { return *s_instance_provider; }
void set_instance_provider(InstanceProvider& p) { s_instance_provider = &p; }

CrashReportProvider& get_crash_provider()      { return *s_crash_provider; }
void set_crash_provider(CrashReportProvider& p) { s_crash_provider = &p; }

HelpContentProvider& get_help_content_provider()     { return *s_help_provider; }
void set_help_content_provider(HelpContentProvider& p) { s_help_provider = &p; }

ToolProvider& get_tool_provider()      { return *s_tool_provider; }
void set_tool_provider(ToolProvider& p) { s_tool_provider = &p; }

// ============================================================================
// HardcodedAccountProvider
// ============================================================================

class HardcodedAccountProvider final : public AccountProvider {
public:
    AccountList get_accounts() override {
        return {
            {"Steve",   "Microsoft", "", "", 0, true},
            {"Alex",    "离线登录",   "", "", 0, false},
            {"M0nst3r", "第三方登录", "", "", 0, false},
        };
    }
};

static HardcodedAccountProvider s_default_account_provider;
static AccountProvider* s_account_provider = &s_default_account_provider;

AccountProvider& get_account_provider()      { return *s_account_provider; }
void set_account_provider(AccountProvider& p) { s_account_provider = &p; }

// ============================================================================
// HardcodedLaunchContentProvider
// ============================================================================

class HardcodedLaunchContentProvider final : public LaunchContentProvider {
public:
    LaunchContent get_content() override {
        return {
            {
                {"PCL-CGRE 开发预览版发布",
                 "欢迎体验全新的 GTK4 原生启动器！", ""},
                {"Minecraft 1.22 即将到来",
                 "Mojang 宣布下一个大版本更新计划。", ""},
                {"社区创作大赛",
                 "参与社区创作，赢取丰厚奖励！", ""},
            },
        };
    }
};

static HardcodedLaunchContentProvider s_default_launch_content_provider;
static LaunchContentProvider* s_launch_content_provider = &s_default_launch_content_provider;

LaunchContentProvider& get_launch_content_provider()      { return *s_launch_content_provider; }
void set_launch_content_provider(LaunchContentProvider& p) { s_launch_content_provider = &p; }

}  // namespace pclcore::local
