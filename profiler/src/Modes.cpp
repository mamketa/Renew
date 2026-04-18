#include "Modes.hpp"
#include "FileIO.hpp"
#include "CpuFreq.hpp"
#include "DevFreq.hpp"
#include "MemoryManager.hpp"
#include <dirent.h>
#include <cstring>
#include <format>
#include <print>
#include <unistd.h>

namespace velfox {

Config readConfig() noexcept {
    Config cfg;
    auto p = [](std::string_view f) {
        return std::format("{}/{}", MODULE_CONFIG, f);
    };
    cfg.soc              = FileIO::readInt(p("soc_recognition"));
    cfg.liteMode         = FileIO::readInt(p("lite_mode"));
    cfg.ppmPolicy        = FileIO::readString(p("ppm_policies_mediatek"));
    cfg.deviceMitigation = FileIO::readInt(p("device_mitigation"));

    auto customGov = p("custom_default_cpu_gov");
    if (FileIO::exists(customGov))
        cfg.defaultCpuGov = FileIO::readString(customGov);
    else
        cfg.defaultCpuGov = FileIO::readString(p("default_cpu_gov"));
    if (cfg.defaultCpuGov.empty()) cfg.defaultCpuGov = "schedutil";
    return cfg;
}

void setDnd(int mode) noexcept {
    if (mode == 0) ::system("cmd notification set_dnd off");
    else           ::system("cmd notification set_dnd priority");
}

void setRpsXps(int mode) noexcept {
    int cores = (int)::sysconf(_SC_NPROCESSORS_ONLN);
    if (cores <= 0) return;
    int use = (mode == 2) ? (cores * 3) / 4
            : (mode == 3) ? cores / 2
            : cores;
    if (use < 1) use = 1;
    if (use > (int)(sizeof(unsigned long) * 8)) use = (int)(sizeof(unsigned long) * 8);
    unsigned long mask = 0;
    for (int i = 0; i < use; ++i) mask |= (1UL << i);
    auto hex = std::format("{:x}", mask);

    DIR* nd = ::opendir("/sys/class/net");
    if (!nd) return;
    struct dirent* iface;
    while ((iface = ::readdir(nd))) {
        if (!iface->d_name[0] || iface->d_name[0] == '.') continue;
        auto qdir_path = std::format("/sys/class/net/{}/queues", iface->d_name);
        DIR* qd = ::opendir(qdir_path.c_str());
        if (!qd) continue;
        struct dirent* q;
        while ((q = ::readdir(qd))) {
            if (!q->d_name[0] || q->d_name[0] == '.') continue;
            if (q->d_name[0]=='r' && q->d_name[1]=='x' && q->d_name[2]=='-')
                FileIO::apply(hex, std::format("{}/{}/rps_cpus", qdir_path, q->d_name));
            else if (q->d_name[0]=='t' && q->d_name[1]=='x' && q->d_name[2]=='-')
                FileIO::apply(hex, std::format("{}/{}/xps_cpus", qdir_path, q->d_name));
        }
        ::closedir(qd);
    }
    ::closedir(nd);
}

void perfCommon() noexcept {
    FileIO::apply("0", "/proc/sys/kernel/panic");
    FileIO::apply("0", "/proc/sys/kernel/panic_on_oops");
    FileIO::apply("0", "/proc/sys/kernel/panic_on_warn");
    FileIO::apply("0", "/proc/sys/kernel/softlockup_panic");
    ::sync();

    DIR* blk = ::opendir("/sys/block");
    if (blk) {
        struct dirent* e;
        while ((e = ::readdir(blk))) {
            if (e->d_name[0] == '.') continue;
            FileIO::apply("0", std::format("/sys/block/{}/queue/iostats", e->d_name));
            FileIO::apply("0", std::format("/sys/block/{}/queue/add_random", e->d_name));
        }
        ::closedir(blk);
    }

    const char* algs[] = {"bbr3","bbr2","bbrplus","bbr","westwood","cubic"};
    for (auto* alg : algs) {
        auto cmd = std::format("grep -q \"{}\" /proc/sys/net/ipv4/tcp_available_congestion_control", alg);
        if (::system(cmd.c_str()) == 0) {
            FileIO::apply(alg, "/proc/sys/net/ipv4/tcp_congestion_control");
            break;
        }
    }

    FileIO::apply("1", "/proc/sys/net/ipv4/tcp_sack");
    FileIO::apply("1", "/proc/sys/net/ipv4/tcp_ecn");
    FileIO::apply("1", "/proc/sys/net/ipv4/tcp_window_scaling");
    FileIO::apply("1", "/proc/sys/net/ipv4/tcp_moderate_rcvbuf");
    FileIO::apply("3", "/proc/sys/net/ipv4/tcp_fastopen");
    FileIO::apply("3", "/proc/sys/kernel/perf_cpu_time_max_percent");
    FileIO::apply("0", "/proc/sys/kernel/sched_schedstats");
    FileIO::apply("0", "/proc/sys/kernel/sched_autogroup_enabled");
    FileIO::apply("1", "/proc/sys/kernel/sched_child_runs_first");
    FileIO::apply("0", "/proc/sys/kernel/task_cpustats_enable");
    FileIO::apply("0", "/sys/module/mmc_core/parameters/use_spi_crc");
    FileIO::apply("0", "/sys/module/opchain/parameters/chain_on");
    FileIO::apply("0", "/sys/module/cpufreq_bouncing/parameters/enable");
    FileIO::apply("0", "/proc/task_info/task_sched_info/task_sched_info_enable");
    FileIO::apply("0", "/proc/oplus_scheduler/sched_assist/sched_assist_enabled");
    FileIO::apply("0 0 0 0", "/proc/sys/kernel/printk");
    FileIO::apply("0", "/dev/cpuset/background/memory_spread_page");
    FileIO::apply("0", "/dev/cpuset/system-background/memory_spread_page");
    FileIO::apply("0", "/proc/sys/vm/page-cluster");
    FileIO::apply("15", "/proc/sys/vm/stat_interval");
    FileIO::apply("80", "/proc/sys/vm/overcommit_ratio");
    FileIO::apply("640", "/proc/sys/vm/extfrag_threshold");
    FileIO::apply("22",  "/proc/sys/vm/watermark_scale_factor");
    FileIO::apply("libunity.so, libil2cpp.so, libmain.so, libUE4.so, libgodot_android.so, "
                  "libgdx.so, libgdx-box2d.so, libminecraftpe.so, libLive2DCubismCore.so, "
                  "libyuzu-android.so, libryujinx.so, libcitra-android.so, libhdr_pro_engine.so, "
                  "libandroidx.graphics.path.so, libeffect.so",
                  "/proc/sys/kernel/sched_lib_name");
    FileIO::apply("255", "/proc/sys/kernel/sched_lib_mask_force");

    DIR* td = ::opendir("/sys/class/thermal");
    if (td) {
        struct dirent* e;
        while ((e = ::readdir(td)))
            if (::strstr(e->d_name, "thermal_zone"))
                FileIO::apply("step_wise", std::format("/sys/class/thermal/{}/policy", e->d_name));
        ::closedir(td);
    }
}

void esportMode(const Config& cfg) noexcept {
    auto dndPath = std::format("{}/dnd_gameplay", MODULE_CONFIG);
    if (FileIO::readInt(dndPath) == 1) setDnd(1);

    FileIO::apply("N", "/sys/module/workqueue/parameters/power_efficient");
    setRpsXps(1);
    FileIO::apply("0", "/proc/sys/net/ipv4/tcp_slow_start_after_idle");
    FileIO::apply("0", "/proc/sys/kernel/split_lock_mitigate");
    FileIO::apply("0", "/proc/sys/kernel/sched_energy_aware");
    FileIO::apply("1", "/proc/sys/kernel/sched_boost");
    FileIO::apply("1", "/dev/stune/top-app/schedtune.prefer_idle");
    FileIO::apply("1", "/dev/stune/top-app/schedtune.boost");
    FileIO::apply("32", "/proc/sys/kernel/sched_nr_migrate");
    FileIO::apply("4",  "/proc/sys/kernel/sched_rr_timeslice_ms");
    FileIO::apply("80", "/proc/sys/kernel/sched_time_avg_ms");
    FileIO::apply("1",  "/proc/sys/kernel/sched_tunable_scaling");
    FileIO::apply("15",     "/proc/sys/kernel/sched_min_task_util_for_boost");
    FileIO::apply("8",      "/proc/sys/kernel/sched_min_task_util_for_colocation");
    FileIO::apply("50000",  "/proc/sys/kernel/sched_migration_cost_ns");
    FileIO::apply("800000", "/proc/sys/kernel/sched_min_granularity_ns");
    FileIO::apply("900000", "/proc/sys/kernel/sched_wakeup_granularity_ns");
    FileIO::apply("NEXT_BUDDY",   "/sys/kernel/debug/sched_features");
    FileIO::apply("NO_TTWU_QUEUE","/sys/kernel/debug/sched_features");
    FileIO::apply("768", "/dev/cpuctl/top-app/cpu.shares");
    FileIO::apply("512", "/dev/cpuctl/foreground/cpu.shares");
    FileIO::apply("1",   "/dev/cpuset/top-app/memory_spread_page");
    FileIO::apply("1",   "/dev/cpuset/foreground/memory_spread_page");
    FileIO::apply("64",  "/dev/cpuctl/background/cpu.shares");
    FileIO::apply("96",  "/dev/cpuctl/system-background/cpu.shares");
    FileIO::apply("192", "/dev/cpuctl/nnapi-hal/cpu.shares");
    FileIO::apply("192", "/dev/cpuctl/dex2oat/cpu.shares");
    FileIO::apply("512", "/dev/cpuctl/system/cpu.shares");
    FileIO::apply("128", "/dev/cpuctl/background/cpu.uclamp.max");
    FileIO::apply("128", "/dev/cpuctl/system-background/cpu.uclamp.max");
    FileIO::apply("128", "/dev/cpuctl/restricted-background/cpu.uclamp.max");
    FileIO::apply("256", "/sys/fs/cgroup/uclamp/top-app.uclamp.min");
    FileIO::apply("128", "/sys/fs/cgroup/uclamp/foreground.uclamp.min");
    FileIO::apply("32",  "/sys/fs/cgroup/uclamp/background.uclamp.min");
    FileIO::apply("1",   "/proc/touchpanel/game_switch_enable");
    FileIO::apply("0",   "/proc/touchpanel/oplus_tp_limit_enable");
    FileIO::apply("0",   "/proc/touchpanel/oppo_tp_limit_enable");
    FileIO::apply("1",   "/proc/touchpanel/oplus_tp_direction");
    FileIO::apply("1",   "/proc/touchpanel/oppo_tp_direction");

    MemoryManager::applyUfsDevfreq(true, cfg.liteMode != 0);

    if (!cfg.liteMode && !cfg.deviceMitigation)
        CpuFreq::changeGov("performance");
    else
        CpuFreq::changeGov(cfg.defaultCpuGov);

    FileIO::exists("/proc/ppm")
        ? CpuFreq::ppmMaxPerf(cfg.liteMode)
        : CpuFreq::maxPerf(cfg.liteMode);

    MemoryManager::applyStorageIO("32", "32");
    MemoryManager::applyEsport();

    if (auto soc = SocManager::create(cfg)) soc->applyEsport();
}

void balancedMode(const Config& cfg) noexcept {
    auto dndPath = std::format("{}/dnd_gameplay", MODULE_CONFIG);
    if (FileIO::readInt(dndPath) == 1) setDnd(0);

    FileIO::apply("N", "/sys/module/workqueue/parameters/power_efficient");
    setRpsXps(2);
    FileIO::apply("1", "/proc/sys/net/ipv4/tcp_slow_start_after_idle");
    FileIO::apply("1", "/proc/sys/kernel/split_lock_mitigate");
    FileIO::apply("1", "/proc/sys/kernel/sched_energy_aware");
    FileIO::apply("0", "/proc/sys/kernel/sched_boost");
    FileIO::apply("0", "/dev/stune/top-app/schedtune.prefer_idle");
    FileIO::apply("1", "/dev/stune/top-app/schedtune.boost");
    FileIO::apply("6",   "/proc/sys/kernel/sched_rr_timeslice_ms");
    FileIO::apply("100", "/proc/sys/kernel/sched_time_avg_ms");
    FileIO::apply("1",   "/proc/sys/kernel/sched_tunable_scaling");
    FileIO::apply("16",  "/proc/sys/kernel/sched_nr_migrate");
    FileIO::apply("25",      "/proc/sys/kernel/sched_min_task_util_for_boost");
    FileIO::apply("15",      "/proc/sys/kernel/sched_min_task_util_for_colocation");
    FileIO::apply("100000",  "/proc/sys/kernel/sched_migration_cost_ns");
    FileIO::apply("1200000", "/proc/sys/kernel/sched_min_granularity_ns");
    FileIO::apply("2000000", "/proc/sys/kernel/sched_wakeup_granularity_ns");
    FileIO::apply("NEXT_BUDDY", "/sys/kernel/debug/sched_features");
    FileIO::apply("TTWU_QUEUE",  "/sys/kernel/debug/sched_features");
    FileIO::apply("512", "/dev/cpuctl/top-app/cpu.shares");
    FileIO::apply("384", "/dev/cpuctl/foreground/cpu.shares");
    FileIO::apply("1",   "/dev/cpuset/top-app/memory_spread_page");
    FileIO::apply("0",   "/dev/cpuset/foreground/memory_spread_page");
    FileIO::apply("128", "/dev/cpuctl/background/cpu.shares");
    FileIO::apply("192", "/dev/cpuctl/system-background/cpu.shares");
    FileIO::apply("192", "/dev/cpuctl/nnapi-hal/cpu.shares");
    FileIO::apply("192", "/dev/cpuctl/dex2oat/cpu.shares");
    FileIO::apply("512", "/dev/cpuctl/system/cpu.shares");
    FileIO::apply("256", "/dev/cpuctl/background/cpu.uclamp.max");
    FileIO::apply("256", "/dev/cpuctl/system-background/cpu.uclamp.max");
    FileIO::apply("256", "/dev/cpuctl/restricted-background/cpu.uclamp.max");
    FileIO::apply("192", "/sys/fs/cgroup/uclamp/top-app.uclamp.min");
    FileIO::apply("96",  "/sys/fs/cgroup/uclamp/foreground.uclamp.min");
    FileIO::apply("32",  "/sys/fs/cgroup/uclamp/background.uclamp.min");
    FileIO::apply("0",   "/proc/touchpanel/game_switch_enable");
    FileIO::apply("1",   "/proc/touchpanel/oplus_tp_limit_enable");
    FileIO::apply("1",   "/proc/touchpanel/oppo_tp_limit_enable");
    FileIO::apply("0",   "/proc/touchpanel/oplus_tp_direction");
    FileIO::apply("0",   "/proc/touchpanel/oppo_tp_direction");

    MemoryManager::applyUfsDevfreq(false, false);
    CpuFreq::changeGov(cfg.defaultCpuGov);
    FileIO::exists("/proc/ppm") ? CpuFreq::ppmUnlock() : CpuFreq::unlock();
    MemoryManager::applyStorageIO("64", "64");
    MemoryManager::applyBalanced();

    if (auto soc = SocManager::create(cfg)) soc->applyBalanced();
}

void efficiencyMode(const Config& cfg) noexcept {
    auto dndPath = std::format("{}/dnd_gameplay", MODULE_CONFIG);
    if (FileIO::readInt(dndPath) == 1) setDnd(0);

    FileIO::apply("Y", "/sys/module/workqueue/parameters/power_efficient");
    setRpsXps(3);
    FileIO::apply("1", "/proc/sys/net/ipv4/tcp_slow_start_after_idle");
    FileIO::apply("1", "/proc/sys/kernel/split_lock_mitigate");
    FileIO::apply("1", "/proc/sys/kernel/sched_energy_aware");
    FileIO::apply("0", "/proc/sys/kernel/sched_boost");
    FileIO::apply("1", "/dev/stune/top-app/schedtune.prefer_idle");
    FileIO::apply("0", "/dev/stune/top-app/schedtune.boost");
    FileIO::apply("10",  "/proc/sys/kernel/sched_rr_timeslice_ms");
    FileIO::apply("150", "/proc/sys/kernel/sched_time_avg_ms");
    FileIO::apply("1",   "/proc/sys/kernel/sched_tunable_scaling");
    FileIO::apply("8",   "/proc/sys/kernel/sched_nr_migrate");
    FileIO::apply("45",      "/proc/sys/kernel/sched_min_task_util_for_boost");
    FileIO::apply("30",      "/proc/sys/kernel/sched_min_task_util_for_colocation");
    FileIO::apply("200000",  "/proc/sys/kernel/sched_migration_cost_ns");
    FileIO::apply("2000000", "/proc/sys/kernel/sched_min_granularity_ns");
    FileIO::apply("3000000", "/proc/sys/kernel/sched_wakeup_granularity_ns");
    FileIO::apply("NO_NEXT_BUDDY", "/sys/kernel/debug/sched_features");
    FileIO::apply("TTWU_QUEUE",    "/sys/kernel/debug/sched_features");
    FileIO::apply("0", "/dev/cpuset/top-app/memory_spread_page");
    FileIO::apply("0", "/dev/cpuset/foreground/memory_spread_page");
    FileIO::apply("384", "/dev/cpuctl/top-app/cpu.shares");
    FileIO::apply("256", "/dev/cpuctl/foreground/cpu.shares");
    FileIO::apply("128", "/dev/cpuctl/background/cpu.shares");
    FileIO::apply("128", "/dev/cpuctl/system-background/cpu.shares");
    FileIO::apply("128", "/dev/cpuctl/nnapi-hal/cpu.shares");
    FileIO::apply("128", "/dev/cpuctl/dex2oat/cpu.shares");
    FileIO::apply("384", "/dev/cpuctl/system/cpu.shares");
    FileIO::apply("128", "/dev/cpuctl/background/cpu.uclamp.max");
    FileIO::apply("128", "/dev/cpuctl/system-background/cpu.uclamp.max");
    FileIO::apply("128", "/dev/cpuctl/restricted-background/cpu.uclamp.max");
    FileIO::apply("128", "/sys/fs/cgroup/uclamp/top-app.uclamp.min");
    FileIO::apply("64",  "/sys/fs/cgroup/uclamp/foreground.uclamp.min");
    FileIO::apply("16",  "/sys/fs/cgroup/uclamp/background.uclamp.min");
    FileIO::apply("0",   "/proc/touchpanel/game_switch_enable");
    FileIO::apply("1",   "/proc/touchpanel/oplus_tp_limit_enable");
    FileIO::apply("1",   "/proc/touchpanel/oppo_tp_limit_enable");
    FileIO::apply("0",   "/proc/touchpanel/oplus_tp_direction");
    FileIO::apply("0",   "/proc/touchpanel/oppo_tp_direction");

    MemoryManager::applyUfsDevfreq(false, false);

    auto effGovPath = std::format("{}/efficiency_cpu_gov", MODULE_CONFIG);
    auto effGov = FileIO::readString(effGovPath);
    CpuFreq::changeGov(effGov.empty() ? "schedutil" : effGov);

    MemoryManager::applyStorageIO("16", "16");
    MemoryManager::applyEfficiency();

    if (auto soc = SocManager::create(cfg)) soc->applyEfficiency();
}

} // namespace velfox
