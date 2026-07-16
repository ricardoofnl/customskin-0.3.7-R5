#include "log.h"

#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include <mutex>
#include <string>

namespace cs {
namespace {

std::mutex g_mutex;
std::string g_path;
bool g_ready = false;

// resolve "<dir of gta_sa.exe>/customskin.log"
std::string ResolveLogPath() {
    char exe[MAX_PATH] = {0};
    DWORD n = GetModuleFileNameA(nullptr, exe, MAX_PATH);
    std::string dir;
    if (n > 0) {
        std::string full(exe, n);
        size_t slash = full.find_last_of("\\/");
        dir = (slash == std::string::npos) ? std::string() : full.substr(0, slash + 1);
    }
    return dir + "customskin.log";
}

} // namespace

void LogInit() {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_path = ResolveLogPath();
    // truncate on init
    if (FILE* f = std::fopen(g_path.c_str(), "w")) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        std::fprintf(f, "==== customskin log opened %04d-%02d-%02d %02d:%02d:%02d ====\n",
                     st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        std::fclose(f);
        g_ready = true;
    }
}

void LogWrite(const char* level, const char* fmt, ...) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_ready) return;

    FILE* f = std::fopen(g_path.c_str(), "a");
    if (!f) return;

    SYSTEMTIME st;
    GetLocalTime(&st);
    std::fprintf(f, "[%02d:%02d:%02d.%03d][%lu][%s] ",
                 st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
                 GetCurrentThreadId(), level ? level : "----");

    va_list args;
    va_start(args, fmt);
    std::vfprintf(f, fmt, args);
    va_end(args);

    std::fputc('\n', f);
    std::fclose(f);
}

} // namespace cs
