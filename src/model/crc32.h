// crc-32/ieee (poly 0xEDB88320, init/xorout 0xFFFFFFFF) - matches open.mp's GetFileCRC32Checksum, so checksums we compute equal what the server sends
#pragma once

#include <cstddef>
#include <cstdint>

namespace crc {

uint32_t Buffer(const void* data, size_t len);
uint32_t File(const char* path); // 0 if the file can't be opened

} // namespace crc
