// http downloader (WinINet). sends user-agent "samp/0.3", which open.mp's artwork webserver requires
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dl {

// get `url` into `destPath`. returns true if any bytes were written. blocking
bool Fetch(const std::string& url, const std::string& destPath);

// a finished async download+verify
struct Result {
    uint32_t crc = 0;   // expected checksum (also identifies which file this is)
    bool isDff = false; // echoed back from the enqueue call
    bool ok = false;    // fetched AND crc-verified against `crc`
};

// queue an async download; the worker fetches, checks crc32 == crc, and posts a Result to Drain
void Enqueue(const std::string& url, const std::string& destPath, uint32_t crc, bool isDff);

// pop all finished results (thread-safe). call from the main thread each frame
std::vector<Result> Drain();

} // namespace dl
