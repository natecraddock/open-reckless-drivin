#ifndef __TRIG
#define __TRIG

#include <math.h>

#define PI 3.1415
#define kSinMask 0x000003ff
#define SINE_TABLE_SIZE 1024

extern float sine_table[SINE_TABLE_SIZE];

#define sin(x)                                                                 \
  sine_table[(int)((x) * (float)SINE_TABLE_SIZE / (2.0 * PI)) & kSinMask]
#define cos(x)                                                                 \
  sine_table[(int)(((x) + PI / 2.0) * (float)SINE_TABLE_SIZE / (2.0 * PI)) &   \
             kSinMask]

#endif
