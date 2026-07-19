// downloaded-model cache under the sa-mp user files dir, keyed by crc + type so files are reused
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
