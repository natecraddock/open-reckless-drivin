#ifndef __SCREEN
#define __SCREEN

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
extern UInt8 gLightningTab[kLightValues][256];
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

inline UInt16 BlendRGB16(UInt16 a, UInt16 b) {
  return ((a & 0x001e) >> 1) + ((b & 0x001e) >> 1) + ((a & 0x03c0) >> 1) +
         ((b & 0x03c0) >> 1) + ((a & 0x7800) >> 1) + ((b & 0x7800) >> 1);
}

inline UInt16 ShadeRGB16(int shade, UInt16 a) {
  return ((a & 0x001e) * shade / kLightValues) +
         ((a & 0x03c0) * shade / kLightValues & 0x03c0) +
         ((a & 0x7800) * shade / kLightValues & 0x7800);
}

#endif
