#include <math.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>

#include "lzrw.h"
#include "resource.h"

#define kUnCryptedHeader 256

uint32_t gKey;

uint32_t CryptData(uint32_t *data, uint32_t len) {
  uint32_t check = 0;
  /* Skip over the header. */
  data += kUnCryptedHeader / 4;
  len -= kUnCryptedHeader;

  while (len >= 4) {
    // *data ^= gKey;
    // check += *data;
    /* Fix for endianness */
    *data = htonl(ntohl(*data) ^ gKey);
    check += ntohl(*data);
    data++;
    len -= 4;
  }

  if (len) {
    *((uint8_t *)data) ^= gKey >> 24;
    /* check += (*((uint8_t *)data)++) << 24; */
    check += ntohl((*((uint8_t *)data))++ << 24);
    if (len > 1) {
      *((uint8_t *)data) ^= (gKey >> 16) & 0xff;
      /* check += (*((uint8_t *)data)++) << 16; */
      check += ntohl((*((uint8_t *)data))++ << 16);
      if (len > 2) {
        *((uint8_t *)data) ^= (gKey >> 8) & 0xff;
        /* check += (*((uint8_t *)data)++) << 8; */
        check += ntohl((*((uint8_t *)data))++ << 8);
      }
    }
  }
  return check;
}

char Hex2Num(char hex) {
  if (hex <= '9') {
    return hex - '0';
  }
  return hex - 'A' + 10;
}

uint32_t CodeStr2Long(const char *code) {
  uint32_t codeNum = 0;
  uint32_t seed = 0;

  /* Validate code. */
  if (strlen(code) != 10) {
    return 0;
  }
  for (int digi = 0; digi < 10; digi++) {
    if (!((code[digi] >= '0' && code[digi] <= '9') ||
          (code[digi] >= 'A' && code[digi] <= 'F'))) {
      return 0;
    }
  }

  for (int digi = 9; digi > 7; digi--) {
    seed += Hex2Num(code[digi]) * pow(16, 9 - digi);
  }
  seed = seed + (seed << 8) + (seed << 16) + (seed << 24);
  for (int digi = 7; digi >= 0; digi--) {
    codeNum += Hex2Num(code[digi]) * pow(16, 7 - digi);
  }
  return codeNum ^ seed;
}

void set_gkey() {
  char name[] = "FREE";
  char code[] = "B3FB09B1EB";

  uint32_t name_num = ntohl(*(uint32_t *)(name + strlen(name) - 4));
  uint32_t code_num = CodeStr2Long(code);

  gKey = code_num ^ name_num;
}

int main(int argc, char **argv) {
  set_gkey();

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

  /* Decrypt levels 4-10 */
  if (strcmp(argv[1], "Pack") == 0 && pack >= 143) {
    CryptData((uint32_t *)*resource, GetHandleSize(resource));
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
