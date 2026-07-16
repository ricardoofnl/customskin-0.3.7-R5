// dl-format packet compatibility (phase 2b, scoped to skins for now)
//
// while masquerading as 0.3DL, open.mp sends dl-format RPCs. their layout differs from what
// stock 0.3.7 parses. we hook the affected handlers, rewrite the payload in place to the
// 0.3.7 layout (so sa-mp applies a valid base skin instead of warning), and capture the
// custom skin id so the clump-swap phase knows which model each player should wear
#pragma once

#include <cstdint>

namespace dlcompat {

// install the dl-format handler hooks. guarded by samp::IsR5Build()
bool Init();

// the custom skin id (20000-30000) the server assigned to `playerId`, or 0 if none
uint32_t GetPlayerCustomSkin(uint16_t playerId);

// first captured custom-skin mapping (single-skin test helper). false if none
bool AnyCustomSkin(uint32_t& customIdOut);

// true if any connected player is currently assigned custom skin `customId`. the renderer
// uses this to only swap base templates that a real player wears (limits ambient bleed)
bool IsCustomAssigned(uint32_t customId);

} // namespace dlcompat
