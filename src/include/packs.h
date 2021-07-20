#ifndef __PACKS
#define __PACKS

#include <stdbool.h>
#include <stdint.h>

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

#define ENCRYPTED_PACK kPackLevel4

extern uint32_t gKey;

uint32_t LoadPack(int);
void UnloadPack(int);
Ptr GetSortedPackEntry(int, int, int *);
Ptr GetUnsortedPackEntry(int, int, int *);
int NumPackEntries(int);
bool CheckPack(int num, uint32_t check);
uint32_t BlockChecksum(uint32_t *data, uint32_t len);

#endif
