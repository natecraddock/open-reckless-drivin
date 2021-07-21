#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "defines.h"
#include "packs.h"
#include "resource.h"
// #include "objects.h"
// #include "error.h"
// #include "preferences.h"
// #include "random.h"
// #include "screen.h"
// #include "trig.h"

#define kNumSprites 300
#define kNumSpecialSprites 100
#define kLifeBarId 318

enum {
  kDrawModeMaskColor = 0,
  kDrawModeTransparent = 1 << 0,
  kDrawModeDeepMask = 1 << 1,
  kDrawModeDoubleSize = 1 << 2
};

typedef struct {
  uint16_t xSize, ySize;
  uint8_t log2xSize;
  uint8_t filler1;
  uint8_t drawMode;
  uint8_t filler2;
  uint8_t data[1];
} tSpriteHeader;

typedef struct {
  uint16_t xSize, ySize;
  uint8_t log2xSize;
  uint8_t filler1;
  uint8_t drawMode;
  uint8_t filler2;
  uint16_t data[1];
} tSpriteHeader16;

typedef struct {
  int x1, x2, u, v;
} tSlopeInfo;

typedef struct {
  float x, y;
  int u, v;
} tVertex;

// tSlopeInfo gSlope[kMaxScreenY];
Handle gSprites[kNumSprites + kNumSpecialSprites];

