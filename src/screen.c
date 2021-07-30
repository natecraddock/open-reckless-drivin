#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <QDOffscreen.h>
#undef CALL_NOT_IN_CARBON
#define CALL_NOT_IN_CARBON 1
#include <DrawSprocket.h>
#undef CALL_NOT_IN_CARBON

#include "defines.h"
#include "error.h"
#include "preferences.h"
#include "screen.h"
#include "screenfx.h"
#include <math.h>

#define kDecimalPrec 3

DSpContextReference gDrawContext;
Ptr gBaseAddr;
short gRowBytes;
char gMessageBuffer[1024];
char *gMessagePos;
int gMessageCount;
short gXSize, gYSize;
int gFlickerMode = false;
int gOddLines;
Handle gTranslucenceTab = NULL, g16BitClut = NULL;
uint8_t gLightningTab[kLightValues][256];
int gScreenMode;
int gScreenBlitSpecial = false;
int abs(int);

void MakeDecString(int value, StringPtr str) {
  int i;
  str[0] = kDecimalPrec + 1;
  str[1] = '.';
  for (i = kDecimalPrec + 1; i > 1; i--) {
    str[i] = '0' + value % 10;
    value /= 10;
  }
}

void AddStringToMessageBuffer(StringPtr str) {
  int i;
  for (i = 0; i < str[0]; i++)
    *(gMessagePos++) = str[i + 1];
}

void AddFloatToMessageBuffer(StringPtr label, float value) {
  Str255 str;

  AddStringToMessageBuffer(label);
  NumToString(trunc(value), str);
  AddStringToMessageBuffer(str);
  MakeDecString(fabs((value - trunc(value)) * 1000), str);
  AddStringToMessageBuffer(str);
  *(gMessagePos++) = '\n';
  gMessageCount++;
}

void DrawMessageBuffer() {
  int i;
  char *writePos = gMessageBuffer + 1;
  char *oldWritePos = gMessageBuffer;

  TextFont(1);
  TextMode(srcOr);
  for (i = 0; i < gMessageCount; i++) {
    MoveTo(10, (i + 1) * 15);
    while (*writePos != '\n')
      writePos++;
    *oldWritePos = writePos - oldWritePos - 1;
    DrawString(oldWritePos);
    oldWritePos = writePos;
    writePos++;
  }
}

void FlushMessageBuffer() {
  gMessagePos = gMessageBuffer + 1;
  gMessageCount = 0;
}

void InitScreen() {
  DSpContextAttributes inDesiredAttributes;
  int32_t cpuSpeed;
  OSErr err;
  err = Gestalt(gestaltProcClkSpeed, &cpuSpeed);
  gXSize = 640;
  gYSize = 480;
  inDesiredAttributes.frequency = 0;
  inDesiredAttributes.displayWidth = gXSize;
  inDesiredAttributes.displayHeight = gYSize;
  inDesiredAttributes.reserved1 = 0;
  inDesiredAttributes.reserved2 = 0;
  inDesiredAttributes.colorNeeds = kDSpColorNeeds_Request;
  inDesiredAttributes.colorTable = gPrefs.hiColor ? NULL : GetCTable(8);
  inDesiredAttributes.contextOptions = 0;
  inDesiredAttributes.backBufferDepthMask =
      gPrefs.hiColor ? kDSpDepthMask_16 : kDSpDepthMask_8;
  inDesiredAttributes.displayDepthMask =
      gPrefs.hiColor ? kDSpDepthMask_16 : kDSpDepthMask_8;
  inDesiredAttributes.backBufferBestDepth = gPrefs.hiColor ? 16 : 8;
  inDesiredAttributes.displayBestDepth = gPrefs.hiColor ? 16 : 8;
  inDesiredAttributes.pageCount = 0;
  inDesiredAttributes.gameMustConfirmSwitch = false;
  inDesiredAttributes.reserved3[0] = 0;
  inDesiredAttributes.reserved3[1] = 0;
  inDesiredAttributes.reserved3[2] = 0;
  inDesiredAttributes.reserved3[3] = 0;
  DoError(DSpStartup());
  DoError(DSpFindBestContext(&inDesiredAttributes, &gDrawContext));
  inDesiredAttributes.contextOptions =
      kDSpContextOption_PageFlip + (((uint32_t)cpuSpeed < 150000000) || err)
          ? 0
          : kDSpContextOption_DontSyncVBL;
  inDesiredAttributes.pageCount = 3;
  DoError(DSpContext_Reserve(gDrawContext, &inDesiredAttributes));
  gScreenMode = kScreenSuspended;
}

