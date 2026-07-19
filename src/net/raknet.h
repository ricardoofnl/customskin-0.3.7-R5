// RakNet access layer
#pragma once

#include <cstdint>

struct RPCParameters;    // raknet/rakclient.h
class BitStream;         // raknet/bitstream.h
class RakClientInterface;

namespace net {

using RpcHandler = void (*)(RPCParameters* p);

// hook onto connection-succeeded. call once after samp.dll is resolved; false if the anchor is missing
bool Init();

// current RakClient interface, or nullptr if not connected yet
RakClientInterface* Rak();

// dispatch incoming rpc id to handler; re-applied on every (re)connect. handlers run on the main thread
void RegisterIncomingRPC(uint8_t id, RpcHandler handler);

// send an rpc (id + payload) to the server. returns false if not connected
bool SendRPC(uint8_t id, BitStream* payload);

// send an rpc with an empty body (e.g. FinishDownload)
bool SendEmptyRPC(uint8_t id);

} // namespace net