/*
void SlopeInitUVLine(tVertex v1, tVertex v2, int dudx, int dvdx) {
  int y;
  int dxdy = ((v2.x - v1.x) * 65536.0) / (v2.y - v1.y);
  int dudy = ((v2.u - v1.u) << 8) / (v2.y - v1.y);
  int dvdy = ((v2.v - v1.v) << 8) / (v2.y - v1.y);
  int firsty, lasty;
  if (v1.y >= gYSize)
    return;
  if (v2.y < 0)
    return;
  if (v1.y < 0) {
    v1.x += ((-v1.y) * dxdy) / 65536.0;
    v1.u += ((-v1.y) * dudy) / 256.0;
    v1.v += ((-v1.y) * dvdy) / 256.0;
    v1.y = 0;
  }
  if (v2.y >= gYSize) {
    v2.x += ((gYSize - v1.y) * dxdy) / 65536.0;
    v2.u += ((gYSize - v1.y) * dudy) / 256.0;
    v2.v += ((gYSize - v1.y) * dvdy) / 256.0;
    v2.y = gYSize;
  }
  firsty = ceil(v1.y);
  lasty = floor(v2.y);
  gSlope[firsty].x2 = (v1.x * 65536.0) + (firsty - v1.y) * dxdy;
  gSlope[firsty].u = (v1.u << 8) + (firsty - v1.y) * dudy;
  gSlope[firsty].v = (v1.v << 8) + (firsty - v1.y) * dvdy;
  for (y = firsty + 1; y <= lasty; y++) {
    gSlope[y].x2 = gSlope[y - 1].x2 + dxdy;
    gSlope[y].u = gSlope[y - 1].u + dudy;
    gSlope[y].v = gSlope[y - 1].v + dvdy;
  }
  gSlope[firsty].u += dudx;
  gSlope[firsty].v += dvdx;
  gSlope[firsty].x2 += 65536;
}

void SlopeInitLine(tVertex v1, tVertex v2) {
  int y;
  int dxdy = ((v2.x - v1.x) * 65536.0) / (v2.y - v1.y);
  int firsty, lasty;
  if (v1.y >= gYSize)
    return;
  if (v2.y < 0)
    return;
  if (v1.y < 0) {
    v1.x += ((-v1.y) * dxdy) / 65535.0;
    v1.y = 0;
  }
  if (v2.y >= gYSize) {
    v2.x += ((gYSize - v1.y) * dxdy) / 65535.0;
    v2.y = gYSize;
  }
  firsty = ceil(v1.y);
  lasty = floor(v2.y);
  gSlope[firsty].x1 = (v1.x * 65536.0) + (firsty - v1.y) * dxdy;
  for (y = firsty + 1; y <= lasty; y++)
    gSlope[y].x1 = gSlope[y - 1].x1 + dxdy;
}

int SlopeInit(float cx, float cy, int *y1, int *y2, float dirCos, float dirSin,
              tSpriteHeader *sprite, int dudx, int dvdx) {
  tVertex vertices[4];
  int hi;
  float hiY = INFINITY;
  int i;
  vertices[0].x =
      cx - (dirCos * sprite->xSize / 2) + (dirSin * sprite->ySize / 2);
  vertices[1].x =
      cx + (dirCos * sprite->xSize / 2) + (dirSin * sprite->ySize / 2);
  vertices[2].x =
      cx + (dirCos * sprite->xSize / 2) - (dirSin * sprite->ySize / 2);
  vertices[3].x =
      cx - (dirCos * sprite->xSize / 2) - (dirSin * sprite->ySize / 2);
  vertices[0].y =
      cy - (dirSin * sprite->xSize / 2) - (dirCos * sprite->ySize / 2);
  vertices[1].y =
      cy + (dirSin * sprite->xSize / 2) - (dirCos * sprite->ySize / 2);
  vertices[2].y =
      cy + (dirSin * sprite->xSize / 2) + (dirCos * sprite->ySize / 2);
  vertices[3].y =
      cy - (dirSin * sprite->xSize / 2) + (dirCos * sprite->ySize / 2);
  vertices[0].u = 0;
  vertices[1].u = sprite->xSize;
  vertices[2].u = sprite->xSize;
  vertices[3].u = 0;
  vertices[0].v = 0;
  vertices[1].v = 0;
  vertices[2].v = sprite->ySize;
  vertices[3].v = sprite->ySize;
  for (i = 0; i < 4; i++)
    if (vertices[i].y < hiY) {
      hi = i;
      hiY = vertices[i].y;
    }
  if ((vertices[(hi + 1) & 3].x < 0) || (vertices[(hi - 1) & 3].x >= gXSize)) {
    *y2 = 0;
    *y1 = 0;
    return false;
  }
  SlopeInitUVLine(vertices[hi], vertices[(hi - 1) & 3], dudx, dvdx);
  SlopeInitUVLine(vertices[(hi - 1) & 3], vertices[(hi - 2) & 3], dudx, dvdx);
  SlopeInitLine(vertices[hi], vertices[(hi + 1) & 3]);
  SlopeInitLine(vertices[(hi + 1) & 3], vertices[(hi + 2) & 3]);
  *y2 = ceil(
      (vertices[(hi + 2) & 3].y > gYSize ? gYSize : vertices[(hi + 2) & 3].y) -
      1);
  if (hiY < 0)
    *y1 = 0;
  else
    *y1 = ceil(hiY);
  return ((vertices[(hi - 1) & 3].x < 0) ||
          (vertices[(hi + 1) & 3].x >= gXSize));
}

void DrawSpriteRotatedClippedTranslucent(tSpriteHeader *sprite, int dudx,
                                         int dvdx, int y, int y2) {
  int vMask = sprite->ySize - 1 << 8;
  int vShift = 8 - sprite->log2xSize;
  uint8_t mask = sprite->data[0];
  uint8_t *trTab = *gTranslucenceTab;
  for (; y < y2; y++) {
    int x = gSlope[y].x2 >> 16;
    int endX = gSlope[y].x1 >> 16;
    uint8_t *dst = gBaseAddr + y * gRowBytes;
    uint8_t *endDst;
    int u = gSlope[y].u;
    int v = gSlope[y].v;
    if (x >= gXSize)
      continue;
    if (endX < 0)
      continue;
    if (x < 0) {
      u -= dudx * x;
      v -= dvdx * x;
      x = 0;
    }
    if (endX >= gXSize)
      endX = gXSize - 1;
    endDst = dst + endX;
    dst += x;
    while (dst < endDst) {
      uint8_t color = sprite->data[(u >> 8) + ((v & vMask) >> vShift)];
      if (color != mask)
        *dst = trTab[(color << 8) + *dst];
      u += dudx;
      v += dvdx;
      dst++;
    }
  }
}

void DrawSpriteRotatedClippedTranslucent16(tSpriteHeader16 *sprite, int dudx,
                                           int dvdx, int y, int y2) {
  int vMask = sprite->ySize - 1 << 8;
  int vShift = 8 - sprite->log2xSize;
  uint16_t mask = sprite->data[0];
  for (; y < y2; y++) {
    int x = gSlope[y].x2 >> 16;
    int endX = gSlope[y].x1 >> 16;
    uint16_t *dst = gBaseAddr + y * gRowBytes;
    uint16_t *endDst;
    int u = gSlope[y].u;
    int v = gSlope[y].v;
    if (x >= gXSize)
      continue;
    if (endX < 0)
      continue;
    if (x < 0) {
      u -= dudx * x;
      v -= dvdx * x;
      x = 0;
    }
    if (endX >= gXSize)
      endX = gXSize - 1;
    endDst = dst + endX;
    dst += x;
    while (dst < endDst) {
      uint16_t color = sprite->data[(u >> 8) + ((v & vMask) >> vShift)];
      if (color != mask)
        *dst = BlendRGB16(color, *dst);
      u += dudx;
      v += dvdx;
      dst++;
    }
  }
}

void DrawSpriteRotatedClipped(tSpriteHeader *sprite, int dudx, int dvdx, int y,
                              int y2) {
  int vMask = sprite->ySize - 1 << 8;
  int vShift = 8 - sprite->log2xSize;
  uint8_t mask = sprite->data[0];
  for (; y < y2; y++) {
    int x = gSlope[y].x2 >> 16;
    int endX = gSlope[y].x1 >> 16;
    uint8_t *dst = gBaseAddr + y * gRowBytes;
    uint8_t *endDst;
    int u = gSlope[y].u;
    int v = gSlope[y].v;
    if (x >= gXSize)
      continue;
    if (endX < 0)
      continue;
    if (x < 0) {
      u -= dudx * x;
      v -= dvdx * x;
      x = 0;
    }
    if (endX >= gXSize)
      endX = gXSize - 1;
    endDst = dst + endX;
    dst += x;
    while (dst < endDst) {
      uint8_t color = sprite->data[(u >> 8) + ((v & vMask) >> vShift)];
      if (color != mask)
        *dst = color;
      u += dudx;
      v += dvdx;
      dst++;
    }
  }
}

void DrawSpriteRotatedClipped16(tSpriteHeader16 *sprite, int dudx, int dvdx,
                                int y, int y2) {
  int vMask = sprite->ySize - 1 << 8;
  int vShift = 8 - sprite->log2xSize;
  uint16_t mask = sprite->data[0];
  for (; y < y2; y++) {
    int x = gSlope[y].x2 >> 16;
    int endX = gSlope[y].x1 >> 16;
    uint16_t *dst = gBaseAddr + y * gRowBytes;
    uint16_t *endDst;
    int u = gSlope[y].u;
    int v = gSlope[y].v;
    if (x >= gXSize)
      continue;
    if (endX < 0)
      continue;
    if (x < 0) {
      u -= dudx * x;
      v -= dvdx * x;
      x = 0;
    }
    if (endX >= gXSize)
      endX = gXSize - 1;
    endDst = dst + endX;
    dst += x;
    while (dst < endDst) {
      uint16_t color = sprite->data[(u >> 8) + ((v & vMask) >> vShift)];
      if (color != mask)
        *dst = color;
      u += dudx;
      v += dvdx;
      dst++;
    }
  }
}

void DrawSpriteRotatedTranslucent(tSpriteHeader *sprite, int dudx, int dvdx,
                                  int y, int y2) {
  int vMask = sprite->ySize - 1 << 8;
  int vShift = 8 - sprite->log2xSize;
  uint8_t mask = sprite->data[0];
  uint8_t *trTab = *gTranslucenceTab;
  for (; y < y2; y++) {
    uint8_t *dst = gBaseAddr + y * gRowBytes + (gSlope[y].x2 >> 16);
    uint8_t *endDst = gBaseAddr + y * gRowBytes + (gSlope[y].x1 >> 16);
    int u = gSlope[y].u;
    int v = gSlope[y].v;
    while (dst < endDst) {
      uint8_t color = sprite->data[(u >> 8) + ((v & vMask) >> vShift)];
      if (color != mask)
        *dst = trTab[(color << 8) + *dst];
      u += dudx;
      v += dvdx;
      dst++;
    }
  }
}

void DrawSpriteRotatedTranslucent16(tSpriteHeader16 *sprite, int dudx, int dvdx,
                                    int y, int y2) {
  int vMask = sprite->ySize - 1 << 8;
  int vShift = 8 - sprite->log2xSize;
  uint16_t mask = sprite->data[0];
  for (; y < y2; y++) {
    uint16_t *dst =
        gBaseAddr + y * gRowBytes + ((gSlope[y].x2 >> 15) & 0xfffffffe);
    uint16_t *endDst =
        gBaseAddr + y * gRowBytes + ((gSlope[y].x1 >> 15) & 0xfffffffe);
    int u = gSlope[y].u;
    int v = gSlope[y].v;
    while (dst < endDst) {
      uint16_t color = sprite->data[(u >> 8) + ((v & vMask) >> vShift)];
      if (color != mask)
        *dst = BlendRGB16(color, *dst);
      u += dudx;
      v += dvdx;
      dst++;
    }
  }
}

void DrawSpriteRotated(tSpriteHeader *sprite, int dudx, int dvdx, int y,
                       int y2) {
  int vMask = sprite->ySize - 1 << 8;
  int vShift = 8 - sprite->log2xSize;
  uint8_t mask = sprite->data[0];
  for (; y < y2; y++) // no clip, no translucence
  {
    uint8_t *dst = gBaseAddr + y * gRowBytes + (gSlope[y].x2 >> 16);
    uint8_t *endDst = gBaseAddr + y * gRowBytes + (gSlope[y].x1 >> 16);
    int u = gSlope[y].u;
    int v = gSlope[y].v;
    while (dst < endDst) {
      uint8_t color = sprite->data[(u >> 8) + ((v & vMask) >> vShift)];
      if (color != mask)
        *dst = color;
      u += dudx;
      v += dvdx;
      dst++;
    }
  }
}

void DrawSpriteRotated16(tSpriteHeader16 *sprite, int dudx, int dvdx, int y,
                         int y2) {
  int vMask = sprite->ySize - 1 << 8;
  int vShift = 8 - sprite->log2xSize;
  uint16_t mask = sprite->data[0];
  for (; y < y2; y++) // no clip, no translucence
  {
    uint16_t *dst =
        gBaseAddr + y * gRowBytes + ((gSlope[y].x2 >> 15) & 0xfffffffe);
    uint16_t *endDst =
        gBaseAddr + y * gRowBytes + ((gSlope[y].x1 >> 15) & 0xfffffffe);
    int u = gSlope[y].u;
    int v = gSlope[y].v;
    while (dst < endDst) {
      uint16_t color = sprite->data[(u >> 8) + ((v & vMask) >> vShift)];
      if (color != mask)
        *dst = color;
      u += dudx;
      v += dvdx;
      dst++;
    }
  }
}

void DrawSprite(int id, float cx, float cy, float dir, float zoom) {
  tSpriteHeader *sprite = (tSpriteHeader *)*(gSprites[id - 128]);
  if (sprite->drawMode & kDrawModeDoubleSize)
    zoom *= 0.5;
  {
    int y, y2;
    float dirCos = cos(dir);
    float dirSin = sin(dir);
    int dudx = dirCos * 256 / zoom;
    int dvdx = -dirSin * 256 / zoom;
    dirCos *= zoom;
    dirSin *= zoom;
    if (gPrefs.hiColor)
      if (sprite->drawMode & kDrawModeTransparent)
        if (SlopeInit(cx, cy, &y, &y2, dirCos, dirSin, sprite, dudx, dvdx))
          DrawSpriteRotatedClippedTranslucent16(sprite, dudx, dvdx, y, y2);
        else
          DrawSpriteRotatedTranslucent16(sprite, dudx, dvdx, y, y2);
      else if (SlopeInit(cx, cy, &y, &y2, dirCos, dirSin, sprite, dudx, dvdx))
        DrawSpriteRotatedClipped16(sprite, dudx, dvdx, y, y2);
      else
        DrawSpriteRotated16(sprite, dudx, dvdx, y, y2);
    else if (sprite->drawMode & kDrawModeTransparent)
      if (SlopeInit(cx, cy, &y, &y2, dirCos, dirSin, sprite, dudx, dvdx))
        DrawSpriteRotatedClippedTranslucent(sprite, dudx, dvdx, y, y2);
      else
        DrawSpriteRotatedTranslucent(sprite, dudx, dvdx, y, y2);
    else if (SlopeInit(cx, cy, &y, &y2, dirCos, dirSin, sprite, dudx, dvdx))
      DrawSpriteRotatedClipped(sprite, dudx, dvdx, y, y2);
    else
      DrawSpriteRotated(sprite, dudx, dvdx, y, y2);
  }
}

void DrawSpriteTranslucent(int id, float cx, float cy, float dir, float zoom) {
  tSpriteHeader *sprite = (tSpriteHeader *)*(gSprites[id - 128]);
  if (sprite->drawMode & kDrawModeDoubleSize)
    zoom *= 0.5;
  {
    int y, y2;
    float dirCos = cos(dir);
    float dirSin = sin(dir);
    int dudx = dirCos * 256 / zoom;
    int dvdx = -dirSin * 256 / zoom;
    if (sprite->drawMode)
      return;
    dirCos *= zoom;
    dirSin *= zoom;
    if (gPrefs.hiColor)
      if (SlopeInit(cx, cy, &y, &y2, dirCos, dirSin, sprite, dudx, dvdx))
        DrawSpriteRotatedClippedTranslucent16(sprite, dudx, dvdx, y, y2);
      else
        DrawSpriteRotatedTranslucent16(sprite, dudx, dvdx, y, y2);
    else if (SlopeInit(cx, cy, &y, &y2, dirCos, dirSin, sprite, dudx, dvdx))
      DrawSpriteRotatedClippedTranslucent(sprite, dudx, dvdx, y, y2);
    else
      DrawSpriteRotatedTranslucent(sprite, dudx, dvdx, y, y2);
  }
}

int GetUniqueSpriteNum() {
  int i;
  for (i = kNumSprites; i < kNumSprites + kNumSpecialSprites; i++)
    if (!gSprites[i])
      return i;
  return 0;
}

void XDistortSprite8(tSpriteHeader *sprite, int startY, int endY, int startX,
                     int endX, int dir, float damage) {
  uint8_t *lineStart;
  int line;
  lineStart = &sprite->data[sprite->xSize * startY + startX];
  if (damage > 2)
    damage = 2;
  if (!dir)
    for (line = 0; line < endY; line++) {
      int i;
      float c = (float)line / endY;
      float crunch = (-c * c + c) * (1 + RanFl(-0.3, 0.3)) * endX * damage;
      if (crunch) {
        float crunchFactor = endX / (endX - crunch);
        float invCrunchFac = 1 / crunchFactor;
        float maxShade = (kLightValues - 1) * (1 - invCrunchFac);
        float crunchLength = (endX - crunch);
        for (i = endX; i > (int)crunch; i--) {
          uint8_t pixelValue =
              lineStart[(int)(endX - ((endX - i) * crunchFactor))];
          if (pixelValue != sprite->data[0]) {
            int shade =
                (kLightValues - 1) - maxShade * ((endX - i) / crunchLength);
            lineStart[i] = gLightningTab[shade][pixelValue];
          }
        }
        for (i = 0; i <= (int)crunch; i++)
          lineStart[i] = sprite->data[0];
      }
      lineStart += sprite->xSize;
    }
  else
    for (line = 0; line < endY; line++) {
      int i;
      float c = (float)line / endY;
      float crunch = (-c * c + c) * (1 + RanFl(-0.3, 0.3)) * endX * damage;
      if (crunch) {
        float crunchFactor = endX / (endX - crunch);
        float invCrunchFac = 1 / crunchFactor;
        float maxShade = (kLightValues - 1) * (1 - invCrunchFac);
        float crunchLength = (endX - crunch);
        for (i = 0; i < endX - (int)crunch; i++) {
          uint8_t pixelValue = lineStart[(int)(i * crunchFactor)];
          if (pixelValue != sprite->data[0]) {
            int shade = (kLightValues - 1) - maxShade * (i / crunchLength);
            lineStart[i] = gLightningTab[shade][pixelValue];
          }
        }
        for (i = endX; i >= endX - (int)crunch; i--)
          lineStart[i] = sprite->data[0];
      }
      lineStart += sprite->xSize;
    }
}

void XDistortSprite16(tSpriteHeader16 *sprite, int startY, int endY, int startX,
                      int endX, int dir, float damage) {
  uint16_t *lineStart;
  int line;
  lineStart = &sprite->data[sprite->xSize * startY + startX];
  if (damage > 2)
    damage = 2;
  if (!dir)
    for (line = 0; line < endY; line++) {
      int i;
      float c = (float)line / endY;
      float crunch = (-c * c + c) * (1 + RanFl(-0.5, 0.5)) * endX * damage;
      if (crunch) {
        float crunchFactor = endX / (endX - crunch);
        float invCrunchFac = 1 / crunchFactor;
        float maxShade = (kLightValues - 1) * (1 - invCrunchFac);
        float crunchLength = (endX - crunch);
        for (i = endX; i > (int)crunch; i--) {
          uint16_t pixelValue =
              lineStart[(int)(endX - ((endX - i) * crunchFactor))];
          if (pixelValue != sprite->data[0]) {
            int shade =
                (kLightValues - 1) - maxShade * ((endX - i) / crunchLength);
            lineStart[i] = ShadeRGB16(shade, pixelValue);
          }
        }
        for (i = 0; i <= (int)crunch; i++)
          lineStart[i] = sprite->data[0];
      }
      lineStart += sprite->xSize;
    }
  else
    for (line = 0; line < endY; line++) {
      int i;
      float c = (float)line / endY;
      float crunch = (-c * c + c) * (1 + RanFl(-0.5, 0.5)) * endX * damage;
      if (crunch) {
        float crunchFactor = endX / (endX - crunch);
        float invCrunchFac = 1 / crunchFactor;
        float maxShade = (kLightValues - 1) * (1 - invCrunchFac);
        float crunchLength = (endX - crunch);
        for (i = 0; i < endX - (int)crunch; i++) {
          uint16_t pixelValue = lineStart[(int)(i * crunchFactor)];
          if (pixelValue != sprite->data[0]) {
            int shade = (kLightValues - 1) - maxShade * (i / crunchLength);
            lineStart[i] = ShadeRGB16(shade, pixelValue);
          }
        }
        for (i = endX; i >= endX - (int)crunch; i--)
          lineStart[i] = sprite->data[0];
      }
      lineStart += sprite->xSize;
    }
}

int XDistortSprite(int id, int startY, int endY, int startX, int endX, int dir,
                   float damage) {
  tSpriteHeader *sprite;
  if (id - 128 < kNumSprites) {
    int newID = GetUniqueSpriteNum();
    if (!newID)
      return id;
    gSprites[newID] = gSprites[id - 128];
    HandToHand(&gSprites[newID]);
    DoError(MemError());
    sprite = (tSpriteHeader *)*(gSprites[newID]);
    id = newID + 128;
  }
  else
    sprite = (tSpriteHeader *)*(gSprites[id - 128]);
  if (sprite->drawMode & kDrawModeDoubleSize) {
    startY *= 2;
    endY *= 2;
    startX *= 2;
    endX *= 2;
  }
  endY -= startY;
  endX -= startX;
  startX += sprite->xSize / 2;
  startY += sprite->ySize / 2;
  if (startX < 0) {
    endX += startX;
    startX = 0;
  }
  if (startY < 0) {
    endY += startY;
    startY = 0;
  }
  if (startX + endX >= sprite->xSize) {
    endX -= startX + endX + 1 - sprite->xSize;
  }
  if (startY + endY >= sprite->ySize) {
    endY -= startY + endY + 1 - sprite->ySize;
  }
  if (gPrefs.hiColor)
    XDistortSprite16(sprite, startY, endY, startX, endX, dir, damage);
  else
    XDistortSprite8(sprite, startY, endY, startX, endX, dir, damage);
  return id;
}

void YDistortSprite8(tSpriteHeader *sprite, int startX, int endX, int startY,
                     int endY, int dir, float damage) {
  uint8_t *columnStart;
  int column;
  columnStart = &sprite->data[sprite->xSize * startY + startX];
  if (damage > 2)
    damage = 2;
  if (!dir)
    for (column = 0; column < endX; column++) {
      int i;
      float c = (float)column / endX;
      float crunch = (-c * c + c) * (1 + RanFl(-0.5, 0.5)) * endY * damage;
      if (crunch) {
        float crunchFactor = endY / (endY - crunch);
        float invCrunchFac = 1 / crunchFactor;
        float maxShade = (kLightValues - 1) * (1 - invCrunchFac);
        float crunchLength = (endY - crunch);
        for (i = endY; i > (int)crunch; i--) {
          uint8_t pixelValue =
              columnStart[(int)(endY - ((endY - i) * crunchFactor)) *
                          sprite->xSize];
          if (pixelValue != sprite->data[0]) {
            int shade =
                (kLightValues - 1) - maxShade * ((endY - i) / crunchLength);
            columnStart[i * sprite->xSize] = gLightningTab[shade][pixelValue];
          }
        }
        for (i = 0; i <= (int)crunch; i++)
          columnStart[i * sprite->xSize] = sprite->data[0];
      }
      columnStart++;
    }
  else
    for (column = 0; column < endX; column++) {
      int i;
      float c = (float)column / endX;
      float crunch = (-c * c + c) * (1 + RanFl(-0.5, 0.5)) * endY * damage;
      if (crunch) {
        float crunchFactor = endY / (endY - crunch);
        float invCrunchFac = 1 / crunchFactor;
        float maxShade = (kLightValues - 1) * (1 - invCrunchFac);
        float crunchLength = (endY - crunch);
        for (i = 0; i < endY - (int)crunch; i++) {
          uint8_t pixelValue =
              columnStart[(int)(i * crunchFactor) * sprite->xSize];
          if (pixelValue != sprite->data[0]) {
            int shade = (kLightValues - 1) - maxShade * (i / crunchLength);
            columnStart[i * sprite->xSize] = gLightningTab[shade][pixelValue];
          }
        }
        for (i = endY; i >= endY - (int)crunch; i--)
          columnStart[i * sprite->xSize] = sprite->data[0];
      }
      columnStart++;
    }
}

void YDistortSprite16(tSpriteHeader16 *sprite, int startX, int endX, int startY,
                      int endY, int dir, float damage) {
  uint16_t *columnStart;
  int column;
  columnStart = &sprite->data[sprite->xSize * startY + startX];
  if (damage > 2)
    damage = 2;
  if (!dir)
    for (column = 0; column < endX; column++) {
      int i;
      float c = (float)column / endX;
      float crunch = (-c * c + c) * (1 + RanFl(-0.5, 0.5)) * endY * damage;
      if (crunch) {
        float crunchFactor = endY / (endY - crunch);
        float invCrunchFac = 1 / crunchFactor;
        float maxShade = (kLightValues - 1) * (1 - invCrunchFac);
        float crunchLength = (endY - crunch);
        for (i = endY; i > (int)crunch; i--) {
          uint16_t pixelValue =
              columnStart[(int)(endY - ((endY - i) * crunchFactor)) *
                          sprite->xSize];
          if (pixelValue != sprite->data[0]) {
            int shade =
                (kLightValues - 1) - maxShade * ((endY - i) / crunchLength);
            columnStart[i * sprite->xSize] = ShadeRGB16(shade, pixelValue);
          }
        }
        for (i = 0; i <= (int)crunch; i++)
          columnStart[i * sprite->xSize] = sprite->data[0];
      }
      columnStart++;
    }
  else
    for (column = 0; column < endX; column++) {
      int i;
      float c = (float)column / endX;
      float crunch = (-c * c + c) * (1 + RanFl(-0.5, 0.5)) * endY * damage;
      if (crunch) {
        float crunchFactor = endY / (endY - crunch);
        float invCrunchFac = 1 / crunchFactor;
        float maxShade = (kLightValues - 1) * (1 - invCrunchFac);
        float crunchLength = (endY - crunch);
        for (i = 0; i < endY - (int)crunch; i++) {
          uint16_t pixelValue =
              columnStart[(int)(i * crunchFactor) * sprite->xSize];
          if (pixelValue != sprite->data[0]) {
            int shade = (kLightValues - 1) - maxShade * (i / crunchLength);
            columnStart[i * sprite->xSize] = ShadeRGB16(shade, pixelValue);
          }
        }
        for (i = endY; i >= endY - (int)crunch; i--)
          columnStart[i * sprite->xSize] = sprite->data[0];
      }
      columnStart++;
    }
}

int YDistortSprite(int id, int startX, int endX, int startY, int endY, int dir,
                   float damage) {
  tSpriteHeader *sprite;
  if (id - 128 < kNumSprites) {
    int newID = GetUniqueSpriteNum();
    if (!newID)
      return id;
    gSprites[newID] = gSprites[id - 128];
    HandToHand(&gSprites[newID]);
    DoError(MemError());
    sprite = (tSpriteHeader *)*(gSprites[newID]);
    id = newID + 128;
  }
  else
    sprite = (tSpriteHeader *)*(gSprites[id - 128]);
  if (sprite->drawMode & kDrawModeDoubleSize) {
    startY *= 2;
    endY *= 2;
    startX *= 2;
    endX *= 2;
  }
  endY -= startY;
  endX -= startX;
  startX += sprite->xSize / 2;
  startY += sprite->ySize / 2;
  if (startX < 0) {
    endX += startX;
    startX = 0;
  }
  if (startY < 0) {
    endY += startY;
    startY = 0;
  }
  if (startX + endX >= sprite->xSize) {
    endX -= startX + endX + 1 - sprite->xSize;
  }
  if (startY + endY >= sprite->ySize) {
    endY -= startY + endY + 1 - sprite->ySize;
  }
  if (gPrefs.hiColor)
    YDistortSprite16(sprite, startX, endX, startY, endY, dir, damage);
  else
    YDistortSprite8(sprite, startX, endX, startY, endY, dir, damage);
  return id;
}
*/
#if 0
int BulletHitSprite8(tSpriteHeader16 *sprite,int x,int y)
{
        int offs;
        if(y-1>=0&&y-1<sprite->ySize)
        {
                if(x-1>=0&&x-1<sprite->xSize)
                        if(sprite->data[offs=((y-1)*sprite->xSize+x-1)]!=sprite->data[0])
                                sprite->data[offs]=gLightningTab[(int)(kLightValues*0.9)][sprite->data[offs]];
                if(x>=0&&x<sprite->xSize)
                        if(sprite->data[offs=((y-1)*sprite->xSize+x)]!=sprite->data[0])
                                sprite->data[offs]=gLightningTab[(int)(kLightValues*0.7)][sprite->data[offs]];
                if(x+1>=0&&x+1<sprite->xSize)
                        if(sprite->data[offs=((y-1)*sprite->xSize+x+1)]!=sprite->data[0])
                                sprite->data[offs]=gLightningTab[(int)(kLightValues*0.9)][sprite->data[offs]];
        }
        if(y>=0&&y<sprite->ySize)
        {
                if(x-1>=0&&x-1<sprite->xSize)
                        if(sprite->data[offs=(y*sprite->xSize+x-1)]!=sprite->data[0])
                                sprite->data[offs]=gLightningTab[(int)(kLightValues*0.7)][sprite->data[offs]];
                if(x>=0&&x<sprite->xSize)
                        if(sprite->data[offs=(y*sprite->xSize+x)]!=sprite->data[0])
                                sprite->data[offs]=gLightningTab[(int)(kLightValues*0.6)][sprite->data[offs]];
                if(x+1>=0&&x+1<sprite->xSize)
                        if(sprite->data[offs=(y*sprite->xSize+x+1)]!=sprite->data[0])
                                sprite->data[offs]=gLightningTab[(int)(kLightValues*0.7)][sprite->data[offs]];
        }
        if(y+1>=0&&y+1<sprite->ySize)
        {
                if(x-1>=0&&x-1<sprite->xSize)
                        if(sprite->data[offs=((y+1)*sprite->xSize+x-1)]!=sprite->data[0])
                                sprite->data[offs]=gLightningTab[(int)(kLightValues*0.9)][sprite->data[offs]];
                if(x>=0&&x<sprite->xSize)
                        if(sprite->data[offs=((y+1)*sprite->xSize+x)]!=sprite->data[0])
                                sprite->data[offs]=gLightningTab[(int)(kLightValues*0.7)][sprite->data[offs]];
                if(x+1>=0&&x+1<sprite->xSize)
                        if(sprite->data[offs=((y+1)*sprite->xSize+x+1)]!=sprite->data[0])
                                sprite->data[offs]=gLightningTab[(int)(kLightValues*0.9)][sprite->data[offs]];
        }
        return id;
}

