
#include <stdint.h>

/* Common constant string sizes. */
typedef unsigned char Str15[16];
typedef unsigned char Str31[32];
typedef unsigned char Str63[64];
typedef unsigned char Str255[256];

typedef char *Ptr;
typedef Ptr *Handle;
typedef unsigned char *StringPtr;

typedef int16_t OSErr;

/* TODO: update these. */
typedef struct WindowPtr {
} WindowPtr;

typedef struct Point {
  short v;
  short h;
} Point;

typedef struct GWorldPtr {
} GWorldPtr;
