#ifndef __INTERFACE
#define __INTERFACE

#include "defines.h"

extern int gExit;
extern short gLevelResFile;
extern Str63 gLevelFileName;

/* void SaveFlushEvents(); */
void Eventloop();
/* void InitInterface(); */
/* void DisposeInterface(); */
/* void ScreenUpdate(WindowPtr win); */
void ShowPicScreen(int id);
/* void ShowPicScreenNoFade(int id); */
/* void WaitForPress(); */

#endif
