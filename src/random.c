#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

/* A C-program for TT800 : July 8th 1996 Version */
/* by M. Matsumoto, email: matumoto@math.keio.ac.jp */
/* genrand() generate one pseudorandom number with double precision */
/* which is uniformly distributed on [0,1]-interval */
/* for each call.  One may choose any initial 25 seeds */
/* except all zeros. */

/* See: ACM Transactions on Modelling and Computer Simulation, */
/* Vol. 4, No. 3, 1994, pages 254-266. */

#define N 25
#define M 7

uint64_t x[N]; /* initial 25 seeds, change as you wish */

static double genrand() {
  uint64_t y;
  static int k = 0;
  static uint64_t mag01[2] = {
      0x0, 0x8ebfd028 /* this is magic vector `a', don't change */
  };
  if (k == N) { /* generate N words at one time */
    int kk;
    for (kk = 0; kk < N - M; kk++) {
      x[kk] = x[kk + M] ^ (x[kk] >> 1) ^ mag01[x[kk] % 2];
    }
    for (; kk < N; kk++) {
      x[kk] = x[kk + (M - N)] ^ (x[kk] >> 1) ^ mag01[x[kk] % 2];
    }
    k = 0;
  }
  y = x[k];
  y ^= (y << 7) & 0x2b5b2500;  /* s and b, magic vectors */
  y ^= (y << 15) & 0xdb8b0000; /* t and c, magic vectors */
  y &= 0xffffffff;             /* you may delete this line if word size = 32 */
                               /*
                                  the following line was added by Makoto Matsumoto in the 1996 version
                                  to improve lower bit's correlation.
                                  Delete this line to o use the code published in 1994.
                               */
  y ^= (y >> 16);              /* added to the 1994 version */
  k++;
  return ((double)y / (unsigned long)0xffffffff);
}

/**
 * Initialize the random seeds.
 */
void Randomize() {
  srand(time(NULL));
  for (int i = 0; i < N; i++) {
    x[i] = rand();
  }
}

/**
 * Gives a random float evenly distributed in the interval [min, max]
 */
float RanFl(float min, float max) {
  return genrand() * (max - min) + min;
}

/**
 * Gives a random int evenly distributed in the interval [min, max-1]
 */
int RanInt(int min, int max) {
  double ran;
  do
    ran = genrand();
  while (ran == 1);
  return floor(ran * (max - min) + min);
}

/**
 * Returns true with a probability of prob
 * (i.e. if prob is is 1 always return true, if prob is 0 always return false)
 */
int RanProb(float prob) {
  return genrand() <= prob;
}
