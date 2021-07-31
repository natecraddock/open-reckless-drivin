#include <stdint.h>

#include "defines.h"
#include "gameinitexit.h"
#include "gamesounds.h"
#include "interface.h"
#include "objects.h"
#include "packs.h"
#include "particlefx.h"
#include "preferences.h"
#include "renderframe.h"
#include "rle.h"
#include "screen.h"
#include "sprites.h"
#include "textfx.h"
#include "trig.h"
#include "vec2d.h"
#include <math.h>

void DrawRoadZoomed(float, float, float);
void DrawRoadZoomed16(float, float, float);

#define kXCameraScreenPos 0.5
#define kYCameraScreenPos 0.55
// horizontal and vertical position of the object the camera is looking at on
// screen, => 0,5/0,5 is center.

#define kPanelHeight 121 // heigth of the cockpit graphic on screen in pixels.

#define kMaxDisplayVelo 89.408
#define kZoomVeloFactor 50

#define kTrackSize 3.4 // width of the Tracks left by cars in pixels
#define kTrackLifeTime                                                         \
  (14.0 * kCalcFPS) // time after which rubber tracks left by cars get removed
                    // in frames.
#define kTrackDeathDuration                                                    \
  (0.8 * kCalcFPS) // time it takes for rubber tracks to fade out in frames.

#define kDigitOffs 18
#define kTimeDecimalOffs (kDigitOffs + 3)
#define kTimeMinuteOffs (-2 * kDigitOffs - 5)
#define kScoreX 610
#define kScoreY 51
#define kLiveX 613
#define kLiveY 76
#define kTimeX 586
#define kTimeY 26
#define kAddOnX 292
#define kAddOnY 25

#define kLifeBarWidth 158
#define kLifeBarY 27
#define kLifeBarX 227
#define kLifeBarFrameId 156

#define kComplYSize 370
#define kTimeTakeX 302
#define kTimeTakeY 204
#define kTimeBestX 302
#define kTimeBestY 233
#define kTimeLeftX 438
#define kTimeLeftY 282
#define kTimeBonusX 459
#define kTimeBonusY 311
#define kTimeBonus2X 459
#define kTimeBonus2Y 342

#define kScrollComplDelay 1.5
#define kShowComplDelay 4
#define kShowBestTimeDelay 6
#define kShowBonusTimeDelay 8
#define kStartBonusDelay 10
#define kCountBonusSpeed 0.1
#define kCountNoiseSpeed 0.1
#define kMultiplyDelay 1
#define kCloseDelay 3.5

#define kMaxMarkLength 128

int gBonusCount;
int gNoBonusScore;

void DrawNum(int h, int v, int num, int digits) {
  int i;
  for (i = 0; i < digits; i++) {
    if (num / (int)pow(10, i) || !i)
      DrawRLE(h, v, 128 + (num / (int)pow(10, i)) % 10);
    h -= kDigitOffs;
  }
}

void DrawNumZeroed(int h, int v, int num, int digits) {
  int i;
  for (i = 0; i < digits; i++) {
    DrawRLE(h, v, 128 + (num / (int)pow(10, i)) % 10);
    h -= kDigitOffs;
  }
}