/**
 * FadeGammaIn:
 * https://mirror.informatimago.com/next/developer.apple.com/documentation/mac/Sprockets/GameSprockets-122.html
 * FadeGamma:
 * https://mirror.informatimago.com/next/developer.apple.com/documentation/mac/Sprockets/GameSprockets-120.html
 *
 * these functions fade in/out over the period of 1 second.
 * it is unclear if this happens during the execution of the call, or if the
 * effect is scheduled
 */
void FadeScreen(int out) {
#if __option(scheduling)
  switch (out) {
    case 1:
      DoError(DSpContext_FadeGammaOut(gDrawContext, 0));
      break;
    case 0:
      DoError(DSpContext_FadeGammaIn(gDrawContext, 0));
      break;
  }
  if (out >= 256)
    DSpContext_FadeGamma(gDrawContext, (out - 256) / 256.0 * 100, NULL);
  if (out > 256 + 256)
    DSpContext_FadeGamma(gDrawContext, 0, NULL);
#endif
}

void ScreenMode(int mode) {
  GWorldPtr gameGW;
  // if(gScreenMode!=mode)
  switch (mode) {
    case kScreenRunning:
      DoError(DSpContext_SetState(gDrawContext, kDSpContextState_Active));
      DoError(DSpContext_GetBackBuffer(gDrawContext, kDSpBufferKind_Normal,
                                       &gameGW));
      gBaseAddr = GetPixBaseAddr(GetGWorldPixMap(gameGW));
      gRowBytes = (**GetGWorldPixMap(gameGW)).rowBytes & 0x3FFF;
      SetGWorld(gameGW, NULL);
      if (gScreenMode == kScreenSuspended)
        SetScreenClut(8);
      break;
    case kScreenPaused:
      DoError(DSpContext_SetState(gDrawContext, kDSpContextState_Paused));
      if (gScreenMode == kScreenSuspended)
        SetScreenClut(8);
      break;
    case kScreenSuspended:
      DoError(DSpContext_SetState(gDrawContext, kDSpContextState_Inactive));
      break;
    case kScreenStopped:
      DoError(DSpContext_Release(gDrawContext));
      DoError(DSpShutdown());
      break;
  }
  gScreenMode = mode;
}

GWorldPtr GetScreenGW() {
  GWorldPtr screenGW;
  DoError(DSpContext_GetFrontBuffer(gDrawContext, &screenGW));
  return screenGW;
}

inline RGBColor FadeColor(RGBColor pRGBColor, float fade) {
  RGBColor tRGBColor;

  tRGBColor.red = fade * pRGBColor.red;
  tRGBColor.green = fade * pRGBColor.green;
  tRGBColor.blue = fade * pRGBColor.blue;
  return tRGBColor;
}

void SetScreenClut(int id) {
  if (!gPrefs.hiColor) {
    long bright, color, bestScore, bestIndex, score, testIndex;
    RGBColor optColor, testColor;
    CTabHandle ct = GetCTable(id);
    DoError(DSpContext_SetCLUTEntries(gDrawContext, &((**ct).ctTable), 0, 255));
    if (gTranslucenceTab)
      ReleaseResource(gTranslucenceTab);
    gTranslucenceTab = GetResource('Trtb', id);
    for (bright = 0; bright < kLightValues; bright++)
      for (color = 0; color < 256; color++) {
        bestScore = 3 * 65536;
        bestIndex = 0;
        optColor = (**ct).ctTable[color].rgb;
        optColor = FadeColor(optColor, (float)bright / kLightValues);
        for (testIndex = 0; testIndex < 256; testIndex++) {
          testColor = (**ct).ctTable[testIndex].rgb;
          score = __abs(optColor.red - testColor.red) +
                  __abs(optColor.green - testColor.green) +
                  __abs(optColor.blue - testColor.blue);
          if (score < bestScore) {
            bestScore = score;
            bestIndex = testIndex;
          }
        }
        gLightningTab[bright][color] = bestIndex;
      }
    DisposeCTable(ct);
  }
  else {
    if (g16BitClut)
      ReleaseResource(g16BitClut);
    g16BitClut = GetResource('Cl16', id);
  }
}

