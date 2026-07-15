// DL masquerade.
//
// Patches the client's RPC_ClientJoin construction so it announces SA-MP 0.3DL
// (version 4062 + matching challenge response) instead of 0.3.7 (4057). This is what
// flips open.mp into treating us as a DL client and sending the artwork/custom-model
// RPCs (179 etc.).
//
// Consequence: with this on, the client always connects as 0.3DL. open.mp accepts DL
// clients, so this is exactly what we want there. It will NOT connect to a vanilla
// SA-MP 0.3.7 server (which rejects version 4062). A per-server toggle is a Phase 5 item.
#pragma once

namespace masq {

// Apply the version + challenge-response byte patches. Guarded by samp::IsR5Build();
// idempotent. Must run before connecting (the patched code executes inside
// CNetGame::Packet_ConnectionSucceeded). Returns true if the masquerade is active.
bool Apply();

// True once Apply() has succeeded.
bool Enabled();

} // namespace masq
