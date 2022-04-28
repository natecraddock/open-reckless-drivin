#include <math.h>

#include "trig.h"

float sine_table[SINE_TABLE_SIZE];

#undef sin
void InitTrig() {
  for (int i = 0; i < SINE_TABLE_SIZE; i++) {
    sine_table[i] = sin(2 * PI * (float)i / (float)SINE_TABLE_SIZE);
  }
}
