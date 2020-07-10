#ifndef __PREFERENCES
#define __PREFERENCES

#include <stdint.h>

#include "defines.h"
#include "input.h"

#define kNumHighScoreEntries 10
#define kPrefsVersion 4

typedef struct {
  Str15 name;
  uint32_t time;
  uint32_t score;
} tScoreRecord;

typedef struct {
  uint16_t version;
  uint16_t volume;
  uint8_t sound, engineSound, hqSound, unused1;
  uint8_t lineSkip, motionBlur, hiColor;
  uint8_t hidElements[kNumElements];
  uint8_t unused[11];
  tScoreRecord high[kNumHighScoreEntries];
  float lapRecords[10];
  Str255 name, code;
  uint8_t keyCodes[kNumElements];
  Str255 lastName;
} tPrefs;

extern tPrefs gPrefs;
void Preferences();
void LoadPrefs();
void WritePrefs(int reset);

#endif
