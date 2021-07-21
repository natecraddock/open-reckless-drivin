#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "defines.h"
#include "interface.h"
#include "packs.h"
#include "random.h"
#include "sprites.h"
#include "trig.h"
// #include "error.h"
// #include "gameinitexit.h"
// #include "gamesounds.h"
// #include "input.h"
// #include "preferences.h"
// #include "register.h"
// #include "screen.h"
// #include <Appearance.h>
// #include <DrawSprocket.h>
// #include <InputSprocket.h>
// #include <Sound.h>

float sine_table[SINE_TABLE_SIZE];

#undef sin
void InitTrig() {
  int i;
  for (i = 0; i < SINE_TABLE_SIZE; i++) {
    sine_table[i] = sin(2 * PI * (float)i / (float)SINE_TABLE_SIZE);
  }
}

bool Init() {
  Randomize();
  // LoadPrefs();
  // CheckRegi();
  // if (!gRegistered) {
  //   Register(false);
  // }
  // InitScreen(0);
  ShowPicScreen(1003);
  LoadPack(PACK_SOUNDS);
  LoadPack(PACK_OBJECT_TYPE);
  LoadPack(PACK_OBJECT_GROUPS);
  LoadPack(PACK_ROAD);
  // if (gPrefs.hiColor) {
  LoadPack(PACK_RLE_16);
  LoadPack(PACK_cRLE_16);
  LoadPack(PACK_TEXTURES_16);
  // }
  // else {
  LoadPack(PACK_RLE);
  LoadPack(PACK_cRLE);
  LoadPack(PACK_TEXTURES);
  // }
  LoadSprites();
  InitTrig();
  // InitInput();
  // SetGameVolume(-1);
  // InitChannels();
  // InitInterface();

  /* TODO: exit early on failure */
  return true;
}

static void free_packs() {
  UnloadPack(PACK_SOUNDS);
  UnloadPack(PACK_OBJECT_TYPE);
  UnloadPack(PACK_OBJECT_GROUPS);
  UnloadPack(PACK_ROAD);
  UnloadPack(PACK_RLE_16);
  UnloadPack(PACK_cRLE_16);
  UnloadPack(PACK_TEXTURES_16);
  UnloadPack(PACK_RLE);
  UnloadPack(PACK_cRLE);
  UnloadPack(PACK_TEXTURES);

  UnloadSprites();
}

void Exit() {
  /* Cleanup all loaded packs */
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
