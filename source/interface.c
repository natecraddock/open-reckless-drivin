#define CALL_IN_SPOCKETS_BUT_NOT_IN_CARBON 1

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "defines.h"
#include "error.h"
#include "gameinitexit.h"
#include "gamesounds.h"
#include "high.h"
#include "input.h"
#include "lzrwHandleInterface.h"
#include "preferences.h"
#include "register.h"
#include "screen.h"
#include <DrawSprocket.h>

enum {
  kNoButton = -1,
  kStartGameButton,
  kPrefsButton,
  kScoreButton,
  kHelpButton,
  kQuitButton,
  kRegisterButton,
  kAboutButton
};

GWorldPtr gMainScreenGW, gHilitGW, gSelectedGW;
Rect **gButtonList;
int gButtonLocation;
int gExit;
short gLevelResFile = 0, gAppResFile;
Str63 gLevelFileName;
RgnHandle gButtonRgn;
int gInterfaceInited = false;

void ShowPicScreen(int id) {
  GWorldPtr screenGW;
  GWorldPtr oldGW;
  GDHandle oldGD;
  Handle pic;
  Rect r = {0, 0, 480, 640};
  FadeScreen(1);
  ScreenMode(kScreenRunning);
  screenGW = GetScreenGW();
  GetGWorld(&oldGW, &oldGD);
  SetGWorld(screenGW, NULL);
  pic = GetResource('PPic', id);
  LZRWDecodeHandle(&pic);
  DrawPicture((PicHandle)pic, &r);
  DisposeHandle(pic);
  SetGWorld(oldGW, oldGD);
  FadeScreen(0);
}

void ShowPicScreenNoFade(int id) {
  GWorldPtr screenGW;
  GWorldPtr oldGW;
  GDHandle oldGD;
  Handle pic;
  Rect r = {0, 0, 480, 640};
  ScreenMode(kScreenRunning);
  screenGW = GetScreenGW();
  GetGWorld(&oldGW, &oldGD);
  SetGWorld(screenGW, NULL);
  pic = GetResource('PPic', id);
  LZRWDecodeHandle(&pic);
  DrawPicture((PicHandle)pic, &r);
  DisposeHandle(pic);
  SetGWorld(oldGW, oldGD);
}

void UpdateButtonRgn() {
  if (gButtonLocation != kNoButton) {
    RectRgn(gButtonRgn, (*gButtonList) + gButtonLocation);
  }
  else {
    int i;
    RgnHandle screenRgn = NewRgn();
    SetRectRgn(screenRgn, 0, 0, 640, 480);
    SetEmptyRgn(gButtonRgn);
    OpenRgn();
    HLock((Handle)gButtonList);
    for (i = 0; i < GetHandleSize((Handle)gButtonList) / sizeof(Rect); i++)
      FrameRect((*gButtonList) + i);
    HUnlock((Handle)gButtonList);
    CloseRgn(gButtonRgn);
    DiffRgn(screenRgn, gButtonRgn, gButtonRgn);
    DisposeRgn(screenRgn);
  }
}

void DrawScreen(int button, GWorldPtr src) {
  GWorldPtr screenGW = GetScreenGW();
  GWorldPtr oldGW;
  GDHandle oldGD;
  GetGWorld(&oldGW, &oldGD);
  SetGWorld(screenGW, NULL);
  if (button != kNoButton)
    CopyBits(GetPortBitMapForCopyBits(src), GetPortBitMapForCopyBits(screenGW),
             (*gButtonList) + button, (*gButtonList) + button, srcCopy, NULL);
  else {
    Rect r = {0, 0, 480, 640};
    CopyBits(GetPortBitMapForCopyBits(src), GetPortBitMapForCopyBits(screenGW),
             &r, &r, srcCopy, NULL);
    ForeColor(whiteColor);
    TextSize(12);
    TextFont(3);
    TextMode(srcOr);
    TextFace(0);
    MoveTo(630, 475);
    if (gRegistered) {
      Move(-StringWidth("\pRegistered To: "), 0);
      Move(-StringWidth(gPrefs.name), 0);
      DrawString("\pRegistered To: ");
      DrawString(gPrefs.name);
    }
    else {
      Move(-StringWidth("\p��� This Copy is not Registered! ���"), 0);
      DrawString("\p��� This Copy is not Registered! ���");
    }
    MoveTo(10, 15);
    if (gLevelResFile) {
      DrawString("\pCustom Level File: ");
      DrawString(gLevelFileName);
    }
    ForeColor(blackColor);
  }
  SetGWorld(oldGW, oldGD);
}

