#!/usr/bin/env bash
# Regenerate placeholder model files for the test server.
#
# These are RANDOM bytes - enough to exercise the artwork protocol end to end
# (RPC 179 -> RequestDFF/TXD -> ModelUrl -> HTTP download -> CRC32 verify), but
# they are NOT valid RenderWare models, so they will not actually render in game.
# For Phase 4 (real rendering) replace models/skin.dff and models/skin.txd with a
# REAL custom skin's files (e.g. from any 0.3DL skin pack).
set -euo pipefail
cd "$(dirname "$0")/models"
head -c 100000 /dev/urandom > skin.dff
head -c 50000  /dev/urandom > skin.txd
# second skin (distinct random bytes -> distinct CRC) for the two-model protocol test
head -c 100000 /dev/urandom > skin2.dff
head -c 50000  /dev/urandom > skin2.txd
echo "generated: models/skin.dff + skin.txd, models/skin2.dff + skin2.txd (random placeholders)"
