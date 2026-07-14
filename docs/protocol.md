# open.mp artwork / custom-model protocol (client view)

Distilled from open.mp `Shared/NetCode/custommodels.hpp` and
`Server/Components/CustomModels/models.cpp` (ref commit `f8058db`), plus
`Server/Components/LegacyNetwork/legacy_network_impl.cpp`. This is what our 0.3.7-R5 client
has to speak to receive and load custom skins.

## Version gate (why we must masquerade as 0.3DL)

open.mp only sends artwork RPCs to players whose `getClientVersion() == ClientVersion_SAMP_03DL`.
The client version is decided at connect from the legacy version code and the auth token:

```
LegacyClientVersion_037  = 4057   (0x0FD9)
LegacyClientVersion_03DL = 4062   (0x0FDE)
accepted token: SAMPRakNet::GetToken() == (challenge ^ version)
```

So to be treated as DL we must send version **4062** and token **`challenge ^ 4062`**.
Consequence: once we do, open.mp sends us **DL-format** packets, several of which carry an
extra 4-byte custom-model field the stock 0.3.7 parser does not expect (see "DL packet
compat" below).

## Artwork RPCs

RakNet RPC id -> direction -> payload. Numbers are the SA-MP/open.mp RPC ids. Encoding is the
SA-MP RakNet BitStream (little-endian, `dynstr8` = 1-byte length prefix then raw bytes).

| id  | name              | dir | payload |
|-----|-------------------|-----|---------|
| 179 | `ModelRequest`      | S->C | `u32 poolID`, `i32 Count`, `u8 type`(0=object,1=skin), `i32 virtualWorld`, `i32 baseId`, `i32 newId`, `u32 dffChecksum`, `u32 txdChecksum`, `u32 dffSize`, `u32 txdSize`, `u8 timeOn`, `u8 timeOff` |
| 181 | `RequestDFF`        | C->S | `u32 checksum` |
| 182 | `RequestTXD`        | C->S | `u32 checksum` |
| 183 | `ModelUrl`          | S->C | `u8 =6` (fixed prefix), `u8 fileType`, `u32 fileChecksum`, `dynstr8 url` |
| 184 | `FinishDownload`    | C->S | (empty) |
| 185 | `DownloadCompleted` | S->C | (empty) |

`type`/`fileType`: DFF vs TXD. In open.mp `ModelDownloadType`: DFF and TXD (values written as
`u8`). Cross-check the concrete DFF/TXD integer values on the wire during Phase 3.

`ModelRequest.write` server-side order (authoritative):
`writeUINT32(poolID); writeINT32(Count); writeUINT8(type); writeINT32(virtualWorld);
writeINT32(baseId); writeINT32(newId); writeUINT32(dffChecksum); writeUINT32(txdChecksum);
writeUINT32(dffSize); writeUINT32(txdSize); writeUINT8(timeOn); writeUINT8(timeOff);`

`ModelUrl.write` server-side order (authoritative):
`writeUINT8(6); writeUINT8(fileType); writeUINT32(fileChecksum); writeDynStr8(downloadLink);`
 note the leading constant byte `6`; consume it before reading `fileType`.

## Client download state machine

1. On client-init the server sends one `179 ModelRequest` per model (`Count` = total). Store
   `{newId, baseId, type, dffChecksum+dffSize, txdChecksum+txdSize}`.
   - open.mp also nudges `SetPlayerVirtualWorld(+1)` then `(-1)` right after, to force the
     client to (re)start downloading if it reconnected. Treat a VW bounce as "ensure files".
2. For each file whose CRC isn't already present+valid in cache, send `181 RequestDFF` /
   `182 RequestTXD` with that `u32 checksum`.
3. Server replies `183 ModelUrl(fileType, checksum, url)`. HTTP `GET url` with header
   **`User-Agent: SAMP/0.3`** (open.mp's webserver returns 401 otherwise, and only serves
   paths ending in `.dff`/`.txd` to IPs that are connected players). Save the bytes, verify
   **CRC32 == checksum**.
4. When every file for all `Count` models is present+verified, send `184 FinishDownload`.
5. Server replies `185 DownloadCompleted`. Now register the models into GTA streaming.

## Hosting details (server side, for context)

- Built-in webserver default port `7777` (config `artwork.port`), serves the `models/`
  folder; or a `artwork.cdn` URL is used instead. Files are addressed by name in the URL.
- The server computes file CRCs with a standard CRC-32 (see `crc32.hpp`); `artwork.show_crc_logs 1`
  makes it print `<file> CRC = 0x...`, handy to validate our CRC implementation.

## CRC32

Standard CRC-32/IEEE: polynomial `0xEDB88320` (reflected), init `0xFFFFFFFF`, final xor
`0xFFFFFFFF`, over the raw file bytes. Must match `artwork.show_crc_logs` output.

## Cache

SA-MP DL caches under `Documents/GTA San Andreas User Files/SAMP/cache/<serverip_port>/`,
files keyed by CRC, with a `cache.db` index. For milestone 1 our own index/layout under that
directory is acceptable; matching SA-MP's `cache.db` exactly is a later nicety.

## DL packet compat (stability, not artwork)

Because we present as DL, open.mp sends the DL variants of these S->C RPCs, each with one
extra `u32` (custom model / skin) the 0.3.7 parser doesn't read. We must consume that dword
(and capture it for skin assignment) at the same read sites `spmn/samp-client-compat` patches,
but in the opposite direction. Affected: SetPlayerSkin (ScrSetPlayerSkin), PlayerStreamIn
(WorldPlayerAdd), ShowActor, RequestClass response, SetSpawnInfo, CreateObject. Exact per-RPC
field offsets come from open.mp's `isDL`/`IsDL` branches (`player.cpp`, `classes_main.cpp`,
`actor.hpp`, `object.hpp`)  nail these down in Phase 2.
