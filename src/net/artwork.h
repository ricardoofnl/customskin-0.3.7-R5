// artwork (custom-model) subsystem
//
// phase 1: registers handlers for the open.mp artwork RPCs and logs receipt so we
// can confirm the dl masquerade (phase 2) makes them arrive
// phase 3+: parses ModelRequest, drives the download pipeline, feeds the streamer
#pragma once

namespace artwork {

// register the artwork rpc handlers with the net layer. call after net::Init()
void Init();

// a downloaded + RenderWare-loaded custom model
struct ReadyModel {
    int newId = 0;
    int baseId = 0;
    void* clump = nullptr; // RpClump*
    void* txd = nullptr;   // RwTexDictionary*
};

// look up a ready (clump+txd loaded) custom model by new id. false if not ready
bool GetReadyModel(int newId, ReadyModel& out);

// visit every ready (clump+txd loaded) custom model. `ctx` is passed through to `visit`.
// allocation-free so the renderer can call it each frame
using ReadyVisitor = void (*)(const ReadyModel&, void* ctx);
void ForEachReady(ReadyVisitor visit, void* ctx);

// the renderer calls this when it notices the game's streamer destroyed one of our custom
// clumps (it was borrowed into a base model's template and unloaded with it). we abandon our
// now-dangling pointer WITHOUT freeing it again; if `reload` we read a fresh clump from cache
// so the skin can be re-applied, otherwise we just drop it (teardown)
void OnCustomClumpLost(void* clump, bool reload);

// rpc ids (see docs/protocol.md)
namespace rpc {
constexpr unsigned char kModelRequest = 179;     // s->c
constexpr unsigned char kRequestDFF = 181;       // c->s
constexpr unsigned char kRequestTXD = 182;       // c->s
constexpr unsigned char kModelUrl = 183;         // s->c
constexpr unsigned char kFinishDownload = 184;   // c->s
constexpr unsigned char kDownloadCompleted = 185; // s->c
} // namespace rpc

} // namespace artwork
