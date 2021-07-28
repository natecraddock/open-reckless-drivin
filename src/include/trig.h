#ifndef __TRIG_H
#define __TRIG_H

#define PI 3.1415
#define SINE_MASK 0x000003ff
#define SINE_TABLE_SIZE 1024

extern float sine_table[SINE_TABLE_SIZE];

#define sin(x)                                                                 \
  sine_table[(int)((x) * (float)SINE_TABLE_SIZE / (2.0 * PI)) & SINE_MASK]
#define cos(x)                                                                 \
  sine_table[(int)(((x) + PI / 2.0) * (float)SINE_TABLE_SIZE / (2.0 * PI)) &   \
             SINE_MASK]

void InitTrig();

#endif /* __TRIG_H */
