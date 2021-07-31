#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "error.h"
#include "gamesounds.h"
#include "input.h"
#include "interface.h"
#include "packs.h"
#include "preferences.h"
#include "screen.h"
#include "sprites.h"

enum {
  kOKButton = 1,
  kCancelButton,
  kTitle,
  kControlsBox,
  kGraphicsBox,
  kSoundBox,
  kControlConfButton,
  kLineSkipCBox,
  kMotionBlurCBox,
  kVolumeTitle,
  kVolumeSlider,
  kEngineSoundCBox,
  kHQSoundCBox,
  kHiColorCBox
};

/* TODO: Replace with local static global and have a PREFS_get() function
   to access the preferences for better encapsulation */
Preferences gPrefs;

/* 4096 bytes should be sufficient for a config path */
const int MAX_PATH = 4096;

/* Store the config in a directory for future-proofing */
const char *PREFS_DIR = "open-reckless-drivin";
const char *PREFS_FILE = "prefs.ini";

/* TODO: add standard paths for OSX and Windows */
static bool get_prefs_path(char *buf, const int size) {
  const char *config_home = getenv("XDG_CONFIG_HOME");
  if (config_home) {
    strncpy(buf, config_home, size);
    snprintf(buf, size, "%s/%s/%s", config_home, PREFS_DIR, PREFS_FILE);
    return true;
  }

  const char *home = getenv("HOME");
  if (home == NULL) {
    return false;
  }

  snprintf(buf, size, "%s/.config/%s/%s", home, PREFS_DIR, PREFS_FILE);
  return true;
}

static FILE *get_prefs_file() {
  char prefs_path[MAX_PATH];
  if (!get_prefs_path(prefs_path, MAX_PATH)) {
    return false;
  }
  printf("DEBUG: %s\n", prefs_path);

  FILE *prefs_file = fopen(prefs_path, "r");
  return prefs_file;
}

const int MAX_LINE = 1024;

static bool read_key(char *line, char *key, int *index) {
  int start = *index;

  while (*line && ((*line >= 'a' && *line <= 'z') ||
                   (*line >= 'A' && *line <= 'Z') || *line == '_')) {
    /* Test for too-long key */
    if (*index == PREFS_MAX_KEY - 1) {
      return false;
    }

    key[*index] = *line;
    (*index)++;
    line++;
  }

  /* No key was read */
  if (*index == start) {
    return false;
  }

  if (*line != ' ' && *line != '=') {
    return false;
  }

  key[*index] = 0;

  return true;
}

static bool read_assignment(char *line, int *index) {
  while (*line == ' ') {
    (*index)++;
    line++;
  }
  if (*line != '=') {
    return false;
  }
  (*index)++;
  line++;
  while (*line == ' ') {
    (*index)++;
    line++;
  }
  return true;
}

static bool read_value(char *line, char *value, int *index) {
  int start = *index;

  while (*line) {
    /* test for too-long value */
    if (*index - start == PREFS_MAX_VALUE - 1) {
      return false;
    }

    value[*index - start] = *line;
    (*index)++;
    line++;
  }

  /* No value was read */
  if (start == *index) {
    return false;
  }

  value[*index - start] = 0;

  return true;
}

bool parse_int(char *value, Pref *pref) {
  char *after;
  pref->value.i = strtol(value, &after, 0);
  return *after == 0;
}

bool parse_value(char *value, Pref *pref) {
  /* Booleans */
  if (STREQ(value, "true")) {
    pref->type = PREF_BOOL;
    pref->value.b = true;
  }
  else if (STREQ(value, "false")) {
    pref->type = PREF_BOOL;
    pref->value.b = false;
  }
  /* Integers */
  else if (parse_int(value, pref)) {
    pref->type = PREF_INT;
  }
  /* Strings */
  else {
    pref->type = PREF_STR;
    strncpy(pref->value.s, value, PREFS_MAX_VALUE);
  }

  return true;
}

bool PREFS_read_prefs(FILE *stream, Pref *pref) {
  char line[MAX_LINE];

  while (fgets(line, MAX_LINE, stream)) {
    int length = strlen(line);
    if (line[length - 1] != '\n') {
      /* Line was too long for the buffer */
      continue;
    }
    else if (STREQ(line, "\n") || line[0] == '#') {
      /* Ignore comments and blank lines */
      continue;
    }

    /* Strip trailing whitespace characters */
    while (line[length - 1] == '\n' || line[length - 1] == ' ') {
      line[length - 1] = 0;
      length--;
    }

    int index = 0;
    char key[PREFS_MAX_KEY];
    if (!read_key(line, key, &index)) {
      continue;
    }
    strncpy(pref->key, key, PREFS_MAX_KEY);

    if (!read_assignment(line + index, &index)) {
      continue;
    }

    char value[PREFS_MAX_VALUE];
    if (!read_value(line + index, value, &index)) {
      continue;
    }
    if (!parse_value(value, pref)) {
      continue;
    }

    return true;
  }

  /* EOF reached */
  return false;
}

