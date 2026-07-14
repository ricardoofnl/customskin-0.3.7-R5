// samp.dll access layer for SA-MP 0.3.7-R5-1.
//
// All offsets are RVAs into samp.dll (add Base() for an absolute address).
// Source of the numbers: BlastHackNet/SAMP-API (branch `multiver`),
// src/sampapi/0.3.7-R5-1/*.cpp  the SAMP-API `GetAddress(rva)` == moduleBase + rva.
// Only R5-1 is supported; the network patches must NOT be pointed at another build.
#pragma once

#include <cstdint>

namespace samp {

// RVA constants for 0.3.7-R5-1.
namespace r5 {

// Global pointers-to-object (the value at base+rva is itself a pointer).
constexpr uintptr_t kRefChat = 0x26EB80;    // CChat**
constexpr uintptr_t kRefNetGame = 0x26EB94; // CNetGame**

// CChat methods (thiscall).
constexpr uintptr_t kCChat_AddMessage = 0x68170; // (this, D3DCOLOR color, const char* text)
constexpr uintptr_t kCChat_AddEntry = 0x67BE0;   // (this, int type, text, prefix, textColor, prefixColor)

// CNetGame methods (thiscall).
constexpr uintptr_t kCNetGame_GetRakClient = 0xBBC0;              // -> RakClientInterface*
constexpr uintptr_t kCNetGame_Connect = 0x8940;                  // anchor
constexpr uintptr_t kCNetGame_Packet_ConnectionSucceeded = 0xAD80; // anchor (auth/token path)

// --- Phase 2 (DL masquerade), still to be resolved on the R5 binary ---------
// The client version code 4057 (0x0FD9) and the auth token (= challenge ^ version)
// are formed along the connect/auth path (near Packet_ConnectionSucceeded above and
// the RPC_ClientJoin send). Find the exact byte-patch sites via the matching 0.3.7
// decompilation at https://github.com/dashr9230/SA-MP plus a disasm of the anchors.
// constexpr uintptr_t kVersionConstSite = 0; // patch imm 0x0FD9 -> 0x0FDE
// constexpr uintptr_t kTokenXorSite     = 0; // patch imm 0x0FD9 -> 0x0FDE

} // namespace r5

// Resolve samp.dll base and read its PE fingerprint. Returns false if samp.dll
// isn't loaded yet. Safe to call repeatedly.
bool Init();

// samp.dll load base, or 0 before a successful Init().
uintptr_t Base();

// RVA -> absolute address in samp.dll.
inline uintptr_t Addr(uintptr_t rva) { return Base() + rva; }

// PE fingerprint of the loaded samp.dll (for verifying the build is really R5-1).
uint32_t ImageSize();
uint32_t TimeDateStamp();

// Log the fingerprint + bytes at a few anchor offsets. Helps confirm the offset
// table matches the user's actual samp.dll.
void DumpFingerprint();

// Print a white line to the SA-MP chat, if the chat object exists yet.
// Returns false (and logs to file only) when chat isn't available.
bool DebugChat(const char* fmt, ...);

} // namespace samp
