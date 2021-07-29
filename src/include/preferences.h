#ifndef __PREFERENCES_H
#define __PREFERENCES_H

#include <stdbool.h>
#include <stdint.h>

#include "defines.h"
#include "input.h"

#define PREFS_NUM_HIGH_SCORE_ENTRIES 10
#define PREFS_VERSION 4

typedef struct {
  Str15 name;
  uint32_t time;
  uint32_t score;
} ScoreRecord;

typedef struct {
  uint16_t version;
  uint16_t volume;
  uint8_t sound, engineSound, hqSound, unused1;
  uint8_t lineSkip, motionBlur, hiColor;
  uint8_t hidElements[kNumElements];
  uint8_t unused[11];
  ScoreRecord high[PREFS_NUM_HIGH_SCORE_ENTRIES];
  float lapRecords[10];
  Str255 name, code;
  uint8_t keyCodes[kNumElements];
  Str255 lastName;
} Preferences;

extern Preferences gPrefs;
/* void Preferences(); */
bool PREFS_load_preferences();

#endif /* __PREFERENCES_H */
