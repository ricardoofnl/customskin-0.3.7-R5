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

// PE fingerprint of the validated 0.3.7-R5-1 samp.dll (confirmed against a
// known-good client). Used as a build guard before applying network patches.
constexpr uint32_t kImageSize = 0x0027E000;
constexpr uint32_t kTimeDateStamp = 0x6372C39E;

// CChat methods (thiscall).
constexpr uintptr_t kCChat_AddMessage = 0x68170; // (this, D3DCOLOR color, const char* text)
constexpr uintptr_t kCChat_AddEntry = 0x67BE0;   // (this, int type, text, prefix, textColor, prefixColor)

// CNetGame methods (thiscall).
constexpr uintptr_t kCNetGame_GetRakClient = 0xBBC0;              // -> RakClientInterface*
constexpr uintptr_t kCNetGame_Connect = 0x8940;                  // anchor
constexpr uintptr_t kCNetGame_Packet_ConnectionSucceeded = 0xAD80; // anchor (auth/token path)

// DL masquerade patch sites (found by disasm of Packet_ConnectionSucceeded).
// The client builds its RPC_ClientJoin here with version 4057 (0x0FD9) and
// ChallengeResponse = cookie ^ 4057. To be treated as 0.3DL by open.mp we change
// 4057 (0x0FD9) -> 4062 (0x0FDE) at both sites: patch the low imm byte 0xD9 -> 0xDE.
//   1000ae62: 81 f2 d9 0f 00 00       xor edx,0xfd9          (ChallengeResponse)
//   1000aeca: c7 44 24 28 d9 0f 00 00 mov [esp+0x28],0xfd9   (VersionNumber)
constexpr uintptr_t kChallengeImmLowByte = 0xAE64; // 0xD9 -> 0xDE
constexpr uintptr_t kVersionImmLowByte = 0xAECE;   // 0xD9 -> 0xDE

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

// True if the loaded samp.dll matches the validated 0.3.7-R5-1 fingerprint.
// Network patches refuse to apply unless this is true.
bool IsR5Build();

// Log the fingerprint + bytes at a few anchor offsets. Helps confirm the offset
// table matches the user's actual samp.dll.
void DumpFingerprint();

// Print a white line to the SA-MP chat, if the chat object exists yet.
// Returns false (and logs to file only) when chat isn't available.
bool DebugChat(const char* fmt, ...);

} // namespace samp