int DrawComplCount() {
  float timeDiff =
      gLevelData->time - gGameTime > 0 ? gLevelData->time - gGameTime : 0;
  float countTime = timeDiff * kCountBonusSpeed;
  int bonus = timeDiff * 10;
  if (gFinishDelay >= kShowComplDelay)
    DrawRLE(0, gYSize / 2 - kComplYSize / 2, 149);
  else if (gFinishDelay >= kScrollComplDelay) {
    int yPos = (gYSize / 2 + kComplYSize / 2) *
                   sqrt((gFinishDelay - kScrollComplDelay) /
                        (kShowComplDelay - kScrollComplDelay)) -
               kComplYSize;
    DrawRLEYClip(0, yPos, 149);
    gNoBonusScore = gPlayerScore;
    gBonusCount = 0;
  }
  if (gFinishDelay >= kShowBestTimeDelay && !gLevelResFile) {
    if (gPrefs.lapRecords[gLevelID] > gGameTime) {
      gPrefs.lapRecords[gLevelID] = gGameTime;
      SimplePlaySound(155);
    }
    DrawNum(kTimeBestX + kTimeMinuteOffs, kTimeBestY,
            floor(gPrefs.lapRecords[gLevelID]) / 60, 2);
    DrawNumZeroed(kTimeBestX, kTimeBestY,
                  (int)floor(gPrefs.lapRecords[gLevelID]) % 60, 2);
    DrawNum(kTimeBestX + kTimeDecimalOffs, kTimeBestY,
            (int)floor(gPrefs.lapRecords[gLevelID] * 10), 1);
    DrawNum(kTimeTakeX + kTimeMinuteOffs, kTimeTakeY, floor(gGameTime) / 60, 2);
    DrawNumZeroed(kTimeTakeX, kTimeTakeY, (int)floor(gGameTime) % 60, 2);
    DrawNum(kTimeTakeX + kTimeDecimalOffs, kTimeTakeY,
            (int)floor(gGameTime * 10) % 10, 1);
  }
  if (gFinishDelay >= kStartBonusDelay) {
    float bonusPerc = (gFinishDelay - kStartBonusDelay) / countTime;
    if (bonusPerc > 1)
      bonusPerc = 1;
    if (bonus * bonusPerc * kCountNoiseSpeed > gBonusCount) {
      gBonusCount++;
      SimplePlaySound(148);
    }
    DrawNum(kTimeLeftX + kTimeMinuteOffs, kTimeLeftY,
            floor(timeDiff - timeDiff * bonusPerc) / 60, 2);
    DrawNumZeroed(kTimeLeftX, kTimeLeftY,
                  (int)floor(timeDiff - timeDiff * bonusPerc) % 60, 2);
    DrawNum(kTimeLeftX + kTimeDecimalOffs, kTimeLeftY,
            (int)floor((timeDiff - timeDiff * bonusPerc) * 10), 1);
    DrawNum(kTimeBonusX, kTimeBonusY, bonus * bonusPerc, 5);
    gPlayerScore = gNoBonusScore + bonus * bonusPerc;
  }
  else if (gFinishDelay >= kShowBonusTimeDelay) {
    DrawNum(kTimeLeftX + kTimeMinuteOffs, kTimeLeftY, floor(timeDiff) / 60, 2);
    DrawNumZeroed(kTimeLeftX, kTimeLeftY, (int)floor(timeDiff) % 60, 2);
    DrawNum(kTimeLeftX + kTimeDecimalOffs, kTimeLeftY,
            (int)floor(timeDiff * 10), 1);
    DrawNum(kTimeBonusX, kTimeBonusY, 0, 5);
  }
  if (gFinishDelay >= kStartBonusDelay + countTime + kMultiplyDelay)
    if (gPlayerBonus != 1) {
      if (gBonusCount != 32767) {
        SimplePlaySound(141);
        gBonusCount = 32767;
      }
      DrawRLE(kTimeBonus2X - 5 * kDigitOffs, kTimeBonus2Y, 137 + gPlayerBonus);
      DrawNum(kTimeBonus2X, kTimeBonus2Y, bonus * gPlayerBonus, 5);
    }
  if (gFinishDelay >= kStartBonusDelay + countTime + kCloseDelay) {
    gPlayerScore = gNoBonusScore + bonus * gPlayerBonus;
    DisposeLevel();
    gLevelID++;
    return LoadLevel();
  }
  return true;
}

