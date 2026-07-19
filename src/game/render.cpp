#include "game/render.h"

#include "core/log.h"
#include "core/samp.h"
#include "game/gta.h"
#include "game/rwmodel.h"
#include "net/artwork.h"
#include "net/dlcompat.h"

#include <windows.h>
#include <urmem.hpp>

#include <cstdint>
#include <unordered_map>

namespace render {
namespace {

urmem::hook g_processHook;

// the swap we applied to one base model's template clump
struct Swap {
    void* original = nullptr; // the game's own template clump for this base
    void* custom = nullptr;   // the custom clump we pointed it at
};
std::unordered_map<int, Swap> g_swapped; // base model id -> swap

uintptr_t GtaDelta() {
    static uintptr_t d = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr)) - 0x400000;
    return d;
}

void* ModelInfo(int id) {
    auto arr = reinterpret_cast<void**>(gta::ms_modelInfoPtrs + GtaDelta());
    return arr[id];
}

void** ClumpField(void* mi) {
    return reinterpret_cast<void**>(reinterpret_cast<char*>(mi) + gta::kModelInfo_RwObject);
}

// point one ready model's base template at our custom clump, but only if a player actually
// wears it (limits ambient bleed onto same-base pedestrians). the base's own clump is saved
// once so RestoreAll can put it back. new peds of that base clone our clump on creation
void SwapOne(const artwork::ReadyModel& rm, void* /*ctx*/) {
    void* mi = ModelInfo(rm.baseId);
    if (!mi) return; // base model slot empty
    void** field = ClumpField(mi);

    // already swapped this base: check the streamer hasn't stolen our clump (independent of
    // whether anyone still wears it, so we always notice the loss)
    auto it = g_swapped.find(rm.baseId);
    if (it != g_swapped.end()) {
        if (*field == it->second.custom) return; // still installed, nothing to do
        // the template no longer holds our clump: the streamer unloaded the base model and
        // destroyed our borrowed clump (field is now null or a fresh real clump). abandon the
        // dangling pointer and reload a fresh one, then re-swap on the next frame
        artwork::OnCustomClumpLost(it->second.custom, /*reload*/ true);
        g_swapped.erase(it);
        return;
    }

    // keep the base ped model resident even before it is worn, so the very first assignment
    // finds a loaded template and renders on the first try (not only after a second trigger)
    if (!*field) { rwmodel::RequestModel(rm.baseId); return; }

    if (!dlcompat::IsCustomAssigned(static_cast<uint32_t>(rm.newId))) return; // loaded, unworn
    if (*field == rm.clump) return; // already ours but unrecorded (rare) - leave it

    g_swapped[rm.baseId] = { *field, rm.clump }; // remember the game's own clump once
    *field = rm.clump;
    CS_LOGI("render: base %d template clump -> custom %d %p", rm.baseId, rm.newId, rm.clump);
}

void Tick() {
    artwork::ForEachReady(&SwapOne, nullptr);
}

// CNetGame::Process(this) - thiscall, per frame
void __fastcall Hooked_Process(void* self, void* /*edx*/) {
    g_processHook.call<urmem::calling_convention::thiscall, void>(self);
    Tick();
}

} // namespace

void RestoreAll() {
    for (auto& kv : g_swapped) {
        void* mi = ModelInfo(kv.first);
        if (!mi) continue;
        void** field = ClumpField(mi);
        if (*field == kv.second.custom) {
            *field = kv.second.original; // put the game's own clump back
        } else {
            // streamer already destroyed our clump: don't clobber the fresh clump, and tell
            // artwork to abandon the dangling pointer so the upcoming free won't double-free
            artwork::OnCustomClumpLost(kv.second.custom, /*reload*/ false);
        }
    }
    if (!g_swapped.empty())
        CS_LOGI("render: restored %zu base template(s) to their original clump", g_swapped.size());
    g_swapped.clear();
}

void EnsureSwapForCustom(unsigned customId) {
    artwork::ReadyModel rm;
    if (artwork::GetReadyModel(static_cast<int>(customId), rm))
        SwapOne(rm, nullptr); // record original + install our clump before the ped is rebuilt
}

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
