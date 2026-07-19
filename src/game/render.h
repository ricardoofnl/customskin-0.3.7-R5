// clump-swap renderer
#pragma once

namespace render {

// install the per-frame tick. guarded by samp::IsR5Build()
bool Init();

// restore every swapped base template to its original clump. call before freeing custom clumps (reconnect/gmx)
void RestoreAll();

// swap in one custom skin's clump right now - called the moment a skin is assigned (before the ped
// rebuild) so it renders on the first try, not the second. no-op if not downloaded yet
void EnsureSwapForCustom(unsigned customId);

} // namespace render
