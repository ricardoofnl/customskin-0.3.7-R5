#include "net/raknet.h"

#include "core/log.h"
#include "core/samp.h"

#include <windows.h>
#include <urmem.hpp>
#include <raknet/rakclient.h> // RakClientInterface, RPCParameters, BitStream, priorities

#include <utility>
#include <vector>

namespace net {
namespace {

urmem::hook g_connHook;
RakClientInterface* g_rak = nullptr;
std::vector<std::pair<uint8_t, RpcHandler>> g_wanted; // desired incoming handlers

// CNetGame::GetRakClient() via RefNetGame(). valid once a CNetGame exists
RakClientInterface* ResolveRak() {
    if (!samp::Base()) return nullptr;
    void* netgame = *reinterpret_cast<void**>(samp::Addr(samp::r5::kRefNetGame));
    if (!netgame) return nullptr;
    return urmem::call_function<urmem::calling_convention::thiscall, RakClientInterface*>(
        samp::Addr(samp::r5::kCNetGame_GetRakClient), netgame);
}

void ApplyRegistrations() {
    g_rak = ResolveRak();
    if (!g_rak) {
        CS_LOGW("rak: no RakClient available at ConnectionSucceeded");
        return;
    }
    for (auto& entry : g_wanted) {
        int uid = entry.first;
        g_rak->RegisterAsRemoteProcedureCall(&uid, entry.second);
    }
    CS_LOGI("rak: RakClient=%p, (re)registered %zu incoming RPC handler(s)",
            reinterpret_cast<void*>(g_rak), g_wanted.size());
}

// CNetGame::Packet_ConnectionSucceeded(this, packet*) - thiscall __fastcall(self=ecx, edx=edx, stack...) captures a thiscall on x86 msvc
void __fastcall Hooked_ConnectionSucceeded(void* self, void* /*edx*/, void* packet) {
    g_connHook.call<urmem::calling_convention::thiscall, void>(self, packet);
    CS_LOGI("rak: ConnectionSucceeded");
    ApplyRegistrations();
}

} // namespace

bool Init() {
    if (!samp::Base()) {
        CS_LOGE("rak: samp base not resolved");
        return false;
    }
    g_connHook.install(
        samp::Addr(samp::r5::kCNetGame_Packet_ConnectionSucceeded),
        urmem::get_func_addr(&Hooked_ConnectionSucceeded),
        urmem::hook::type::jmp);
    CS_LOGI("rak: hooked CNetGame::Packet_ConnectionSucceeded @ 0x%08X",
            static_cast<unsigned>(samp::Addr(samp::r5::kCNetGame_Packet_ConnectionSucceeded)));
    return true;
}

RakClientInterface* Rak() {
    if (!g_rak) g_rak = ResolveRak();
    return g_rak;
}

void RegisterIncomingRPC(uint8_t id, RpcHandler handler) {
    g_wanted.emplace_back(id, handler);
    if (RakClientInterface* rc = Rak()) { // already connected: apply now too
        int uid = id;
        rc->RegisterAsRemoteProcedureCall(&uid, handler);
        CS_LOGI("rak: registered incoming RPC %u (immediate)", id);
    }
}

bool SendRPC(uint8_t id, BitStream* payload) {
    RakClientInterface* rc = Rak();
    if (!rc) {
        CS_LOGW("rak: SendRPC %u but not connected", id);
        return false;
    }
    int uid = id;
    bool ok = rc->RPC(&uid, payload, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false);
    CS_LOGI("rak: sent RPC %u ok=%d", id, static_cast<int>(ok));
    return ok;
}

bool SendEmptyRPC(uint8_t id) {
    BitStream bs;
    return SendRPC(id, &bs);
}

} // namespace net
