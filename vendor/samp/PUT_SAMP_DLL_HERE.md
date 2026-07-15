# Local samp.dll (not committed)

Drop your **0.3.7-R5** `samp.dll` here as `vendor/samp/samp.dll` so the toolchain can
disassemble it while resolving byte-patch sites (version/token for the DL masquerade,
DL-format packet read sites, etc.). It is git-ignored (`*.dll`) and never committed.

Validated fingerprint for this project: SizeOfImage `0x27E000`, TimeDateStamp `0x6372C39E`.

```
cp "/path/to/GTA San Andreas/samp.dll" vendor/samp/samp.dll
```
