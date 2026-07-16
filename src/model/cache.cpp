#include "model/cache.h"

#include "core/log.h"
#include "model/crc32.h"

#include <windows.h>
#include <shlobj.h>

#include <cstdio>

namespace cache {
namespace {

std::string g_dir;

// create every component of a path (like mkdir -p), backslash-separated
void MakeDirs(const std::string& path) {
    for (size_t i = 0; i < path.size(); ++i) {
        if (path[i] == '\\' || path[i] == '/') {
            if (i > 0) CreateDirectoryA(path.substr(0, i).c_str(), nullptr);
        }
    }
    CreateDirectoryA(path.c_str(), nullptr);
}

} // namespace

const std::string& Dir() {
    if (!g_dir.empty()) return g_dir;

    char docs[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_PERSONAL, nullptr, 0, docs))) {
        std::string base = std::string(docs) + "\\GTA San Andreas User Files\\SAMP\\cache\\customskin037";
        MakeDirs(base);
        g_dir = base + "\\";
        CS_LOGI("cache: %s", g_dir.c_str());
    } else {
        CS_LOGE("cache: failed to resolve Documents folder");
    }
    return g_dir;
}

std::string PathFor(uint32_t checksum, bool isDff) {
    char name[32];
    std::snprintf(name, sizeof(name), "%08X.%s", checksum, isDff ? "dff" : "txd");
    return Dir() + name;
}

bool HasValid(uint32_t checksum, bool isDff) {
    if (checksum == 0) return false;
    std::string path = PathFor(checksum, isDff);
    return crc::File(path.c_str()) == checksum;
}

} // namespace cache
