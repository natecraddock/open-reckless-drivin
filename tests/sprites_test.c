#include <math.h>
#include <stdio.h>

#include <arpa/inet.h>

#include "defines.h"
#include "lzrw.h"
#include "resource.h"

enum {
  kPackObTy,
  kPackSprt,
  kPackOgrp,
  kPacksRLE,
  kPackcRLE,
  kPackTxtR,
  kPackSnds,
  kPackRoad,
  kPackTx16,
  kPackSp16,
  kPacksR16,
  kPackcR16,
  kPackLevel1,
  kPackLevel2,
  kPackLevel3,
  kPackLevel4,
  kPackLevel5,
  kPackLevel6,
  kPackLevel7,
  kPackLevel8,
  kPackLevel9,
  kPackLevel10,
  kNumPacks
};

#define FLIP_SHORT(var) var = ntohs((var))
#define FLIP_LONG(var) var = ntohl((var))

typedef struct {
  int16_t id;
  int16_t placeHolder;
  uint32_t offs;
} tPackHeader;
typedef tPackHeader **tPackHandle;

typedef struct {
  uint16_t xSize, ySize;
  uint8_t log2xSize;
  uint8_t filler1;
  uint8_t drawMode;
  uint8_t filler2;
  uint8_t data[1];
} tSpriteHeader;

#define kNumSprites 300
#define kNumSpecialSprites 100

#define kUnCryptedHeader 256
#define kEncryptedPack kPackLevel4

Handle gPacks[kNumPacks];
uint32_t gKey;

Handle gSprites[kNumSprites + kNumSpecialSprites];

uint32_t CryptData(uint32_t *data, uint32_t len) {
  uint32_t check = 0;
  /* Skip over the header. */
  data += kUnCryptedHeader / 4;
  len -= kUnCryptedHeader;

  while (len >= 4) {
    // *data ^= gKey;
    // check += *data;
    /* Fix for endianness */
    *data = htonl(ntohl(*data) ^ gKey);
    check += ntohl(*data);
    data++;
    len -= 4;
  }

  if (len) {
    *((uint8_t *)data) ^= gKey >> 24;
    /* check += (*((uint8_t *)data)++) << 24; */
    check += ntohl((*((uint8_t *)data))++ << 24);
    if (len > 1) {
      *((uint8_t *)data) ^= (gKey >> 16) & 0xff;
      /* check += (*((uint8_t *)data)++) << 16; */
      check += ntohl((*((uint8_t *)data))++ << 16);
      if (len > 2) {
        *((uint8_t *)data) ^= (gKey >> 8) & 0xff;
        /* check += (*((uint8_t *)data)++) << 8; */
        check += ntohl((*((uint8_t *)data))++ << 8);
      }
    }
  }
  return check;
}

char Hex2Num(char hex) {
  if (hex <= '9') {
    return hex - '0';
  }
  return hex - 'A' + 10;
}

uint32_t CodeStr2Long(const char *code) {
  uint32_t codeNum = 0;
  uint32_t seed = 0;

  /* Validate code. */
  if (strlen(code) != 10) {
    return 0;
  }
  for (int digi = 0; digi < 10; digi++) {
    if (!((code[digi] >= '0' && code[digi] <= '9') ||
          (code[digi] >= 'A' && code[digi] <= 'F'))) {
      return 0;
    }
  }

  for (int digi = 9; digi > 7; digi--) {
    seed += Hex2Num(code[digi]) * pow(16, 9 - digi);
  }
  seed = seed + (seed << 8) + (seed << 16) + (seed << 24);
  for (int digi = 7; digi >= 0; digi--) {
    codeNum += Hex2Num(code[digi]) * pow(16, 7 - digi);
  }
  return codeNum ^ seed;
}

void set_gkey() {
  char name[] = "FREE";
  char code[] = "B3FB09B1EB";

  uint32_t name_num = ntohl(*(uint32_t *)(name + strlen(name) - 4));
  uint32_t code_num = CodeStr2Long(code);

  gKey = code_num ^ name_num;
}

uint32_t LoadPack(int num) {
  uint32_t check = 0;
  if (!gPacks[num]) {
    gPacks[num] = GetResource("Pack", num + 128);
    if (gPacks[num]) {
      if (num >= kEncryptedPack) {
        check = CryptData((uint32_t *)*gPacks[num], GetHandleSize(gPacks[num]));
      }
      LZRWDecodeHandle(&gPacks[num]);
    }
  }
  return check;
}

int ComparePackHeaders(const void *p1, const void *p2) {
  tPackHeader pack1 = *(tPackHeader *)p1;
  tPackHeader pack2 = *(tPackHeader *)p2;
  FLIP_SHORT(pack2.id);
  return pack1.id - pack2.id;
}

Ptr GetUnsortedPackEntry(int packNum, int entryID, int *size) {
  tPackHeader pack = *(tPackHeader *)*gPacks[packNum];
  FLIP_SHORT(pack.id);
  FLIP_LONG(pack.offs);

  tPackHeader key, *found;
  uint32_t offs;
  key.id = entryID;
  found = bsearch(&key, (tPackHeader *)*gPacks[packNum] + 1, pack.id,
                  sizeof(tPackHeader), ComparePackHeaders);
  if (!found) {
    return NULL;
  }

  offs = found->offs;
  FLIP_LONG(offs);
  if (size) {
    if (pack.id == found - (tPackHeader *)*gPacks[packNum]) {
      *size = GetHandleSize(gPacks[packNum]) - offs;
    }
    else {
      tPackHeader other = *(found + 1);
      FLIP_LONG(other.offs);
      *size = other.offs - offs;
    }
  }
  return (char *)*gPacks[packNum] + offs;
}

void LoadSprites() {
  int spritePack = kPackSprt;
  LoadPack(spritePack);
  for (int i = 128; i < 128 + kNumSprites; i++) {
    int size;
    Ptr data = GetUnsortedPackEntry(spritePack, i, &size);
    if (data) {
      PtrToHandle(data, &gSprites[i - 128], size);
    }
    else {
    }
  }
}

void write_ppm(uint8_t *pixels, int w, int h, const char *file) {
  FILE *out = fopen(file, "w");
  fprintf(out, "P6\n");
  fprintf(out, "%u %u\n", w, h);
  fprintf(out, "255\n");

  /* Write each pixel */
  for (int i = 0; i < w * h; i++) {
    fwrite(&pixels[i], sizeof(char), 1, out);
    fwrite(&pixels[i], sizeof(char), 1, out);
    fwrite(&pixels[i], sizeof(char), 1, out);
  }

  fclose(out);
}

int main(int argc, char **argv) {
  set_gkey();

  LoadSprites();

  tSpriteHeader *sprite = (tSpriteHeader *)*gSprites[147 - 128];
  FLIP_SHORT(sprite->xSize);
  FLIP_SHORT(sprite->ySize);
  printf("Got the sprite!\n");

  printf("%p%p\n%p\n", *gSprites[0], sprite, sprite->data);
  write_ppm((uint8_t *)&sprite->data[0], sprite->xSize, sprite->ySize,
            "car.ppm");

  return 0;
}
