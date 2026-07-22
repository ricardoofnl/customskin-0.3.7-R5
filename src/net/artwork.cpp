#include "net/artwork.h"

#include "core/log.h"
#include "game/render.h"
#include "game/rwmodel.h"
#include "net/nativeart.h"

#include <cstdint>
#include <vector>

// Custom SKINS can't render through R5's native 0.3DL client - only objects can. But samp.dll still
// downloads every skin's dff/txd into its per-server cache and tracks it in the native model
// manager. So we don't own the artwork RPCs (nativeart does, for the object path); instead we mirror
// the skin entries out of the native manager and hand their loaded clumps to the clump-swap renderer.

namespace artwork {
namespace {

struct Model {
    int32_t newId = 0;
    int32_t baseId = 0;
    uint32_t dffCrc = 0;
    uint32_t txdCrc = 0;
    bool ready = false;     // samp.dll finished downloading + verifying both files
    rwmodel::Loaded loaded; // RW clump+txd, loaded from the native cache once ready
};

std::vector<Model> g_models;              // custom skins mirrored from the native manager
const void* g_managerToken = nullptr;     // last-seen manager identity (changes on reconnect/gmx)
int g_lastSkinCount = 0;

Model* Find(int32_t newId) {
    for (auto& m : g_models) if (m.newId == newId) return &m;
    return nullptr;
}

// forget every mirrored skin: put swapped base templates back first, then free our clumps
void ResetAll() {
    render::RestoreAll();
    for (auto& m : g_models) rwmodel::Free(m.loaded);
    g_models.clear();
    g_lastSkinCount = 0;
}

// mirror one native-manager skin entry (add if new, refresh its ready flag)
void Sync(const nativeart::ManagerSkin& s, void*) {
    Model* m = Find(s.newId);
    if (!m) {
        g_models.push_back(Model{ s.newId, s.baseId, s.dffCrc, s.txdCrc, false, {} });
        m = &g_models.back();
        CS_LOGI("artwork: skin %d (base %d) dff=0x%08X txd=0x%08X mirrored from native manager",
                s.newId, s.baseId, s.dffCrc, s.txdCrc);
    }
    m->ready = s.ready;
}

} // namespace

void Init() {
    CS_LOGI("artwork: skin clump-swap active (skins sourced from samp.dll's native manager + cache)");
}

// each frame: resync the skin set from the native manager, then load the clump for any skin whose
// files samp.dll has finished caching. render::Tick() then swaps those clumps onto peds.
void Pump() {
    // reconnect / gmx: samp.dll rebuilt (or cleared) the manager - drop stale mirrors + clumps
    const void* token = nativeart::ManagerToken();
    int skinCount = nativeart::ForEachSkin(nullptr, nullptr);
    bool reset = (token != g_managerToken) || (skinCount < g_lastSkinCount);
    if (reset && (g_managerToken != nullptr || !g_models.empty())) ResetAll();
    g_managerToken = token;
    g_lastSkinCount = skinCount;

    nativeart::ForEachSkin(&Sync, nullptr);

    // drive samp.dll's own download dialog off the files our hybrid finished (else it sticks at 0%)
    nativeart::PumpProgress();

    for (auto& m : g_models) {
        if (m.loaded.ok() || !m.ready) continue;
        char dffPath[512], txdPath[512];
        if (!nativeart::CachePath(m.dffCrc, true, dffPath, sizeof(dffPath))) continue;
        if (!nativeart::CachePath(m.txdCrc, false, txdPath, sizeof(txdPath))) continue;
        m.loaded = rwmodel::LoadModel(dffPath, txdPath);
        if (m.loaded.ok())
            CS_LOGI("artwork: skin %d RenderWare-loaded from native cache (clump+txd)", m.newId);
        else
            CS_LOGW("artwork: skin %d RW-load incomplete (clump=%p txd=%p) - bad file?",
                    m.newId, m.loaded.clump, m.loaded.txd);
    }
}

bool GetReadyModel(int newId, ReadyModel& out) {
    for (auto& m : g_models) {
        if (m.newId == newId && m.loaded.ok()) {
            out.newId = m.newId;
            out.baseId = m.baseId;
            out.clump = m.loaded.clump;
            out.txd = m.loaded.txd;
            return true;
        }
    }
    return false;
}

void ForEachReady(ReadyVisitor visit, void* ctx) {
    if (!visit) return;
    for (auto& m : g_models) {
        if (!m.loaded.ok()) continue;
        ReadyModel rm;
        rm.newId = m.newId;
        rm.baseId = m.baseId;
        rm.clump = m.loaded.clump;
        rm.txd = m.loaded.txd;
        visit(rm, ctx);
    }
}

void OnCustomClumpLost(void* clump, bool reload) {
    for (auto& m : g_models) {
        if (m.loaded.clump != clump) continue;
        m.loaded.clump = nullptr; // streamer already destroyed it - don't re-free
        if (reload) {
            char dffPath[512];
            if (nativeart::CachePath(m.dffCrc, true, dffPath, sizeof(dffPath)) &&
                rwmodel::ReloadClump(m.loaded, dffPath))
                CS_LOGI("artwork: skin %d clump was unloaded by streamer -> reloaded %p",
                        m.newId, m.loaded.clump);
            else
                CS_LOGW("artwork: skin %d clump lost, reload failed (reverts to base until reconnect)",
                        m.newId);
        } else {
            CS_LOGI("artwork: skin %d clump abandoned (teardown)", m.newId);
        }
        return;
    }
}

} // namespace artwork