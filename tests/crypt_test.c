#include <math.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>

#include "defines.h"
#include "lzrw.h"
#include "resource.h"

// 15
#define kEncryptedPack 15
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
  printf("Decrypt check: %u\n", check);
  return check;
}

/* Only used to validate the FIRST encrypted level data (level 4)
 * The remainder will not match Chck from the resource fork.
 */
bool CheckPack(int num, uint32_t check) {
  bool ok = false;
  Handle pack = GetResource("Pack", num + 128);

  if (pack) {
    if (num >= kEncryptedPack) {
      ok = check == CryptData((uint32_t *)*pack, GetHandleSize(pack));
    }
  }

  /* Attempt to decompress... */
  LZRWDecodeHandle(&pack);

  return ok;
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
  if (strlen(code) != 11) {
    return 0;
  }
  for (int digi = 10; digi > 0; digi--) {
    if (!((code[digi] >= '0' && code[digi] <= '9') ||
          (code[digi] >= 'A' && code[digi] <= 'F'))) {
      return 0;
    }
  }

  for (int digi = 10; digi > 8; digi--) {
    seed += Hex2Num(code[digi]) * pow(16, 10 - digi);
  }
  seed = seed + (seed << 8) + (seed << 16) + (seed << 24);
  for (int digi = 8; digi > 0; digi--) {
    codeNum += Hex2Num(code[digi]) * pow(16, 8 - digi);
  }
  return codeNum ^ seed;
}

int main() {
  const int pack = 128;
  const char *pack_name = "Chck";

  Handle check = GetResource(pack_name, pack);
  if (!check) {
    fprintf(stderr, "Unable to read crypt data\n");
    return 1;
  }
  uint32_t check_n = ntohl(**(uint32_t **)check);
  printf("Check: %u\n", check_n);

  char name[] = "NOWHEREMAN";

  /* It appears that valid registered usernames were >= 4 characters as this
   * grabs the last 4 for decryption */
  uint32_t name_num = ntohl(*(uint32_t *)(name + strlen(name) - 4)); // FREE
  printf("name_num: %u\n", name_num);

  uint32_t code_num = CodeStr2Long(" CB9F76C190");
  printf("code_num: %u\n", code_num);

  gKey = code_num ^ name_num;
  // gKey = code_num ^ 0x46524545;
  printf("gKey: %u\n", gKey);

  if (CheckPack(kEncryptedPack, check_n)) {
    printf("pack is valid\n");
  }
  else {
    printf("Pack is invalid\n");
  }

  return 0;
}
