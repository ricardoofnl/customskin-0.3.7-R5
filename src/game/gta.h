// gta sa 1.0 us (gta_sa.exe, ImageBase 0x400000) addresses
// absolute VAs; source: DK22Pac/plugin-sdk (plugin_sa/game_sa). a runtime delta is
// applied in case the exe is rebased (normally 0 for the classic 1.0 us build)
#pragma once

#include <cstdint>

namespace gta {

// --- RenderWare stream / model / txd ---
constexpr uintptr_t RwStreamOpen = 0x7ECEF0;              // (RwStreamType, RwStreamAccessType, const void*) -> RwStream*
constexpr uintptr_t RwStreamClose = 0x7ECE20;             // (RwStream*, void*)
constexpr uintptr_t RwStreamFindChunk = 0x7ED2D0;         // (RwStream*, RwUInt32 type, RwUInt32* lenOut, RwUInt32* verOut) -> RwBool
constexpr uintptr_t RpClumpStreamRead = 0x74B420;         // (RwStream*) -> RpClump*
constexpr uintptr_t RpClumpDestroy = 0x74A310;            // (RpClump*)
constexpr uintptr_t RwTexDictionaryStreamRead = 0x804C30; // (RwStream*) -> RwTexDictionary*
constexpr uintptr_t RwTexDictionaryGetCurrent = 0x7F3A90; // () -> RwTexDictionary*
constexpr uintptr_t RwTexDictionarySetCurrent = 0x7F3A70; // (RwTexDictionary*)
constexpr uintptr_t RwTexDictionaryDestroy = 0x7F36A0;    // (RwTexDictionary*)

// --- model system / entity ---
constexpr uintptr_t ms_modelInfoPtrs = 0xA9B0C8; // CBaseModelInfo*[20000]
constexpr uintptr_t FindPlayerPed = 0x56E210;    // __cdecl (int id, -1 = local) -> CPed*
constexpr int kModelInfo_RwObject = 0x1C;        // CBaseModelInfo::m_pRwClump offset
// CEntity vtable indices (thiscall via *(void***)ped)
constexpr int kVt_SetModelIndex = 5;
constexpr int kVt_CreateRwObject = 7;
constexpr int kVt_DeleteRwObject = 8;

// rw enums / chunk ids
constexpr int kStreamFileName = 2;        // rwSTREAMFILENAME
constexpr int kStreamRead = 1;            // rwSTREAMREAD
constexpr uint32_t kChunkClump = 0x10;    // rwID_CLUMP
constexpr uint32_t kChunkTexDict = 0x16;  // rwID_TEXDICTIONARY

} // namespace gta
