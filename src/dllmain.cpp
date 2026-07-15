// customskin-037 - ASI entry point.
//
// Loaded into gta_sa.exe by an ASI loader (vorbisFile.dll). Spawns a worker
// thread that waits for samp.dll, resolves the 0.3.7-R5 base, logs a fingerprint,
// and (Phase 1+) boots the networking/model subsystems.
#include <windows.h>

#include "core/log.h"
#include "core/samp.h"
#include "net/artwork.h"
#include "net/masquerade.h"
#include "net/raknet.h"

namespace {

// Poll for a module to appear, up to timeoutMs. Returns its handle or nullptr.
// GTA loads samp.dll shortly after the process starts; in single player it never
// loads, so this doubles as our "is this a SA-MP session?" check.
HMODULE WaitForModule(const char* name, DWORD timeoutMs) {
    const DWORD step = 100;
    DWORD waited = 0;
    for (;;) {
        if (HMODULE h = GetModuleHandleA(name)) return h;
        if (waited >= timeoutMs) return nullptr;
        Sleep(step);
        waited += step;
    }
}

DWORD WINAPI MainThread(LPVOID) {
    cs::LogInit();
    CS_LOGI("customskin-037 loading (built %s %s)", __DATE__, __TIME__);

    if (!WaitForModule("samp.dll", 60000)) {
        CS_LOGW("samp.dll not present after 60s; not a SA-MP session, exiting");
        return 0;
    }
    // Let samp.dll finish its own startup after the module maps in.
    Sleep(500);

    if (!samp::Init()) {
        CS_LOGE("failed to resolve samp.dll base");
        return 0;
    }
    samp::DumpFingerprint();
    CS_LOGI("Phase 0 online (samp.dll @ 0x%08X)", static_cast<unsigned>(samp::Base()));

    // Phase 1: RakNet/RPC plumbing + artwork RPC handlers (fire once the DL
    // masquerade in Phase 2 makes the server send them).
    if (net::Init()) {
        artwork::Init();
        CS_LOGI("Phase 1 online (RakNet hooks + artwork RPC handlers)");
    } else {
        CS_LOGE("Phase 1 init failed (net::Init)");
    }

    // Phase 2a: identify as 0.3DL so open.mp enables the artwork path and sends us
    // the custom-model RPCs. Must be applied before connecting.
    if (masq::Apply()) {
        CS_LOGI("Phase 2a online (DL masquerade). Connect to your open.mp server; "
                "expect 'artwork: RPC179 ModelRequest' in the log.");
    } else {
        CS_LOGW("Phase 2a NOT applied (see masq errors above); still a plain 0.3.7 client");
    }
    // Phase 2b (DL-format packet compat) + Phase 3/4 boot here next.

    // Announce ourselves in chat once the chat object exists (created around the
    // main menu). Poll up to ~120s, then give up quietly.
    for (int i = 0; i < 1200; ++i) {
        if (samp::DebugChat("customskin-037 loaded (Phase 0) - samp.dll @ 0x%08X",
                            static_cast<unsigned>(samp::Base()))) {
            break;
        }
        Sleep(100);
    }

    return 0;
}

} // namespace

BOOL APIENTRY DllMain(HINSTANCE hinst, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinst);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
