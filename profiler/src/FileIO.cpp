#include "FileIO.hpp"
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <format>

namespace velfox {

bool FileIO::exists(std::string_view path) noexcept {
    return path.empty() ? false : ::access(path.data(), F_OK) == 0;
}

static bool doWrite(std::string_view value, std::string_view path, bool force) noexcept {
    if (path.empty() || value.empty()) return false;
    if (!FileIO::exists(path)) return false;

    char buf[256]{};
    if (FILE* r = ::fopen(path.data(), "r")) {
        if (::fgets(buf, sizeof(buf), r)) {
            buf[::strcspn(buf, "\n")] = '\0';
            if (::strcmp(buf, value.data()) == 0) { ::fclose(r); return true; }
        }
        ::fclose(r);
    }

    FILE* w = ::fopen(path.data(), "w");
    if (!w) {
        ::chmod(path.data(), 0644);
        w = ::fopen(path.data(), "w");
        if (!w) { ::chmod(path.data(), 0444); return false; }
    }
    int ret = ::fprintf(w, "%s", value.data());
    ::fflush(w);
    ::fclose(w);
    if (force) ::chmod(path.data(), 0444);
    return ret >= 0;
}

bool FileIO::apply(std::string_view value, std::string_view path) noexcept {
    return doWrite(value, path, true);
}

bool FileIO::write(std::string_view value, std::string_view path) noexcept {
    return doWrite(value, path, true);
}

bool FileIO::applyLL(long long v, std::string_view path) noexcept {
    return apply(std::to_string(v), path);
}

bool FileIO::writeLL(long long v, std::string_view path) noexcept {
    return write(std::to_string(v), path);
}

int FileIO::readInt(std::string_view path) noexcept {
    FILE* fp = ::fopen(path.data(), "r");
    if (!fp) return 0;
    int v = 0;
    if (::fscanf(fp, "%d", &v) != 1) v = 0;
    ::fclose(fp);
    return v;
}

std::string FileIO::readString(std::string_view path) noexcept {
    FILE* fp = ::fopen(path.data(), "r");
    if (!fp) return {};
    char buf[1024]{};
    if (::fgets(buf, sizeof(buf), fp))
        buf[::strcspn(buf, "\n")] = '\0';
    ::fclose(fp);
    return buf;
}

} // namespace velfox
