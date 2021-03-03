#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>

#define FLIP_SHORT(var) var = ntohs((var))
#define FLIP_LONG(var) var = ntohl((var))

#include "lzrw.h"
#include "quickdraw.h"
#include "resource.h"

/* Writes a P6 version PPM file which stores the values in binary to save space,
 * making reads and writes faster. */
void write_ppm(RGBValue *pixels, Rect *rect, const char *file) {
  uint32_t width = rect->right;
  uint32_t height = rect->bottom;
  FILE *out = fopen(file, "w");
  fprintf(out, "P6\n");
  fprintf(out, "%u %u\n", width, height);
  fprintf(out, "31\n");

  /* Write each pixel */
  for (uint32_t pix = 0; pix < width * height; pix++) {
    RGBValue rgb = pixels[pix];
    fwrite(&rgb.r, sizeof rgb.r, 1, out);
    fwrite(&rgb.g, sizeof rgb.g, 1, out);
    fwrite(&rgb.b, sizeof rgb.b, 1, out);
  }

  fclose(out);
}

int main(char argc, char **argv) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s type number outfile\n", argv[0]);
    return 1;
  }
  const int pack = atoi(argv[2]);
  Handle resource = GetResource(argv[1], pack);
  if (!resource) {
    fprintf(stderr, "Bad Pack\n");
    return 1;
  }
  /* Decompress */
  LZRWDecodeHandle(&resource);

  char *bytes = *resource;

  /* QuickDrawHeader */
  QuickDrawHeader *header = (QuickDrawHeader *)bytes;
  QD_flip_endianness(header);
  bytes += sizeof(QuickDrawHeader);

  /* Now read the DirectBitsRect */
  uint16_t op = *(uint16_t *)bytes;
  FLIP_SHORT(op);
  bytes += sizeof(uint16_t);

  /* DirectBitsRect */
  if (op == 0x009A) {
    DirectBitsRect direct_bits = DBR_read(&bytes);
    write_ppm(direct_bits.pixels, &direct_bits.dst_rect, argv[3]);
  }
  /* PackBitsRect */
  else if (op == 0x0098) {
    PackBitsRect pack_bits = PBR_read(&bytes);
    write_ppm(pack_bits.pixels, &pack_bits.dst_rect, argv[3]);
  }
  else {
    fprintf(stderr, "Invalid: %s %d\n", argv[1], pack);
    ReleaseResource(resource);
    return 1;
  }
  ReleaseResource(resource);

  return 0;
}
