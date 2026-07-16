# customskin-037

A client-side `.asi` plugin for **SA-MP 0.3.7-R5** (GTA: San Andreas) that adds **custom
player skin** support against an **open.mp** server  i.e. it lets a stock 0.3.7 client use
open.mp's 0.3DL "artwork" custom models without switching to the 0.3DL client.

> Status: **early / in development.** See `PROGRESS` below for what works. This is a
> reverse-engineering project; the network patches only run on the exact client build they
> were mapped for (0.3.7-R5). Do not point the network patches at another build without
> re-validating offsets.

## What it does (target design)

open.mp only sends custom models to clients that identify as **0.3DL**. This plugin:

1. **Masquerades as 0.3DL** during connect (client version `4062` + auth token
   `challenge ^ 4062`) so open.mp enables artwork for us.
2. **Speaks the artwork protocol**  receives the model list (`RPC 179`), downloads the
   `.dff`/`.txd` over HTTP (`User-Agent: SAMP/0.3`), CRC32-verifies them, and caches them.
3. **Loads the models into GTA SA**  registers custom skin IDs `20000..30000` into the
   streaming system and applies them to the right peds.

Milestone 1 = **skins only** (`AddCharModel`). Objects come later.

The protocol, RPC ids, version constants and cache layout are documented in
[`docs/protocol.md`](docs/protocol.md).

## Requirements

- GTA: San Andreas (1.0 US) + **SA-MP 0.3.7-R5** client.
- An **ASI loader** (Silent's ASI Loader / the `vorbisFile.dll` proxy). The user is already
  using `vorbisFile.dll`  drop `customskin.asi` next to `gta_sa.exe`.
- An **open.mp** server with `artwork.enable 1` and at least one `AddCharModel` in
  `models/artconfig.txt`.

## Build (MSVC, incl. under Wine)

This is a **32-bit (Win32/x86)** DLL. Static CRT (`/MT`) so it has no runtime dependency
inside the game process.

MSBuild:

```
msbuild build/customskin.vcxproj /p:Configuration=Release /p:Platform=Win32
```

or CMake with the MSVC toolchain:

```
cmake -B build/cmake -A Win32 -DCMAKE_BUILD_TYPE=Release
cmake --build build/cmake --config Release
```

The post-build step copies the resulting `customskin.dll` to `customskin.asi`.

## Layout

```
src/
  dllmain.cpp        ASI entry: waits for samp.dll, resolves R5, boots subsystems
  core/              logging, samp.dll access, R5 offset/signature table
  net/               (Phase 1+) RakNet/RPC hooks, DL masquerade, packet compat, artwork RPCs
  model/             (Phase 3+) download pipeline, CRC32, cache, streaming integration
vendor/
  urmem/             header-only hooking library (spmn/urmem)
docs/protocol.md     the open.mp artwork protocol reference
build/               msbuild project + CMake
```

## Credits / references

- Protocol: [openmultiplayer/open.mp](https://github.com/openmultiplayer/open.mp)
  `Shared/NetCode/custommodels.hpp`, `Server/Components/CustomModels/`.
- Packet-compat reference (the mirror problem): [spmn/samp-client-compat](https://github.com/spmn/samp-client-compat).
- Hooking: [spmn/urmem](https://github.com/spmn/urmem).
- R5 struct layouts / addresses: [BlastHackNet/SAMP-API](https://github.com/BlastHackNet/SAMP-API) (`multiver`).
- GTA SA class layouts: [AmyrAhmady/sampvoice](https://github.com/AmyrAhmady/sampvoice) `client/include/game`.
