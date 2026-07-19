// artwork (custom-model) subsystem
#pragma once

namespace artwork {

// register the artwork rpc handlers with the net layer. call after net::Init()
void Init();

// drain finished downloads; once all are in, send FinishDownload. call each frame (main thread)
void Pump();

// a downloaded + RenderWare-loaded custom model
struct ReadyModel {
    int newId = 0;
    int baseId = 0;
    void* clump = nullptr; // RpClump*
    void* txd = nullptr;   // RwTexDictionary*
};

// look up a ready (clump+txd loaded) custom model by new id. false if not ready
bool GetReadyModel(int newId, ReadyModel& out);

// visit every loaded custom model (ctx passed through). allocation-free, safe to call each frame
using ReadyVisitor = void (*)(const ReadyModel&, void* ctx);
void ForEachReady(ReadyVisitor visit, void* ctx);

// the streamer freed a clump we lent to a base template: forget the stale pointer (no re-free),
// and reload a fresh one from cache if `reload`
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