void DrawDisplays() {
  int i;
  float timeDisp =
      gLevelData->time - gGameTime > 0 ? gLevelData->time - gGameTime : 0;
  float veloDisplay;
  if (!gCameraObj->jumpHeight)
    veloDisplay = -PI / 2 + VEC2D_DotProduct(gCameraObj->velo,
                                             P2D(sin(gCameraObj->dir),
                                                 cos(gCameraObj->dir))) *
                                PI / kMaxDisplayVelo;
  else
    veloDisplay = -PI / 2 + gCameraObj->input.throttle * PI;
  DrawRLE(0, gYSize - kPanelHeight, 148);
  DrawSprite(189, 148, gYSize - 14, veloDisplay, 1);
  DrawNum(kScoreX, gYSize - kScoreY, gDisplayScore, 6);
  DrawNum(kLiveX, gYSize - kLiveY, gPlayerLives, 2);
  DrawNum(kTimeX + kTimeMinuteOffs, gYSize - kTimeY, floor(timeDisp) / 60, 2);
  DrawNumZeroed(kTimeX, gYSize - kTimeY, (int)floor(timeDisp) % 60, 2);
  DrawNum(kTimeX + kTimeDecimalOffs, gYSize - kTimeY, (int)floor(timeDisp * 10),
          1);
  for (i = 0; i < 6; i++)
    if (gPlayerAddOns & 1 << i)
      DrawRLE(kAddOnX + 20 * i, gYSize - kAddOnY, 142 + i);
  if (gPlayerBonus != 1)
    DrawRLE(kAddOnX + 6 * 20, gYSize - kAddOnY, 137 + gPlayerBonus);
  if (gNumMissiles) {
    Str31 numStr;
    DrawRLE(gXSize - 120, 5, 154);
    NumToString(gNumMissiles, numStr);
    MakeFXStringFromNumStr(numStr, numStr);
    SimpleDrawText(numStr, gXSize - 60, 5);
  }
  if (gNumMines) {
    Str31 numStr;
    DrawRLE(gXSize - 120, gNumMissiles ? 30 : 5, 155);
    NumToString(gNumMines, numStr);
    MakeFXStringFromNumStr(numStr, numStr);
    SimpleDrawText(numStr, gXSize - 60, gNumMissiles ? 30 : 5);
  }
}

int DrawPanel() {
  if (gPlayerLives)
    if (gFinishDelay)
      return DrawComplCount();
    else if (gPlayerDeathDelay)
      DrawRLE(0, gYSize - kPanelHeight, 150);
    else
      DrawDisplays();
  return true;
}

void DrawTextureBlock(int x, int y, int size, uint32_t zoom, uint32_t u,
                      uint32_t v, uint8_t *texture) {
  int line, pix;
  uint8_t *dst = gBaseAddr + y * gRowBytes + x;
  for (line = 0; line < size; line++) {
    uint8_t *data = texture + ((v >> 9) & 0x3f80);
    for (pix = 0; pix < size; pix++) {
      u += zoom;
      *dst = data[(u >> 16) & 0x7f];
      dst++;
    }
    v += zoom;
    dst += gRowBytes - size;
  }
}

void DrawTextureBlockClipped(int x, int y, int size, uint32_t zoom, uint32_t u,
                             uint32_t v, uint8_t *texture) {
  int line, pix;
  int xSize = size, ySize = size;
  uint8_t *dst;
  if (x < 0) {
    xSize += x;
    x = 0;
  }
  if (y < 0) {
    ySize += y;
    y = 0;
  }
  if (x + size > gXSize)
    xSize += gXSize - x - size;
  if (y + size > gYSize)
    ySize += gYSize - y - size;
  dst = gBaseAddr + y * gRowBytes + x;
  for (line = 0; line < ySize; line++) {
    uint8_t *data = texture + ((v >> 9) & 0x3f80);
    for (pix = 0; pix < xSize; pix++) {
      u += zoom;
      *dst = data[(u >> 16) & 0x7f];
      dst++;
    }
    v += zoom;
    dst += gRowBytes - xSize;
  }
}

#pragma global_optimizer 2
#pragma global_optimizer off

