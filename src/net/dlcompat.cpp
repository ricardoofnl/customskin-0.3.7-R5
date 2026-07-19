#include "net/dlcompat.h"

#include "core/log.h"
#include "core/samp.h"
#include "game/render.h"
#include "net/masquerade.h"

#include <windows.h>
#include <urmem.hpp>
#include <raknet/rakclient.h> // RPCParameters

#include <unordered_map>

namespace dlcompat {
namespace {

urmem::hook g_skinHook;
std::unordered_map<uint16_t, uint32_t> g_playerCustomSkin; // playerId -> custom skin id

// sa-mp ScrSetPlayerSkin(RPCParameters*), __cdecl
// dl wire  : u16 PlayerID, u32 Skin(base), u32 CustomSkin   (10 bytes / 80 bits)
// 0.3.7 wire: u32 PlayerID, u32 skin                        (8 bytes / 64 bits)
void __cdecl Hooked_ScrSetPlayerSkin(RPCParameters* p) {
    if (masq::Enabled() && p && p->input && p->numberOfBitsOfData >= 80) {
        unsigned char* d = p->input;
        // read the dl fields first (byte-aligned), before overwriting the buffer
        uint16_t playerId = *reinterpret_cast<uint16_t*>(d + 0);
        uint32_t skin = *reinterpret_cast<uint32_t*>(d + 2);
        uint32_t customSkin = *reinterpret_cast<uint32_t*>(d + 6);

        if (customSkin != 0) {
            g_playerCustomSkin[playerId] = customSkin;
            CS_LOGI("dlcompat: SetPlayerSkin player %u -> custom %u (base %u)",
                    playerId, customSkin, skin);
        } else {
            g_playerCustomSkin.erase(playerId);
        }

        // rewrite in place to the 0.3.7 layout so sa-mp applies the (valid) base skin
        *reinterpret_cast<uint32_t*>(d + 0) = playerId;
        *reinterpret_cast<uint32_t*>(d + 4) = skin;
        p->numberOfBitsOfData = 64;

        // swap the base template to the custom clump NOW, before sa-mp rebuilds the ped in
        // response to this skin. otherwise the ped clones the un-swapped template and the
        // custom only shows up on a second assignment (the "trigger twice" bug)
        if (customSkin != 0)
            render::EnsureSwapForCustom(customSkin);
    }
    g_skinHook.call<urmem::calling_convention::cdeclcall, void>(p);
}

} // namespace

uint32_t GetPlayerCustomSkin(uint16_t playerId) {
    auto it = g_playerCustomSkin.find(playerId);
    return it == g_playerCustomSkin.end() ? 0u : it->second;
}

bool AnyCustomSkin(uint32_t& customIdOut) {
    if (g_playerCustomSkin.empty()) return false;
    customIdOut = g_playerCustomSkin.begin()->second;
    return true;
}

bool IsCustomAssigned(uint32_t customId) {
    for (auto& kv : g_playerCustomSkin)
        if (kv.second == customId) return true;
    return false;
}

bool Init() {
    if (!samp::IsR5Build()) {
        CS_LOGE("dlcompat: not R5 build; skipping");
        return false;
    }
    g_skinHook.install(samp::Addr(samp::r5::kScrSetPlayerSkin),
                       urmem::get_func_addr(&Hooked_ScrSetPlayerSkin),
                       urmem::hook::type::jmp);
    CS_LOGI("dlcompat: hooked ScrSetPlayerSkin @ 0x%08X",
            static_cast<unsigned>(samp::Addr(samp::r5::kScrSetPlayerSkin)));
    return true;
}

} // namespace dlcompat
