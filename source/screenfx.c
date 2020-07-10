#include "gameframe.h"
#include "input.h"
#include "preferences.h"
#include "random.h"
#include "screen.h"
#include "sprites.h"
#include "trig.h"

#define kGameOverSprite 265
#define kFadeStart 1.4
#define kAnimDuration 2.1
#define kShiftInDuration 0.8

enum {
  kSpinIn,
  kFlyIn,
  // kBlur,
  // kWobble,
  kNumGameOverTypes
};

void ScreenClear() {
  GWorldPtr screenGW = GetScreenGW();
  GWorldPtr oldGW;
  GDHandle oldGD;
  Rect r;
  Pattern black;
  GetQDGlobalsBlack(&black);
  GetGWorld(&oldGW, &oldGD);
  SetGWorld(screenGW, nil);
  SetRect(&r, 0, 0, gXSize, gYSize);
  FillRect(&r, &black);
  SetGWorld(oldGW, oldGD);
}

void ShiftInPictureFrame8(float per) {
  int y;
  int skip = gPrefs.lineSkip ? 4 : 2;
  GWorldPtr screenGW = GetScreenGW();
  short screenRowBytes = (**GetGWorldPixMap(screenGW)).rowBytes & 0x3fff;
  UInt8 *screenBaseAddr = GetPixBaseAddr(GetGWorldPixMap(screenGW));
  for (y = 0; y < gYSize; y += skip) {
    float relY = y / (float)gYSize;
    UInt8 *screen, *gw;
    int x = (4 / (relY * relY + 2 * relY + 1) - 4 / (relY + 1) - 1) *
            (1 - per) * gXSize;
    int pixCount;
    if (x > 0)
      x = 0;
    screen = y * screenRowBytes + screenBaseAddr;
    gw = y * gRowBytes + gBaseAddr - x;
    x += gXSize;
    pixCount = 0;
    while (pixCount < x - (int)sizeof(UInt32)) {
      *(UInt32 *)(screen + pixCount) = *(UInt32 *)(gw + pixCount);
      pixCount += sizeof(UInt32);
    }
    while (pixCount < x) {
      *(screen + pixCount) = *(gw + pixCount);
      pixCount++;
    }
  }
  for (y = gPrefs.lineSkip ? 2 : 1; y < gYSize; y += skip) {
    float relY = 1 - (y / (float)gYSize);
    UInt8 *screen, *gw;
    int x = (-4 / (relY * relY + 2 * relY + 1) + 4 / (relY + 1) + 1) *
            (1 - per) * gXSize;
    int pixCount;
    if (x < 0)
      x = 0;
    screen = y * screenRowBytes + screenBaseAddr + x;
    gw = y * gRowBytes + gBaseAddr;
    x = gXSize - x;
    pixCount = 0;
    while (pixCount < x - (int)sizeof(UInt32)) {
      *(UInt32 *)(screen + pixCount) = *(UInt32 *)(gw + pixCount);
      pixCount += sizeof(UInt32);
    }
    while (pixCount < x) {
      *(screen + pixCount) = *(gw + pixCount);
      pixCount++;
    }
  }
}

void ShiftInPictureFrame16(float per) {
  int y;
  int skip = gPrefs.lineSkip ? 4 : 2;
  GWorldPtr screenGW = GetScreenGW();
  short screenRowBytes = (**GetGWorldPixMap(screenGW)).rowBytes & 0x3fff;
  UInt8 *screenBaseAddr = GetPixBaseAddr(GetGWorldPixMap(screenGW));
  for (y = 0; y < gYSize; y += skip) {
    float relY = y / (float)gYSize;
    UInt16 *screen, *gw;
    int x = (4 / (relY * relY + 2 * relY + 1) - 4 / (relY + 1) - 1) *
            (1 - per) * gXSize;
    int pixCount;
    if (x > 0)
      x = 0;
    screen = y * screenRowBytes + screenBaseAddr;
    gw = y * gRowBytes + gBaseAddr - 2 * x;
    x += gXSize;
    pixCount = 0;
    while (pixCount < x - 2) {
      *(UInt32 *)(screen + pixCount) = *(UInt32 *)(gw + pixCount);
      pixCount += 2;
    }
    while (pixCount < x) {
      *(screen + pixCount) = *(gw + pixCount);
      pixCount++;
    }
  }
  for (y = gPrefs.lineSkip ? 2 : 1; y < gYSize; y += skip) {
    float relY = 1 - (y / (float)gYSize);
    UInt16 *screen, *gw;
    int x = (-4 / (relY * relY + 2 * relY + 1) + 4 / (relY + 1) + 1) *
            (1 - per) * gXSize;
    int pixCount;
    if (x < 0)
      x = 0;
    screen = y * screenRowBytes + screenBaseAddr + 2 * x;
    gw = y * gRowBytes + gBaseAddr;
    x = gXSize - x;
    pixCount = 0;
    while (pixCount < x - 2) {
      *(UInt32 *)(screen + pixCount) = *(UInt32 *)(gw + pixCount);
      pixCount += 2;
    }
    while (pixCount < x) {
      *(screen + pixCount) = *(gw + pixCount);
      pixCount++;
    }
  }
}

