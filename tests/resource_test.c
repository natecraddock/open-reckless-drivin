#include <stdint.h>
#include <stdio.h>

#include "resource.h"

static const uint32_t CHCK_VALUE = 3285832799;

/**
 * Tests loading of the "Chck" resource
 */
int main() {
  Handle chck = GetResource("Chck", 128);
  if (chck == NULL) {
    printf("Unable to load resource\n");
    return 1;
  }

  /* Verify the correct number was read from the file */
  uint32_t check_n = **(uint32_t **)chck;
  if (check_n != CHCK_VALUE) {
    printf("Incorrect value for Chck. Expected %u but got %u\n", CHCK_VALUE, check_n);
    return 1;
  }

  /* This handle is still a resource handle, so release the resource */
  ReleaseResource(chck);
  return 0;
}
