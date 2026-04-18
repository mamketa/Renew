#include "MemoryManager.hpp"
#include "FileIO.hpp"
#include "DevFreq.hpp"
#include <dirent.h>
#include <cstring>
#include <format>

namespace velfox {

void MemoryManager::applyEsport() noexcept {
    FileIO::apply("20",   "/proc/sys/vm/swappiness");
    FileIO::apply("60",   "/proc/sys/vm/vfs_cache_pressure");
    FileIO::apply("30",   "/proc/sys/vm/compaction_proactiveness");
    FileIO::apply("5",    "/proc/sys/vm/dirty_background_ratio");
    FileIO::apply("15",   "/proc/sys/vm/dirty_ratio");
    FileIO::apply("1500", "/proc/sys/vm/dirty_expire_centisecs");
    FileIO::apply("1500", "/proc/sys/vm/dirty_writeback_centisecs");
}

void MemoryManager::applyBalanced() noexcept {
    FileIO::apply("40",   "/proc/sys/vm/swappiness");
    FileIO::apply("80",   "/proc/sys/vm/vfs_cache_pressure");
    FileIO::apply("40",   "/proc/sys/vm/compaction_proactiveness");
    FileIO::apply("10",   "/proc/sys/vm/dirty_background_ratio");
    FileIO::apply("25",   "/proc/sys/vm/dirty_ratio");
    FileIO::apply("3000", "/proc/sys/vm/dirty_expire_centisecs");
    FileIO::apply("3000", "/proc/sys/vm/dirty_writeback_centisecs");
}

void MemoryManager::applyEfficiency() noexcept {
    FileIO::apply("10",   "/proc/sys/vm/swappiness");
    FileIO::apply("100",  "/proc/sys/vm/vfs_cache_pressure");
    FileIO::apply("10",   "/proc/sys/vm/compaction_proactiveness");
    FileIO::apply("20",   "/proc/sys/vm/dirty_background_ratio");
    FileIO::apply("35",   "/proc/sys/vm/dirty_ratio");
    FileIO::apply("6000", "/proc/sys/vm/dirty_writeback_centisecs");
    FileIO::apply("6000", "/proc/sys/vm/dirty_expire_centisecs");
}

void MemoryManager::applyStorageIO(std::string_view readAheadKb,
                                    std::string_view nrRequests) noexcept {
    const char* fixed[] = {"mmcblk0", "mmcblk1"};
    for (auto* dev : fixed) {
        auto ra  = std::format("/sys/block/{}/queue/read_ahead_kb", dev);
        auto nr  = std::format("/sys/block/{}/queue/nr_requests", dev);
        if (FileIO::exists(ra)) FileIO::apply(readAheadKb, ra);
        if (FileIO::exists(nr)) FileIO::apply(nrRequests, nr);
    }
    DIR* dir = ::opendir("/sys/block");
    if (!dir) return;
    struct dirent* e;
    while ((e = ::readdir(dir))) {
        if (!::strstr(e->d_name, "sd")) continue;
        auto ra = std::format("/sys/block/{}/queue/read_ahead_kb", e->d_name);
        auto nr = std::format("/sys/block/{}/queue/nr_requests", e->d_name);
        if (FileIO::exists(ra)) FileIO::apply(readAheadKb, ra);
        if (FileIO::exists(nr)) FileIO::apply(nrRequests, nr);
    }
    ::closedir(dir);
}

void MemoryManager::applyUfsDevfreq(bool maxPerf, bool lite) noexcept {
    DIR* dir = ::opendir("/sys/class/devfreq");
    if (!dir) return;
    struct dirent* e;
    while ((e = ::readdir(dir))) {
        if (!::strstr(e->d_name, ".ufshc") && !::strstr(e->d_name, "mmc")) continue;
        auto path = std::format("/sys/class/devfreq/{}", e->d_name);
        if (maxPerf) {
            lite ? DevFreq::midPerf(path) : DevFreq::maxPerf(path);
        } else {
            DevFreq::minPerf(path);
        }
    }
    ::closedir(dir);
}

} // namespace velfox
