#pragma once
#include "Common.hpp"

namespace velfox {

class MemoryManager {
public:
    static void applyEsport() noexcept;
    static void applyBalanced() noexcept;
    static void applyEfficiency() noexcept;
    static void applyStorageIO(std::string_view readAheadKb,
                               std::string_view nrRequests) noexcept;
    static void applyUfsDevfreq(bool maxPerf, bool lite) noexcept;
};

} // namespace velfox