void DrawTracksZoomed(float xDrawStart, float yDrawStart, float zoom) {
  float invZoom = 1 / zoom;
  int32_t fixZoom = zoom * 65536.0;
  int size = kTrackSize * invZoom;
  uint32_t shiftClip = gXSize - size << 16;
  int i;
  int y1Clip = yDrawStart - (gYSize - (gFinishDelay ? 0 : kInvLines)) * zoom;
  uint8_t *textures =
      GetUnsortedPackEntry(PACK_TEXTURES, (*gRoadInfo).tracks, 0);
  for (i = 0; i < gTrackCount; i++) {
    if (gTracks[i].p2.y <= yDrawStart + size && gTracks[i].p1.y > y1Clip &&
        gTracks[i].time + kTrackLifeTime + kTrackDeathDuration > gFrameCount) {
      float x2 = (gTracks[i].p2.x - xDrawStart) * invZoom - size / 2;
      float x1 = (gTracks[i].p1.x - xDrawStart) * invZoom - size / 2;
      float intensity =
          gTracks[i].intensity * 3 *
          ((gTracks[i].time + kTrackLifeTime > gFrameCount)
               ? 1
               : 1 - (gFrameCount - gTracks[i].time - kTrackLifeTime) /
                         kTrackDeathDuration);
      uint8_t *texture = textures + (int)intensity * 128 * 128;
      if ((x1 > -size || x2 > -size) && (x1 < gXSize || x2 < gXSize)) {
        float y1 = (yDrawStart - gTracks[i].p1.y) * invZoom - size / 2;
        float y2 = (yDrawStart - gTracks[i].p2.y) * invZoom - size / 2;
        uint32_t v1 = (int)gTracks[i].p1.y << 16;
        int32_t u1 = (int)gTracks[i].p1.x << 16;
        uint32_t v2 = (int)gTracks[i].p2.y << 16;
        int32_t u2 = (int)gTracks[i].p2.x << 16;
        int dy = y2 - y1;
        if (dy) {
          int y;
          int32_t dxdy = ((x2 - x1) / (y2 - y1) * 65536.0);
          int32_t dudy = (u2 - u1) / (y2 - y1);
          int32_t x = x1 * 65536.0;
          int32_t u = u1, v = v1;
          int numBlocks = ceil(fabs(((x2 - x1) / (y2 - y1))) - size);
          if (numBlocks < 0)
            numBlocks = 0;
          numBlocks++;
          for (y = y1; y < y2; y++) {
            int32_t blockU = u;
            int32_t blockX = x >> 16;
            int i;
            for (i = 0; i < numBlocks; i++) {
              if (blockX >= 0 && blockX < gXSize - size && y >= 0 &&
                  y < gYSize - size)
                DrawTextureBlock(blockX, y, size, fixZoom, blockU, v, texture);
              else
                DrawTextureBlockClipped(blockX, y, size, fixZoom, blockU, v,
                                        texture);
              blockU += fixZoom;
              blockX++;
            }
            x += dxdy;
            u += dudy;
            v += fixZoom;
          }
        }
        else {
          int x;
          if (x2 < 0)
            x2 = 0;
          if (x1 > gXSize - size)
            x1 = gXSize - size;
          if (x1 < x2) {
            int32_t u = u1;
            if (x1 < 0)
              x1 = 0;
            if (x2 > gXSize - size)
              x2 = gXSize - size;
            for (x = x1; x < x2; x++)
              DrawTextureBlockClipped(x, y1, size, fixZoom, u += fixZoom, v1,
                                      texture);
          }
          else {
            int32_t u = u2;
            if (x2 < 0)
              x2 = 0;
            if (x1 > gXSize - size)
              x1 = gXSize - size;
            for (x = x2; x < x1; x++)
              DrawTextureBlockClipped(x, y1, size, fixZoom, u += fixZoom, v1,
                                      texture);
          }
        }
      }
    }
  }
}