void CopyBits_Interlaced(BitMap *srcPixMapP, BitMap *destPixMapP,
                         Rect *srcRectP, Rect *destRectP, uint32_t sourceOdd,
                         uint32_t destOdd) {
  Rect srcRect, destRect;
  PixMap srcMap, destMap;
  int32_t destTopShift, sourceTopShift;

  //  Make local copies of the whole structs; this way, no values need to
  //  be restored. Since this routine only works with PixMaps, assume that
  //  we've been passed PixMaps, whether they're accessed indirectly
  //  through BitMaps or directly.
  if ((srcPixMapP->rowBytes & 0xC000) == 0xC000)
    srcMap = **((PixMapHandle)srcPixMapP->baseAddr);
  else
    srcMap = *((PixMap *)srcPixMapP);

  if ((destPixMapP->rowBytes & 0xC000) == 0xC000)
    destMap = **((PixMapHandle)destPixMapP->baseAddr);
  else
    destMap = *((PixMap *)destPixMapP);

  //  Make local copies of the blit Rects
  srcRect = *srcRectP;
  destRect = *destRectP;

  //  back up 2 rows
  //
  //  This prevents QuickDraw from using an accelerated (hardware)
  //  blitter that's optimized to go directly to VRAM (which might ignore
  //  our hacked rowBytes value). Most of these accelerators compare base
  //  addresses against their VRAM base to determine when they can
  //  accelerate. I'm just messing up this compare.
  //
  //  Backing up two full rows ensures that alignment stays the same and
  //  that the even/odd relationships of the bounding Rects and blit Rects
  //  stays the same.
  {
    destMap.baseAddr -= ((0x3FFF & destMap.rowBytes) << 8);
    destMap.bounds.top -= 256;
    destMap.bounds.bottom -= 2;

    srcMap.baseAddr -= ((0x3FFF & srcMap.rowBytes) << 8);
    srcMap.bounds.top -= 256;
    srcMap.bounds.bottom -= 256;
  }

  //  flips whether even or odd scanlines are being drawn this frame
  if (sourceOdd) {
    //  This bit of adjustment is required to ensure that we're going
    //  to be working with the right set of scanlines. Otherwise, once
    //  the rowBytes have doubled, we would have no way of getting at
    //  the "in-between" scanlines.
    srcMap.baseAddr += (0x3FFF & srcMap.rowBytes);
    // destMap.baseAddr += (0x3FFF & destMap.rowBytes);

    //  Adjust the blit Rect sizes to eliminate rounding errors that
    //  can be introduced if the Rect height is odd. For the odd scanline
    //  pass we force the adjusted Rect height to be floor'ed.
    if ((srcRect.bottom - srcRect.top) & 1)
      srcRect.bottom--;

    // if ((destRect.bottom - destRect.top) & 1)
    //     destRect.bottom--;
  }
  else {
    //  Adjust the blit Rect sizes to eliminate rounding errors that
    //  can be introduced if the Rect height is odd. For the even
    //  scanline pass we force the adjusted Rect height to be ceiling'ed.
    if ((srcRect.bottom - srcRect.top) & 1)
      srcRect.bottom++;

    // if ((destRect.bottom - destRect.top) & 1)
    //     destRect.bottom++;
  }
  if (destOdd) {
    //  This bit of adjustment is required to ensure that we're going
    //  to be working with the right set of scanlines. Otherwise, once
    //  the rowBytes have doubled, we would have no way of getting at
    //  the "in-between" scanlines.
    // srcMap.baseAddr += (0x3FFF & srcMap.rowBytes);
    destMap.baseAddr += (0x3FFF & destMap.rowBytes);

    //  Adjust the blit Rect sizes to eliminate rounding errors that
    //  can be introduced if the Rect height is odd. For the odd scanline
    //  pass we force the adjusted Rect height to be floor'ed.
    // if ((srcRect.bottom - srcRect.top) & 1)
    //    srcRect.bottom--;

    if ((destRect.bottom - destRect.top) & 1)
      destRect.bottom--;
  }
  else {
    //  Adjust the blit Rect sizes to eliminate rounding errors that
    //  can be introduced if the Rect height is odd. For the even
    //  scanline pass we force the adjusted Rect height to be ceiling'ed.
    // if ((srcRect.bottom - srcRect.top) & 1)
    //    srcRect.bottom++;

    if ((destRect.bottom - destRect.top) & 1)
      destRect.bottom++;
  }

  //  Define vertical shift values. Vertical shifting may be necessary if
  //  a rounding error gets introduced in the coming calculations. Both the
  //  srcPixMapP/destPixMapP rectangles and the baseAddress must be
  //  shifted. Note that this would not be necessary if the Rects to be
  //  blitted were restricted to even top values.
  sourceTopShift = (srcRect.top - srcMap.bounds.top) & 1;
  destTopShift = (destRect.top - destMap.bounds.top) & 1;

  if (sourceTopShift)
    srcMap.baseAddr -= srcMap.rowBytes & 0x3FFF;

  if (destTopShift)
    destMap.baseAddr -= destMap.rowBytes & 0x3FFF;

  // scale the source rect to reflect the new rowBytes value
  srcRect.bottom = (srcRect.bottom - srcMap.bounds.top) >> 1;
  srcRect.top = (srcRect.top - srcMap.bounds.top) >> 1;

  // scale the source PixMap bounds to reflect the new rowBytes value
  srcMap.bounds.top >>= 1;
  srcMap.bounds.bottom >>= 1;

  // scale the dest rect to reflect the new rowBytes value
  destRect.bottom = (destRect.bottom - destMap.bounds.top) >> 1;
  destRect.top = (destRect.top - destMap.bounds.top) >> 1;

  // scale the dest PixMap bounds to reflect the new rowBytes value
  destMap.bounds.top >>= 1;
  destMap.bounds.bottom >>= 1;

  // translate the source rect to reflect a change in the srcMap bounds origin
  srcRect.bottom += (srcMap.bounds.top + sourceTopShift);
  srcRect.top += (srcMap.bounds.top + sourceTopShift);

  // translate the dest rect to reflect a change in the destMap bounds origin
  destRect.bottom += (destMap.bounds.top + destTopShift);
  destRect.top += (destMap.bounds.top + destTopShift);

  //  double srcPixMapP rowBytes (keep top two flag bits)
  srcMap.rowBytes =
      (0x3FFF & (srcMap.rowBytes << 1)) | (0xC000 & srcMap.rowBytes);

  //  double destination rowBytes (keep top two flag bits)
  destMap.rowBytes =
      (0x3FFF & (destMap.rowBytes << 1)) | (0xC000 & destMap.rowBytes);

  CopyBits((BitMap *)&srcMap, (BitMap *)&destMap, &srcRect, &destRect, srcCopy,
           NULL);
}

