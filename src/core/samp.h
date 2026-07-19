// samp.dll access layer for sa-mp 0.3.7-R5-1
#pragma once

#include <cstdint>

namespace samp {

// rva constants for 0.3.7-R5-1
namespace r5 {

// global pointers-to-object (the value at base+rva is itself a pointer)
constexpr uintptr_t kRefChat = 0x26EB80;    // CChat**
constexpr uintptr_t kRefNetGame = 0x26EB94; // CNetGame**

// pe fingerprint of the validated 0.3.7-R5-1 samp.dll (confirmed against a known-good client). used as a build guard before applying network patches
constexpr uint32_t kImageSize = 0x0027E000;
constexpr uint32_t kTimeDateStamp = 0x6372C39E;

// CChat methods (thiscall)
constexpr uintptr_t kCChat_AddMessage = 0x68170; // (this, D3DCOLOR color, const char* text)
constexpr uintptr_t kCChat_AddEntry = 0x67BE0;   // (this, int type, text, prefix, textColor, prefixColor)

// CNetGame methods (thiscall)
constexpr uintptr_t kCNetGame_GetRakClient = 0xBBC0;              // > RakClientInterface*
constexpr uintptr_t kCNetGame_Process = 0xB5B0;                  // per-frame; our main-thread tick
constexpr uintptr_t kCNetGame_Connect = 0x8940;                  // anchor
constexpr uintptr_t kCNetGame_Packet_ConnectionSucceeded = 0xAD80; // anchor (auth/token path)

// ScrSetPlayerSkin(RPCParameters*), __cdecl. dl adds a u32 CustomSkin that misaligns the 0.3.7 parser
constexpr uintptr_t kScrSetPlayerSkin = 0x19190;

// dl masquerade patch sites: RPC_ClientJoin builds version 4057 (0x0FD9) at the challenge xor and
// the version store; we flip the low byte to 0xDE (4062) at each
constexpr uintptr_t kChallengeImmLowByte = 0xAE64; // 0xD9 -> 0xDE
constexpr uintptr_t kVersionImmLowByte = 0xAECE;   // 0xD9 -> 0xDE

} // namespace r5

// resolve samp.dll base and read its fingerprint. false if not loaded yet; safe to call repeatedly
bool Init();

// samp.dll load base, or 0 before a successful Init()
uintptr_t Base();

// rva -> absolute address in samp.dll
inline uintptr_t Addr(uintptr_t rva) { return Base() + rva; }

// pe fingerprint of the loaded samp.dll (for verifying the build is really R5-1)
uint32_t ImageSize();
uint32_t TimeDateStamp();

// true if the loaded samp.dll matches the validated R5-1 fingerprint (patches refuse to apply otherwise)
bool IsR5Build();

// log the fingerprint + bytes at a few anchors, to confirm the offset table matches this samp.dll
void DumpFingerprint();

// print a white line to sa-mp chat; false (file log only) if the chat object doesn't exist yet
bool DebugChat(const char* fmt, ...);

} // namespace samp
