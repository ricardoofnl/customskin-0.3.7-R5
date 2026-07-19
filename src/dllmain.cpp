// customskin-037 - asi entry point
#include <windows.h>

#include "core/log.h"
#include "core/samp.h"
#include "game/render.h"
#include "net/artwork.h"
#include "net/dlcompat.h"
#include "net/masquerade.h"
#include "net/raknet.h"

namespace {

// wait up to timeoutMs for a module to load - also our "is this a sa-mp session?" check, since
// samp.dll never loads in single player
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
    // let samp.dll finish its own startup after the module maps in
    Sleep(500);

    if (!samp::Init()) {
        CS_LOGE("failed to resolve samp.dll base");
        return 0;
    }
    samp::DumpFingerprint();
    CS_LOGI("samp.dll resolved @ 0x%08X", static_cast<unsigned>(samp::Base()));

    // RakNet/rpc plumbing + artwork rpc handlers
    if (net::Init()) {
        artwork::Init();
        CS_LOGI("net + artwork RPC handlers ready");
    } else {
        CS_LOGE("net init failed");
    }

    // identify as 0.3DL so open.mp enables artwork. must be applied before connecting
    if (masq::Apply()) {
        CS_LOGI("DL masquerade on; connect and expect 'artwork: RPC179 ModelRequest' in the log");
    } else {
        CS_LOGW("DL masquerade NOT applied (see masq errors above); still a plain 0.3.7 client");
    }
    // fix dl SetPlayerSkin so sa-mp applies a valid base skin, and capture the custom skin per player
    if (dlcompat::Init()) {
        CS_LOGI("DL packet compat on (ScrSetPlayerSkin)");
    }

    // per-frame clump swap - render the custom model on the player's ped
    if (render::Init()) {
        CS_LOGI("clump-swap renderer on");
    }

    // announce ourselves in chat once it exists; poll up to ~120s then give up
    for (int i = 0; i < 1200; ++i) {
        if (samp::DebugChat("customskin-037 loaded - samp.dll @ 0x%08X",
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
