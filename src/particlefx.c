#include <stdbool.h>
#include <stdint.h>

#include "objects.h"
#include "preferences.h"
#include "random.h"
#include "screen.h"
#include "vec2d.h"

#define kMaxParticles 64
#define kMaxPFX 64
#define E 2.71828182846
#define kParticleDuration 0.5

typedef struct {
  int active;
  int numParticles;
  int layer;
  uint32_t fxStartFrame;
  uint16_t color16;
  uint8_t color;
  t2DPoint pos;
  t2DPoint p[kMaxParticles * 4];
} tParticleFX;

tParticleFX gParticleFX[kMaxPFX];

void NewParticleFX(t2DPoint pos, t2DPoint velo, int num, uint8_t color,
                   int layer, float spread) {
  int numFX;
  for (numFX = 0; numFX < kMaxPFX && gParticleFX[numFX].active; numFX++)
    ;
  if (numFX < kMaxPFX) {
    int i;
    tParticleFX *fx = &gParticleFX[numFX];
    fx->numParticles = num;
    fx->active = true;
    fx->layer = layer;
    fx->fxStartFrame = gFrameCount;
    fx->pos = pos;
    if (gPrefs.full_color)
      fx->color16 = ((uint16_t *)*g16BitClut)[color];
    else
      fx->color = color;
    num *= 4;
    if (num > kMaxParticles)
      num = kMaxParticles;
    for (i = 0; i < num; i++)
      fx->p[i] =
          VEC2D_Sum(velo, P2D(RanFl(-spread, spread), RanFl(-spread, spread)));
  }
}

void DrawParticleFXZoomed(float xDrawStart, float yDrawStart, float zoom,
                          int layer) {
  int numFX;
  float invZoom = 1 / zoom;
  for (numFX = 0; numFX < kMaxPFX; numFX++)
    if (gParticleFX[numFX].active && gParticleFX[numFX].layer == layer) {
      tParticleFX *fx = &gParticleFX[numFX];
      float dt = (gFrameCount - fx->fxStartFrame) * kFrameDuration;
      int numDrawParticles = fx->numParticles * invZoom * invZoom;
      int i;
      if (numDrawParticles > 4 * kMaxParticles)
        numDrawParticles = 4 * kMaxParticles;
      if (dt < kParticleDuration)
        if (gPrefs.full_color)
          for (i = 0; i < numDrawParticles; i++) {
            float epdt = -pow(E, -dt);
            t2DPoint pos =
                VEC2D_Sum(fx->pos, P2D(fx->p[i].x * epdt + fx->p[i].x,
                                       fx->p[i].y * epdt + fx->p[i].y));
            int x = (pos.x - xDrawStart) * invZoom,
                y = (yDrawStart - pos.y) * invZoom;
            if (x > 0 && y > 0)
              if (x < gXSize && y < gYSize)
                *((uint16_t *)(gBaseAddr + y * gRowBytes + x * 2)) =
                    fx->color16;
          }
        else
          for (i = 0; i < numDrawParticles; i++) {
            float epdt = -pow(E, -dt);
            t2DPoint pos =
                VEC2D_Sum(fx->pos, P2D(fx->p[i].x * epdt + fx->p[i].x,
                                       fx->p[i].y * epdt + fx->p[i].y));
            int x = (pos.x - xDrawStart) * invZoom,
                y = (yDrawStart - pos.y) * invZoom;
            if (x > 0 && y > 0)
              if (x < gXSize && y < gYSize)
                *(gBaseAddr + y * gRowBytes + x) = fx->color;
          }
      else
        fx->active = false;
    }
}
