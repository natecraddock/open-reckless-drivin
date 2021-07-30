#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* short GetPrefsFile(FSSpec *spec) { */
/*   int err; */
/*   long dirID; */
/*   short vRef; */
/*   DoError(FindFolder(kOnSystemDisk, kPreferencesFolderType, kCreateFolder, */
/*                      &vRef, &dirID)); */
/*   err = FSMakeFSSpec(vRef, dirID, "\pReckless Drivin' Prefs", spec); */
/*   if (err == fnfErr) { */
/*     DoError(FSpCreate(spec, '????', 'pref', smSystemScript)); */
/*     WritePrefs(true); */
/*   } */
/*   else */
/*     DoError(err); */
/* } */

/* void FirstRun() { */
/*   Handle prefDefault; */
/*   int32_t cpuSpeed; */
/*   OSErr err; */
/*   err = Gestalt(gestaltProcClkSpeed, &cpuSpeed); */
/*   if (((uint32_t)cpuSpeed < 120000000) || err) */
/*     prefDefault = GetResource('Pref', 128); */
/*   else if ((uint32_t)cpuSpeed < 250000000) */
/*     prefDefault = GetResource('Pref', 129); */
/*   else */
/*     prefDefault = GetResource('Pref', 130); */
/*   BlockMoveData(*prefDefault, &gPrefs, sizeof(Preferences)); */
/*   ReleaseResource(prefDefault); */
/* } */

const int MAX_LINE = 1024;
const int MAX_KEY = 64;
const int MAX_VALUE = 512;

static bool read_key(char *line, char *key, int *index) {
  while (*line && ((*line >= 'a' && *line <= 'z') ||
                   (*line >= 'A' && *line <= 'Z') || *line == '_')) {
    key[*index] = *line;
    (*index)++;
    line++;

    if (*index == MAX_KEY - 1) {
      return false;
    }
  }

  if (*line != ' ' || *line != '=') {
    false;
  }

  if (*index == MAX_KEY) {
    *index = MAX_KEY - 1;
  }
  key[*index] = 0;

  return true;
}

static bool read_assignment(char *line, int *index) {
  bool found_equals = false;
  while (*line && (*line == ' ' || *line == '=')) {
    if (*line == '=') {
      found_equals = true;
    }
    (*index)++;
    line++;
  }
  return found_equals;
}

static bool read_value(char *line, char *value, int *index) {
  int start = *index;

  while (*line) {
    value[*index - start] = *line;
    (*index)++;
    line++;

    if (*index - start == MAX_VALUE - 1) {
      return false;
    }
  }

  /* No value was read */
  if (start == *index) {
    return false;
  }

  if (*index - start == MAX_VALUE) {
    *index -= 1;
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
  if (strcmp(value, "true") == 0) {
    pref->type = PREF_BOOL;
    pref->value.b = true;
  }
  else if (strcmp(value, "false") == 0) {
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
    strncpy(pref->value.s, value, MAX_VALUE);
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
    else if (strcmp(line, "\n") == 0 || line[0] == '#') {
      /* Ignore comments and blank lines */
      continue;
    }

    /* Strip trailing whitespace characters */
    while (line[length - 1] == '\n' || line[length - 1] == ' ') {
      line[length - 1] = 0;
      length--;
    }

    int index = 0;
    char key[MAX_KEY];
    if (!read_key(line, key, &index)) {
      continue;
    }
    strncpy(pref->key, key, MAX_KEY);

    if (!read_assignment(line + index, &index)) {
      continue;
    }

    char value[MAX_VALUE];
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
    switch (pref.type) {
      case PREF_ERR:
        break;
      case PREF_BOOL:
        printf("Read bool pref `%s=%s`\n", pref.key, pref.value.b ? "true" : "false");
        break;
      case PREF_INT:
        printf("Read int pref `%s=%d`\n", pref.key, pref.value.i);
        break;
      case PREF_STR:
        printf("Read str pref `%s=%s`\n", pref.key, pref.value.s);
        break;
    }
  }

  /* FSSpec spec; */
  /* short refNum; */
  /* long count = sizeof(Preferences), eof; */
  /* GetPrefsFile(&spec); */
  /* DoError(FSpOpenDF(&spec, fsRdWrPerm, &refNum)); */
  /* DoError(GetEOF(refNum, &eof)); */
  /* if (eof == count) { */
  /*   DoError(FSRead(refNum, &count, &gPrefs)); */
  /*   DoError(FSClose(refNum)); */
  /*   if (gPrefs.version != PREFS_VERSION) */
  /*     WritePrefs(true); */
  /* } */
  /* else if (eof < count) { */
  /*   FirstRun(); */
  /*   DoError(FSRead(refNum, &eof, &gPrefs)); */
  /*   gPrefs.version = PREFS_VERSION; */
  /*   DoError(FSClose(refNum)); */
  /* } */
  /* else { */
  /*   DoError(FSClose(refNum)); */
  /*   WritePrefs(true); */
  /* } */
  /* if (false /1* gOSX *1/) */
  /*   gPrefs.lineSkip = false; */

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
/*   if (gPrefs.hiColor) { */
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
/*   SetControlValue(cnt, gPrefs.hiColor); */
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
/*     if (gPrefs.hiColor != GetControlValue(cnt)) */
/*       modeSwitch = true; */
/*     gPrefs.hiColor = GetControlValue(cnt); */
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