void DrawMarksZoomed(float xDrawStart, float yDrawStart, float zoom) {
  float invZoom = 1 / zoom;
  int32_t fixZoom = zoom * 65536.0;
  int size = 4.2 * invZoom;
  uint32_t shiftClip = gXSize - size << 16;
  int i;
  int y1Clip = yDrawStart - (gYSize - (gFinishDelay ? 0 : kInvLines)) * zoom;
  uint8_t *texture = GetUnsortedPackEntry(PACK_TEXTURES, (*gRoadInfo).marks, 0);
  int l = 0, r = gMarkSize;
  while (r - 1 > l)
    if (gMarks[(l + r) / 2].p1.y + gMarks[(l + r) / 2].p2.y >
        yDrawStart * 2 + kMaxMarkLength)
      l = (l + r) / 2;
    else
      r = (l + r) / 2;
  for (i = l; i < gMarkSize &&
              gMarks[i].p1.y + gMarks[i].p2.y > y1Clip * 2 - kMaxMarkLength;
       i++) {
    if (gMarks[i].p2.y <= yDrawStart + size && gMarks[i].p1.y > y1Clip) {
      float x2 = (gMarks[i].p2.x - xDrawStart) * invZoom - size / 2;
      float x1 = (gMarks[i].p1.x - xDrawStart) * invZoom - size / 2;
      if ((x1 > -size || x2 > -size) && (x1 < gXSize || x2 < gXSize)) {
        float y1 = (yDrawStart - gMarks[i].p1.y) * invZoom - size / 2;
        float y2 = (yDrawStart - gMarks[i].p2.y) * invZoom - size / 2;
        uint32_t v1 = (int)gMarks[i].p1.y << 16;
        int32_t u1 = (int)gMarks[i].p1.x << 16;
        uint32_t v2 = (int)gMarks[i].p2.y << 16;
        int32_t u2 = (int)gMarks[i].p2.x << 16;
        int dy = y2 - y1;
        if (dy) {
          int y;
          int32_t dxdy = ((x2 - x1) / (y2 - y1) * 65536.0);
          int32_t dudy = (u2 - u1) / (y2 - y1);
          int32_t x = x1 * 65536.0;
          int32_t u = u1, v = v1;
          int numBlocks = ceil(fabs(((x2 - x1) / (y2 - y1))) - size);
          if (numBlocks < 0)
            numBlocks = 0;
          numBlocks++;
          for (y = y1; y < y2; y++) {
            int32_t blockU = u;
            int32_t blockX = x >> 16;
            int i;
            for (i = 0; i < numBlocks; i++) {
              if (blockX >= 0 && blockX < gXSize - size && y >= 0 &&
                  y < gYSize - size)
                DrawTextureBlock(blockX, y, size, fixZoom, blockU, v, texture);
              else
                DrawTextureBlockClipped(blockX, y, size, fixZoom, blockU, v,
                                        texture);
              blockU += fixZoom;
              blockX++;
            }
            x += dxdy;
            u += dudy;
            v += fixZoom;
          }
        }
        else {
          int x;
          if (x2 < 0)
            x2 = 0;
          if (x1 > gXSize - size)
            x1 = gXSize - size;
          if (x1 < x2) {
            int32_t u = u1;
            if (x1 < 0)
              x1 = 0;
            if (x2 > gXSize - size)
              x2 = gXSize - size;
            for (x = x1; x < x2; x++)
              DrawTextureBlockClipped(x, y1, size, fixZoom, u += fixZoom, v1,
                                      texture);
          }
          else {
            int32_t u = u2;
            if (x2 < 0)
              x2 = 0;
            if (x1 > gXSize - size)
              x1 = gXSize - size;
            for (x = x2; x < x1; x++)
              DrawTextureBlockClipped(x, y1, size, fixZoom, u += fixZoom, v1,
                                      texture);
          }
        }
      }
    }
  }
}

#pragma global_optimizer default

void DrawTextureBlock16(uint32_t x, uint32_t y, int size, uint32_t zoom,
                        uint32_t u, uint32_t v, uint16_t *texture) {
  int line, pix;
  uint16_t *dst = gBaseAddr + y * gRowBytes + x * 2;
  for (line = 0; line < size; line++) {
    uint16_t *data = texture + ((v >> 9) & 0x3f80);
    for (pix = 0; pix < size; pix++) {
      u += zoom;
      *dst = data[(u >> 16) & 0x7f];
      dst++;
    }
    v += zoom;
    dst += gRowBytes / 2 - size;
  }
}

void DrawTextureBlockClipped16(int x, int y, int size, uint32_t zoom,
                               uint32_t u, uint32_t v, uint16_t *texture) {
  int line, pix;
  int xSize = size, ySize = size;
  uint16_t *dst;
  if (x < 0) {
    xSize += x;
    x = 0;
  }
  if (y < 0) {
    ySize += y;
    y = 0;
  }
  if (x + size > gXSize)
    xSize += gXSize - x - size;
  if (y + size > gYSize)
    ySize += gYSize - y - size;
  dst = gBaseAddr + y * gRowBytes + x * 2;
  for (line = 0; line < ySize; line++) {
    uint16_t *data = texture + ((v >> 9) & 0x3f80);
    for (pix = 0; pix < xSize; pix++) {
      u += zoom;
      *dst = data[(u >> 16) & 0x7f];
      dst++;
    }
    v += zoom;
    dst += gRowBytes / 2 - xSize;
  }
}

#pragma global_optimizer off