void ScreenUpdate(WindowPtr win) {
  if (win) {
    BeginUpdate(win);
    EndUpdate(win);
  }
  gButtonLocation = kNoButton;
  UpdateButtonRgn();
  DrawScreen(kNoButton, gMainScreenGW);
}

void DisposeInterface() {
  if (gInterfaceInited) {
    gInterfaceInited = false;
    DisposeGWorld(gMainScreenGW);
    DisposeGWorld(gHilitGW);
    DisposeGWorld(gSelectedGW);
    ReleaseResource(gButtonList);
    DisposeRgn(gButtonRgn);
  }
}

void SaveFlushEvents() {
  int eventMask = mDownMask + mUpMask + keyDownMask + keyUpMask + autoKeyMask;
  EventRecord event;
  FlushEvents(eventMask, 0);
  while (WaitNextEvent(eventMask, &event, 0, NULL))
    ;
}

void InitInterface() {
  if (!gInterfaceInited) {
    Rect gwSize;
    GWorldPtr oldGW;
    GDHandle oldGD;
    Handle pic;
    SetRect(&gwSize, 0, 0, 640, 480);
    DoError(NewGWorld(&gMainScreenGW, gPrefs.hiColor ? 16 : 8, &gwSize, NULL,
                      NULL, 0));
    DoError(
        NewGWorld(&gHilitGW, gPrefs.hiColor ? 16 : 8, &gwSize, NULL, NULL, 0));
    DoError(NewGWorld(&gSelectedGW, gPrefs.hiColor ? 16 : 8, &gwSize, NULL,
                      NULL, 0));
    LockPixels(GetGWorldPixMap(gMainScreenGW));
    LockPixels(GetGWorldPixMap(gHilitGW));
    LockPixels(GetGWorldPixMap(gSelectedGW));
    GetGWorld(&oldGW, &oldGD);
    SetGWorld(gMainScreenGW, NULL);
    pic = GetResource('PPic', 1000);
    LZRWDecodeHandle(&pic);
    DrawPicture((PicHandle)pic, &gwSize);
    DisposeHandle(pic);
    SetGWorld(gHilitGW, NULL);
    pic = GetResource('PPic', 1001);
    LZRWDecodeHandle(&pic);
    DrawPicture((PicHandle)pic, &gwSize);
    DisposeHandle(pic);
    SetGWorld(gSelectedGW, NULL);
    pic = GetResource('PPic', 1002);
    LZRWDecodeHandle(&pic);
    DrawPicture((PicHandle)pic, &gwSize);
    DisposeHandle(pic);
    SetGWorld(oldGW, oldGD);
    (Handle) gButtonList = GetResource('Recs', 1000);
    gButtonRgn = NewRgn();
    gInterfaceInited = true;
  }
  InputMode(kInputSuspended);
  FadeScreen(1);
  ScreenMode(kScreenRunning);
  ScreenUpdate(NULL);
  FadeScreen(0);
  SaveFlushEvents();
  gGameOn = false;
}

void UpdateButtonLocation() {
  Point mPos = GetScreenPos(NULL);
  int i, button = kNoButton;
  HLock((Handle)gButtonList);
  for (i = 0; i < GetHandleSize((Handle)gButtonList) / sizeof(Rect); i++)
    if (PtInRect(mPos, (*gButtonList) + i))
      button = i;
  HUnlock((Handle)gButtonList);
  if (button != gButtonLocation) {
    if (button != kNoButton)
      DrawScreen(button, gHilitGW);
    if (gButtonLocation != kNoButton)
      DrawScreen(gButtonLocation, gMainScreenGW);
    gButtonLocation = button;
    UpdateButtonRgn();
  }
}

