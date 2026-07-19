// clump-swap renderer (phase 4.3 -> production per-base)
//
// per frame (hooked on CNetGame::Process), for every custom model that is downloaded and
// renderware-loaded AND currently assigned to some player, point that model's base template
// clump at our loaded custom clump. peds the game builds afterwards (players on spawn) clone
// the custom clump through the game's own creation path, so they render the custom model with
// full ped setup - we never re-instance a live ped (that corrupts anim/skeleton refs and
// crashes). the base's own clump is saved so it can be restored on reconnect/gmx/shutdown
#pragma once

namespace render {

// install the per-frame tick. guarded by samp::IsR5Build()
bool Init();

// restore every swapped base template to the game's original clump. call before the custom
// clumps are freed (reconnect / gmx / shutdown) so no base template is left dangling
void RestoreAll();

// apply the base-template swap for one custom skin id right now, if its model is downloaded.
// called the instant a skin is assigned (before the game rebuilds the ped) so the custom
// renders on the first assignment instead of only after a second one. no-op if not ready
void EnsureSwapForCustom(unsigned customId);

} // namespace render
