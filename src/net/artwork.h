// Artwork (custom-model) subsystem.
//
// Phase 1: registers handlers for the open.mp artwork RPCs and logs receipt so we
// can confirm the DL masquerade (Phase 2) makes them arrive.
// Phase 3+: parses ModelRequest, drives the download pipeline, feeds the streamer.
#pragma once

namespace artwork {

// Register the artwork RPC handlers with the net layer. Call after net::Init().
void Init();

// RPC ids (see docs/protocol.md).
namespace rpc {
constexpr unsigned char kModelRequest = 179;     // S->C
constexpr unsigned char kRequestDFF = 181;       // C->S
constexpr unsigned char kRequestTXD = 182;       // C->S
constexpr unsigned char kModelUrl = 183;         // S->C
constexpr unsigned char kFinishDownload = 184;   // C->S
constexpr unsigned char kDownloadCompleted = 185; // S->C
} // namespace rpc

} // namespace artwork
