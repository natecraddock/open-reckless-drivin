#include "lzrw.h"
#include "packs.h"
#include "resource.h"

int main() {
  /* LZRW decode all packs that aren't encrypted */
  for (int i = 0; i < kPackLevel4; i++) {
    Handle resource = GetResource("Pack", 128 + i);
    LZRWDecodeHandle(&resource);
    DisposeHandle(resource);
  }

  /* Repeat for the quickdraw images which are also compressed */
  for (int i = 0; i < 10; i++) {
    Handle resource = GetResource("PPic", 1000 + i);
    LZRWDecodeHandle(&resource);
    DisposeHandle(resource);
  }

  return 0;
}
