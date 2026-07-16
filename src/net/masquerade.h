// dl masquerade
//
// patches the client's RPC_ClientJoin construction so it announces sa-mp 0.3DL
// (version 4062 + matching challenge response) instead of 0.3.7 (4057). this is what
// flips open.mp into treating us as a dl client and sending the artwork/custom-model
// RPCs (179 etc.)
//
// consequence: with this on, the client always connects as 0.3DL. open.mp accepts dl
// clients, so this is exactly what we want there. it will not connect to a vanilla
// sa-mp 0.3.7 server (which rejects version 4062). a per-server toggle is a phase 5 item
#pragma once

namespace masq {

// apply the version + challenge-response byte patches. guarded by samp::IsR5Build();
// idempotent. must run before connecting (the patched code executes inside
// CNetGame::Packet_ConnectionSucceeded). returns true if the masquerade is active
bool Apply();

// true once Apply() has succeeded
bool Enabled();

} // namespace masq
