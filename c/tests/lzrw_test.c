#include "lzrw.h"
#include "packs.h"
#include "register.h"
#include "resource.h"

int main() {
  // LZRW decode all packs that aren't encrypted
  for (int i = 0; i < PACK_LEVEL_04; i++) {
    Handle resource = GetResource("Pack", 128 + i);
    if (resource == NULL) {
      return 1;
    }
    LZRWDecodeHandle(&resource);
    DisposeHandle(resource);
  }

  // Repeat for the quickdraw images which are also compressed
  for (int i = 0; i < 10; i++) {
    Handle resource = GetResource("PPic", 1000 + i);
    if (resource == NULL) {
      return 1;
    }
    LZRWDecodeHandle(&resource);
    DisposeHandle(resource);
  }

  return 0;
}
