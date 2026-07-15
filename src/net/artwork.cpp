#include "net/artwork.h"

#include "core/log.h"
#include "net/raknet.h"

#include <cstdint>
#include <raknet/rakclient.h> // RPCParameters, BitStream, BITS_TO_BYTES

namespace artwork {
namespace {

// 179 ModelRequest (S->C): full field layout per docs/protocol.md. Phase 1 parses
// and logs it (this doubles as verification that the DL masquerade works); Phase 3
// will turn these into cache lookups + download requests.
void OnModelRequest(RPCParameters* p) {
    if (!p || !p->input) return;
    BitStream bs(p->input, BITS_TO_BYTES(p->numberOfBitsOfData), false);

    uint32_t poolID = 0, dffChecksum = 0, txdChecksum = 0, dffSize = 0, txdSize = 0;
    int32_t count = 0, virtualWorld = 0, baseId = 0, newId = 0;
    uint8_t type = 0, timeOn = 0, timeOff = 0;

    bs.Read(poolID);
    bs.Read(count);
    bs.Read(type);
    bs.Read(virtualWorld);
    bs.Read(baseId);
    bs.Read(newId);
    bs.Read(dffChecksum);
    bs.Read(txdChecksum);
    bs.Read(dffSize);
    bs.Read(txdSize);
    bs.Read(timeOn);
    bs.Read(timeOff);

    CS_LOGI("artwork: RPC179 ModelRequest #%u/%d type=%u vw=%d base=%d new=%d "
            "dff=0x%08X(%u) txd=0x%08X(%u)",
            poolID, count, type, virtualWorld, baseId, newId,
            dffChecksum, dffSize, txdChecksum, txdSize);
    // Phase 3: model_store.Add(...) + kick download state machine.
}

// 183 ModelUrl (S->C): u8(=6), u8 fileType, u32 checksum, dynstr8 url.
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

    CS_LOGI("artwork: RPC183 ModelUrl type=%u checksum=0x%08X url=\"%s\"",
            fileType, checksum, url);
    // Phase 3: downloader.Fetch(url, checksum) -> cache -> verify CRC32.
}

// 185 DownloadCompleted (S->C): empty body.
void OnDownloadCompleted(RPCParameters* /*p*/) {
    CS_LOGI("artwork: RPC185 DownloadCompleted");
    // Phase 4: register all downloaded models into the streamer.
}

} // namespace

void Init() {
    net::RegisterIncomingRPC(rpc::kModelRequest, &OnModelRequest);
    net::RegisterIncomingRPC(rpc::kModelUrl, &OnModelUrl);
    net::RegisterIncomingRPC(rpc::kDownloadCompleted, &OnDownloadCompleted);
    CS_LOGI("artwork: registered RPC handlers 179/183/185");
}

} // namespace artwork
