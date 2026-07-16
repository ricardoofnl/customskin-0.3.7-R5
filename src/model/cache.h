// downloaded-model cache, under the sa-mp user files dir:
//   documents/gta san andreas user files/samp/cache/customskin037/
// files are keyed by crc + type, so a file already present with the right crc is
// reused instead of re-downloaded
#pragma once

#include <cstdint>
#include <string>

namespace cache {

// cache directory (created on first call, trailing separator). empty on failure
const std::string& Dir();

// absolute path where a dff/txd with the given checksum is stored
std::string PathFor(uint32_t checksum, bool isDff);

// true if a cached file for `checksum` exists and its CRC32 matches
bool HasValid(uint32_t checksum, bool isDff);

} // namespace cache
