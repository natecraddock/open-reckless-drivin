#ifndef __ERROR
#define __ERROR

#include "defines.h"

void HandleError(int id);

inline void DoError(OSErr id) {
  if (id) {
    HandleError(id);
  }
}

#endif
