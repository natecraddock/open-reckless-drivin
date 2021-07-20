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
  int16_t placeholder;
  uint32_t offset;
} PackHeader;
typedef PackHeader **PackHandle;

static Handle packs[kNumPacks];

/* Length of unencrypted header */
#define UNENCRYPTED_HEADER_LEN 256

/* TODO: Remove once register.c is added */
uint32_t gKey;

uint32_t CryptData(uint32_t *data, uint32_t len) {
  uint32_t check = 0;
  data += UNENCRYPTED_HEADER_LEN / 4;
  len -= UNENCRYPTED_HEADER_LEN;
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
  if (!packs[num]) {
    packs[num] = GetResource("Pack", num + 128);
    if (packs[num]) {
      /* TODO */
      if (num >= ENCRYPTED_PACK /* || gLevelResFile*/) {
        check = CryptData((uint32_t *)*packs[num], GetHandleSize(packs[num]));
      }
      LZRWDecodeHandle(&packs[num]);
      /* HLockHi(packs[num]); Locks a handle in memory. Unneeded. */
    }
  }
  return check;
}

/* Only used to check if registration is valid. */
bool CheckPack(int num, uint32_t check) {
  bool ok = false;
  // UseResFile(gAppResFile);
  if (!packs[num]) {
    packs[num] = GetResource("Pack", num + 128);
    if (packs[num]) {
      if (num >= ENCRYPTED_PACK) {
        ok = check ==
             CryptData((uint32_t *)*packs[num], GetHandleSize(packs[num]));
      }
      ReleaseResource(packs[num]);
      packs[num] = NULL;
    }
  }
  // if (gLevelResFile) {
  //   UseResFile(gLevelResFile);
  // }
  return ok;
}

void UnloadPack(int num) {
  if (packs[num]) {
    /* Any valid pack has been decompressed, which means it is no longer a
       resource handle, but a memory handle, so we must use DisposeHandle to
       free that allocated memory. */
    DisposeHandle(packs[num]);
    packs[num] = NULL;
  }
}

Ptr GetSortedPackEntry(int packNum, int entryID, int *size) {
  PackHeader *pack = (PackHeader *)*packs[packNum];
  int startId = pack[1].id;
  uint32_t offset = pack[entryID - startId + 1].offset;
  if (size) {
    if (entryID - startId + 1 == pack->id) {
      *size = GetHandleSize(packs[packNum]) - offset;
    }
    else {
      *size = pack[entryID - startId + 2].offset - offset;
    }
  }
  return (Ptr)pack + offset;
}

int ComparePackHeaders(const void *p1, const void *p2) {
  PackHeader pack1 = *(PackHeader *)p1;
  PackHeader pack2 = *(PackHeader *)p2;
  FLIP_SHORT(pack2.id);
  return pack1.id - pack2.id;
}

/**
 * HACK: This was changed a great deal from the original code.
 * It seems to work okay, but it would be good to revisit the logic
 * at some point.
 */
Ptr GetUnsortedPackEntry(int packNum, int entryID, int *size) {
  PackHeader pack = *(PackHeader *)*packs[packNum];
  FLIP_SHORT(pack.id);
  FLIP_LONG(pack.offset);

  PackHeader key, *found;
  uint32_t offset;
  key.id = entryID;
  found = bsearch(&key, (PackHeader *)*packs[packNum] + 1, pack.id,
                  sizeof(PackHeader), ComparePackHeaders);
  if (!found) {
    return NULL;
  }

  offset = found->offset;
  FLIP_LONG(offset);
  if (size) {
    if (pack.id == found - (PackHeader *)*packs[packNum]) {
      *size = GetHandleSize(packs[packNum]) - offset;
    }
    else {
      PackHeader other = *(found + 1);
      FLIP_LONG(other.offset);
      *size = other.offset - offset;
    }
  }
  return (char *)*packs[packNum] + offset;
}

int NumPackEntries(int num) {
  return packs[num] ? TO_LITTLE_S((**(PackHandle)packs[num]).id) : 0;
}
