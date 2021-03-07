#include <stdint.h>
#include <stdlib.h>

#include "defines.h"
#include "lzrw.h"
#include "packs.h"
#include "resource.h"
// #include "register.h"
// #include "interface.h"

typedef struct {
  int16_t id;
  int16_t placeHolder;
  uint32_t offs;
} tPackHeader;
typedef tPackHeader **tPackHandle;

Handle gPacks[kNumPacks];
#define kUnCryptedHeader 256

/* TODO: Remove once register.c is added */
uint32_t gKey;

uint32_t CryptData(uint32_t *data, uint32_t len) {
  uint32_t check = 0;
  data += kUnCryptedHeader / 4;
  len -= kUnCryptedHeader;
  while (len >= 4) {
    *data = htonl(ntohl(*data) ^ gKey);
    check += ntohl(*data);
    data++;
    len -= 4;
  }
  if (len) {
    *((uint8_t *)data) ^= gKey >> 24;
    check += ntohl((*((uint8_t *)data))++ << 24);
    if (len > 1) {
      *((uint8_t *)data) ^= (gKey >> 16) & 0xff;
      check += ntohl((*((uint8_t *)data))++ << 16);
      if (len > 2) {
        *((uint8_t *)data) ^= (gKey >> 8) & 0xff;
        check += ntohl((*((uint8_t *)data))++ << 8);
      }
    }
  }
  return check;
}

uint32_t LoadPack(int num) {
  uint32_t check = 0;
  if (!gPacks[num]) {
    gPacks[num] = GetResource("Pack", num + 128);
    if (gPacks[num]) {
      /* TODO */
      if (num >= kEncryptedPack /* || gLevelResFile*/) {
        check = CryptData((uint32_t *)*gPacks[num], GetHandleSize(gPacks[num]));
      }
      LZRWDecodeHandle(&gPacks[num]);
      /* HLockHi(gPacks[num]); Locks a handle in memory. Unneeded. */
    }
  }
  return check;
}

/* Only used to check if registration is valid. */
bool CheckPack(int num, uint32_t check) {
  bool ok = false;
  // UseResFile(gAppResFile);
  if (!gPacks[num]) {
    gPacks[num] = GetResource("Pack", num + 128);
    if (gPacks[num]) {
      if (num >= kEncryptedPack) {
        ok = check ==
             CryptData((uint32_t *)*gPacks[num], GetHandleSize(gPacks[num]));
      }
      ReleaseResource(gPacks[num]);
      gPacks[num] = NULL;
    }
  }
  // if (gLevelResFile) {
  //   UseResFile(gLevelResFile);
  // }
  return ok;
}

void UnloadPack(int num) {
  if (gPacks[num]) {
    /* Changed this from DisposeHandle(num) due to information found on
     * https://mirror.informatimago.com/next/developer.apple.com/documentation/mac/Memory/Memory-73.html
     */
    ReleaseResource(gPacks[num]);
    gPacks[num] = NULL;
  }
}

Ptr GetSortedPackEntry(int packNum, int entryID, int *size) {
  tPackHeader *pack = (tPackHeader *)*gPacks[packNum];
  int startId = pack[1].id;
  uint32_t offs = pack[entryID - startId + 1].offs;
  if (size) {
    if (entryID - startId + 1 == pack->id) {
      *size = GetHandleSize(gPacks[packNum]) - offs;
    }
    else {
      *size = pack[entryID - startId + 2].offs - offs;
    }
  }
  return (Ptr)pack + offs;
}

int ComparePackHeaders(const void *p1, const void *p2) {
  tPackHeader pack1 = *(tPackHeader *)p1;
  tPackHeader pack2 = *(tPackHeader *)p2;
  FLIP_SHORT(pack2.id);
  return pack1.id - pack2.id;
}

/**
 * HACK: This was changed a great deal from the original code.
 * It seems to work okay, but it would be good to revisit the logic
 * at some point.
 */
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

int NumPackEntries(int num) {
  return gPacks[num] ? TO_LITTLE_S((**(tPackHandle)gPacks[num]).id) : 0;
}
