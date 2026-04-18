#pragma once
#include "Common.hpp"
#include <optional>

namespace velfox {

class FileIO {
public:
    static bool exists(std::string_view path) noexcept;
    static bool write(std::string_view value, std::string_view path) noexcept;
    static bool apply(std::string_view value, std::string_view path) noexcept;
    static bool writeLL(long long value, std::string_view path) noexcept;
    static bool applyLL(long long value, std::string_view path) noexcept;
    static int readInt(std::string_view path) noexcept;
    static std::string readString(std::string_view path) noexcept;
};

} // namespace velfox
