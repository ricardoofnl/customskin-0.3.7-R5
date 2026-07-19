// resident RenderWare model loader (phase 4.1)
//
// reads a downloaded .dff into an RpClump and its .txd into an RwTexDictionary,
// binding textures. the handles are kept resident (we never stream these) and used
// by the clump-swap phases. this does not touch the game's model pool
#pragma once

namespace rwmodel {

struct Loaded {
    void* clump = nullptr; // RpClump*
    void* txd = nullptr;   // RwTexDictionary*
    bool ok() const { return clump && txd; }
};

// load txd first (for texture binding), then the clump. safe on invalid files:
// RwStreamFindChunk simply won't find the chunk and returns nullptr
Loaded LoadModel(const char* dffPath, const char* txdPath);

// destroy loaded rw objects
void Free(Loaded& m);

// load a fresh clump into m (binding the existing m.txd) and assign m.clump, WITHOUT freeing
// the previous m.clump - the caller uses this when the old clump was already destroyed for
// us (e.g. the streamer unloaded the base model whose template we had borrowed). returns ok
bool ReloadClump(Loaded& m, const char* dffPath);

// ask the game streamer to load a model by id (CStreaming::RequestModel). used to keep a
// custom skin's base ped model resident so the clump swap installs before the ped is rebuilt
void RequestModel(int modelId);

} // namespace rwmodel
