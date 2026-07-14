// customskin-037 - ASI entry point.
//
// Loaded into gta_sa.exe by an ASI loader (vorbisFile.dll). Spawns a worker
// thread that waits for samp.dll, resolves the 0.3.7-R5 base, logs a fingerprint,
// and (Phase 1+) boots the networking/model subsystems.
#include <windows.h>

#include "core/log.h"
#include "core/samp.h"

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

    // Phase 1+ subsystems will boot here, e.g.:
    //   net::Init();     // RakNet/RPC hooks, DL masquerade, artwork RPCs
    //   model::Init();   // download pipeline + streaming integration

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
