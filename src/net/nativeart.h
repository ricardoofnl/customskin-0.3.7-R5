// native 0.3DL artwork activation
#pragma once

#include <cstdint>

namespace nativeart {

// Register samp.dll's own (dormant) 0.3DL artwork RPC handlers with the net layer so the built-in
// custom-model client downloads every custom model into its per-server cache and renders custom
// OBJECTS natively (ScrCreateObject -> CObjectPool::Create). Requires samp base resolved +
// net::Init() done. Guarded by IsR5Build().
bool Init();

// --- skin bridge ---------------------------------------------------------------------------------
// R5's native client can't render custom SKINS, only objects. But it DOES download skin files into
// its cache and tracks them in the model manager. So our clump-swap renderer sources skins from the
// native manager (below) instead of owning the artwork RPCs itself - no second RPC handler, no
// conflict with the native object path.

struct ManagerSkin {
    int32_t newId = 0;   // custom skin id (20000-30000)
    int32_t baseId = 0;  // base ped the skin rides on
    uint32_t dffCrc = 0;
    uint32_t txdCrc = 0;
    bool ready = false;  // samp.dll has both files downloaded + verified in its cache
};

// visit every type=1 (skin) entry in the native manager; returns how many exist. visit may be null
// to only count. main-thread only; returns 0 until the manager exists (i.e. before connecting).
using SkinVisitor = void (*)(const ManagerSkin&, void* ctx);
int ForEachSkin(SkinVisitor visit, void* ctx);

// samp.dll's per-server cache path for a checksum ("<dir>/0x<CRC>.<ext>"). false if the manager /
// cache dir isn't ready yet.
bool CachePath(uint32_t crc, bool isDff, char* out, unsigned n);

// changes whenever samp.dll (re)creates the manager - a reset signal for reconnect / gmx
const void* ManagerToken();

// R5 stubbed the download worker, so the native "Updating models.." dialog never advances past 0%
// even though our hybrid does fetch the files. This re-reports each finished file to samp.dll's own
// dialog updater so it shows real progress. MAIN THREAD ONLY (touches the UI) - call it each frame.
void PumpProgress();

} // namespace nativeart
