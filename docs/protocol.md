# open.mp artwork / custom-model protocol (client view)

Distilled from open.mp `Shared/NetCode/custommodels.hpp`,
`Server/Components/CustomModels/models.cpp`, and
`Server/Components/LegacyNetwork/legacy_network_impl.cpp`. This is what a 0.3.7-R5 client has
to speak to receive and load custom skins.

## Version gate

open.mp only sends artwork RPCs to players whose `getClientVersion()` is 0.3DL. The version is
decided at connect from the legacy version code and the auth token:

```text
037  = 4057 (0x0FD9)
03DL = 4062 (0x0FDE)
accepted token: SAMPRakNet::GetToken() == (challenge ^ version)
```

So to be treated as DL we send version `4062` and token `challenge ^ 4062`. The catch: once we
do, open.mp sends DL-format packets, several of which carry an extra 4-byte custom-model field
the stock 0.3.7 parser does not expect (see "DL packet compat" below).

## Artwork RPCs

RakNet RPC id, direction, and payload. Encoding is the SA-MP RakNet BitStream (little-endian;
`dynstr8` is a 1-byte length prefix followed by the raw bytes).

| id | name | dir | payload |
| --- | --- | --- | --- |
| 179 | `ModelRequest` | S->C | `u32 poolID`, `i32 Count`, `u8 type` (0=object,1=skin), `i32 virtualWorld`, `i32 baseId`, `i32 newId`, `u32 dffChecksum`, `u32 txdChecksum`, `u32 dffSize`, `u32 txdSize`, `u8 timeOn`, `u8 timeOff` |
| 181 | `RequestDFF` | C->S | `u32 checksum` |
| 182 | `RequestTXD` | C->S | `u32 checksum` |
| 183 | `ModelUrl` | S->C | `u8 =6` (fixed prefix), `u8 fileType`, `u32 fileChecksum`, `dynstr8 url` |
| 184 | `FinishDownload` | C->S | (empty) |
| 185 | `DownloadCompleted` | S->C | (empty) |

Server-side write order for `179` (authoritative): `poolID, Count, type, virtualWorld, baseId,
newId, dffChecksum, txdChecksum, dffSize, txdSize, timeOn, timeOff`.

Server-side write order for `183`: `u8 6, u8 fileType, u32 fileChecksum, dynstr8 url`. The
leading constant `6` must be consumed before reading `fileType`.

## Client download flow

1. On connect the server sends one `179` per model (`Count` is the total). Store
   `{newId, baseId, type, dffChecksum+size, txdChecksum+size}`. open.mp also bounces
   `SetPlayerVirtualWorld(+1)` then `(-1)` right after, to nudge a reconnecting client into
   re-downloading; treat that as "make sure the files are present".
2. For each file whose CRC isn't already valid in cache, send `181`/`182` with its `u32 checksum`.
3. The server replies `183` with the file's URL. Fetch it over HTTP with header
   `User-Agent: SAMP/0.3` (the webserver returns 401 otherwise, and only serves `.dff`/`.txd`
   paths to connected players). Save the bytes and verify `CRC32 == checksum`.
4. When every file for all `Count` models is present and verified, send `184`.
5. The server replies `185`. Now register the models into GTA streaming.

## Hosting (server side, for context)

The built-in webserver defaults to port `7777` (`artwork.port`) and serves the `models/`
folder, unless an `artwork.cdn` URL is set. Files are addressed by name. The server computes
CRCs with a standard CRC-32; `artwork.show_crc_logs 1` prints `<file> CRC = 0x...`, handy for
checking our CRC against theirs.

## CRC32

Standard CRC-32/IEEE: polynomial `0xEDB88320` (reflected), init `0xFFFFFFFF`, final xor
`0xFFFFFFFF`, over the raw file bytes. Must match `artwork.show_crc_logs`.

## Cache

SA-MP DL caches under `Documents/GTA San Andreas User Files/SAMP/cache/<serverip_port>/`, keyed
by CRC. Our own layout under that directory is fine; matching SA-MP's `cache.db` exactly is a
later nicety.

## DL packet compat (stability, not artwork)

Because we present as DL, open.mp sends the DL variants of some S->C RPCs, each with one extra
`u32` (the custom model / skin) the 0.3.7 parser doesn't read. We consume that dword (and
capture it for skin assignment) at the same read sites `spmn/samp-client-compat` patches, but
in the opposite direction. Affected: SetPlayerSkin (ScrSetPlayerSkin), PlayerStreamIn
(WorldPlayerAdd), ShowActor, RequestClass response, SetSpawnInfo, CreateObject. Skins only need
ScrSetPlayerSkin; the rest matter once objects are added.
