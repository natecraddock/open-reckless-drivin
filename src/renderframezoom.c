#include <stdint.h>

#include "gameinitexit.h"
#include "objects.h"
#include "packs.h"
#include "particlefx.h"
#include "preferences.h"
#include "renderframe.h"
#include "roads.h"
#include "screen.h"
#include "sprites.h"
#include "textfx.h"
#include "trig.h"
#include "vec2d.h"
#include <math.h>

#define kXCameraScreenPos 0.5
#define kYCameraScreenPos 6.70833333333E-01

#define kPanelHeight 121

#define kMaxDisplayVelo 53.6423130565

#define kDigitOffs 18
#define kScoreX 610
#define kScoreY 51
#define kLiveX 613
#define kLiveY 76
#define kTimeX 607
#define kTimeY 26
#define kAddOnX 292
#define kAddOnY 25
#define kKidDispX 10
#define kKidDispY 10
#define kKidOffset 15

#define kCountX 33
#define kTimeLeftY -17
#define kKidsCountY 14
#define kScoreCountY 45

#define kComplXSize 277
#define kComplYSize 191

#define kScrollComplDelay 1.5
#define kShowComplDelay 4
#define kStartTimeDelay 5
#define kCountTimeSpeed 25
#define kStartKidsDelay 7
#define kCountKidsSpeed 0.2
#define kStartScoreDelay 9
#define kCountScoreSpeed 500
#define kMultiplyDelay 1.6
#define kCloseDelay 3.2

Ptr DrawLineZoomed(Ptr drawPos, int xDrawStart, int x1, int x2, int y,
                   int xDrift, int yDrift, Ptr data, float zoom) {
  uint32_t u, v;
  uint32_t dudx;
  if (x1 < 0)
    x1 = 0;
  if (x2 > gXSize)
    x2 = gXSize;
  if (x2 < x1)
    return drawPos;
  v = (-y + yDrift & 0x007f) << 7;
  u = ((int)(x1 * zoom + xDrawStart + xDrift) & 0x007f) << 8;
  x2 -= x1;
  data += v;
  dudx = (int)(zoom * 256);
  while (x2) {
    *(drawPos++) = data[((u >> 8) & 0x007f)];
    u += dudx;
    x2--;
  }
  return drawPos;
}

Ptr DrawBorderZoomed(Ptr drawPos, int xDrawStart, int x1, int x2, int y,
                     Ptr data, float zoom) {
  uint32_t dudx;
  uint32_t v = (-y & 0x007f) << 4;
  uint32_t u = 0;
  if (x2 < 0)
    return drawPos;
  if (x1 < 0) {
    u = -x1 << 8;
    x1 = 0;
  }
  if (x2 > gXSize)
    x2 = gXSize;
  if (x2 < x1)
    return drawPos;
  x2 -= x1;
  data += v;
  dudx = (int)(zoom * 256);
  while (x2) {
    *(drawPos++) = data[u >> 8];
    u += dudx;
    x2--;
  }
  return drawPos;
}

Ptr DrawBorderLineZoomed(Ptr drawPos, int xDrawStart, int x1, int x2, int y,
                         Ptr data, Ptr leftBorder, Ptr rightBorder,
                         float zoom) {
  int leftBordEnd = x1 + 16 / zoom;
  int rightBordEnd = x2 - 16 / zoom;
  if (leftBordEnd > rightBordEnd) {
    leftBordEnd = x1 + ((x2 - x1) >> 1);
    rightBordEnd = leftBordEnd;
  }
  drawPos = DrawBorderZoomed(drawPos, xDrawStart, x1, leftBordEnd, y,
                             leftBorder, zoom);
  drawPos = DrawLineZoomed(drawPos, xDrawStart, leftBordEnd, rightBordEnd, y,
                           gXDriftPos, gYDriftPos, data, zoom);
  return DrawBorderZoomed(drawPos, xDrawStart, rightBordEnd, x2, y, rightBorder,
                          zoom);
}

