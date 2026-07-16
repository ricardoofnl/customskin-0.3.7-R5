#include "game/render.h"

#include "core/log.h"
#include "core/samp.h"
#include "game/gta.h"
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
    if (!dlcompat::IsCustomAssigned(static_cast<uint32_t>(rm.newId))) return; // nobody wears it
    void* mi = ModelInfo(rm.baseId);
    if (!mi) return; // base model slot empty
    void** field = ClumpField(mi);
    if (!*field) return;            // base skin not streamed in yet - wait
    if (*field == rm.clump) return; // already pointing at our custom clump

    auto it = g_swapped.find(rm.baseId);
    if (it == g_swapped.end()) {
        g_swapped[rm.baseId] = { *field, rm.clump }; // remember the game's own clump once
        CS_LOGI("render: base %d template clump -> custom %d %p (spawn/respawn to see it)",
                rm.baseId, rm.newId, rm.clump);
    } else {
        it->second.custom = rm.clump; // base re-pointed at a different custom on the same base
    }
    *field = rm.clump;
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
        // only restore if the template still holds our custom clump; if the streamer already
        // replaced it we must not clobber the fresh clump with a stale pointer
        if (*field == kv.second.custom) *field = kv.second.original;
    }
    if (!g_swapped.empty())
        CS_LOGI("render: restored %zu base template(s) to their original clump", g_swapped.size());
    g_swapped.clear();
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