void DrawTracksZoomed16(float xDrawStart, float yDrawStart, float zoom) {
  float invZoom = 1 / zoom;
  int32_t fixZoom = zoom * 65536.0;
  int size = kTrackSize * invZoom;
  uint32_t shiftClip = gXSize - size << 16;
  int i;
  int y1Clip = yDrawStart - (gYSize - (gFinishDelay ? 0 : kInvLines)) * zoom;
  uint16_t *textures =
      GetUnsortedPackEntry(PACK_TEXTURES_16, (*gRoadInfo).tracks, 0);
  for (i = 0; i < gTrackCount; i++) {
    if (gTracks[i].p2.y <= yDrawStart + size && gTracks[i].p1.y > y1Clip &&
        gTracks[i].time + kTrackLifeTime + kTrackDeathDuration > gFrameCount) {
      float x2 = (gTracks[i].p2.x - xDrawStart) * invZoom - size / 2;
      float x1 = (gTracks[i].p1.x - xDrawStart) * invZoom - size / 2;
      float intensity =
          gTracks[i].intensity * 3 *
          ((gTracks[i].time + kTrackLifeTime > gFrameCount)
               ? 1
               : 1 - (gFrameCount - gTracks[i].time - kTrackLifeTime) /
                         kTrackDeathDuration);
      uint16_t *texture = textures + (int)intensity * 128 * 128;
      if ((x1 > -size || x2 > -size) && (x1 < gXSize || x2 < gXSize)) {
        float y1 = (yDrawStart - gTracks[i].p1.y) * invZoom - size / 2;
        float y2 = (yDrawStart - gTracks[i].p2.y) * invZoom - size / 2;
        uint32_t v1 = (int)gTracks[i].p1.y << 16;
        int32_t u1 = (int)gTracks[i].p1.x << 16;
        uint32_t v2 = (int)gTracks[i].p2.y << 16;
        int32_t u2 = (int)gTracks[i].p2.x << 16;
        int dy = y2 - y1;
        if (dy) {
          int y;
          int32_t dxdy = ((x2 - x1) / (y2 - y1) * 65536.0);
          int32_t dudy = (u2 - u1) / (y2 - y1);
          int32_t x = x1 * 65536.0;
          int32_t u = u1, v = v1;
          int numBlocks = ceil(fabs(((x2 - x1) / (y2 - y1))) - size);
          if (numBlocks < 0)
            numBlocks = 0;
          numBlocks++;
          for (y = y1; y < y2; y++) {
            int32_t blockU = u;
            int32_t blockX = x >> 16;
            int i;
            for (i = 0; i < numBlocks; i++) {
              if (blockX >= 0 && blockX < gXSize - size && y >= 0 &&
                  y < gYSize - size)
                DrawTextureBlock16(blockX, y, size, fixZoom, blockU, v,
                                   texture);
              else
                DrawTextureBlockClipped16(blockX, y, size, fixZoom, blockU, v,
                                          texture);
              blockU += fixZoom;
              blockX++;
            }
            x += dxdy;
            u += dudy;
            v += fixZoom;
          }
        }
        else {
          int x;
          if (x2 < 0)
            x2 = 0;
          if (x1 > gXSize - size)
            x1 = gXSize - size;
          if (x1 < x2) {
            int32_t u = u1;
            if (x1 < 0)
              x1 = 0;
            if (x2 > gXSize - size)
              x2 = gXSize - size;
            for (x = x1; x < x2; x++)
              DrawTextureBlockClipped16(x, y1, size, fixZoom, u += fixZoom, v1,
                                        texture);
          }
          else {
            int32_t u = u2;
            if (x2 < 0)
              x2 = 0;
            if (x1 > gXSize - size)
              x1 = gXSize - size;
            for (x = x2; x < x1; x++)
              DrawTextureBlockClipped16(x, y1, size, fixZoom, u += fixZoom, v1,
                                        texture);
          }
        }
      }
    }
  }
}