void DrawRoadZoomed(float xDrawStart, float yDrawStart, float zoom) {
  float invZoom = 1 / zoom;
  int screenY;
  int rowBytesSkip = gRowBytes - gXSize;
  Ptr drawPos = gBaseAddr;
  Ptr backgrTex =
      GetUnsortedPackEntry(PACK_TEXTURES, (*gRoadInfo).backgroundTex);
  Ptr roadTex = GetUnsortedPackEntry(PACK_TEXTURES, (*gRoadInfo).foregroundTex);
  Ptr leftBorder =
      GetUnsortedPackEntry(PACK_TEXTURES, (*gRoadInfo).roadLeftBorder);
  Ptr rightBorder =
      GetUnsortedPackEntry(PACK_TEXTURES, (*gRoadInfo).roadRightBorder);
  if (gPrefs.lineSkip)
    rowBytesSkip += gRowBytes;
  for (screenY = 0; screenY < gYSize; screenY += (gPrefs.lineSkip ? 2 : 1)) {
    float worldY = yDrawStart - screenY * zoom;
    float ceilRoadLine = ceil(worldY * 0.5);
    float floorRoadLine = floor(worldY * 0.5);
    float floorPerc = ceilRoadLine - worldY * 0.5;
    tRoadSeg roadData;
    tRoad ceilRoad = (gRoadData + (int)(ceilRoadLine));
    tRoad floorRoad = (gRoadData + (int)(floorRoadLine));
    roadData[0] =
        ((floorPerc * (*floorRoad)[0] + (1 - floorPerc) * (*ceilRoad)[0]) -
         xDrawStart) *
        invZoom;
    roadData[1] =
        ((floorPerc * (*floorRoad)[1] + (1 - floorPerc) * (*ceilRoad)[1]) -
         xDrawStart) *
        invZoom;
    roadData[2] =
        ((floorPerc * (*floorRoad)[2] + (1 - floorPerc) * (*ceilRoad)[2]) -
         xDrawStart) *
        invZoom;
    roadData[3] =
        ((floorPerc * (*floorRoad)[3] + (1 - floorPerc) * (*ceilRoad)[3]) -
         xDrawStart) *
        invZoom;
    drawPos =
        DrawBorderLineZoomed(drawPos, xDrawStart, 0x80000000, roadData[0],
                             worldY, backgrTex, leftBorder, rightBorder, zoom);
    drawPos =
        DrawLineZoomed(drawPos, xDrawStart, roadData[0], roadData[1], worldY,
                       gXFrontDriftPos, gYFrontDriftPos, roadTex, zoom);
    drawPos =
        DrawBorderLineZoomed(drawPos, xDrawStart, roadData[1], roadData[2],
                             worldY, backgrTex, leftBorder, rightBorder, zoom);
    drawPos =
        DrawLineZoomed(drawPos, xDrawStart, roadData[2], roadData[3], worldY,
                       gXFrontDriftPos, gYFrontDriftPos, roadTex, zoom);
    drawPos =
        DrawBorderLineZoomed(drawPos, xDrawStart, roadData[3], 0x7fffffff,
                             worldY, backgrTex, leftBorder, rightBorder, zoom);
    drawPos += rowBytesSkip;
  }
}

void DrawMarksZoomed(float xDrawStart, float yDrawStart, float zoom) {
  float invZoom = 1 / zoom;
  int l = 0, r = gMarkSize, i;
  int yClipWorld = (gYSize - (gFinishDelay ? 0 : kInvLines)) * zoom;
  int yClip = (gYSize - (gFinishDelay ? 0 : kInvLines));
  Ptr trackTex = GetUnsortedPackEntry(PACK_TEXTURES, (*gRoadInfo).marks);
  while (r - 1 > l)
    if (gMarks[(l + r) / 2].y > yDrawStart)
      l = (l + r) / 2;
    else
      r = (l + r) / 2;
  for (i = l; i > 0 && gMarks[i].y > yDrawStart - yClipWorld; i++) {
    int x = (gMarks[i].x - xDrawStart) * invZoom - 2;
    int y = (yDrawStart - gMarks[i].y) * invZoom;
    if ((y > 0) && (y < yClip))
      if ((x > 0) && (x < gXSize - 4)) {
        int v = (-gMarks[i].y & 0x001f) << 2;
        *(long *)(gBaseAddr + y * gRowBytes + x) = *(long *)(trackTex + v);
      }
  }
}

