#include "model/download.h"

#include "core/log.h"

#include <windows.h>
#include <wininet.h>

#include <cstdio>

namespace dl {

bool Fetch(const std::string& url, const std::string& destPath) {
    // open.mp's webserver rejects anything whose user-agent isn't exactly "samp/0.3"
    HINTERNET hNet = InternetOpenA("SAMP/0.3", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hNet) {
        CS_LOGE("dl: InternetOpen failed (%lu)", GetLastError());
        return false;
    }

    HINTERNET hUrl = InternetOpenUrlA(hNet, url.c_str(), nullptr, 0,
                                      INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hUrl) {
        CS_LOGE("dl: InternetOpenUrl failed for %s (%lu)", url.c_str(), GetLastError());
        InternetCloseHandle(hNet);
        return false;
    }

    FILE* f = std::fopen(destPath.c_str(), "wb");
    if (!f) {
        CS_LOGE("dl: cannot open %s for writing", destPath.c_str());
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hNet);
        return false;
    }

    char buf[65536];
    DWORD read = 0;
    size_t total = 0;
    while (InternetReadFile(hUrl, buf, sizeof(buf), &read) && read > 0) {
        std::fwrite(buf, 1, read, f);
        total += read;
    }

    std::fclose(f);
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hNet);

    CS_LOGI("dl: %s -> %s (%zu bytes)", url.c_str(), destPath.c_str(), total);
    return total > 0;
}

} // namespace dl