void DrawMarksZoomed16(float xDrawStart, float yDrawStart, float zoom) {
  float invZoom = 1 / zoom;
  int32_t fixZoom = zoom * 65536.0;
  int size = 4.2 * invZoom;
  uint32_t shiftClip = gXSize - size << 16;
  int i;
  int y1Clip = yDrawStart - (gYSize - (gFinishDelay ? 0 : kInvLines)) * zoom;
  uint16_t *texture =
      GetUnsortedPackEntry(PACK_TEXTURES_16, (*gRoadInfo).marks, 0);
  int l = 0, r = gMarkSize;
  while (r - 1 > l)
    if (gMarks[(l + r) / 2].p1.y + gMarks[(l + r) / 2].p2.y >
        yDrawStart * 2 + kMaxMarkLength)
      l = (l + r) / 2;
    else
      r = (l + r) / 2;
  for (i = l; i < gMarkSize &&
              gMarks[i].p1.y + gMarks[i].p2.y > y1Clip * 2 - kMaxMarkLength;
       i++) {
    if (gMarks[i].p2.y <= yDrawStart + size && gMarks[i].p1.y > y1Clip) {
      float x2 = (gMarks[i].p2.x - xDrawStart) * invZoom - size / 2;
      float x1 = (gMarks[i].p1.x - xDrawStart) * invZoom - size / 2;
      if ((x1 > -size || x2 > -size) && (x1 < gXSize || x2 < gXSize)) {
        float y1 = (yDrawStart - gMarks[i].p1.y) * invZoom - size / 2;
        float y2 = (yDrawStart - gMarks[i].p2.y) * invZoom - size / 2;
        uint32_t v1 = (int)gMarks[i].p1.y << 16;
        int32_t u1 = (int)gMarks[i].p1.x << 16;
        uint32_t v2 = (int)gMarks[i].p2.y << 16;
        int32_t u2 = (int)gMarks[i].p2.x << 16;
        int dy = y2 - y1;
        if (dy) {
          int y;
          int32_t dxdy = ((x2 - x1) / (y2 - y1) * 65536.0);
          int32_t dudy = (u2 - u1) / (y2 - y1);
          int32_t x = x1 * 65536.0;
          int32_t u = u1, v = v1;
          int numBlocks = ceil(fabs(((x2 - x1) / (y2 - y1))) - size);
          if (numBlocks < 0)
            numBlocks = 0;
          numBlocks++;
          for (y = y1; y < y2; y++) {
            int32_t blockU = u;
            int32_t blockX = x >> 16;
            int i;
            for (i = 0; i < numBlocks; i++) {
              if (blockX >= 0 && blockX < gXSize - size && y >= 0 &&
                  y < gYSize - size)
                DrawTextureBlock16(blockX, y, size, fixZoom, blockU, v,
                                   texture);
              else
                DrawTextureBlockClipped16(blockX, y, size, fixZoom, blockU, v,
                                          texture);
              blockU += fixZoom;
              blockX++;
            }
            x += dxdy;
            u += dudy;
            v += fixZoom;
          }
        }
        else {
          int x;
          if (x2 < 0)
            x2 = 0;
          if (x1 > gXSize - size)
            x1 = gXSize - size;
          if (x1 < x2) {
            int32_t u = u1;
            if (x1 < 0)
              x1 = 0;
            if (x2 > gXSize - size)
              x2 = gXSize - size;
            for (x = x1; x < x2; x++)
              DrawTextureBlockClipped16(x, y1, size, fixZoom, u += fixZoom, v1,
                                        texture);
          }
          else {
            int32_t u = u2;
            if (x2 < 0)
              x2 = 0;
            if (x1 > gXSize - size)
              x1 = gXSize - size;
            for (x = x2; x < x1; x++)
              DrawTextureBlockClipped16(x, y1, size, fixZoom, u += fixZoom, v1,
                                        texture);
          }
        }
      }
    }
  }
}

#pragma global_optimizer default

void DrawSpriteLayerZoomed(float xDrawStart, float yDrawStart, float zoom,
                           float tide, int layer) {
  tObject *theObj = gFirstVisObj;
  float invZoom = 1 / zoom;
  float maxDrawOffs = 128 * invZoom;
  while (theObj != gLastVisObj) {
    if (theObj->layer == layer)
      if (theObj->frame && !theObj->jumpHeight) {
        float x = (theObj->pos.x - xDrawStart) * invZoom;
        float y = (yDrawStart - theObj->pos.y) * invZoom;
        if ((y > -maxDrawOffs) && (y < gYSize + maxDrawOffs)) {
          float objTide =
              ((*gRoadInfo).water && (*theObj->type).flags2 & kObjectFloating)
                  ? 1 + tide * 0.5 + tide * VEC2D_Value(theObj->velo) * 0.04
                  : 1;
          DrawSprite(theObj->frame, x, y, theObj->dir, objTide * invZoom);
        }
      }
    theObj = (tObject *)theObj->next;
  }
}

