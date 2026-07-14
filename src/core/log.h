// Simple thread-safe file logger. Writes customskin.log next to gta_sa.exe.
// This is the primary, offset-independent Phase 0 output: it works before any
// samp.dll symbol is resolved, so it can report why later stages did/didn't run.
#pragma once

namespace cs {

// Opens (truncates) the log file. Safe to call once from the init thread.
void LogInit();

// printf-style. Thread-safe; each line is timestamped and tagged with `level`.
void LogWrite(const char* level, const char* fmt, ...);

} // namespace cs

#define CS_LOGI(...) ::cs::LogWrite("INFO", __VA_ARGS__)
#define CS_LOGW(...) ::cs::LogWrite("WARN", __VA_ARGS__)
#define CS_LOGE(...) ::cs::LogWrite("ERR ", __VA_ARGS__)
#define CS_LOGD(...) ::cs::LogWrite("DBG ", __VA_ARGS__)