bool PREFS_load_preferences() {
  FILE *prefs_file = get_prefs_file();
  if (!prefs_file) {
    return false;
  }

  Pref pref;
  while (PREFS_read_prefs(prefs_file, &pref)) {
    /* TODO: While this does work, it would be better to have some sort of
       automated method of assigning the preferences and also validating (min,
       max, length, etc) */
    if (pref.type == PREF_STR && STREQ(pref.key, "name")) {
      memcpy(gPrefs.name, pref.value.s, sizeof(gPrefs.name));
    }
    else if (pref.type == PREF_STR && STREQ(pref.key, "code")) {
      memcpy(gPrefs.code, pref.value.s, sizeof(gPrefs.code));
    }
    else if (pref.type == PREF_BOOL && STREQ(pref.key, "full_color")) {
      gPrefs.full_color = pref.value.b;
    }
  }

  return true;
}

/* static void WritePrefs(bool reset) { */
/*   FSSpec spec; */
/*   short refNum; */
/*   long count = sizeof(Preferences); */
/*   GetPrefsFile(&spec); */
/*   if (reset) */
/*     FirstRun(); */
/*   DoError(FSpOpenDF(&spec, fsRdWrPerm, &refNum)); */
/*   DoError(SetEOF(refNum, sizeof(Preferences))); */
/*   DoError(FSWrite(refNum, &count, &gPrefs)); */
/*   DoError(FSClose(refNum)); */
/* } */

/* TODO: Allow setting preferences from a UI */
/* void ReInitGraphics() { */
/*   DisposeInterface(); */
/*   ScreenMode(kScreenStopped); */
/*   InitScreen(0); */
/*   ShowPicScreen(1003); */
/*   UnloadPack(PACK_RLE_16); */
/*   UnloadPack(PACK_cRLE_16); */
/*   UnloadPack(PACK_TEXTURES_16); */
/*   UnloadPack(PACK_RLE); */
/*   UnloadPack(PACK_cRLE); */
/*   UnloadPack(PACK_TEXTURES); */
/*   UnloadSprites(); */
/*   if (gPrefs.full_color) { */
/*     LoadPack(PACK_RLE_16); */
/*     LoadPack(PACK_cRLE_16); */
/*     LoadPack(PACK_TEXTURES_16); */
/*   } */
/*   else { */
/*     LoadPack(PACK_RLE); */
/*     LoadPack(PACK_cRLE); */
/*     LoadPack(PACK_TEXTURES); */
/*   } */
/*   LoadSprites(); */
/*   InitInterface(); */
/*   ScreenUpdate(NULL); */
/* } */

/* void DeactivateSubControls(ControlHandle cnt) { */
/*   uint16_t i, max; */
/*   DoError(CountSubControls(cnt, &max)); */
/*   for (i = 1; i <= max; i++) { */
/*     ControlHandle subCnt; */
/*     DoError(GetIndexedSubControl(cnt, i, &subCnt)); */
/*     DoError(DeactivateControl(subCnt)); */
/*   } */
/* } */

/* void ActivateSubControls(ControlHandle cnt) { */
/*   uint16_t i, max; */
/*   DoError(CountSubControls(cnt, &max)); */
/*   for (i = 1; i <= max; i++) { */
/*     ControlHandle subCnt; */
/*     DoError(GetIndexedSubControl(cnt, i, &subCnt)); */
/*     DoError(ActivateControl(subCnt)); */
/*   } */
/* } */

