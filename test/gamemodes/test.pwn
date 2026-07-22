// customskin-037 local test gamemode (open.mp)
//
// defines two custom player skins on DIFFERENT base ids, to exercise the per-base
// multi-model swap:
//   skin A: id 20001, base 1  <- models/skin.dff  + models/skin.txd
//   skin B: id 20002, base 2  <- models/skin2.dff + models/skin2.txd
// their model files are served by open.mp's artwork webserver, so a dl client (our
// customskin.asi masquerading as 0.3DL) receives rpc 179 for each and downloads them.
//
// on spawn the player wears a NORMAL skin - custom skins are applied only on demand with
// /skin1 or /skin2, so the custom render is isolated from the spawn flow.
//
// note: skin2.dff/skin2.txd default to a copy of skin.dff/skin.txd (so it renders out of
// the box, just looking the same). drop a DIFFERENT real 0.3DL skin's files in as
// models/skin2.* to see two visually distinct skins.
//
// compile to gamemodes/test.amx (see test/readme.md) with a recent omp-stdlib

#include <open.mp>

#define SKIN_A_ID   20001
#define SKIN_A_BASE 1
#define SKIN_B_ID   20002
#define SKIN_B_BASE 2

// custom OBJECT (AddSimpleModel, type 0). NOTE: open.mp object custom ids must be NEGATIVE,
// in the range -1000 .. -30000 (see open.mp AddSimpleModel docs). OBJ_BASE is an existing
// object model whose properties are cloned. object.dff/object.txd live in test/models.
#define OBJ_ID   -1001
#define OBJ_BASE 1225

// normal spawn skin (cj). deliberately NOT a custom base id (1/2) so it never picks up a
// swapped template
#define DEFAULT_SKIN 0

// ls international airport (terminal apron, on the ground, facing the doors)
#define SPAWN_X 1685.9645
#define SPAWN_Y -2329.8933
#define SPAWN_Z 13.5469
#define SPAWN_A 180.0

public OnGameModeInit()
{
    SetGameModeText("customskin-037 test");

    // register both custom skins. open.mp sends rpc 179 (ModelRequest) to dl clients
    if (AddCharModel(SKIN_A_BASE, SKIN_A_ID, "skin.dff", "skin.txd"))
        print("[test] AddCharModel 20001 (base 1) OK");
    else
        print("[test] AddCharModel 20001 FAILED - check models/skin.dff + skin.txd");

    if (AddCharModel(SKIN_B_BASE, SKIN_B_ID, "skin2.dff", "skin2.txd"))
        print("[test] AddCharModel 20002 (base 2) OK");
    else
        print("[test] AddCharModel 20002 FAILED - check models/skin2.dff + skin2.txd");

    // register a custom object and place one next to the spawn so it renders on connect
    if (AddSimpleModel(-1, OBJ_BASE, OBJ_ID, "object.dff", "object.txd"))
        print("[test] AddSimpleModel -1001 (base 1225) OK");
    else
        print("[test] AddSimpleModel -1001 FAILED - check base id / models/object.*");
    CreateObject(OBJ_ID, SPAWN_X + 3.0, SPAWN_Y, SPAWN_Z, 0.0, 0.0, 0.0);

    // one spawn class with the normal skin, at ls airport
    AddPlayerClass(DEFAULT_SKIN, SPAWN_X, SPAWN_Y, SPAWN_Z, SPAWN_A);
    return 1;
}

public OnPlayerConnect(playerid)
{
    SendClientMessage(playerid, 0xFFFFFFFF,
        "customskin-037 test: spawn with a normal skin, then type /skin1 or /skin2 for a custom skin");
    return 1;
}

public OnPlayerRequestClass(playerid, classid)
{
    SetPlayerPos(playerid, SPAWN_X, SPAWN_Y, SPAWN_Z);
    SetPlayerFacingAngle(playerid, SPAWN_A);
    // camera in front of the player (facing angle 180 = looking toward -Y) for a preview
    SetPlayerCameraPos(playerid, SPAWN_X, SPAWN_Y - 3.0, SPAWN_Z + 0.7);
    SetPlayerCameraLookAt(playerid, SPAWN_X, SPAWN_Y, SPAWN_Z);
    return 1;
}

public OnPlayerCommandText(playerid, cmdtext[])
{
    // custom skins are opt-in via command. the base differs (1 vs 2), so the per-base swap
    // renders each one
    if (strcmp(cmdtext, "/skin1", true) == 0 || strcmp(cmdtext, "/a", true) == 0)
    {
        SetPlayerSkin(playerid, SKIN_A_ID);
        SendClientMessage(playerid, 0xFFFFFFFF, "switched to custom skin 20001 (base 1)");
        return 1;
    }
    if (strcmp(cmdtext, "/skin2", true) == 0 || strcmp(cmdtext, "/b", true) == 0)
    {
        SetPlayerSkin(playerid, SKIN_B_ID);
        SendClientMessage(playerid, 0xFFFFFFFF, "switched to custom skin 20002 (base 2)");
        return 1;
    }
    if (strcmp(cmdtext, "/normal", true) == 0)
    {
        SetPlayerSkin(playerid, DEFAULT_SKIN);
        SendClientMessage(playerid, 0xFFFFFFFF, "back to the normal skin");
        return 1;
    }
    return 0;
}

// open.mp artwork callbacks - handy server-side visibility during testing
public OnPlayerRequestDownload(playerid, DOWNLOAD_REQUEST:type, crc)
{
    printf("[test] player %d requests download type=%d crc=0x%08x", playerid, _:type, crc);
    return 1; // allow the default webserver download
}

public OnPlayerFinishedDownloading(playerid, virtualworld)
{
    printf("[test] player %d finished downloading (vw %d)", playerid, virtualworld);
    return 1;
}

public OnPlayerSpawn(playerid)
{
    // always spawn with the normal skin at ls airport; force the position so the spawn is
    // definitely there (not only during class selection)
    SetPlayerSkin(playerid, DEFAULT_SKIN);
    SetPlayerPos(playerid, SPAWN_X, SPAWN_Y, SPAWN_Z);
    SetPlayerFacingAngle(playerid, SPAWN_A);
    SendClientMessage(playerid, 0xFFFFFFFF,
        "spawned at ls airport with the normal skin - type /skin1 or /skin2 for a custom skin");
    return 1;
}
