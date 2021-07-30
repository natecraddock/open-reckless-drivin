#ifndef __PREFERENCES_H
#define __PREFERENCES_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "defines.h"
#include "input.h"

#define PREFS_MAX_KEY 64
#define PREFS_MAX_VALUE 512

typedef enum {
  PREF_ERR, /* TODO: report type of error */
  PREF_BOOL,
  PREF_INT,
  PREF_STR,
} PrefType;

typedef struct {
  PrefType type;
  char key[PREFS_MAX_KEY];
  union {
    int i;
    bool b;
    char s[PREFS_MAX_VALUE];
  } value;
} Pref;

bool PREFS_read_prefs(FILE *stream, Pref *pref);

#define PREFS_NUM_HIGH_SCORE_ENTRIES 10
#define PREFS_VERSION 4

/* TODO: Scores should be stored separate from config (.local/share) */
typedef struct {
  Str15 name;
  uint32_t time;
  uint32_t score;
} ScoreRecord;

typedef struct {
  uint16_t version;
  uint16_t volume;
  uint8_t sound, engineSound, hqSound;
  uint8_t lineSkip, motionBlur, hiColor;
  uint8_t hidElements[kNumElements];
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
