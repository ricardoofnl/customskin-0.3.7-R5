# Local test server (open.mp)

A self-contained **open.mp** server that defines one custom skin (id `20001`, base `1`) and
serves its files over the artwork webserver on `127.0.0.1`, so you can test
`customskin.asi` end to end locally.

```
test/
  config.json          open.mp config: artwork enabled, public_addr 127.0.0.1, loads test.amx
  gamemodes/test.pwn   registers AddCharModel(1, 20001, "skin.dff", "skin.txd") + a class
  models/skin.dff      RANDOM placeholder (git-ignored; regenerate with make-models.sh)
  models/skin.txd      RANDOM placeholder
  make-models.sh       regenerate the placeholder model files
```

## Prerequisites

- An **open.mp server** build (`omp-server` + its `components/` folder). Linux-native
  `omp-server` running directly on Fedora is simplest — the Wine GTA client reaches it via
  `127.0.0.1`. (A Windows `omp-server.exe` under Wine works too.)
- A Pawn compiler with a **recent omp-stdlib** (the `qawno/` that ships with open.mp), so
  `AddCharModel` and the artwork callbacks are declared.

## Run it

1. Compile the gamemode to `gamemodes/test.amx`:
   ```
   # from the open.mp qawno/ dir, or with pawncc + omp-stdlib includes:
   pawncc test/gamemodes/test.pwn -o test/gamemodes/test.amx -i/path/to/omp-stdlib
   ```
2. Make this folder the server's working dir (copy `omp-server` + `components/` here, or copy
   `config.json` + `gamemodes/` + `models/` into your open.mp install).
3. Start the server:
   ```
   ./omp-server            # from the test/ folder
   ```
   Expected server log lines:
   - `[test] AddCharModel 20001 (base 1) OK`
   - `[artwork:crc] skin.dff CRC = 0x...` and `skin.txd CRC = 0x...` (from `show_crc_logs`)
   - `Web server is running on http://127.0.0.1:7777/`
4. Launch GTA SA + SA-MP 0.3.7-R5 with `customskin.asi`, connect to **127.0.0.1:7777**.

## What to expect on the client (`customskin.log`)

- At load: `masq: DL masquerade ENABLED`.
- On connect: `rak: ConnectionSucceeded`, then
  `artwork: RPC179 ModelRequest #0/1 type=1 ... base=1 new=20001 dff=0x... txd=0x...`
  -> proves open.mp is treating us as 0.3DL and sent the skin list.
- (Phase 3) `artwork: RPC183 ModelUrl ... url="http://127.0.0.1:7777/skin.dff"` and the file
  downloading + CRC32 matching the server's `show_crc_logs` value.

## Notes

- `skin.dff` / `skin.txd` are **random placeholders** — enough to test masquerade + download +
  CRC (Phases 2b/3). They are not valid RenderWare models, so they will **not render**
  (Phase 4). For real rendering, drop a genuine custom skin's `.dff`/`.txd` in `models/`
  (keep the same names, or update `test.pwn`).
- Regenerate placeholders: `bash test/make-models.sh`.
- The webserver only serves clients that are connected players and send
  `User-Agent: SAMP/0.3` — our downloader (Phase 3) does exactly that.
