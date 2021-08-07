#include <stdint.h>

#include "defines.h"
#include "lzrw.h"
#include "packs.h"
#include "register.h"
#include "resource.h"

uint32_t gKey;

int main() {
  // Get check value for verifying decryption
  Handle resource = GetResource("Chck", 128);
  if (resource == NULL) {
    return 1;
  }

  uint32_t check = FLIP_LONG(**(uint32_t **)resource);

  // Levels 4 through 10 are encrypted. This test loads level 4 and verifies
  // that it decrypts properly by comparing against a valid global key. Level
  // 4 is currently the only level that can be checked for valid decryption.
  //
  // Typically a global key would come from a username and registration
  // code rather than being hard-coded.
  gKey = 0x1E42A71F;
  return !CheckPack(PACK_ENCRYPTED, check);
}
