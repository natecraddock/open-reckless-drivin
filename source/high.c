#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "defines.h"
#include "error.h"
#include "gamesounds.h"
#include "interface.h"
#include "preferences.h"
#include "screen.h"

void ShowHighScores(int hilite) {
  int i;
  GWorldPtr screenGW;
  GWorldPtr oldGW;
  GDHandle oldGD;
  ShowPicScreen(1004);
  screenGW = GetScreenGW();
  GetGWorld(&oldGW, &oldGD);
  SetGWorld(screenGW, NULL);
  TextMode(srcOr);
  TextSize(24);
  TextFont(3);
  ForeColor(cyanColor);
  for (i = 0; i < kNumHighScoreEntries; i++) {
    Str255 scoreString;
    if (i == hilite)
      TextFace(bold + underline);
    else
      TextFace(0);
    MoveTo(170, 180 + i * 30);
    DrawString(gPrefs.high[i].name);
    NumToString(gPrefs.high[i].score, scoreString);
    MoveTo(480, 180 + i * 30);
    Move(-StringWidth(scoreString), 0);
    DrawString(scoreString);
  }
  ForeColor(blackColor);
  SetGWorld(oldGW, oldGD);
  WaitForPress();
  FadeScreen(1);
  ScreenUpdate(NULL);
  FadeScreen(0);
}

extern int gOSX;

void SetHighScoreEntry(int index, uint32_t score) {
  DialogPtr highDlg;
  short type;
  Rect box;
  Handle item;
  short hit;
  Str255 text;
  if (gOSX) {
    FadeScreen(1);
    ScreenMode(kScreenSuspended);
    FadeScreen(0);
  }

  highDlg = GetNewDialog(130, NULL, (WindowPtr)-1);
  DoError(SetDialogDefaultItem(highDlg, 1));
  GetDialogItem(highDlg, 2, &type, &item, &box);
  SetDialogItemText(item, gPrefs.lastName);
  SelectDialogItemText(highDlg, 2, 0, 32767);
  do
    ModalDialog(NULL, &hit);
  while (hit != 1);
  GetDialogItemText(item, text);
  BlockMove(text, gPrefs.lastName, text[0] + 1);
  DisposeDialog(highDlg);
  if (text[0] > 15) {
    text[0] = 15;
    text[15] = '�';
  }
  BlockMove(text, gPrefs.high[index].name, text[0] + 1);
  gPrefs.high[index].score = score;
  GetDateTime(&gPrefs.high[index].time);

  if (gOSX) {
    FadeScreen(1);
    ScreenMode(kScreenRunning);
    FadeScreen(512);
  }
}

void CheckHighScore(uint32_t score) {
  int i;
  if (gLevelResFile)
    return;
  for (i = kNumHighScoreEntries; score > gPrefs.high[i - 1].score && i > 0; i--)
    ;
  if (i < kNumHighScoreEntries) {
    BlockMoveData(gPrefs.high + i, gPrefs.high + i + 1,
                  sizeof(tScoreRecord) * (kNumHighScoreEntries - i - 1));
    SimplePlaySound(153);
    SetHighScoreEntry(i, score);
    WritePrefs(false);
    ShowHighScores(i);
  }
}

void ClearHighScores() {
  Handle prefDefault = GetResource('Pref', 128);
  BlockMoveData(&(((tPrefs *)*prefDefault)->high), &(gPrefs.high),
                sizeof(tScoreRecord) * kNumHighScoreEntries);
  ReleaseResource(prefDefault);
  WritePrefs(false);
}
