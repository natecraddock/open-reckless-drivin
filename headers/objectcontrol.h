#ifndef __OBJECTCONTROL
#define __OBJECTCONTROL

#include "input.h"
#include "objects.h"

#define kStartChaseDist 42.0

/* Number of frames after new life in which player won't be attacked. */
#define kSaveDelay 3.0 * kCalcFPS
/* Velocity which new cops who are called will initially have. */
#define kStartChaseSpeed 28.0

void ObjectControl(tObject *theObj, tInputData *input);
void ObjectStartChase(tObject *theObj, int distance, float velo);

#endif
