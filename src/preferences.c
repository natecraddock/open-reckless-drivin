#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

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

tPrefs gPrefs;

short GetPrefsFile(FSSpec *spec) {
  int err;
  long dirID;
  short vRef;
  DoError(FindFolder(kOnSystemDisk, kPreferencesFolderType, kCreateFolder,
                     &vRef, &dirID));
  err = FSMakeFSSpec(vRef, dirID, "\pReckless Drivin' Prefs", spec);
  if (err == fnfErr) {
    DoError(FSpCreate(spec, '????', 'pref', smSystemScript));
    WritePrefs(true);
  }
  else
    DoError(err);
}

void ReInitGraphics() {
  DisposeInterface();
  ScreenMode(kScreenStopped);
  InitScreen(0);
  ShowPicScreen(1003);
  UnloadPack(PACK_RLE_16);
  UnloadPack(PACK_cRLE_16);
  UnloadPack(PACK_TEXTURES_16);
  UnloadPack(PACK_RLE);
  UnloadPack(PACK_cRLE);
  UnloadPack(PACK_TEXTURES);
  UnloadSprites();
  if (gPrefs.hiColor) {
    LoadPack(PACK_RLE_16);
    LoadPack(PACK_cRLE_16);
    LoadPack(PACK_TEXTURES_16);
  }
  else {
    LoadPack(PACK_RLE);
    LoadPack(PACK_cRLE);
    LoadPack(PACK_TEXTURES);
  }
  LoadSprites();
  InitInterface();
  ScreenUpdate(NULL);
}

void FirstRun() {
  Handle prefDefault;
  int32_t cpuSpeed;
  OSErr err;
  err = Gestalt(gestaltProcClkSpeed, &cpuSpeed);
  if (((uint32_t)cpuSpeed < 120000000) || err)
    prefDefault = GetResource('Pref', 128);
  else if ((uint32_t)cpuSpeed < 250000000)
    prefDefault = GetResource('Pref', 129);
  else
    prefDefault = GetResource('Pref', 130);
  BlockMoveData(*prefDefault, &gPrefs, sizeof(tPrefs));
  ReleaseResource(prefDefault);
}

void LoadPrefs() {
  FSSpec spec;
  short refNum;
  long count = sizeof(tPrefs), eof;
  GetPrefsFile(&spec);
  DoError(FSpOpenDF(&spec, fsRdWrPerm, &refNum));
  DoError(GetEOF(refNum, &eof));
  if (eof == count) {
    DoError(FSRead(refNum, &count, &gPrefs));
    DoError(FSClose(refNum));
    if (gPrefs.version != kPrefsVersion)
      WritePrefs(true);
  }
  else if (eof < count) {
    FirstRun();
    DoError(FSRead(refNum, &eof, &gPrefs));
    gPrefs.version = kPrefsVersion;
    DoError(FSClose(refNum));
  }
  else {
    DoError(FSClose(refNum));
    WritePrefs(true);
  }
  if (false /* gOSX */)
    gPrefs.lineSkip = false;
}

void WritePrefs(int reset) {
  FSSpec spec;
  short refNum;
  long count = sizeof(tPrefs);
  GetPrefsFile(&spec);
  if (reset)
    FirstRun();
  DoError(FSpOpenDF(&spec, fsRdWrPerm, &refNum));
  DoError(SetEOF(refNum, sizeof(tPrefs)));
  DoError(FSWrite(refNum, &count, &gPrefs));
  DoError(FSClose(refNum));
}

void DeactivateSubControls(ControlHandle cnt) {
  uint16_t i, max;
  DoError(CountSubControls(cnt, &max));
  for (i = 1; i <= max; i++) {
    ControlHandle subCnt;
    DoError(GetIndexedSubControl(cnt, i, &subCnt));
    DoError(DeactivateControl(subCnt));
  }
}

void ActivateSubControls(ControlHandle cnt) {
  uint16_t i, max;
  DoError(CountSubControls(cnt, &max));
  for (i = 1; i <= max; i++) {
    ControlHandle subCnt;
    DoError(GetIndexedSubControl(cnt, i, &subCnt));
    DoError(ActivateControl(subCnt));
  }
}

