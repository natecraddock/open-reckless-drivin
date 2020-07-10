#include <stdint.h>

#include "gameinitexit.h"
#include "gamesounds.h"
#include "input.h"
#include "objects.h"
#include "packs.h"
#include "random.h"
#include "renderframe.h"
#include "roads.h"
#include "screen.h"
#include "textfx.h"
#include <timer.h>

#define kCalcFPMS (kCalcFPS / (float)1000000)
#define kGraphFrameCount 12
#define kDisplayScoreIncr 360
#define kZoomAcceleration 80.0

#define kExtraLiveScore 5000
#define kPenaltyTime 0 // penalty seconds for loosing a life.
#define kMinCopDist                                                            \
  1200 // minumum distance resurrecting player has from cops in pixels
uint64_t gStartMS, gPauseMS, gLastGraphFrameMS[kGraphFrameCount];
unsigned long gFrameCount, gGraphFrameCount;
int gEndGame;

void InitFrameCount() {
  gFrameCount = 0;
  gStartMS = GetMSTime();
}

void PauseFrameCount() {
  gPauseMS = GetMSTime();
}

void ResumeFrameCount() {
  gStartMS += GetMSTime() - gPauseMS;
}

inline Boolean CheckFrameTime() {
  unsigned long optFrameCount;
  uint64_t curMS;

  curMS = GetMSTime() - gStartMS;
  optFrameCount = curMS * kCalcFPMS;
  if (gFrameCount > optFrameCount) {
    // AddFloatToMessageBuffer("\pFPS:
    // ",(float)1000000/((curMS-gLastGraphFrameMS[0])/kGraphFrameCount));
    BlockMoveData(gLastGraphFrameMS + 1, gLastGraphFrameMS,
                  sizeof(uint64_t) * (kGraphFrameCount)-1);
    gLastGraphFrameMS[kGraphFrameCount - 1] = curMS;
    return true;
  }
  return false;
}

inline CheckTimeSkip() {
  unsigned long optFrameCount;
  uint64_t curMS;
  do {
    curMS = GetMSTime() - gStartMS;
    optFrameCount = curMS * kCalcFPMS;
  } while (gFrameCount > optFrameCount);
}

t2DPoint GetUniquePos(int16_t minOffs, int16_t maxOffs, float *objDir,
                      int *dir);

void CopClear() {
  tObject *theObj = (tObject *)(gFirstObj->next);
  while (theObj != gFirstObj) {
    if (theObj->type->flags & kObjectCop) {
      while (fabs(theObj->pos.y - gPlayerObj->pos.y) < kMinCopDist) {
        theObj->dir = -1;
        theObj->pos = GetUniquePos(20, 200, &theObj->dir, &theObj->control);
      }
    }
    theObj = (tObject *)theObj->next;
  }
}

void ResurrectPlayer() {
  int i;
  for (i = 0; (i < gTrackUp->num) && (gTrackUp->track[i].y < gPlayerObj->pos.y);
       i++)
    ;
  gPlayerObj = NewObject(gFirstObj,
                         gRoadInfo->water ? kNormalPlayerBoatID : gPlayerCarID);
  gPlayerObj->pos.x = gTrackUp->track[i - 1].x;
  gPlayerObj->pos.y = gTrackUp->track[i - 1].y;
  gPlayerObj->control = kObjectDriveUp;
  if (gPlayerObj->pos.y >= 500) {
    gPlayerObj->target = i;
    {
      t2DPoint targetPos = P2D(gTrackUp->track[i].x, gTrackUp->track[i].y);
      if (gPlayerObj->pos.y - targetPos.y)
        gPlayerObj->dir = atan((gPlayerObj->pos.x - targetPos.x) /
                               (gPlayerObj->pos.y - targetPos.y));
      else
        gPlayerObj->dir = 0;
    }
  }
  else {
    gPlayerObj->pos.x = gLevelData->xStartPos;
    gPlayerObj->pos.y = 500;
    gPlayerObj->target = 1;
  }
  gCameraObj = gPlayerObj;
  gScreenBlitSpecial = true;
  gZoomVelo = kMaxZoomVelo;
  gGameTime += kPenaltyTime;
  CopClear();
}

