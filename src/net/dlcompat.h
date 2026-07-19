// dl-format packet compatibility
#pragma once

#include <cstdint>

namespace dlcompat {

// install the dl-format handler hooks. guarded by samp::IsR5Build()
bool Init();

// the custom skin id (20000-30000) the server assigned to `playerId`, or 0 if none
uint32_t GetPlayerCustomSkin(uint16_t playerId);

// first captured custom-skin mapping (single-skin test helper). false if none
bool AnyCustomSkin(uint32_t& customIdOut);

// true if any player currently wears custom skin customId. the renderer only swaps bases a player wears
bool IsCustomAssigned(uint32_t customId);

} // namespace dlcompat
