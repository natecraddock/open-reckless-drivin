#include <stdint.h>
#include <stdio.h>

#include "resource.h"

static const uint32_t CHCK_VALUE = 3285832799;

/**
 * Tests loading of the "Chck" resource
 */
int main() {
  Handle chck = GetResource("Chck", 128);

  /* Verify the correct number was read from the file */
  uint32_t check_n = **(uint32_t **)chck;
  if (check_n != CHCK_VALUE) {
    printf("Incorrect value for Chck. Expected %u but got %u\n", CHCK_VALUE, check_n);
    return 1;
  }

  ReleaseResource(chck);
  return 0;
}
