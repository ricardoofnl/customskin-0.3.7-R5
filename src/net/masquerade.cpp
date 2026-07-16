#include "net/masquerade.h"

#include "core/log.h"
#include "core/samp.h"

#include <urmem.hpp>

#include <memory>
#include <vector>

namespace masq {
namespace {

std::vector<std::unique_ptr<urmem::patch>> g_patches;
bool g_enabled = false;

// patch a single code byte at `rva`, but only if it currently holds `expected`
// this guards against a wrong build, an already-applied patch, or another mod
// having modified the site
bool PatchByte(uintptr_t rva, uint8_t expected, uint8_t value) {
    auto addr = samp::Addr(rva);
    uint8_t cur = *reinterpret_cast<volatile uint8_t*>(addr);
    if (cur == value) {
        CS_LOGW("masq: rva 0x%06X already 0x%02X; leaving as-is", static_cast<unsigned>(rva), value);
        return true;
    }
    if (cur != expected) {
        CS_LOGE("masq: rva 0x%06X = 0x%02X, expected 0x%02X; refusing to patch",
                static_cast<unsigned>(rva), cur, expected);
        return false;
    }
    g_patches.emplace_back(std::make_unique<urmem::patch>(addr, urmem::bytearray_t { value }));
    CS_LOGI("masq: rva 0x%06X  0x%02X -> 0x%02X", static_cast<unsigned>(rva), expected, value);
    return true;
}

} // namespace

bool Apply() {
    if (g_enabled) return true;
    if (!samp::IsR5Build()) {
        CS_LOGE("masq: not the validated 0.3.7-R5-1 build; refusing to apply DL masquerade");
        return false;
    }
    // 4057 (0x0FD9) -> 4062 (0x0FDE): flip the low imm byte 0xD9 -> 0xDE at both the
    // ChallengeResponse xor and the VersionNumber store
    if (!PatchByte(samp::r5::kChallengeImmLowByte, 0xD9, 0xDE)
        || !PatchByte(samp::r5::kVersionImmLowByte, 0xD9, 0xDE)) {
        g_patches.clear(); // rollback: urmem::patch dtor restores original bytes
        CS_LOGE("masq: apply failed; rolled back");
        return false;
    }
    g_enabled = true;
    CS_LOGI("masq: DL masquerade ENABLED - client now identifies as 0.3DL (4062)");
    return true;
}

bool Enabled() { return g_enabled; }

} // namespace masq
