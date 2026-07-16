// RakNet access layer
//
// wraps sa-mp's RakClientInterface: lets us register handlers for incoming RPCs
// (including ids the stock 0.3.7 client never uses, like the artwork RPCs) and
// send RPCs to the server. registration is re-applied automatically on every
// (re)connect via a hook on CNetGame::Packet_ConnectionSucceeded
#pragma once

#include <cstdint>

struct RPCParameters;    // raknet/rakclient.h
class BitStream;         // raknet/bitstream.h
class RakClientInterface;

namespace net {

using RpcHandler = void (*)(RPCParameters* p);

// install the plumbing (hook onto connection-succeeded). call once after samp.dll
// is resolved. returns false if the anchor can't be resolved
bool Init();

// current RakClient interface, or nullptr if not connected yet
RakClientInterface* Rak();

// request that incoming rpc `id` be dispatched to `handler`. applied now if already
// connected, and re-applied on every future (re)connect. handlers run on the game's
// main thread
void RegisterIncomingRPC(uint8_t id, RpcHandler handler);

// send an rpc (id + payload) to the server. returns false if not connected
bool SendRPC(uint8_t id, BitStream* payload);

// send an rpc with an empty body (e.g. FinishDownload)
bool SendEmptyRPC(uint8_t id);

} // namespace net
