#ifndef __PREFERENCES
#define __PREFERENCES

#include "input.h"

#define kNumHighScoreEntrys 10
#define kPrefsVersion 4

typedef struct {
  Str15 name;
  UInt32 time;
  UInt32 score;
} tScoreRecord;

typedef struct {
  UInt16 version;
  UInt16 volume;
  UInt8 sound, engineSound, hqSound, unused1;
  UInt8 lineSkip, motionBlur, hiColor;
  UInt8 hidElements[kNumElements];
  UInt8 unused[11];
  tScoreRecord high[kNumHighScoreEntrys];
  float lapRecords[10];
  Str255 name, code;
  UInt8 keyCodes[kNumElements];
  Str255 lastName;
} tPrefs;

extern tPrefs gPrefs;
void Preferences();
void LoadPrefs();
void WritePrefs(int reset);

#endif