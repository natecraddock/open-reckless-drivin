#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// #include "defines.h"
// #include "error.h"
// #include "gameframe.h"
// #include "gamesounds.h"
// #include "high.h"
// #include "input.h"
// #include "interface.h"
// #include "objects.h"
// #include "packs.h"
// #include "preferences.h"
// #include "register.h"
// #include "renderframe.h"
// #include "roads.h"
// #include "screen.h"
// #include "screenfx.h"
// #include "sprites.h"
// #include "textfx.h"
// #include "trig.h"

// tRoad gRoadData;
// uint32_t *gRoadLength;
// tRoadInfo *gRoadInfo;
// tLevelData *gLevelData;
// tTrackInfo *gTrackUp, *gTrackDown;
// tMarkSeg *gMarks;
// int gMarkSize;
// int gLevelID;
// tObject *gFirstObj, *gCameraObj, *gPlayerObj, *gSpikeObj, *gBrakeObj,
//     *gFirstVisObj, *gLastVisObj;
// tTrackSeg gTracks[kMaxTracks];
// int gTrackCount;
// int gPlayerLives, gExtraLives;
// int gNumMissiles, gNumMines;
// float gPlayerDeathDelay, gFinishDelay;
// int gPlayerScore, gDisplayScore;
// int gPlayerBonus;
// uint32_t gPlayerAddOns;
// float gGameTime;
// float gXDriftPos, gYDriftPos, gXFrontDriftPos, gYFrontDriftPos, gZoomVelo;
int gGameOn;
// int gPlayerCarID;
// float gPlayerSlide[4] = {0, 0, 0, 0};
// float gSpikeFrame;
// int gLCheat;

// int abs(int x);
// void CopClear();

// Ptr LoadObjs(Ptr dataPos) {
//   int i;
//   tObjectPos *objs = (tObjectPos *)(dataPos + sizeof(uint32_t));
//   for (i = 0; i < *(uint32_t *)dataPos; i++) {
//     tObject *theObj = NewObject(gFirstObj, objs[i].typeRes);
//     theObj->dir = objs[i].dir;
//     theObj->pos.x = objs[i].x;
//     theObj->pos.y = objs[i].y;
//   }
//   return (objs + *(uint32_t *)dataPos);
// }

// int NumLevels() {
//   int i = PACK_LEVEL_01;
//   SetResLoad(false);
//   for (i = 140; Get1Resource('Pack', i); i++) {
//   }
//   SetResLoad(true);
//   if (i == 140) {
//     i = 150;
//   }
//   return i - 140;
// }

// void GameEndSequence();

// int LoadLevel() {
//   int i, sound;
//   if (gLevelID >= PACK_ENCRYPTED - PACK_LEVEL_01 || gLevelResFile) {
//     if (!gRegistered) {
//       ShowPicScreen(1005);
//       WaitForPress();
//       BeQuiet();
//       InitInterface();
//       ShowCursor();
//       if (!gLCheat)
//         CheckHighScore(gPlayerScore);
//       return false;
//     }
//   }
//   gFirstObj = NewPtrClear(sizeof(tObject));
//   gFirstObj->next = gFirstObj;
//   gFirstObj->prev = gFirstObj;

//   if (gLevelID >= NumLevels()) {
//     GameEndSequence();
//     gLevelID = 0;
//   }

//   PACK_load(PACK_LEVEL_01 + gLevelID);
//   gLevelData = PACK_get_sorted_entry(PACK_LEVEL_01 + gLevelID, 1, NULL);
//   gMarks = PACK_get_sorted_entry(PACK_LEVEL_01 + gLevelID, 2, &gMarkSize);
//   gMarkSize /= sizeof(tMarkSeg);
//   gRoadInfo = PACK_get_sorted_entry(kPackRoad, gLevelData->roadInfo, NULL);
//   gTrackUp = (Ptr)gLevelData + sizeof(tLevelData);
//   gTrackDown =
//       (Ptr)gTrackUp + sizeof(uint32_t) + gTrackUp->num *
//       sizeof(tTrackInfoSeg);
//   gRoadLength = LoadObjs((Ptr)gTrackDown + sizeof(uint32_t) +
//                          gTrackDown->num * sizeof(tTrackInfoSeg));
//   gRoadData = (Ptr)gRoadLength + sizeof(uint32_t);

