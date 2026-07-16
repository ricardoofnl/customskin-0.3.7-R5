// clump-swap renderer (phase 4.3)
//
// per frame (hooked on CNetGame::Process), if the local player has been assigned a custom
// skin whose model is loaded, apply that custom clump to the player's ped: temporarily point
// the base model's clump at our loaded custom clump, re-instance the ped (which clones it),
// then restore - so only that ped wears the custom model, and the base model is untouched
#pragma once

namespace render {

// install the per-frame tick. guarded by samp::IsR5Build()
bool Init();

} // namespace render
