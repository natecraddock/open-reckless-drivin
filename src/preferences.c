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
const char *CONFIG_SUFFIX = "open-reckless-drivin/config.ini";

/* TODO: add standard paths for OSX and Windows */
static bool get_config_path(char *buf, const int size) {
  const char *config_home = getenv("XDG_CONFIG_HOME");
  if (config_home) {
    strncpy(buf, config_home, size);
    snprintf(buf, size, "%s/%s", config_home, CONFIG_SUFFIX);
    return true;
  }

  const char *home = getenv("HOME");
  if (home == NULL) {
    return false;
  }

  snprintf(buf, size, "%s/.config/%s", home, CONFIG_SUFFIX);
  return true;
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

bool PREFS_load_preferences() {
  char config_path[MAX_PATH];
  if (!get_config_path(config_path, MAX_PATH)) {
    return false;
  }
  printf("%s\n", config_path);

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