//   for (i = 0; i < 9; i++) {
//     if ((*gLevelData).objGrps[i].resID) {
//       InsertObjectGroup((*gLevelData).objGrps[i]);
//     }
//   }

//   gPlayerObj = NewObject(gFirstObj, gRoadInfo->water ? kNormalPlayerBoatID :
//                  gPlayerCarID);
//   gPlayerObj->pos.x = gLevelData->xStartPos;
//   gPlayerObj->pos.y = 500;
//   gPlayerObj->control = kObjectDriveUp;
//   gPlayerObj->target = 1;
//   gCameraObj = gPlayerObj;
//   gPlayerBonus = 1;
//	gPlayerObj=NULL; 	Uncomment this line to make the player car ai
// controlled
//   gSpikeObj = NULL;
//   gBrakeObj = NULL;
//   CopClear();
//   SortObjects();

//   gGameTime = 0;
//   gTrackCount = 0;
//   gPlayerDeathDelay = 0;
//   gFinishDelay = 0;
//   gPlayerBonus = 1;
//   gDisplayScore = gPlayerScore;
//   gXDriftPos = 0;
//   gYDriftPos = 0;
//   gXFrontDriftPos = 0;
//   gYFrontDriftPos = 0;
//   gZoomVelo = kMaxZoomVelo;
//   ClearTextFX();
//   StartCarChannels();
//   gScreenBlitSpecial = true;
//   return true;
// }

// void DisposeLevel() {
//   PACK_unload(PACK_LEVEL_01 + gLevelID);
//   gPlayerObj = NULL;
//   while ((tObject *)gFirstObj->next != gFirstObj) {
//     SpriteUnused((*(tObject *)gFirstObj->next).frame);
//     RemoveObject((tObject *)gFirstObj->next);
//   }
//   DisposePtr(gFirstObj);
// }

// void GetLevelNumber() {
//   DialogPtr cheatDlg;
//   short type;
//   Rect box;
//   Handle item;
//   short hit;
//   long num;
//   Str255 text;
//   if (false gOSX ) {
//     FadeScreen(1);
//     ScreenMode(kScreenSuspended);
//     FadeScreen(0);
//   }
//   cheatDlg = GetNewDialog(129, NULL, (WindowPtr)-1);
//   DoError(SetDialogDefaultItem(cheatDlg, 1));
//   do {
//     ModalDialog(NULL, &hit);
//   } while (hit != 1);
//   GetDialogItem(cheatDlg, 2, &type, &item, &box);
//   GetDialogItemText(item, text);
//   StringToNum(text, &num);
//   gLevelID = num - 1;
//   if (gLevelID >= NumLevels()) {
//     gLevelID = 0;
//   }
//   GetDialogItem(cheatDlg, 5, &type, &item, &box);
//   GetDialogItemText(item, text);
//   StringToNum(text, &num);
//   gPlayerCarID = num;
//   DisposeDialog(cheatDlg);
//   if (false gOSX) {
//     FadeScreen(1);
//     ScreenMode(kScreenRunning);
//     FadeScreen(512);
//   }
// }

// void StartGame(int lcheat) {
//   DisposeInterface();
//   gPlayerLives = 3;
//   gExtraLives = 0;
//   gPlayerAddOns = 0;
//   gPlayerDeathDelay = 0;
//   gFinishDelay = 0;
//   gPlayerScore = 0;
//   gLevelID = 0;
//   gPlayerCarID = kNormalPlayerCarID;
//   gNumMissiles = 0;
//   gNumMines = 0;
//   gGameOn = true;
//   gEndGame = false;
//   if (lcheat) {
//     GetLevelNumber();
//   }
//   gLCheat = lcheat;
//   FadeScreen(1);
//   HideCursor();
//   ScreenMode(kScreenRunning);
//   InputMode(kInputRunning);
//   if (LoadLevel()) {
//     ScreenClear();
//     FadeScreen(512);
//     RenderFrame();
//     InitFrameCount();
//   }
// }

// void EndGame() {
// so RenderFrame will not draw Panel.    gPlayerLives = 0;
//   RenderFrame();
//   DisposeLevel();
//   BeQuiet();
//   SimplePlaySound(152);
//   GameOverAnim();
//   InitInterface();
//   ShowCursor();
//   if (!gLCheat) {
//     CheckHighScore(gPlayerScore);
//   }
// }
