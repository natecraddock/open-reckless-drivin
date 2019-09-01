#ifndef __TRIG
#define __TRIG

#include <math.h>

#define PI					3.1415
#define kSinMask			0x000003ff
#define kSinTabSize			1024

extern float gSinTab[kSinTabSize];

#define sin(x) gSinTab[(int)((x)*(float)kSinTabSize/(2.0*PI))&kSinMask]
#define cos(x) gSinTab[(int)(((x)+PI/2.0)*(float)kSinTabSize/(2.0*PI))&kSinMask]

#endif