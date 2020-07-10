#include "packs.h"
#include "interface.h"
#include "lzrwHandleInterface.h"
#include "register.h"
#include <stdlib.h>

typedef struct {
  SInt16 id;
  SInt16 placeHolder;
  UInt32 offs;
} tPackHeader;
typedef tPackHeader **tPackHandle;

Handle gPacks[kNumPacks];
#define kUnCryptedHeader 256

UInt32 CryptData(UInt32 *data, UInt32 len) {
  UInt32 check = 0;
  data += kUnCryptedHeader / 4;
  len -= kUnCryptedHeader;
  while (len >= 4) {
    *data ^= gKey;
    check += *data;
    data++;
    len -= 4;
  }
  if (len) {
    *((UInt8 *)data) ^= gKey >> 24;
    check += (*((UInt8 *)data)++) << 24;
    if (len > 1) {
      *((UInt8 *)data) ^= (gKey >> 16) & 0xff;
      check += (*((UInt8 *)data)++) << 16;
      if (len > 2) {
        *((UInt8 *)data) ^= (gKey >> 8) & 0xff;
        check += (*((UInt8 *)data)++) << 8;
      }
    }
  }
  return check;
}

UInt32 LoadPack(int num) {
  UInt32 check = 0;
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

int CheckPack(int num, UInt32 check) {
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
  UInt32 offs = pack[entryID - startId + 1].offs;
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
  UInt32 offs;
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
  } else
    return 0;
}

int NumPackEntries(int num) {
  return gPacks[num] ? (**(tPackHandle)gPacks[num]).id : 0;
}