int abs(int);

void PlayerHandling() {
  // weapons handling and engine sound.
  if (gPlayerDeathDelay) {
    gPlayerDeathDelay -= kFrameDuration;
    if (gPlayerDeathDelay <= 0) {
      gPlayerDeathDelay = 0;
      if (gPlayerLives)
        ResurrectPlayer();
      else
        gEndGame = true;
    }
    SetCarSound(-1, 0, 0, 0);
    FFBDirect(0, 0);
  }
  else {
    if (!gCameraObj->jumpHeight) {
      float lMag = (gPlayerSlide[0] + gPlayerSlide[2]) * 0.1;
      float rMag = (gPlayerSlide[1] + gPlayerSlide[3]) * 0.1;
      SetCarSound(
          fabs(gCameraObj->throttle) +
              (gRoadInfo->water ? gCameraObj->input.brake : 0),
          (gPlayerSlide[0] + gPlayerSlide[2]) * 0.5,
          (gPlayerSlide[1] + gPlayerSlide[3]) * 0.5,
          VEC2D_DotProduct(gCameraObj->velo,
                           P2D(sin(gCameraObj->dir), cos(gCameraObj->dir))));
      if (gRoadInfo->skidSound != 145)
        FFBDirect(lMag > 0.3 ? 0.3 : lMag, rMag > 0.3 ? 0.3 : rMag);
      else
        FFBDirect(0, 0);
    }
    else {
      SetCarSound(fabs(gCameraObj->throttle) +
                      (gRoadInfo->water ? gCameraObj->input.brake : 0),
                  0, 0, fabs(gCameraObj->input.throttle) * 71);
      FFBDirect(0, 0);
    }
    if (!gPlayerObj->jumpHeight) {
      if (gFire && gNumMines) {
        FireWeapon(gPlayerObj, (*gRoadInfo).water ? 254 : 206);
        gNumMines--;
        FFBJolt(1.0, 1.0, 0.1);
      }
      if (gMissile && gNumMissiles) {
        FireWeapon(gPlayerObj, (*gRoadInfo).water ? 221 : 162);
        gNumMissiles--;
        FFBJolt(0.8, 0.8, 0.8);
      }
    }
  }
  // time handling.
  if (gFinishDelay)
    gFinishDelay += kFrameDuration;
  else
    gGameTime += kFrameDuration;
  if (!(gFrameCount % (int)(kCalcFPS / 10))) {
    if (gGameTime > gLevelData->time)
      if (gPlayerScore)
        gPlayerScore -= 1;
    if (!(gFrameCount % (int)kCalcFPS) && !gFinishDelay)
      if (gGameTime >= gLevelData->time - 10) {
        if (gGameTime < gLevelData->time)
          SimplePlaySound(144);
        if ((int)gGameTime == (int)gLevelData->time) {
          tTextEffect fx = {320, 240, kEffectSinLines + kEffectMoveDown, 0,
                            "\pTIMEhUPee"};
          NewTextEffect(&fx);
          SimplePlaySound(149);
        }
      }
  }
  // score handling
  if (gDisplayScore < gPlayerScore) {
    gDisplayScore += kDisplayScoreIncr * kFrameDuration;
    if (gDisplayScore > gPlayerScore)
      gDisplayScore = gPlayerScore;
  }
  if (gDisplayScore > gPlayerScore)
    gDisplayScore = gPlayerScore;
  if (gPlayerScore > (gExtraLives + 1) * kExtraLiveScore) {
    tTextEffect fx = {320, 240, kEffectSinLines + kEffectMoveUp, 0,
                      "\pEXTRAhLIFEee"};
    NewTextEffect(&fx);
    gExtraLives++;
    gPlayerLives++;
    SimplePlaySound(154);
  }
  // finish detection
  if (gCameraObj->pos.y < 320)
    gCameraObj->pos.y = 320;
  if (gPlayerObj->pos.y > gLevelData->levelEnd)
    if (!gPlayerDeathDelay) {
      tTextEffect fx = {320, 240, kEffectExplode, 0, "\pLEVELhCOMPLETED"};
      NewTextEffect(&fx);
      if (!gFinishDelay)
        gFinishDelay = 0.001;
    }
  // background drift
  gXDriftPos += (*gRoadInfo).xDrift * kFrameDuration;
  gYDriftPos += (*gRoadInfo).yDrift * kFrameDuration;
  gXFrontDriftPos += (*gRoadInfo).xFrontDrift * kFrameDuration;
  gYFrontDriftPos += (*gRoadInfo).yFrontDrift * kFrameDuration;
  // camera distance
  if (gZoomVelo > VEC2D_Value(gPlayerObj->velo)) {
    gZoomVelo -= kZoomAcceleration * kFrameDuration;
    if (gZoomVelo < VEC2D_Value(gPlayerObj->velo))
      gZoomVelo = VEC2D_Value(gPlayerObj->velo);
  }
  else {
    gZoomVelo += kZoomAcceleration * kFrameDuration;
    if (gZoomVelo > VEC2D_Value(gPlayerObj->velo))
      gZoomVelo = VEC2D_Value(gPlayerObj->velo);
  }
  if (gZoomVelo > kMaxZoomVelo)
    gZoomVelo = kMaxZoomVelo;
  // spike handling
  gSpikeFrame += VEC2D_Value(gPlayerObj->velo) / 20;
  if (gPlayerAddOns & kAddOnSpikes && !(gPlayerDeathDelay != 0) &&
      !(*gRoadInfo).water) {
    if (!gSpikeObj)
      gSpikeObj = NewObject(gPlayerObj, 207);
    gSpikeObj->pos = gPlayerObj->pos;
    gSpikeObj->dir = gPlayerObj->dir;
    gSpikeObj->jumpHeight = gPlayerObj->jumpHeight;
    gSpikeObj->frame =
        gSpikeObj->type->frame + (int)gSpikeFrame % gSpikeObj->type->numFrames;
  }
  else if (gSpikeObj) {
    KillObject(gSpikeObj);
    gSpikeObj = nil;
  }
  // brake light handling
  if (gBrakeObj) {
    KillObject(gBrakeObj);
    gBrakeObj = nil;
  }
  if ((gPlayerObj->input.brake || gPlayerObj->input.reverse) &&
      !(*gRoadInfo).water && !(gPlayerDeathDelay != 0) && gPlayerLives) {
    gBrakeObj = NewObject(gPlayerObj, 228);
    if (gPlayerObj->input.reverse)
      gBrakeObj->frame = 394;
    if (!gPlayerObj->input.brake)
      gBrakeObj->frame = 395;
    gBrakeObj->pos = gPlayerObj->pos;
    gBrakeObj->dir = gPlayerObj->dir;
    gBrakeObj->jumpHeight = gPlayerObj->jumpHeight;
  }
  // invincibility
  gPlayerObj->damage = 0;
  /*AddFloatToMessageBuffer("\pVelo: ",VEC2D_Value(gCameraObj->velo)*3.6);
  AddFloatToMessageBuffer("\pSlide: ",gCameraObj->slide);
  AddFloatToMessageBuffer("\pBrake: ",gCameraObj->input.brake);0*/
}

void GameFrame() {
  MoveObjects();
  PlayerHandling();
  gFrameCount++;
  if (CheckFrameTime()) {
    RenderFrame();
    gGraphFrameCount++;
    CheckTimeSkip();
  }
  if (gEndGame)
    EndGame();
}