void Blit2Screen() {
  Rect rec;
  GWorldPtr gameGW, screenGW;

  DrawMessageBuffer();
  SetRect(&rec, 0, 0, gXSize, gYSize);
  if (gScreenBlitSpecial) {
    gScreenBlitSpecial = false;
    ShiftInPicture();
  }
  else if (gPrefs.lineSkip) {
    DoError(
        DSpContext_GetBackBuffer(gDrawContext, kDSpBufferKind_Normal, &gameGW));
    DoError(DSpContext_GetFrontBuffer(gDrawContext, &screenGW));
    CopyBits_Interlaced((BitMap *)*GetGWorldPixMap(gameGW),
                        (BitMap *)*GetGWorldPixMap(screenGW), &rec, &rec,
                        gOddLines, gOddLines);
    // CopyBits_Interlaced((BitMap*)*GetGWorldPixMap(gameGW),(BitMap*)*GetGWorldPixMap(screenGW),&rec,&rec,gOddLines,!gOddLines);
    if (gFlickerMode)
      gOddLines = !gOddLines;
    SetGWorld(gameGW, NULL);
  }
  else if (false /* gOSX */) {
    DoError(
        DSpContext_GetBackBuffer(gDrawContext, kDSpBufferKind_Normal, &gameGW));
    DoError(DSpContext_GetFrontBuffer(gDrawContext, &screenGW));
    CopyBits(GetPortBitMapForCopyBits(gameGW),
             GetPortBitMapForCopyBits(screenGW), &rec, &rec, NULL, srcCopy);
    SetGWorld(gameGW, NULL);
  }
  else {
    DoError(DSpContext_InvalBackBufferRect(gDrawContext, &rec));
    DoError(DSpContext_SwapBuffers(gDrawContext, NULL, 0));
    DoError(
        DSpContext_GetBackBuffer(gDrawContext, kDSpBufferKind_Normal, &gameGW));
    gBaseAddr = GetPixBaseAddr(GetGWorldPixMap(gameGW));
    gRowBytes = (**GetGWorldPixMap(gameGW)).rowBytes & 0x3fff;
    SetGWorld(gameGW, NULL);
  }
}

PicHandle MakeGWPicture(Rect *size) {
  PicHandle thePicture;
  OpenCPicParams picHeader = {{0, 0, 0, 0}, 0x00480000, 0x00480000, -2, 0, 0};
  GDHandle gd;
  GWorldPtr gw;
  Rect bounds;

  picHeader.srcRect = *size;
  GetGWorld(&gw, &gd);
  thePicture = OpenCPicture(&picHeader);
  GetPortBounds(gw, &bounds);
  CopyBits(*(BitMapHandle)GetGWorldPixMap(gw),
           *(BitMapHandle)GetGWorldPixMap(gw), &bounds, size, NULL, srcCopy);
  ClosePicture();
  return thePicture;
}

void TakeScreenshot() {
  Rect rec;
  PicHandle screenPic;
  SetRect(&rec, 0, 0, gXSize, gYSize);
  screenPic = MakeGWPicture(&rec);
  AddResource((Handle)screenPic, 'PICT', Count1Resources('PICT') + 1000, "\p");
  ReleaseResource((Handle)screenPic);
}

Point GetScreenPos(Point *inPos) {
  Point pos;
  if (inPos)
    pos = *inPos;
  else
    DoError(DSpGetMouse(&pos));
  DoError(DSpContext_GlobalToLocal(gDrawContext, &pos));
  return pos;
}
