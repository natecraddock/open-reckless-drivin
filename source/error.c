#include <stdbool.h>

#include "defines.h"
#include "initexit.h"
#include "screen.h"

typedef struct {
  unsigned long fSaveSP, fSaveCR, fSaveLR, fResv0, fResv1, fSaveRTOC;
} tStackFrame;

asm unsigned long GetCallersSP(void){lwz r3, 0(SP)blr}

Str255 *FindRoutineName(unsigned long *codeAddress) {
  // look for the callers' "blr" instruction
  // assume it's going to be within 8K instructions of the call site.
  // this may or may not work for your code, worked for me.

  // the MacsBug name follows shortly after the 'blr'
  // and at a fixed offset that I figured out empirically.
  int i;
  for (i = 0; i < 8000; i++) {
    if (codeAddress[i] == 0x4E800020) {
      // found the 'blr'
      if (codeAddress[i + 1] == 0x00000000) {
        return (Str255 *)(((unsigned char *)&codeAddress[i]) + 21);
      }
    }
  }
  return nil;
}

inline void GetCallerName(Str255 callerName) {
  tStackFrame *frame = (tStackFrame *)GetCallersSP();
  unsigned long *address = (unsigned long *)frame->fSaveLR;
  Str255 *name = FindRoutineName(address);
  if (name)
    BlockMoveData(*name, callerName, (*name)[0] + 1);
  else
    BlockMoveData("\p<Anonymous Routine>", callerName, 20);
}

void HandleError(int id) {
  short hit;
  int err;
  Str255 idStr;
  Str255 help;
  AlertStdAlertParamRec alertParam = {false,
                                      false,
                                      nil,
                                      "\pExit",
                                      nil,
                                      nil,
                                      kAlertStdAlertOKButton,
                                      0,
                                      kWindowDefaultPosition};
  NumToString(id, idStr);
  BlockMoveData(" @ ", idStr + idStr[0] + 1, 3);
  idStr[0] += 3;
  GetCallerName(help);
  BlockMoveData(help + 2, idStr + idStr[0] + 1, help[0] - 1);
  idStr[0] += help[0] - 1;
#if __option(scheduling)
  ShowCursor();
  ScreenMode(kScreenSuspended);
  err = StandardAlert(kAlertStopAlert, "\pA fatal error has occured!!", idStr,
                      &alertParam, &hit);
  if (err)
    ExitToShell();
#else
  DebugStr(idStr);
#endif
  Exit();
}
