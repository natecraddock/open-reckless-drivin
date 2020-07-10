#include <stdint.h>

#include "interface.h"
#include "lzrwHandleInterface.h"
#include "packs.h"
#include "register.h"
#include <stdlib.h>

typedef struct {
  int16_t id;
  int16_t placeHolder;
  uint32_t offs;
} tPackHeader;
typedef tPackHeader **tPackHandle;

Handle gPacks[kNumPacks];
#define kUnCryptedHeader 256

uint32_t CryptData(uint32_t *data, uint32_t len) {
  uint32_t check = 0;
  data += kUnCryptedHeader / 4;
  len -= kUnCryptedHeader;
  while (len >= 4) {
    *data ^= gKey;
    check += *data;
    data++;
    len -= 4;
  }
  if (len) {
    *((uint8_t *)data) ^= gKey >> 24;
    check += (*((uint8_t *)data)++) << 24;
    if (len > 1) {
      *((uint8_t *)data) ^= (gKey >> 16) & 0xff;
      check += (*((uint8_t *)data)++) << 16;
      if (len > 2) {
        *((uint8_t *)data) ^= (gKey >> 8) & 0xff;
        check += (*((uint8_t *)data)++) << 8;
      }
    }
  }
  return check;
}

uint32_t LoadPack(int num) {
  uint32_t check = 0;
  if (!gPacks[num]) {
    gPacks[num] = GetResource('Pack', num + 128);
    if (gPacks[num]) {
      if (num >= kEncryptedPack || gLevelResFile)
        check = CryptData(*gPacks[num], GetHandleSize(gPacks[num]));
      LZRWDecodeHandle(&gPacks[num]);
      HLockHi(gPacks[num]);
    }
  }
  return check;
}

int CheckPack(int num, uint32_t check) {
  int ok = false;
  UseResFile(gAppResFile);
  if (!gPacks[num]) {
    gPacks[num] = GetResource('Pack', num + 128);
    if (gPacks[num]) {
      if (num >= kEncryptedPack)
        ok = check == CryptData(*gPacks[num], GetHandleSize(gPacks[num]));
      ReleaseResource(gPacks[num]);
      gPacks[num] = nil;
    }
  }
  if (gLevelResFile)
    UseResFile(gLevelResFile);
  return ok;
}

void UnloadPack(int num) {
  if (gPacks[num]) {
    DisposeHandle(gPacks[num]);
    gPacks[num] = nil;
  }
}

Ptr GetSortedPackEntry(int packNum, int entryID, int *size) {
  tPackHeader *pack = (tPackHeader *)*gPacks[packNum];
  int startId = pack[1].id;
  uint32_t offs = pack[entryID - startId + 1].offs;
  if (size)
    if (entryID - startId + 1 == pack->id)
      *size = GetHandleSize(gPacks[packNum]) - offs;
    else
      *size = pack[entryID - startId + 2].offs - offs;
  return (Ptr)pack + offs;
}

int ComparePackHeaders(const tPackHeader *p1, const tPackHeader *p2) {
  return p1->id - p2->id;
}

Ptr GetUnsortedPackEntry(int packNum, int entryID, int *size) {
  tPackHeader *pack = (tPackHeader *)*gPacks[packNum];
  tPackHeader key, *found;
  uint32_t offs;
  key.id = entryID;
  found = bsearch(&key, pack + 1, pack->id, sizeof(tPackHeader),
                  ComparePackHeaders);
  if (found) {
    offs = found->offs;
    if (size)
      if (pack->id == found - pack)
        *size = GetHandleSize(gPacks[packNum]) - offs;
      else
        *size = (found + 1)->offs - offs;
    return (Ptr)pack + offs;
  }
  else
    return 0;
}

int NumPackEntries(int num) {
  return gPacks[num] ? (**(tPackHandle)gPacks[num]).id : 0;
}
