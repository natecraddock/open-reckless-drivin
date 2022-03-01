#ifndef __PACKS_H
#define __PACKS_H

#include <stdbool.h>
#include <stdint.h>

#include "defines.h"

// TODO: Start this enum at 128 and remove the `128 + _` references elsewhere
typedef enum {
  PACK_OBJECT_TYPE,
  PACK_SPRITES,
  PACK_OBJECT_GROUPS,
  PACK_RLE,
  PACK_cRLE,
  PACK_TEXTURES,
  PACK_SOUNDS,
  PACK_ROAD,
  PACK_TEXTURES_16,
  PACK_SPRITES_16,
  PACK_RLE_16,
  PACK_cRLE_16,
  PACK_LEVEL_01,
  PACK_LEVEL_02,
  PACK_LEVEL_03,
  PACK_LEVEL_04,
  PACK_LEVEL_05,
  PACK_LEVEL_06,
  PACK_LEVEL_07,
  PACK_LEVEL_08,
  PACK_LEVEL_09,
  PACK_LEVEL_10,
  PACK_NUM_PACKS
} Packs;

#define PACK_ENCRYPTED PACK_LEVEL_04

uint32_t PACK_load(int);
bool PACK_check(int num, uint32_t check);
void PACK_unload(int);
Ptr PACK_get_sorted_entry(int, int, int *);
Ptr PACK_get_unsorted_entry(int, int, int *);
int PACK_num_entries(int);

#endif // __PACKS_H
