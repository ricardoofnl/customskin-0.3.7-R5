# customskin-037

A client-side `.asi` for **SA-MP 0.3.7-R5** (GTA: San Andreas) that adds **custom player
skins** when you connect to an **open.mp** server. It borrows open.mp's 0.3DL "artwork"
custom-model system so a stock 0.3.7 client can use custom skins without switching to the
0.3DL client.

## How it works

open.mp only sends custom models to clients that identify as 0.3DL, so the plugin makes a
0.3.7-R5 client do exactly that:

- Masquerades as 0.3DL at connect (version `4062`, token `challenge ^ 4062`) so the server
  turns artwork on for us.
- Speaks the artwork protocol: receives the model list, downloads each `.dff`/`.txd` over
  HTTP with `User-Agent: SAMP/0.3`, checks the CRC32, and caches the files.
- Loads each model into GTA SA and swaps its clump onto the right ped, so the custom skin
  actually renders.

Scope is skins (`AddCharModel`). The wire format lives in `docs/protocol.md`.

## Requirements

- GTA: San Andreas 1.0 US with the SA-MP 0.3.7-R5 client.
- An ASI loader (for example `vorbisFile.dll`); drop `customskin.asi` next to `gta_sa.exe`.
- An open.mp server with artwork enabled and at least one `AddCharModel`.

## Build

A 32-bit (Win32) DLL with a static CRT, built with MSVC (works under Wine).

MSBuild:

```sh
msbuild build/customskin.vcxproj /p:Configuration=Release /p:Platform=Win32
```

or CMake:

```sh
cmake -B build/cmake -A Win32 -DCMAKE_BUILD_TYPE=Release
cmake --build build/cmake --config Release
```

The output is `customskin.asi`.

## Layout

`src/core` is logging and the samp.dll access layer. `src/net` has the RakNet/RPC hooks, the
DL masquerade, the DL packet compat, and the artwork RPC handlers. `src/model` is the
download worker, CRC32, and the file cache. `src/game` loads the RenderWare model and does the
clump swap that renders it. `vendor` holds urmem and the RakNet headers.

## Notes

The network patches are mapped to the exact 0.3.7-R5 build and refuse to run on any other, so
they need re-checking if you point them at a different client. Only skins are handled for now.

## References

- Protocol: [openmultiplayer/open.mp](https://github.com/openmultiplayer/open.mp) 
  `Shared/NetCode/custommodels.hpp`, `Server/Components/CustomModels/`.
- Packet-compat reference: [spmn/samp-client-compat](https://github.com/spmn/samp-client-compat).
- Hooking: [spmn/urmem](https://github.com/spmn/urmem).
- R5 addresses: [BlastHackNet/SAMP-API](https://github.com/BlastHackNet/SAMP-API).
- RenderWare and GTA SA addresses: [DK22Pac/plugin-sdk](https://github.com/DK22Pac/plugin-sdk).
- GTA SA class layouts: [AmyrAhmady/sampvoice](https://github.com/AmyrAhmady/sampvoice).
