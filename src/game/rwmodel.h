// resident RenderWare model loader
#pragma once

namespace rwmodel {

struct Loaded {
    void* clump = nullptr; // RpClump*
    void* txd = nullptr;   // RwTexDictionary*
    bool ok() const { return clump && txd; }
};

// load the txd (for texture binding) then the clump. safe on junk files: no chunk found -> null
Loaded LoadModel(const char* dffPath, const char* txdPath);

// destroy loaded rw objects
void Free(Loaded& m);

// load a fresh clump (binding m.txd) into m.clump without freeing the old one - used after the
// streamer already destroyed it. returns ok
bool ReloadClump(Loaded& m, const char* dffPath);

// ask the streamer to load a model (CStreaming::RequestModel), keeping a custom skin's base ped resident
void RequestModel(int modelId);

} // namespace rwmodel