/* void Preferences() { */
/*   DialogPtr prefDlg; */
/*   short hit; */
/*   int modeSwitch = false; */
/*   uint8_t soundOn = gPrefs.sound; */
/*   ControlHandle cnt; */
/*   FadeScreen(1); */
/*   ScreenMode(kScreenSuspended); */
/*   FadeScreen(0); */
/*   prefDlg = GetNewDialog(128, NULL, (WindowPtr)-1L); */
/*   DoError(SetDialogDefaultItem(prefDlg, kOKButton)); */
/*   DoError(SetDialogCancelItem(prefDlg, kCancelButton)); */
/*   DoError(GetDialogItemAsControl(prefDlg, kLineSkipCBox, &cnt)); */
/*   SetControlValue(cnt, gPrefs.lineSkip); */
/*   DoError(GetDialogItemAsControl(prefDlg, kMotionBlurCBox, &cnt)); */
/*   SetControlValue(cnt, gPrefs.motionBlur); */
/*   DoError(GetDialogItemAsControl(prefDlg, kEngineSoundCBox, &cnt)); */
/*   SetControlValue(cnt, gPrefs.engineSound); */
/*   DoError(GetDialogItemAsControl(prefDlg, kHQSoundCBox, &cnt)); */
/*   SetControlValue(cnt, gPrefs.hqSound); */
/*   DoError(GetDialogItemAsControl(prefDlg, kVolumeSlider, &cnt)); */
/*   SetControlValue(cnt, gPrefs.volume); */
/*   DoError(GetDialogItemAsControl(prefDlg, kHiColorCBox, &cnt)); */
/*   SetControlValue(cnt, gPrefs.full_color); */
/*   DoError(GetDialogItemAsControl(prefDlg, kSoundBox, &cnt)); */
/*   SetControlValue(cnt, gPrefs.sound); */
/*   if (!gPrefs.sound) */
/*     DeactivateSubControls(cnt); */
/*   gPrefs.sound = true; */
/*   do { */
/*     short type; */
/*     Rect box; */
/*     Handle item; */
/*     ModalDialog(NULL, &hit); */
/*     GetDialogItem(prefDlg, hit, &type, &item, &box); */
/*     if (hit == kSoundBox) */
/*       if (!GetControlValue((ControlHandle)item)) */
/*         ActivateSubControls((ControlHandle)item); */
/*       else */
/*         DeactivateSubControls((ControlHandle)item); */
/*     if (type == chkCtrl + ctrlItem || hit == kSoundBox) */
/*       SetControlValue((ControlHandle)item, */
/*                       !GetControlValue((ControlHandle)item)); */
/*     if (hit == kControlConfButton) { */
/*       HideWindow(prefDlg); */
/*       ConfigureInput(); */
/*       ShowWindow(prefDlg); */
/*       SelectWindow(prefDlg); */
/*     } */
/*     if (hit == kVolumeSlider) { */
/*       int hq = gPrefs.hqSound; */
/*       gPrefs.hqSound = false; */
/*       DoError(GetDialogItemAsControl(prefDlg, kVolumeSlider, &cnt)); */
/*       SetGameVolume(GetControlValue(cnt)); */
/*       SimplePlaySound(129); */
/*       gPrefs.hqSound = hq; */
/*     } */
/*   } while (hit != kOKButton && hit != kCancelButton); */
/*   if (hit == kOKButton) { */
/*     DoError(GetDialogItemAsControl(prefDlg, kLineSkipCBox, &cnt)); */
/*     gPrefs.lineSkip = GetControlValue(cnt); */
/*     DoError(GetDialogItemAsControl(prefDlg, kMotionBlurCBox, &cnt)); */
/*     gPrefs.motionBlur = GetControlValue(cnt); */
/*     DoError(GetDialogItemAsControl(prefDlg, kEngineSoundCBox, &cnt)); */
/*     gPrefs.engineSound = GetControlValue(cnt); */
/*     DoError(GetDialogItemAsControl(prefDlg, kHQSoundCBox, &cnt)); */
/*     gPrefs.hqSound = GetControlValue(cnt); */
/*     DoError(GetDialogItemAsControl(prefDlg, kVolumeSlider, &cnt)); */
/*     gPrefs.volume = GetControlValue(cnt); */
/*     DoError(GetDialogItemAsControl(prefDlg, kSoundBox, &cnt)); */
/*     gPrefs.sound = GetControlValue(cnt); */
/*     DoError(GetDialogItemAsControl(prefDlg, kHiColorCBox, &cnt)); */
/*     if (gPrefs.full_color != GetControlValue(cnt)) */
/*       modeSwitch = true; */
/*     gPrefs.full_color = GetControlValue(cnt); */
/*     WritePrefs(false); */
/*     InitChannels(); */
/*   } */
/*   else */
/*     gPrefs.sound = soundOn; */
/*   SetGameVolume(-1); */
/*   DisposeDialog(prefDlg); */
/*   if (modeSwitch) */
/*     ReInitGraphics(); */
/*   FadeScreen(1); */
/*   ScreenMode(kScreenRunning); */
/*   ScreenUpdate(NULL); */
/*   FadeScreen(0); */
/* } */