void Preferences() {
  DialogPtr prefDlg;
  short hit;
  int modeSwitch = false;
  uint8_t soundOn = gPrefs.sound;
  ControlHandle cnt;
  FadeScreen(1);
  ScreenMode(kScreenSuspended);
  FadeScreen(0);
  prefDlg = GetNewDialog(128, NULL, (WindowPtr)-1L);
  DoError(SetDialogDefaultItem(prefDlg, kOKButton));
  DoError(SetDialogCancelItem(prefDlg, kCancelButton));
  DoError(GetDialogItemAsControl(prefDlg, kLineSkipCBox, &cnt));
  SetControlValue(cnt, gPrefs.lineSkip);
  DoError(GetDialogItemAsControl(prefDlg, kMotionBlurCBox, &cnt));
  SetControlValue(cnt, gPrefs.motionBlur);
  DoError(GetDialogItemAsControl(prefDlg, kEngineSoundCBox, &cnt));
  SetControlValue(cnt, gPrefs.engineSound);
  DoError(GetDialogItemAsControl(prefDlg, kHQSoundCBox, &cnt));
  SetControlValue(cnt, gPrefs.hqSound);
  DoError(GetDialogItemAsControl(prefDlg, kVolumeSlider, &cnt));
  SetControlValue(cnt, gPrefs.volume);
  DoError(GetDialogItemAsControl(prefDlg, kHiColorCBox, &cnt));
  SetControlValue(cnt, gPrefs.hiColor);
  DoError(GetDialogItemAsControl(prefDlg, kSoundBox, &cnt));
  SetControlValue(cnt, gPrefs.sound);
  if (!gPrefs.sound)
    DeactivateSubControls(cnt);
  gPrefs.sound = true;
  do {
    short type;
    Rect box;
    Handle item;
    ModalDialog(NULL, &hit);
    GetDialogItem(prefDlg, hit, &type, &item, &box);
    if (hit == kSoundBox)
      if (!GetControlValue((ControlHandle)item))
        ActivateSubControls((ControlHandle)item);
      else
        DeactivateSubControls((ControlHandle)item);
    if (type == chkCtrl + ctrlItem || hit == kSoundBox)
      SetControlValue((ControlHandle)item,
                      !GetControlValue((ControlHandle)item));
    if (hit == kControlConfButton) {
      HideWindow(prefDlg);
      ConfigureInput();
      ShowWindow(prefDlg);
      SelectWindow(prefDlg);
    }
    if (hit == kVolumeSlider) {
      int hq = gPrefs.hqSound;
      gPrefs.hqSound = false;
      DoError(GetDialogItemAsControl(prefDlg, kVolumeSlider, &cnt));
      SetGameVolume(GetControlValue(cnt));
      SimplePlaySound(129);
      gPrefs.hqSound = hq;
    }
  } while (hit != kOKButton && hit != kCancelButton);
  if (hit == kOKButton) {
    DoError(GetDialogItemAsControl(prefDlg, kLineSkipCBox, &cnt));
    gPrefs.lineSkip = GetControlValue(cnt);
    DoError(GetDialogItemAsControl(prefDlg, kMotionBlurCBox, &cnt));
    gPrefs.motionBlur = GetControlValue(cnt);
    DoError(GetDialogItemAsControl(prefDlg, kEngineSoundCBox, &cnt));
    gPrefs.engineSound = GetControlValue(cnt);
    DoError(GetDialogItemAsControl(prefDlg, kHQSoundCBox, &cnt));
    gPrefs.hqSound = GetControlValue(cnt);
    DoError(GetDialogItemAsControl(prefDlg, kVolumeSlider, &cnt));
    gPrefs.volume = GetControlValue(cnt);
    DoError(GetDialogItemAsControl(prefDlg, kSoundBox, &cnt));
    gPrefs.sound = GetControlValue(cnt);
    DoError(GetDialogItemAsControl(prefDlg, kHiColorCBox, &cnt));
    if (gPrefs.hiColor != GetControlValue(cnt))
      modeSwitch = true;
    gPrefs.hiColor = GetControlValue(cnt);
    WritePrefs(false);
    InitChannels();
  }
  else
    gPrefs.sound = soundOn;
  SetGameVolume(-1);
  DisposeDialog(prefDlg);
  if (modeSwitch)
    ReInitGraphics();
  FadeScreen(1);
  ScreenMode(kScreenRunning);
  ScreenUpdate(NULL);
  FadeScreen(0);
}
