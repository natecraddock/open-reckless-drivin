#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "defines.h"
#include "interface.h"
#include "packs.h"
#include "preferences.h"
#include "random.h"
#include "register.h"
#include "sprites.h"
#include "trig.h"
// #include "error.h"
// #include "gameinitexit.h"
// #include "gamesounds.h"
// #include "input.h"
// #include "screen.h"
// #include <Appearance.h>
// #include <DrawSprocket.h>
// #include <InputSprocket.h>
// #include <Sound.h>

bool Init() {
  Randomize();
  InitTrig();
  PREFS_load_preferences();

  if (!REG_check_registration()) {
    printf("Invalid registration %s %s\n", gPrefs.name, gPrefs.code);
    // Register(false);
  }
  else {
    printf("Registered to %s\n", gPrefs.name);
  }
  // InitScreen(0);
  ShowPicScreen(1003);
  PACK_load(PACK_SOUNDS);
  PACK_load(PACK_OBJECT_TYPE);
  PACK_load(PACK_OBJECT_GROUPS);
  PACK_load(PACK_ROAD);
  if (gPrefs.full_color) {
    PACK_load(PACK_RLE_16);
    PACK_load(PACK_cRLE_16);
    PACK_load(PACK_TEXTURES_16);
  }
  else {
    PACK_load(PACK_RLE);
    PACK_load(PACK_cRLE);
    PACK_load(PACK_TEXTURES);
  }
  LoadSprites();
  // InitInput();
  // SetGameVolume(-1);
  // InitChannels();
  // InitInterface();

  // TODO: exit early on failure
  return true;
}

static void free_packs() {
  PACK_unload(PACK_SOUNDS);
  PACK_unload(PACK_OBJECT_TYPE);
  PACK_unload(PACK_OBJECT_GROUPS);
  PACK_unload(PACK_ROAD);
  PACK_unload(PACK_RLE_16);
  PACK_unload(PACK_cRLE_16);
  PACK_unload(PACK_TEXTURES_16);
  PACK_unload(PACK_RLE);
  PACK_unload(PACK_cRLE);
  PACK_unload(PACK_TEXTURES);

  UnloadSprites();
}

void Exit() {
  // Cleanup all loaded packs
  free_packs();
  //   WritePrefs(false);
  //   FadeScreen(1);
  //   ScreenMode(kScreenSuspended);
  //   SetSystemVolume();
  //   FadeScreen(256);
  //   FadeScreen(0);
  //   ScreenMode(kScreenStopped);
  //   InputMode(kInputStopped);
}