void ShiftInPicture() {
  UInt64 animStart;
  float t;
  PauseFrameCount();
  animStart = GetMSTime();
  do {
    UInt64 msTime = GetMSTime();
    float size, xPos, yPos, dir;
    msTime -= animStart;
    t = msTime / 1000000.0;
    if (gPrefs.hiColor)
      ShiftInPictureFrame16(t / kShiftInDuration);
    else
      ShiftInPictureFrame8(t / kShiftInDuration);
  } while (t < kShiftInDuration);
  ResumeFrameCount();
}

/*void BlurScreen()
{
        GWorldPtr screenGW=GetScreenGW();
        short rowBytes=(**GetGWorldPixMap(screenGW)).rowBytes&0x3fff;
        UInt8 *baseAddr=GetPixBaseAddr(GetGWorldPixMap(screenGW));
        UInt8 *row=baseAddr+(gYSize-1)*rowBytes;
        UInt8 *upperRow=row-rowBytes;
        UInt8 *trTab=*gTranslucenceTab;
        int x,y;
        for(y=gYSize;y>0;y--)
        {
                for(x=0;x<gXSize;x++)
                        *(row+x)=trTab[(*(row+x)<<8)+*(upperRow+x)];
                row=upperRow;
                upperRow-=rowBytes;
        }
        for(y=gYSize;y>0;y--)
        {
                for(x=0;x<gXSize;x++)
                        *(upperRow+x)=trTab[(*(row+x)<<8)+*(upperRow+x)];
                upperRow=row;
                row+=rowBytes;
        }
}

void BlurScreen16()
{
        GWorldPtr screenGW=GetScreenGW();
        short rowBytes=(**GetGWorldPixMap(screenGW)).rowBytes&0x3fff;
        UInt8 *baseAddr=GetPixBaseAddr(GetGWorldPixMap(screenGW));
        UInt16 *row=baseAddr+(gYSize-1)*rowBytes;
        UInt16 *upperRow=row-rowBytes;
        int x,y;
        for(y=gYSize;y>0;y--)
        {
                for(x=0;x<gXSize;x++)
                        *(row+x)=BlendRGB16(*(row+x),*(upperRow+x));
                row=upperRow;
                upperRow-=rowBytes/2;
        }
        for(y=gYSize;y>0;y--)
        {
                for(x=0;x<gXSize;x++)
                        *(upperRow+x)=BlendRGB16(*(row+x),*(upperRow+x));
                upperRow=row;
                row+=rowBytes/2;
        }
}
*/
void GameOverAnim() {
  Ptr oldBaseAddr = gBaseAddr;
  short oldRowBytes = gRowBytes;
  GWorldPtr screenGW = GetScreenGW();
  int type = RanInt(0, kNumGameOverTypes);
  int side = RanProb(0.5);
  UInt64 animStart;
  float t;
  gBaseAddr = GetPixBaseAddr(GetGWorldPixMap(screenGW));
  gRowBytes = (**GetGWorldPixMap(screenGW)).rowBytes & 0x3fff;
  /*if(type==kBlur)
          DrawSprite(kGameOverSprite,gXSize/2,gYSize/2,0,2);
*/ animStart = GetMSTime();
  do {
    UInt64 msTime = GetMSTime();
    float size, xPos, yPos, dir;
    msTime -= animStart;
    t = msTime / 1000000.0;
    switch (type) {
    case kSpinIn:
      size = t * t;
      xPos = gXSize / 2;
      yPos = gYSize / 2;
      dir = (side ? 1 : -1) * 2 * PI * t;
      break;
    case kFlyIn:
      size = t * t;
      xPos = gXSize / 2;
      if (side)
        yPos = (gYSize / 2) * (4.0 / 3.0) * (1.0 / (-t - 1.0) + 1.0);
      else
        yPos = gYSize - (gYSize / 2) * (4.0 / 3.0) * (1.0 / (-t - 1.0) + 1.0);
      dir = 0;
      break;
      /*		case kBlur:
                              BlurScreen();
                              break;*/
    }
    DrawSprite(kGameOverSprite, xPos, yPos, dir, size);
    if (t > kFadeStart)
      FadeScreen(256 - (t - kFadeStart) / (kAnimDuration - kFadeStart) * 256 +
                 256);
  } while (t < kAnimDuration);
  gBaseAddr = oldBaseAddr;
  gRowBytes = oldRowBytes;
  ScreenClear();
  FadeScreen(512);
}
