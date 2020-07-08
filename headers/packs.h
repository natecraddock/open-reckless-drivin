#ifndef __PACKS
#define __PACKS

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

#define kEncryptedPack kPackLevel4

UInt32 LoadPack(int);
void UnloadPack(int);
Ptr GetSortedPackEntry(int, int, int *);
Ptr GetUnsortedPackEntry(int, int, int *);
int NumPackEntries(int);
int CheckPack(int num, UInt32 check);
UInt32 BlockChecksum(UInt32 *data, UInt32 len);

#endif