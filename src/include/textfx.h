#ifndef __TEXTFX
#define __TEXTFX

#include <stdint.h>

#include "defines.h"

enum {
  kEffectExplode = 1 << 0,
  kEffectSinLines = 1 << 2,
  kEffectMoveUp = 1 << 3,
  kEffectMoveDown = 1 << 4,
  kEffectMoveLeft = 1 << 5,
  kEffectMoveRight = 1 << 6,
  kEffectTiny = 1 << 7,
  kEffectAbsPos = 1 << 8
};

typedef struct {
  int32_t x, y;
  uint32_t effectFlags;
  uint32_t fxStartFrame;
  Str31 text;
} tTextEffect;

void NewTextEffect(tTextEffect *);
void DrawTextFX(int, int);
void DrawTextFXZoomed(float, float, float);
void SimpleDrawText(Str255, int, int);
void MakeFXStringFromNumStr(Str31, Str31);
void ClearTextFX();

#endif
