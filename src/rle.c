#include <stdint.h>
#include <stdlib.h>

#include "error.h"
#include "packs.h"
#include "preferences.h"
#include "screen.h"

#define kEndShapeToken 0   // the end of shape maker
#define kLineStartToken 1  // the line start marker
#define kDrawPixelsToken 2 // the draw run marker
#define kSkipPixelsToken 3 // the skip pixels marker

void DrawRLE8(int h, int v, int id) {
  int rowBytes = gRowBytes;
  uint8_t *spritePos = GetSortedPackEntry(PACK_RLE, id, NULL) + sizeof(Rect);
  uint8_t *lineStart = gBaseAddr + h + v * rowBytes;
  uint8_t *dst = lineStart;
  int stop = 0;
  do {
    int32_t tokenData = (*((unsigned long *)spritePos)) & 0x00ffffff;
    switch (*spritePos) {
      case kDrawPixelsToken: {
        int i = 0;
        uint8_t *src = spritePos + 4;
        spritePos += 4 + tokenData + (tokenData & 3 ? (4 - tokenData & 3) : 0);
        while (tokenData - (int)sizeof(long) >= i) {
          *((long *)(dst + i)) = *((long *)(src + i));
          i += sizeof(long);
        }
        if (tokenData - (int)sizeof(short) >= i) {
          *((short *)(dst + i)) = *((short *)(src + i));
          i += sizeof(short);
        }
        if (tokenData - (int)sizeof(char) >= i)
          *((char *)(dst + i)) = *((char *)(src + i));
        dst += tokenData;
      } break;
      case kSkipPixelsToken:
        dst += tokenData;
        spritePos += 4;
        break;
      case kLineStartToken:
        lineStart += rowBytes;
        dst = lineStart;
        spritePos += 4;
        break;
      case kEndShapeToken:
        stop = true;
        break;
      default:
        DoError(paramErr);
    }
  } while (!stop);
}

void DrawRLE16(int h, int v, int id) {
  int rowBytes = gRowBytes;
  uint8_t *spritePos = GetSortedPackEntry(PACK_RLE_16, id, NULL) + sizeof(Rect);
  uint8_t *lineStart = gBaseAddr + h * 2 + v * rowBytes;
  uint16_t *dst = lineStart;
  int stop = 0;
  do {
    int32_t tokenData = (*((unsigned long *)spritePos)) & 0x00ffffff;
    switch (*spritePos) {
      case kDrawPixelsToken: {
        int i = 0;
        uint16_t *src = spritePos + 4;
        int tokenSize = tokenData * 2;
        spritePos += 4 + tokenSize + (tokenSize & 3 ? (4 - tokenSize & 3) : 0);
        while (tokenData - 2 >= i) {
          *((long *)(dst + i)) = *((long *)(src + i));
          i += 2;
        }
        if (tokenData - (int)sizeof(char) >= i)
          *((char *)(dst + i)) = *((char *)(src + i));
        dst += tokenData;
      } break;
      case kSkipPixelsToken:
        dst += tokenData;
        spritePos += 4;
        break;
      case kLineStartToken:
        lineStart += rowBytes;
        dst = lineStart;
        spritePos += 4;
        break;
      case kEndShapeToken:
        stop = true;
        break;
      default:
        DoError(paramErr);
    }
  } while (!stop);
}

void DrawRLEYClip8(int h, int v, int id) {
  int rowBytes = gRowBytes;
  uint8_t *spritePos = GetSortedPackEntry(PACK_RLE, id, NULL) + sizeof(Rect);
  uint8_t *lineStart = gBaseAddr + h + v * rowBytes;
  uint8_t *dst = lineStart;
  int stop = 0;
  do {
    int32_t tokenData = (*((unsigned long *)spritePos)) & 0x00ffffff;
    switch (*spritePos) {
      case kDrawPixelsToken: {
        int i = 0;
        uint8_t *src = spritePos + 4;
        spritePos += 4 + tokenData + (tokenData & 3 ? (4 - tokenData & 3) : 0);
        if (v >= 0) {
          while (tokenData - (int)sizeof(long) >= i) {
            *((long *)(dst + i)) = *((long *)(src + i));
            i += sizeof(long);
          }
          if (tokenData - (int)sizeof(short) >= i) {
            *((short *)(dst + i)) = *((short *)(src + i));
            i += sizeof(short);
          }
          if (tokenData - (int)sizeof(char) >= i)
            *((char *)(dst + i)) = *((char *)(src + i));
        }
        dst += tokenData;
      } break;
      case kSkipPixelsToken:
        dst += tokenData;
        spritePos += 4;
        break;
      case kLineStartToken:
        lineStart += rowBytes;
        dst = lineStart;
        spritePos += 4;
        v++;
        if (v >= gYSize)
          return;
        break;
      case kEndShapeToken:
        stop = true;
        break;
      default:
        DoError(paramErr);
    }
  } while (!stop);
}

void DrawRLEYClip16(int h, int v, int id) {
  int rowBytes = gRowBytes;
  uint8_t *spritePos = GetSortedPackEntry(PACK_RLE_16, id, NULL) + sizeof(Rect);
  uint8_t *lineStart = gBaseAddr + h * 2 + v * rowBytes;
  uint16_t *dst = lineStart;
  int stop = 0;
  do {
    int32_t tokenData = (*((unsigned long *)spritePos)) & 0x00ffffff;
    switch (*spritePos) {
      case kDrawPixelsToken: {
        int i = 0;
        uint16_t *src = spritePos + 4;
        int tokenSize = tokenData * 2;
        spritePos += 4 + tokenSize + (tokenSize & 3 ? (4 - tokenSize & 3) : 0);
        if (v >= 0) {
          while (tokenData - 2 >= i) {
            *((long *)(dst + i)) = *((long *)(src + i));
            i += 2;
          }
          if (tokenData - (int)sizeof(char) >= i)
            *((char *)(dst + i)) = *((char *)(src + i));
        }
        dst += tokenData;
      } break;
      case kSkipPixelsToken:
        dst += tokenData;
        spritePos += 4;
        break;
      case kLineStartToken:
        lineStart += rowBytes;
        dst = lineStart;
        spritePos += 4;
        v++;
        if (v >= gYSize)
          return;
        break;
      case kEndShapeToken:
        stop = true;
        break;
      default:
        DoError(paramErr);
    }
  } while (!stop);
}

void DrawRLE(int h, int v, int id) {
  if (gPrefs.full_color)
    DrawRLE16(h, v, id);
  else
    DrawRLE8(h, v, id);
}

void DrawRLEYClip(int h, int v, int id) {
  if (gPrefs.full_color)
    DrawRLEYClip16(h, v, id);
  else
    DrawRLEYClip8(h, v, id);
}