int BulletHitSprite16(tSpriteHeader16 *sprite,int x,int y)
{
        int offs;
        if(y-1>=0&&y-1<sprite->ySize)
        {
                if(x-1>=0&&x-1<sprite->xSize)
                        if(sprite->data[offs=((y-1)*sprite->xSize+x-1)]!=sprite->data[0])
                                sprite->data[offs]=ShadeRGB16((int)(kLightValues*0.9),sprite->data[offs]);
                if(x>=0&&x<sprite->xSize)
                        if(sprite->data[offs=((y-1)*sprite->xSize+x)]!=sprite->data[0])
                                sprite->data[offs]=ShadeRGB16((int)(kLightValues*0.7),sprite->data[offs]);
                if(x+1>=0&&x+1<sprite->xSize)
                        if(sprite->data[offs=((y-1)*sprite->xSize+x+1)]!=sprite->data[0])
                                sprite->data[offs]=ShadeRGB16((int)(kLightValues*0.9),sprite->data[offs]);
        }
        if(y>=0&&y<sprite->ySize)
        {
                if(x-1>=0&&x-1<sprite->xSize)
                        if(sprite->data[offs=(y*sprite->xSize+x-1)]!=sprite->data[0])
                                sprite->data[offs]=ShadeRGB16((int)(kLightValues*0.7),sprite->data[offs]);
                if(x>=0&&x<sprite->xSize)
                        if(sprite->data[offs=(y*sprite->xSize+x)]!=sprite->data[0])
                                sprite->data[offs]=ShadeRGB16((int)(kLightValues*0.6),sprite->data[offs]);
                if(x+1>=0&&x+1<sprite->xSize)
                        if(sprite->data[offs=(y*sprite->xSize+x+1)]!=sprite->data[0])
                                sprite->data[offs]=ShadeRGB16((int)(kLightValues*0.7),sprite->data[offs]);
        }
        if(y+1>=0&&y+1<sprite->ySize)
        {
                if(x-1>=0&&x-1<sprite->xSize)
                        if(sprite->data[offs=((y+1)*sprite->xSize+x-1)]!=sprite->data[0])
                                sprite->data[offs]=ShadeRGB16((int)(kLightValues*0.9),sprite->data[offs]);
                if(x>=0&&x<sprite->xSize)
                        if(sprite->data[offs=((y+1)*sprite->xSize+x)]!=sprite->data[0])
                                sprite->data[offs]=ShadeRGB16((int)(kLightValues*0.7),sprite->data[offs]);
                if(x+1>=0&&x+1<sprite->xSize)
                        if(sprite->data[offs=((y+1)*sprite->xSize+x+1)]!=sprite->data[0])
                                sprite->data[offs]=ShadeRGB16((int)(kLightValues*0.9),sprite->data[offs]);
        }
        return id;
}
#endif

