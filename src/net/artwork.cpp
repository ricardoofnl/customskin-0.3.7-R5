#include "net/artwork.h"

#include "core/log.h"
#include "game/render.h"
#include "game/rwmodel.h"
#include "model/cache.h"
#include "model/download.h"
#include "net/raknet.h"

#include <cstdint>
#include <string>
#include <vector>

#include <raknet/rakclient.h> // RPCParameters, BitStream, BITS_TO_BYTES

namespace artwork {
namespace {

struct FileReq {
    uint32_t crc = 0;
    uint32_t size = 0;
    bool isDff = false;
    bool ready = false;
    bool requested = false;
};

struct Model {
    int32_t newId = 0;
    int32_t baseId = 0;
    uint8_t type = 0;
    FileReq dff;
    FileReq txd;
    rwmodel::Loaded loaded; // filled on DownloadCompleted
};

std::vector<Model> g_models;
uint32_t g_expected = 0;
bool g_finished = false;

// true if some already-registered model's file with this checksum was requested or is ready. several models can share a file (e.g. a common txd), so we only ask the server once
bool AlreadyInFlight(uint32_t crc) {
    for (auto& m : g_models) {
        if (m.dff.crc == crc && (m.dff.requested || m.dff.ready)) return true;
        if (m.txd.crc == crc && (m.txd.requested || m.txd.ready)) return true;
    }
    return false;
}

// ask the server for a file's download url (or mark ready if already cached)
void EnsureFile(FileReq& fr) {
    if (cache::HasValid(fr.crc, fr.isDff)) {
        fr.ready = true;
        CS_LOGI("artwork: %s 0x%08X already cached", fr.isDff ? "dff" : "txd", fr.crc);
        return;
    }
    // a sibling model may already be downloading this exact file - piggyback on it
    if (AlreadyInFlight(fr.crc)) {
        fr.requested = true;
        CS_LOGI("artwork: %s 0x%08X already requested by another model", fr.isDff ? "dff" : "txd", fr.crc);
        return;
    }
    BitStream bs;
    bs.Write(fr.crc); // u32 checksum
    net::SendRPC(fr.isDff ? rpc::kRequestDFF : rpc::kRequestTXD, &bs);
    fr.requested = true;
    CS_LOGI("artwork: requested %s 0x%08X", fr.isDff ? "dff" : "txd", fr.crc);
}

FileReq* FindByCrc(uint32_t crc) {
    for (auto& m : g_models) {
        if (m.dff.crc == crc) return &m.dff;
        if (m.txd.crc == crc) return &m.txd;
    }
    return nullptr;
}

// mark every registered file with this checksum ready (models sharing a file all complete off a single download). returns how many file slots were flipped
int MarkReadyByCrc(uint32_t crc) {
    int n = 0;
    for (auto& m : g_models) {
        if (m.dff.crc == crc && !m.dff.ready) { m.dff.ready = true; ++n; }
        if (m.txd.crc == crc && !m.txd.ready) { m.txd.ready = true; ++n; }
    }
    return n;
}

// when every file of every expected model is present + verified, tell the server
void MaybeFinish() {
    if (g_finished || g_expected == 0 || g_models.size() < g_expected) return;
    for (auto& m : g_models)
        if (!m.dff.ready || !m.txd.ready) return;
    g_finished = true;
    net::SendEmptyRPC(rpc::kFinishDownload);
    CS_LOGI("artwork: all %zu model(s) ready -> sent FinishDownload(184)", g_models.size());
}

// 179 ModelRequest (s->c): store the model, then request/verify its files
void OnModelRequest(RPCParameters* p) {
    if (!p || !p->input) return;
    BitStream bs(p->input, BITS_TO_BYTES(p->numberOfBitsOfData), false);

    uint32_t poolID = 0, dffCrc = 0, txdCrc = 0, dffSize = 0, txdSize = 0;
    int32_t count = 0, vw = 0, baseId = 0, newId = 0;
    uint8_t type = 0, timeOn = 0, timeOff = 0;

    bs.Read(poolID);
    bs.Read(count);
    bs.Read(type);
    bs.Read(vw);
    bs.Read(baseId);
    bs.Read(newId);
    bs.Read(dffCrc);
    bs.Read(txdCrc);
    bs.Read(dffSize);
    bs.Read(txdSize);
    bs.Read(timeOn);
    bs.Read(timeOff);

    CS_LOGI("artwork: RPC179 ModelRequest #%u/%d type=%u base=%d new=%d dff=0x%08X(%u) txd=0x%08X(%u)",
            poolID, count, type, baseId, newId, dffCrc, dffSize, txdCrc, txdSize);

    // poolID 0 begins a fresh model list (also handles reconnect / gmx re-send). put any swapped base templates back to the game's own clump BEFORE freeing the custom clumps they point at, then drop the old models so nothing is left dangling
    if (poolID == 0) {
        render::RestoreAll();
        for (auto& m : g_models) rwmodel::Free(m.loaded);
        g_models.clear();
        g_expected = static_cast<uint32_t>(count);
        g_finished = false;
    }

    Model m;
    m.newId = newId;
    m.baseId = baseId;
    m.type = type;
    m.dff = { dffCrc, dffSize, /*isDff*/ true, false, false };
    m.txd = { txdCrc, txdSize, /*isDff*/ false, false, false };
    EnsureFile(m.dff);
    EnsureFile(m.txd);
    g_models.push_back(m);

    MaybeFinish();
}

// 183 ModelUrl (s->c): u8(=6), u8 fileType, u32 checksum, dynstr8 url. download + verify
void OnModelUrl(RPCParameters* p) {
    if (!p || !p->input) return;
    BitStream bs(p->input, BITS_TO_BYTES(p->numberOfBitsOfData), false);

    uint8_t prefix = 0, fileType = 0, urlLen = 0;
    uint32_t checksum = 0;
    char url[256] = {0};
    bs.Read(prefix);
    bs.Read(fileType);
    bs.Read(checksum);
    bs.Read(urlLen);
    if (urlLen && urlLen < sizeof(url)) bs.Read(url, urlLen);

    FileReq* fr = FindByCrc(checksum);
    if (!fr) {
        CS_LOGW("artwork: RPC183 ModelUrl for unknown crc 0x%08X (url=%s)", checksum, url);
        return;
    }
    CS_LOGI("artwork: RPC183 ModelUrl %s 0x%08X url=\"%s\"", fr->isDff ? "dff" : "txd", checksum, url);

    // hand the fetch+verify to the background worker so a slow/large file never freezes the net/main thread. the result is applied in Pump() (main thread) once the worker finishes
    std::string dest = cache::PathFor(checksum, fr->isDff);
    dl::Enqueue(url, dest, checksum, fr->isDff);
}

// 185 DownloadCompleted (s->c): server acknowledged our FinishDownload
void OnDownloadCompleted(RPCParameters* /*p*/) {
    CS_LOGI("artwork: RPC185 DownloadCompleted - server acknowledged (%zu models cached)",
            g_models.size());

    // prove RenderWare can parse the downloaded files (resident load) with the random placeholder files this fails gracefully (no clump chunk); with a real skin's .dff/.txd it produces a clump + txd
    for (auto& m : g_models) {
        if (m.loaded.ok()) continue; // already loaded
        std::string dffPath = cache::PathFor(m.dff.crc, true);
        std::string txdPath = cache::PathFor(m.txd.crc, false);
        m.loaded = rwmodel::LoadModel(dffPath.c_str(), txdPath.c_str());
        if (m.loaded.ok())
            CS_LOGI("artwork: model %d RenderWare-loaded OK (clump+txd resident)", m.newId);
        else
            CS_LOGW("artwork: model %d RW-load incomplete (clump=%p txd=%p) - invalid/random file?",
                    m.newId, m.loaded.clump, m.loaded.txd);
    }
    // (render::Tick) applies these loaded clumps to peds
}

} // namespace

bool GetReadyModel(int newId, ReadyModel& out) {
    for (auto& m : g_models) {
        if (m.newId == newId && m.loaded.ok()) {
            out.newId = m.newId;
            out.baseId = m.baseId;
            out.clump = m.loaded.clump;
            out.txd = m.loaded.txd;
            return true;
        }
    }
    return false;
}

void ForEachReady(ReadyVisitor visit, void* ctx) {
    if (!visit) return;
    for (auto& m : g_models) {
        if (!m.loaded.ok()) continue;
        ReadyModel rm;
        rm.newId = m.newId;
        rm.baseId = m.baseId;
        rm.clump = m.loaded.clump;
        rm.txd = m.loaded.txd;
        visit(rm, ctx);
    }
}

void OnCustomClumpLost(void* clump, bool reload) {
    for (auto& m : g_models) {
        if (m.loaded.clump != clump) continue;
        m.loaded.clump = nullptr; // abandon: the streamer already destroyed it, don't re-free
        if (reload) {
            std::string dffPath = cache::PathFor(m.dff.crc, true);
            if (rwmodel::ReloadClump(m.loaded, dffPath.c_str()))
                CS_LOGI("artwork: model %d clump was unloaded by streamer -> reloaded %p",
                        m.newId, m.loaded.clump);
            else
                CS_LOGW("artwork: model %d clump lost, reload failed (skin reverts to base "
                        "until next reconnect)", m.newId);
        } else {
            CS_LOGI("artwork: model %d clump abandoned (teardown)", m.newId);
        }
        return;
    }
}

void Pump() {
    std::vector<dl::Result> results = dl::Drain();
    for (const auto& r : results) {
        if (r.ok) {
            int n = MarkReadyByCrc(r.crc); // mark all models sharing this file
            CS_LOGI("artwork: verified %s 0x%08X (%d file slot(s))", r.isDff ? "dff" : "txd", r.crc, n);
        } else {
            CS_LOGE("artwork: async download/verify failed for 0x%08X", r.crc);
        }
    }
    if (!results.empty()) MaybeFinish();
}

void Init() {
    net::RegisterIncomingRPC(rpc::kModelRequest, &OnModelRequest);
    net::RegisterIncomingRPC(rpc::kModelUrl, &OnModelUrl);
    net::RegisterIncomingRPC(rpc::kDownloadCompleted, &OnDownloadCompleted);
    CS_LOGI("artwork: registered RPC handlers 179/183/185");
}

} // namespace artwork