void DrawTracksZoomed(float xDrawStart, float yDrawStart, float zoom) {
  float invZoom = 1 / zoom;
  int i;
  int yClip = gYSize - (gFinishDelay ? 0 : kInvLines);
  Ptr trackTex = GetUnsortedPackEntry(PACK_TEXTURES, (*gRoadInfo).tracks);
  for (i = 0; i < gTrackCount; i++) {
    int x = (gTracks[i].x - xDrawStart) * invZoom - 2;
    int y = (yDrawStart - gTracks[i].y) * invZoom;
    if ((y > 0) && (y < yClip))
      if ((x > 0) && (x < gXSize - 4)) {
        int v = (-gTracks[i].y & 0x001f) << 2;
        *(long *)(gBaseAddr + y * gRowBytes + x) = *(long *)(trackTex + v);
      }
  }
}

void DrawSpriteLayerZoomed(float xDrawStart, float yDrawStart, float zoom,
                           float tide, int layer) {
  tObject *theObj = gFirstVisObj;
  float invZoom = 1 / zoom;
  while (theObj != gLastVisObj) {
    if (theObj->layer == layer)
      if (theObj->frame && !theObj->jumpHeight) {
        float x = (theObj->pos.x - xDrawStart) * invZoom;
        float y = (yDrawStart - theObj->pos.y) * invZoom;
        if ((y > -75) && (y < gYSize + 75)) {
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
  while (theObj != gLastVisObj) {
    if (theObj->layer == layer)
      if (theObj->frame && !theObj->jumpHeight) {
        float x = (theObj->pos.x - xDrawStart) * invZoom;
        float y = (yDrawStart - theObj->pos.y) * invZoom;
        if ((y > -75) && (y < gYSize + 75)) {
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
  float tide = (*gRoadInfo).water ? 0.05 * sin(gGameTime * 2.6) : 0;
  tObject *theObj;
  if (gPrefs.motionBlur)
    for (layer = 0; layer < kNumLayers; layer++)
      DrawSpriteLayerBlurZoomed(xDrawStart, yDrawStart, zoom, tide, layer);
  else
    for (layer = 0; layer < kNumLayers; layer++)
      DrawSpriteLayerZoomed(xDrawStart, yDrawStart, zoom, tide, layer);
  theObj = gFirstVisObj;
  while (theObj != gLastVisObj) {
    if (theObj->frame && theObj->jumpHeight) {
      float x = (theObj->pos.x - xDrawStart) * invZoom;
      float y = (yDrawStart - theObj->pos.y) * invZoom;
      if ((y > -75) && (y < gYSize + 75))
        DrawSprite(theObj->frame, x, y, theObj->dir,
                   (theObj->jumpHeight * 0.18 + 1) * invZoom);
    }
    theObj = (tObject *)theObj->next;
  }
}

void RenderFrameZoomed() {
  float zoom = 0.5 + VEC2D_Value(gCameraObj->velo) / kMaxDisplayVelo;
  float xDrawStart = gCameraObj->pos.x - gXSize * kXCameraScreenPos * zoom;
  float yDrawStart = gCameraObj->pos.y + gYSize * kYCameraScreenPos * zoom;
  int preSpecBlit = gScreenBlitSpecial;
  DrawRoadZoomed(xDrawStart, yDrawStart, zoom);
  DrawMarksZoomed(xDrawStart, yDrawStart, zoom);
  DrawTracksZoomed(xDrawStart, yDrawStart, zoom);
  DrawSpritesZoomed(xDrawStart, yDrawStart, zoom);
  DrawTextFXZoomed(xDrawStart, yDrawStart, zoom);
  /*	DrawParticleFX(xDrawStart,yDrawStart,0);
          DrawParticleFX(xDrawStart,yDrawStart,1);*/
  DrawPanel();
  if (preSpecBlit != gScreenBlitSpecial)
    RenderFrameZoomed();
  else
    Blit2Screen();
}
