#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "packs.h"
#include "preferences.h"
#include "register.h"
#include "resource.h"
/* #include "initexit.h" */
/* #include "interface.h" */

uint32_t gKey;
int gRegistered = false;

/* Convert a hex character to its numeric representation */
static int8_t hex_to_number(char hex) {
  if (hex <= '9') {
    return hex - '0';
  }
  return hex - 'A' + 10;
}

static uint32_t code_string_to_long(const char *code) {
  uint32_t codeNum = 0;
  uint32_t seed = 0;

  /* Check that code is the correct length and consists of only [0-9A-F] */
  if (strlen(code) != 10) {
    return 0;
  }
  for (int digi = 0; digi < 10; digi++) {
    if (!((code[digi] >= '0' && code[digi] <= '9') ||
          (code[digi] >= 'A' && code[digi] <= 'F'))) {
      return 0;
    }
  }

  for (int digi = 9; digi > 7; digi--) {
    seed += hex_to_number(code[digi]) * pow(16, 9 - digi);
  }
  seed = seed + (seed << 8) + (seed << 16) + (seed << 24);
  for (int digi = 7; digi >= 0; digi--) {
    codeNum += hex_to_number(code[digi]) * pow(16, 7 - digi);
  }

  return codeNum ^ seed;
}

static void remove_spaces(char *str) {
  while (*str) {
    if (*str == ' ') {
      memmove(str, str + 1, strlen(str));
      continue;
    }
    str++;
  }
}

static void upper_string(char *str) {
  while (*str) {
    *str = toupper(*str);
    str++;
  }
}

bool REG_check_registration() {
  char name[256];
  memcpy(name, gPrefs.name, 256);
  upper_string(name);
  remove_spaces(name);
  uint32_t name_num = FLIP_LONG(*(uint32_t *)(name + strlen(name) - 4));

  char code[256];
  memcpy(code, gPrefs.code, 16);
  uint32_t code_num = code_string_to_long(code);

  gKey = code_num ^ name_num;

  Handle check = GetResource("Chck", 128);
  uint32_t check_num = FLIP_LONG(**(uint32_t **)check);
  ReleaseResource(check);

  gRegistered = CheckPack(PACK_ENCRYPTED, check_num);
  return gRegistered;
}

/* In-game registration popup */
/* enum { kRegAppButton = 1, kOnlineButton, kCodeButton, kCancelButton }; */
/* enum { kCodeOKButton = 1, kCodeCancelButton, kCodeNameField, kCodeCodeField
 * }; */

/* void EnterCode() { */
/*   int repeat; */
/*   DialogPtr regDlg = GetNewDialog(133, NULL, (WindowPtr)-1L); */
/*   do { */
/*     short hit; */
/*     short type; */
/*     Rect box; */
/*     Handle item; */
/*     repeat = false; */
/*     DoError(SetDialogDefaultItem(regDlg, kCodeOKButton)); */
/*     DoError(SetDialogCancelItem(regDlg, kCodeCancelButton)); */
/*     GetDialogItem(regDlg, kCodeNameField, &type, &item, &box); */
/*     SetDialogItemText(item, gPrefs.name); */
/*     GetDialogItem(regDlg, kCodeCodeField, &type, &item, &box); */
/*     SetDialogItemText(item, gPrefs.code); */
/*     do { */
/*       EventRecord event; */
/*       WindowPtr win; */
/*       DialogPtr dlg; */
/*       hit = 0; */
/*       TEFromScrap(); */
/*       if (WaitNextEvent(everyEvent, &event, 60, NULL)) { */
/*         if (event.what == keyDown) */
/*           switch ((event.message & keyCodeMask) >> 8) { */
/*             case 0x24: */
/*             case 0x4c: */
/*               hit = kCodeOKButton; */
/*               break; */
/*             case 0x35: */
/*               hit = kCodeCancelButton; */
/*               break; */
/*           } */
/*         if (!hit) { */
/*           if (IsDialogEvent(&event)) */
/*             DialogSelect(&event, &dlg, &hit); */
/*           else if (event.what == mouseDown) */
/*             if (FindWindow(event.where, &win) == inDrag) { */
/*               Rect rgnBBox; */
/*               GetRegionBounds(GetGrayRgn(), &rgnBBox); */
/*               DragWindow(win, event.where, &rgnBBox); */
/*             } */
/*         } */
/*         else { */
/*           unsigned long ticks; */
/*           GetDialogItem(regDlg, hit, &type, &item, &box); */
/*           HiliteControl(item, 1); */
/*           ticks = TickCount(); */
/*           while (ticks + 8 > TickCount()) */
/*             ; */
/*           HiliteControl(item, 0); */
/*         } */
/*       } */
/*     } while (hit != kCodeOKButton && hit != kCodeCancelButton); */
/*     if (hit == kCodeOKButton) { */
/*       AlertStdAlertParamRec alertParam = {false, */
/*                                           false, */
/*                                           NULL, */
/*                                           "\pOK", */
/*                                           NULL, */
/*                                           NULL, */
/*                                           kAlertStdAlertOKButton, */
/*                                           0, */
/*                                           kWindowDefaultPosition}; */
/*       GetDialogItem(regDlg, kCodeNameField, &type, &item, &box); */
/*       GetDialogItemText(item, gPrefs.name); */
/*       GetDialogItem(regDlg, kCodeCodeField, &type, &item, &box); */
/*       GetDialogItemText(item, gPrefs.code); */
/*       UpperString(gPrefs.code, false); */
/*       remove_spaces(gPrefs.code); */
/*       WritePrefs(false); */
/*       if (CheckRegi()) */
/*         DoError(StandardAlert(kAlertNoteAlert, */
/*                               "\pThank you very much for registering.", */
/*                               "\pHave fun!", &alertParam, &hit)); */
/*       else { */
/*         DoError(StandardAlert( */
/*             kAlertStopAlert, */
/*             "\pSorry, your registration code and/or name are not correct.",
 */
