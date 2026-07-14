// RakNet access layer.
//
// Wraps SA-MP's RakClientInterface: lets us register handlers for incoming RPCs
// (including ids the stock 0.3.7 client never uses, like the artwork RPCs) and
// send RPCs to the server. Registration is re-applied automatically on every
// (re)connect via a hook on CNetGame::Packet_ConnectionSucceeded.
#pragma once

#include <cstdint>

struct RPCParameters;    // raknet/rakclient.h
class BitStream;         // raknet/bitstream.h
class RakClientInterface;

namespace net {

using RpcHandler = void (*)(RPCParameters* p);

// Install the plumbing (hook onto connection-succeeded). Call once after samp.dll
// is resolved. Returns false if the anchor can't be resolved.
bool Init();

// Current RakClient interface, or nullptr if not connected yet.
RakClientInterface* Rak();

// Request that incoming RPC `id` be dispatched to `handler`. Applied now if already
// connected, and re-applied on every future (re)connect. Handlers run on the game's
// main thread.
void RegisterIncomingRPC(uint8_t id, RpcHandler handler);

// Send an RPC (id + payload) to the server. Returns false if not connected.
bool SendRPC(uint8_t id, BitStream* payload);

// Send an RPC with an empty body (e.g. FinishDownload).
bool SendEmptyRPC(uint8_t id);

} // namespace net
