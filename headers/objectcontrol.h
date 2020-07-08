#ifndef __OBJECTCONTROL
#define __OBJECTCONTROL

#include "input.h"
#include "objects.h"

#define kStartChaseDist 42.0
#define kSaveDelay                                                             \
  3.0 * kCalcFPS // Number of frames after new life in which player won't be
                 // attacked.
#define kStartChaseSpeed                                                       \
  28.0 // velocity which new cops who are called will initially have

void ObjectControl(tObject *theObj, tInputData *input);
void ObjectStartChase(tObject *theObj, int distance, float velo);

#endif