// thread-safe file logger, writes customskin.log next to gta_sa.exe. works before any samp.dll symbol is resolved
#pragma once

namespace cs {

// opens (truncates) the log file. safe to call once from the init thread
void LogInit();

// printf-style. thread-safe; each line is timestamped and tagged with `level`
void LogWrite(const char* level, const char* fmt, ...);

} // namespace cs

#define CS_LOGI(...) ::cs::LogWrite("INFO", __VA_ARGS__)
#define CS_LOGW(...) ::cs::LogWrite("WARN", __VA_ARGS__)
#define CS_LOGE(...) ::cs::LogWrite("ERR ", __VA_ARGS__)
#define CS_LOGD(...) ::cs::LogWrite("DBG ", __VA_ARGS__)
