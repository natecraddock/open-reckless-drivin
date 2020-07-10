#ifndef __REGISTER
#define __REGISTER

#include <stdint.h>

extern uint32_t gKey;
extern int gRegistered;

void Register(int fullscreen);
int CheckRegi();

#endif
