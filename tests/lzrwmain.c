#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lzrw.h"

#define STREQ(a, b) (strcmp(a, b) == 0)

int main(int argc, char **argv) {
  if (argc != 4) {
    fprintf(stderr, "Incorrect number of arguments\n");
    fprintf(stderr, "USAGE: %s [-d|-c] infile outfile\n", argv[0]);
    exit(1);
  }

  const char *action_flag = argv[1];
  const char *in_path = argv[2];
  const char *out_path = argv[3];

  int action = 0;
  if (STREQ(action_flag, "-c")) {
    action = COMPRESS_ACTION_COMPRESS;
  }
  else if (STREQ(action_flag, "-d")) {
    action = COMPRESS_ACTION_DECOMPRESS;
  }
  else {
    fprintf(stderr, "Unknown action \"%s\"\n", action_flag);
    exit(1);
  }

  uint32_t working_memory_len = lzrw3_req_mem();

  /* Open files. */
  FILE *in_file = fopen(in_path, "rb");
  if (!in_file) {
    perror(in_path);
    exit(1);
  }

  FILE *out_file = fopen(out_path, "wb");
  if (!out_file) {
    perror(out_path);
    exit(1);
  }

  /* Get file size. */
  fseek(in_file, 0L, SEEK_END);
  long size = ftell(in_file);
  rewind(in_file);

  unsigned char *buffer = malloc(sizeof *buffer * size);
  if (!buffer) {
    fprintf(stderr, "Failed to allocate memory\n");
    exit(1);
  }

  fread(buffer, size, 1, in_file);

  /* Now it's read, let's take action with lzrw. */
  uint32_t size_dst;
  unsigned char *buffer_dst = malloc(sizeof *buffer * size * 10);
  if (!buffer_dst) {
    fprintf(stderr, "Failed to allocate memory\n");
    exit(1);
  }

  unsigned char *working_memory =
      malloc(sizeof *working_memory * working_memory_len);
  if (!working_memory) {
    fprintf(stderr, "Failed to allocate memory\n");
    exit(1);
  }

  lzrw3a_compress(action, working_memory, buffer, size, buffer_dst, &size_dst);

  printf("In size: %ld\nOut size: %d\n", size, size_dst);

  fwrite(buffer_dst, size_dst, 1, out_file);

  fclose(in_file);
  fclose(out_file);

  free(buffer);
  free(buffer_dst);
  free(working_memory);

  return 0;
}
