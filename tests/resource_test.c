#include <stdio.h>

#include "lzrw.h"
#include "resource.h"

int main(int argc, char **argv) {

  Handle pic = GetResource("Pack", 134);
  if (!pic) {
    printf("BAD PACK\n");
    return 1;
  }

  printf("fetched length %d\n", GetHandleSize(pic));

  printf("Decompressing...\n");
  LZRWDecodeHandle(&pic);

  printf("fetched decompressed length %d\n", GetHandleSize(pic));

  if (argc == 2) {
    printf("Writing decompressed block to file %s\n", argv[1]);
    FILE *out = fopen(argv[1], "w");

    fwrite(*pic, GetHandleSize(pic), 1, out);

    fclose(out);
  }

  ReleaseResource(pic);

  return 0;
}