/*             "\pPlease make sure you entered both exactly as you recieved
 * them.", */
/*             &alertParam, &hit)); */
/*         repeat = true; */
/*       } */
/*     } */
/*   } while (repeat); */
/*   DisposeDialog(regDlg); */
/* } */

/* void Register(int fullscreen) { */
/*   AlertStdAlertParamRec alertParam = {false, */
/*                                       false, */
/*                                       NULL, */
/*                                       "\pOK", */
/*                                       NULL, */
/*                                       NULL, */
/*                                       kAlertStdAlertOKButton, */
/*                                       0, */
/*                                       kWindowDefaultPosition}; */
/*   short alertHit; */
/*   if (fullscreen) */
/*     ScreenMode(kScreenSuspended); */
/*   if (!gRegistered) { */
/*     DialogPtr regDlg = GetNewDialog(132, NULL, (WindowPtr)-1L); */
/*     short hit; */
/*     DoError(SetDialogDefaultItem(regDlg, kCancelButton)); */
/*     DoError(SetDialogCancelItem(regDlg, kCancelButton)); */
/*     do { */
/*       ModalDialog(NULL, &hit); */
/*     } while (hit > kCancelButton); */
/*     DisposeDialog(regDlg); */
/*     switch (hit) { */
/*       case kRegAppButton: { */
/*         int err; */
/*err=RR_Launch();
if(err==memFullErr)
        DoError(StandardAlert(kAlertStopAlert,
                "\pNot enough memory to launch Register.",
                "\pPlease quit Reckless Drivin' and launch Register
from the Finder.", &alertParam, &alertHit)); else if(err==dskFulErr)
        DoError(StandardAlert(kAlertStopAlert,
                "\pNot enough free space to put Register on your
drive.",
                "\pPlease delete some files.",
                &alertParam,
                &alertHit));
else DoError(err);*/
/* } break; */
/* case kOnlineButton: { */
/*   ICInstance inst; */
/*   int icFailed = true; */
/*   Str255 url = "\phttp://order.kagi.com/?F6"; */
/*   long start = 0, end = url[0]; */
/*   if (ICStart != kUnresolvedCFragSymbolAddress) */
/*     if (!ICStart(&inst, kCreator)) { */
/*       icFailed = false; */
/*       DoError(ICLaunchURL(inst, "\p", url + 1, url[0], &start, &end)); */
/*       DoError(ICStop(inst)); */
/*     } */
/*   if (icFailed) */
/*     DoError(StandardAlert( */
/*         kAlertStopAlert, "\pUnable to access Internet Config", */
/*         "\pPlease manually enter the URL " */
/*         "'http://order.kagi.com/?F6' into your browser.", */
/*         &alertParam, &alertHit)); */
/*   Exit(); */
/* } break; */
/* case kCodeButton: */
/*   EnterCode(); */
/*   break; */
/* } */
/* } */
/* else { */
/* DoError(StandardAlert(kAlertNoteAlert, */
/*                     "\pYou have already registered this game.", */
/*                     "\pThank you once again!", &alertParam, &alertHit)); */
/* } */
/* if (fullscreen) { */
/* ScreenMode(kScreenRunning); */
/* ScreenUpdate(NULL); */
/* } */
/* } */

/*			{
MANUALLY LANUCH REGISTER APP
                                int err;
                                FSSpec regApp;
                                LaunchParamBlockRec launchParams;
                                FSMakeFSSpec(0,0,"\pRegister",&regApp);
                                launchParams.launchBlockID=extendedBlock;
                                launchParams.launchEPBLength=extendedBlockLen;
                                launchParams.launchAppSpec=&regApp;
                                launchParams.launchControlFlags=launchNoFileFlags+launchContinue;
                                launchParams.launchAppParameters=NULL;
                                err=LaunchApplication(&launchParams);
                                if(err==memFullErr)
                                        DoError(StandardAlert(kAlertStopAlert,
                                                "\pNot enough memory to launch
Register.",
                                                "\pPlease quit Reckless Drivin'
and launch Register from the Finder", &alertParam, &hit)); else if(err==fnfErr)
                                        DoError(StandardAlert(kAlertStopAlert,
                                                "\pThe Register Application
could not be found.",
                                                "\p",
                                                &alertParam,
                                                &hit));
                                else DoError(err);
                        }
                        break;*/