/*
void BulletHitSprite8(tSpriteHeader *sprite, int x, int y, int size) {
  int offs;
  float invSize = 1 / size;
  int xPos, yPos;
  for (yPos = y - size; yPos <= y + size; yPos++)
    if (yPos >= 0 && yPos < sprite->ySize)
      for (xPos = x - size; xPos <= x + size; xPos++)
        if (xPos >= 0 && xPos < sprite->ySize)
          if (sprite->data[offs = (yPos * sprite->xSize + xPos)] !=
              sprite->data[0])
            sprite->data[offs] = gLightningTab[(
                int)(kLightValues *
                     (0.6 + 0.2 * invSize * (fabs(x - xPos) + fabs(y - yPos))))]
                                              [sprite->data[offs]];
}

void BulletHitSprite16(tSpriteHeader16 *sprite, int x, int y, int size) {
  int offs;
  float invSize = 1 / size;
  int xPos, yPos;
  for (yPos = y - size; yPos <= y + size; yPos++)
    if (yPos >= 0 && yPos < sprite->ySize)
      for (xPos = x - size; xPos <= x + size; xPos++)
        if (xPos >= 0 && xPos < sprite->ySize)
          if (sprite->data[offs = (yPos * sprite->xSize + xPos)] !=
              sprite->data[0])
            sprite->data[offs] =
                ShadeRGB16((int)(kLightValues *
                                 (0.6 + 0.2 * invSize *
                                            (fabs(x - xPos) + fabs(y - yPos)))),
                           sprite->data[offs]);
}

int BulletHitSprite(int id, int x, int y) {
  tSpriteHeader *sprite;
  int size;
  if (id - 128 < kNumSprites) {
    int newID = GetUniqueSpriteNum();
    if (!newID)
      return id;
    gSprites[newID] = gSprites[id - 128];
    HandToHand(&gSprites[newID]);
    sprite = (tSpriteHeader *)*(gSprites[newID]);
    id = newID + 128;
  }
  else
    sprite = (tSpriteHeader *)*(gSprites[id - 128]);
  if (sprite->drawMode & kDrawModeDoubleSize) {
    x *= 2;
    y *= 2;
    size = 2;
  }
  else
    size = 1;
  x += sprite->xSize / 2;
  y -= sprite->ySize / 2;
  y = -y;
  if (gPrefs.hiColor)
    BulletHitSprite16(sprite, x, y, size);
  else
    BulletHitSprite8(sprite, x, y, size);
  return id;
}

void DrawLifeBar8(int cy, int cx, int shift) {
  tSpriteHeader *sprite = (tSpriteHeader *)*(gSprites[kLifeBarId - 128]);
  int y = cy - sprite->ySize / 2;
  int x = cx - sprite->xSize / 2 - shift;
  int vShift = sprite->log2xSize;
  uint8_t mask = sprite->data[0];
  int v = 0;
  int endU = sprite->xSize;
  int startU = shift;
  int endV = sprite->ySize;
  uint8_t *dst = gBaseAddr + y * gRowBytes + x;
  uint8_t *trTab = *gTranslucenceTab;
  for (; v < endV; v++) {
    int u = startU;
    for (; u < endU; u++) {
      uint8_t color = sprite->data[u + (v << vShift)];
      if (color != mask)
        *(dst + u) = trTab[(color << 8) + *(dst + u)];
    }
    dst += gRowBytes;
  }
}

void DrawLifeBar16(int cy, int cx, int shift) {
  tSpriteHeader16 *sprite = (tSpriteHeader *)*(gSprites[kLifeBarId - 128]);
  int y = cy - sprite->ySize / 2;
  int x = cx - sprite->xSize / 2 - shift;
  int vShift = sprite->log2xSize;
  uint16_t mask = sprite->data[0];
  int v = 0;
  int endU = sprite->xSize;
  int startU = shift;
  int endV = sprite->ySize;
  uint16_t *dst = gBaseAddr + y * gRowBytes + 2 * x;
  for (; v < endV; v++) {
    int u = startU;
    for (; u < endU; u++) {
      uint16_t color = sprite->data[u + (v << vShift)];
      if (color != mask)
        *(dst + u) = BlendRGB16(color, *(dst + u));
    }
    dst += gRowBytes / 2;
  }
}

void DrawLifeBar(int cx, int cy, int shift) {
  if (gPrefs.hiColor)
    DrawLifeBar16(cy, cx, shift);
  else
    DrawLifeBar8(cy, cx, shift);
}
*/

void SpriteUnused(int id) {
  if (id - 128 >= kNumSprites)
    if (gSprites[id - 128]) {
      DisposeHandle(gSprites[id - 128]);
      gSprites[id - 128] = NULL;
    }
}

void LoadSprites() {
  // int spritePack = gPrefs.hiColor ? PACK_SPRITES_16 : PACK_SPRITES;
  int spritePack = PACK_SPRITES;
  LoadPack(spritePack);
  for (int i = 128; i < 128 + kNumSprites; i++) {
    int size;
    Ptr data = GetUnsortedPackEntry(spritePack, i, &size);
    if (data)
      // DoError(PtrToHand(data, &gSprites[i - 128], size));
      PtrToHandle(data, &gSprites[i - 128], size);
    else
      gSprites[i - 128] = NULL;
  }
  for (int i = kNumSprites; i < kNumSprites + kNumSpecialSprites; i++) {
    gSprites[i] = NULL;
  }
  UnloadPack(spritePack);
}

void UnloadSprites() {
  int i;
  for (i = 0; i < kNumSprites + kNumSpecialSprites; i++)
    if (gSprites[i]) {
      DisposeHandle(gSprites[i]);
      gSprites[i] = NULL;
    }
}
