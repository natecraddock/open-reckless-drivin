#define CALL_IN_SPOCKETS_BUT_NOT_IN_CARBON 1

#include "error.h"
#include "gameframe.h"
#include "gamesounds.h"
#include "input.h"
#include "interface.h"
#include "screen.h"
#include "screenfx.h"
#include <DrawSprocket.h>

int HandleMouseDownPause(EventRecord *event) { return true; }

void PauseGame() {
  EventRecord event;
  int end = false;
  PauseFrameCount();
  SaveFlushEvents();
  InputMode(kInputSuspended);
  BeQuiet();
  ShowPicScreen(1006);
  ShowCursor();
  do {
    if (WaitNextEvent(everyEvent, &event, 60, nil)) {
      int eventWasProcessed;
      DoError(DSpProcessEvent(&event, &eventWasProcessed));
      if (event.what == mouseDown)
        end = HandleMouseDownPause(&event);
      if (event.what == updateEvt) {
        BeginUpdate((WindowPtr)event.message);
        ShowPicScreen(1006);
        EndUpdate((WindowPtr)event.message);
      }
      if (event.what == osEvt)
        if (event.message >> 24 == suspendResumeMessage)
          if (event.message & resumeFlag)
            ShowPicScreenNoFade(1006);
    }
  } while (!end);
  InputMode(kInputRunning);
  HideCursor();
  ScreenClear();
  StartCarChannels();
  ResumeFrameCount();
}