#include "game/render.h"

#include "core/log.h"
#include "core/samp.h"
#include "game/gta.h"
#include "net/artwork.h"
#include "net/dlcompat.h"

#include <windows.h>
#include <urmem.hpp>

#include <cstdint>

namespace render {
namespace {

urmem::hook g_processHook;
bool g_done = false;
void* g_origClump = nullptr; // saved base-model clump (restored on cleanup, phase 5)
int g_origBase = 0;

uintptr_t GtaDelta() {
    static uintptr_t d = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr)) - 0x400000;
    return d;
}

void* ModelInfo(int id) {
    auto arr = reinterpret_cast<void**>(gta::ms_modelInfoPtrs + GtaDelta());
    return arr[id];
}

// point the base model's template clump at our custom clump. we do not touch any live ped
// (re-instancing an active ped corrupts its anim/skeleton refs and crashes). instead, any
// ped created afterwards - notably the player on spawn - clones our clump via the game's
// own creation path, with full ped setup. existing instances keep their own clump
void Tick() {
    if (g_done) return;

    uint32_t custom = 0;
    if (!dlcompat::AnyCustomSkin(custom)) return; // no custom skin assigned yet
    artwork::ReadyModel rm;
    if (!artwork::GetReadyModel(static_cast<int>(custom), rm)) return; // clump not loaded yet

    void* mi = ModelInfo(rm.baseId);
    if (!mi) return; // base model slot empty
    void** clumpField = reinterpret_cast<void**>(reinterpret_cast<char*>(mi) + gta::kModelInfo_RwObject);
    if (!*clumpField) return; // base skin not streamed in yet - wait
    if (*clumpField == rm.clump) { g_done = true; return; }

    g_origClump = *clumpField;
    g_origBase = rm.baseId;
    *clumpField = rm.clump; // new peds (player on spawn) now clone our clump
    g_done = true;
    CS_LOGI("render: base model %d template clump -> custom %p (spawn/respawn to see it)",
            rm.baseId, rm.clump);
}

// CNetGame::Process(this) - thiscall, per frame
void __fastcall Hooked_Process(void* self, void* /*edx*/) {
    g_processHook.call<urmem::calling_convention::thiscall, void>(self);
    Tick();
}

} // namespace

bool Init() {
    if (!samp::IsR5Build()) {
        CS_LOGE("render: not R5 build; skipping");
        return false;
    }
    g_processHook.install(samp::Addr(samp::r5::kCNetGame_Process),
                          urmem::get_func_addr(&Hooked_Process),
                          urmem::hook::type::jmp);
    CS_LOGI("render: hooked CNetGame::Process @ 0x%08X (per-frame tick)",
            static_cast<unsigned>(samp::Addr(samp::r5::kCNetGame_Process)));
    return true;
}

} // namespace render
