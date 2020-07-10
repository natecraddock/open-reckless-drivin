#ifndef __SCREEN
#define __SCREEN

#include <stdint.h>

enum { kScreenSuspended, kScreenRunning, kScreenStopped, kScreenPaused };

#define kXOrigin (gXSize / 2)
#define kYOrigin (gYSize / 2)
#define kMaxScreenY 640
#define kLightValues 16

extern Ptr gBaseAddr;
extern short gRowBytes;
extern short gXSize, gYSize;
extern int gOddLines;
extern Handle gTranslucenceTab, g16BitClut;
extern uint8_t gLightningTab[kLightValues][256];
extern int gScreenBlitSpecial;

void InitScreen();
void SetScreenClut(int);
void ScreenMode(int);
void Blit2Screen();
void AddFloatToMessageBuffer(StringPtr, float);
void FlushMessageBuffer();
void TakeScreenshot();
Point GetScreenPos(Point *);
GWorldPtr GetScreenGW();
void FadeScreen(int);

inline uint16_t BlendRGB16(uint16_t a, uint16_t b) {
  return ((a & 0x001e) >> 1) + ((b & 0x001e) >> 1) + ((a & 0x03c0) >> 1) +
         ((b & 0x03c0) >> 1) + ((a & 0x7800) >> 1) + ((b & 0x7800) >> 1);
}

inline uint16_t ShadeRGB16(int shade, uint16_t a) {
  return ((a & 0x001e) * shade / kLightValues) +
         ((a & 0x03c0) * shade / kLightValues & 0x03c0) +
         ((a & 0x7800) * shade / kLightValues & 0x7800);
}

#endif
