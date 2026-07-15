// customskin-037 local test gamemode (open.mp).
//
// Defines one custom player skin (id 20001, base 1). Its model files skin.dff /
// skin.txd live in ../models/ and are served by open.mp's artwork webserver, so a
// DL client (our customskin.asi masquerading as 0.3DL) receives RPC 179 for it and
// downloads them from http://<public_addr>:<artwork.port>/.
//
// Compile to gamemodes/test.amx (see test/README.md) with a recent omp-stdlib.

#include <open.mp>

#define CUSTOM_SKIN_ID   20001
#define CUSTOM_SKIN_BASE 1

public OnGameModeInit()
{
    SetGameModeText("customskin-037 test");

    // Register the custom skin. open.mp sends RPC 179 (ModelRequest) to DL clients.
    if (AddCharModel(CUSTOM_SKIN_BASE, CUSTOM_SKIN_ID, "skin.dff", "skin.txd"))
        print("[test] AddCharModel 20001 (base 1) OK");
    else
        print("[test] AddCharModel FAILED - check models/skin.dff and models/skin.txd exist");

    // A spawn class wearing the custom skin: this is the Phase 4 render target, and it
    // also exercises the DL-format class/spawn RPCs that Phase 2b must handle.
    AddPlayerClass(CUSTOM_SKIN_ID, 1958.3783, 1343.1572, 15.3746, 269.1425, 0, 0, 0, 0, 0, 0);
    return 1;
}

public OnPlayerConnect(playerid)
{
    SendClientMessage(playerid, 0xFFFFFFFF, "customskin-037 test: custom skin 20001 should download");
    return 1;
}

public OnPlayerRequestClass(playerid, classid)
{
    SetPlayerPos(playerid, 1958.3783, 1343.1572, 15.3746);
    SetPlayerFacingAngle(playerid, 269.1425);
    SetPlayerCameraPos(playerid, 1960.5, 1343.1572, 15.9);
    SetPlayerCameraLookAt(playerid, 1958.3783, 1343.1572, 15.3746);
    return 1;
}

// open.mp artwork callbacks - handy server-side visibility during testing.
public OnPlayerRequestDownload(playerid, type, crc)
{
    printf("[test] player %d requests download type=%d crc=0x%08x", playerid, type, crc);
    return 1; // allow the default webserver download
}

public OnPlayerFinishedDownloading(playerid, virtualworld)
{
    printf("[test] player %d finished downloading (vw %d)", playerid, virtualworld);
    return 1;
}

public OnPlayerSpawn(playerid)
{
    SetPlayerSkin(playerid, CUSTOM_SKIN_ID);
    SendClientMessage(playerid, 0xFFFFFFFF, "spawned with custom skin 20001");
    return 1;
}