void DrawSpriteLayerBlurZoomed(float xDrawStart, float yDrawStart, float zoom,
                               float tide, int layer) {
  tObject *theObj = gFirstVisObj;
  float invZoom = 1 / zoom;
  float maxDrawOffs = 128 * invZoom;
  while (theObj != gLastVisObj) {
    if (theObj->layer == layer)
      if (theObj->frame && !theObj->jumpHeight) {
        float x = (theObj->pos.x - xDrawStart) * invZoom;
        float y = (yDrawStart - theObj->pos.y) * invZoom;
        if ((y > -maxDrawOffs) && (y < gYSize + maxDrawOffs)) {
          t2DPoint velDiff = VEC2D_Difference(gCameraObj->velo, theObj->velo);
          float objTide =
              ((*gRoadInfo).water && (*theObj->type).flags2 & kObjectFloating)
                  ? 1 + tide * 0.5 + tide * VEC2D_Value(theObj->velo) * 0.04
                  : 1;
          DrawSprite(theObj->frame, x, y, theObj->dir, objTide * invZoom);
          if (velDiff.x * velDiff.x + velDiff.y * velDiff.y > 35 * 35) {
            t2DPoint fuzzPos =
                VEC2D_Scale(velDiff, kFrameDuration * kScale * 0.2);
            DrawSpriteTranslucent(theObj->frame, x + fuzzPos.x, y + fuzzPos.y,
                                  theObj->dir -
                                      theObj->rotVelo * kFrameDuration,
                                  objTide * invZoom);
          }
        }
      }
    theObj = (tObject *)theObj->next;
  }
}

void DrawSpritesZoomed(float xDrawStart, float yDrawStart, float zoom) {
  float invZoom = 1 / zoom;
  int layer;
  float tide = 0.05 * sin(gGameTime * 2.6);
  tObject *theObj;
  for (layer = 0; layer < kNumLayers; layer++)
    if (gPrefs.motionBlur)
      DrawSpriteLayerBlurZoomed(xDrawStart, yDrawStart, zoom, tide, layer);
    else
      DrawSpriteLayerZoomed(xDrawStart, yDrawStart, zoom, tide, layer);
  theObj = gFirstVisObj;
  while (theObj != gLastVisObj) {
    if (theObj->frame && theObj->jumpHeight) {
      float x = (theObj->pos.x - xDrawStart) * invZoom;
      float y = (yDrawStart - theObj->pos.y) * invZoom;
      if ((y > -256) && (y < gYSize + 256))
        DrawSprite(theObj->frame, x, y, theObj->dir,
                   (theObj->jumpHeight * 0.18 + 1) * invZoom);
    }
    theObj = (tObject *)theObj->next;
  }
}

void RenderFrame() {
  float zoom =
      0.5 + gZoomVelo / kZoomVeloFactor + gCameraObj->jumpHeight * 0.05;
  float xDrawStart = gCameraObj->pos.x - gXSize * kXCameraScreenPos * zoom;
  float yDrawStart = gCameraObj->pos.y + gYSize * kYCameraScreenPos * zoom;
  int preSpecBlit = gScreenBlitSpecial;
  if (gFinishDelay)
    yDrawStart = gLevelData->levelEnd + gYSize * kYCameraScreenPos * zoom;
  if (gPrefs.full_color) {
    DrawRoadZoomed16(xDrawStart, yDrawStart, zoom);
    DrawMarksZoomed16(xDrawStart, yDrawStart, zoom);
    DrawTracksZoomed16(xDrawStart, yDrawStart, zoom);
  }
  else {
    DrawRoadZoomed(xDrawStart, yDrawStart, zoom);
    DrawMarksZoomed(xDrawStart, yDrawStart, zoom);
    DrawTracksZoomed(xDrawStart, yDrawStart, zoom);
  }
  DrawParticleFXZoomed(xDrawStart, yDrawStart, zoom, 0);
  DrawSpritesZoomed(xDrawStart, yDrawStart, zoom);
  DrawParticleFXZoomed(xDrawStart, yDrawStart, zoom, 1);
  DrawTextFXZoomed(xDrawStart, yDrawStart, zoom);
  if (DrawPanel())
    if (preSpecBlit != gScreenBlitSpecial)
      RenderFrame();
    else
      Blit2Screen();
}
