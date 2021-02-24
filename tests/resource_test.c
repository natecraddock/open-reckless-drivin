#include <stdio.h>

#include "lzrw.h"
#include "resource.h"

int main(int argc, char **argv) {

  if (argc != 4) {
    fprintf(stderr, "Usage: %s type number outfile\n", argv[0]);
    return 1;
  }

  const int pack = atoi(argv[2]);
  printf("Getting \"data '%s' (%d)\"\n", argv[1], pack);
  Handle resource = GetResource(argv[1], pack);
  if (!resource) {
    fprintf(stderr, "BAD PACK\n");
    return 1;
  }

  printf("Fetched length %d\n", GetHandleSize(resource));

  printf("Decompressing... ");
  LZRWDecodeHandle(&resource);
  printf("Done!\n");

  printf("Fetched decompressed length %d\n", GetHandleSize(resource));

  printf("Writing decompressed block to file %s\n", argv[3]);
  FILE *out = fopen(argv[3], "w");
  fwrite(*resource, GetHandleSize(resource), 1, out);
  fclose(out);

  ReleaseResource(resource);

  return 0;
}
