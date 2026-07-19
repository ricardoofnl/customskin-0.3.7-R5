#include "game/rwmodel.h"

#include "core/log.h"
#include "game/gta.h"

#include <windows.h>
#include <urmem.hpp>

namespace rwmodel {
namespace {

// gta_sa.exe is normally loaded at its ImageBase (0x400000); delta guards a rebase
uintptr_t Delta() {
    static uintptr_t d = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr)) - 0x400000;
    return d;
}

template <typename Ret = void, typename... A>
Ret Cdecl(uintptr_t va, A... args) {
    return urmem::call_function<urmem::calling_convention::cdeclcall, Ret>(va + Delta(), args...);
}

void* LoadTxd(const char* path) {
    void* s = Cdecl<void*>(gta::RwStreamOpen, gta::kStreamFileName, gta::kStreamRead,
                           reinterpret_cast<const void*>(path));
    if (!s) {
        CS_LOGE("rw: RwStreamOpen failed for txd %s", path);
        return nullptr;
    }
    void* txd = nullptr;
    if (Cdecl<int>(gta::RwStreamFindChunk, s, gta::kChunkTexDict, nullptr, nullptr))
        txd = Cdecl<void*>(gta::RwTexDictionaryStreamRead, s);
    Cdecl<>(gta::RwStreamClose, s, nullptr);
    return txd;
}

void* LoadClump(const char* path, void* txd) {
    void* s = Cdecl<void*>(gta::RwStreamOpen, gta::kStreamFileName, gta::kStreamRead,
                           reinterpret_cast<const void*>(path));
    if (!s) {
        CS_LOGE("rw: RwStreamOpen failed for dff %s", path);
        return nullptr;
    }
    // bind the model's own textures while reading its geometry
    void* prev = Cdecl<void*>(gta::RwTexDictionaryGetCurrent);
    if (txd) Cdecl<>(gta::RwTexDictionarySetCurrent, txd);

    void* clump = nullptr;
    if (Cdecl<int>(gta::RwStreamFindChunk, s, gta::kChunkClump, nullptr, nullptr))
        clump = Cdecl<void*>(gta::RpClumpStreamRead, s);

    Cdecl<>(gta::RwTexDictionarySetCurrent, prev);
    Cdecl<>(gta::RwStreamClose, s, nullptr);
    return clump;
}

} // namespace

Loaded LoadModel(const char* dffPath, const char* txdPath) {
    Loaded r;
    r.txd = LoadTxd(txdPath);
    r.clump = LoadClump(dffPath, r.txd);
    CS_LOGI("rw: LoadModel clump=%p txd=%p (dff=%s)", r.clump, r.txd, dffPath);
    return r;
}

void Free(Loaded& m) {
    if (m.clump) Cdecl<>(gta::RpClumpDestroy, m.clump);
    if (m.txd) Cdecl<>(gta::RwTexDictionaryDestroy, m.txd);
    m.clump = m.txd = nullptr;
}

bool ReloadClump(Loaded& m, const char* dffPath) {
    // deliberately does NOT free m.clump: the old clump was already destroyed by whoever
    // stole it (the streamer). bind the existing txd and read a fresh clump
    m.clump = LoadClump(dffPath, m.txd);
    CS_LOGI("rw: ReloadClump clump=%p (dff=%s)", m.clump, dffPath);
    return m.clump != nullptr;
}

void RequestModel(int modelId) {
    Cdecl<>(gta::CStreaming_RequestModel, modelId, gta::kStreamPriorityLoad);
}

} // namespace rwmodel