int GetButtonClick(Point mPos) {
  int i, clicked = false, oldclicked = false, button = kNoButton;
  mPos = GetScreenPos(&mPos);
  HLock((Handle)gButtonList);
  for (i = 0; i < GetHandleSize((Handle)gButtonList) / sizeof(Rect); i++)
    if (PtInRect(mPos, (*gButtonList) + i))
      button = i;
  HUnlock((Handle)gButtonList);
  if (button != kNoButton) {
    SimplePlaySound(147);
    while (StillDown()) {
      mPos = GetScreenPos(NULL);
      if (clicked = PtInRect(mPos, (*gButtonList) + button)) {
        if (clicked != oldclicked)
          DrawScreen(button, gSelectedGW);
      }
      else
        DrawScreen(button, gMainScreenGW);
      oldclicked = clicked;
    }
  }
  if (clicked) {
    DrawScreen(button, gHilitGW);
    return button;
  }
  else
    return kNoButton;
}

int GetKeyClick(long key) {
  switch (key & charCodeMask) {
  case 's':
  case 'n':
    return kStartGameButton;
  case 'p':
    return kPrefsButton;
  case 'c':
  case 'o':
    return kScoreButton;
  case '/':
  case '?':
  case 'h':
    return kHelpButton;
  case 'q':
    return kQuitButton;
  case 'r':
    return kRegisterButton;
  }
  switch ((key & keyCodeMask) >> 8) {
  case 0x31:
  case 0x24:
  case 0x4c:
    return kStartGameButton;
  case 0x72:
    return kHelpButton;
  case 0x35:
    return kQuitButton;
  }
  return kNoButton;
}

int KeyClick(long key) {
  uint32_t ticks;
  int button = GetKeyClick(key);
  if (button == kNoButton)
    return kNoButton;
  DrawScreen(button, gSelectedGW);
  SimplePlaySound(147);
  Delay(10, &ticks);
  if (gButtonLocation == button)
    DrawScreen(button, gHilitGW);
  else
    DrawScreen(button, gMainScreenGW);
  return button;
}

void WaitForPress() {
  int pressed = false;
  do {
    EventRecord event;
    KeyMap theKeys;
    GetKeys(theKeys);
    pressed = theKeys[0] | theKeys[1] | theKeys[2] | theKeys[3] | Button() |
              ContinuePress();
    WaitNextEvent(everyEvent, &event, 0, NULL);
  } while (!pressed);
  while (Button())
    ;
  SaveFlushEvents();
}

void HandleCommand(int cmd, int modifiers) {
  switch (cmd) {
  case kNoButton:
    return;
  case kRegisterButton:
    Register(true);
    return;
  case kStartGameButton:
    StartGame(modifiers & optionKey);
    break;
  case kPrefsButton:
    Preferences();
    break;
  case kHelpButton:
    ShowPicScreen(1007);
    WaitForPress();
    ShowPicScreen(1008);
    WaitForPress();
    FadeScreen(1);
    ScreenUpdate(NULL);
    FadeScreen(0);
    break;
  case kScoreButton:
    ShowHighScores(-1);
    break;
  case kQuitButton:
    gExit = true;
    break;
  }
}

void HandleMouseDown(EventRecord *event) {
  HandleCommand(GetButtonClick(event->where), event->modifiers);
}

extern int gOSX;

void Eventloop() {
  EventRecord event;
  ShowCursor();
  if (WaitNextEvent(everyEvent, &event, 2, gButtonRgn)) {
    int eventWasProcessed;
    if (!gOSX)
      DoError(DSpProcessEvent(&event, &eventWasProcessed));
    switch (event.what) {
    case mouseDown:
      HandleMouseDown(&event);
      break;
    case keyDown:
      HandleCommand(KeyClick(event.message), event.modifiers);
      break;
    case osEvt:
      switch (event.message >> 24) {
      case mouseMovedMessage:
        UpdateButtonLocation();
        break;
      case suspendResumeMessage:
        if (event.message & resumeFlag)
          ScreenUpdate(NULL);
        break;
      }
      break;
    case kHighLevelEvent:
      DoError(AEProcessAppleEvent(&event));
      ScreenUpdate(NULL);
      break;
    case updateEvt:
      ScreenUpdate((WindowPtr)event.message);
      break;
    }
  }
  if (ContinuePress())
    HandleCommand(kStartGameButton, 0);
}
