#include <stdio.h>
#include <string.h>

#include "preferences.h"

#define BOOL(k, v)                                                             \
  { .type = PREF_BOOL, .key = (k), .value.b = (v) }
#define INT(k, v)                                                              \
  { .type = PREF_INT, .key = (k), .value.i = (v) }
#define STR(k, v)                                                              \
  { .type = PREF_STR, .key = (k), .value.s = (v) }

void print_pref(Pref pref) {
  switch (pref.type) {
    case PREF_ERR:
      break;
    case PREF_BOOL:
      printf("`(bool) %s=%s`", pref.key, pref.value.b ? "true" : "false");
      break;
    case PREF_INT:
      printf("`(int) %s=%d`", pref.key, pref.value.i);
      break;
    case PREF_STR:
      printf("`(str) %s=%s`", pref.key, pref.value.s);
      break;
  }
}

int print_err(Pref pref, Pref expected) {
  printf("read ");
  print_pref(pref);
  printf(" and expected ");
  print_pref(expected);
  printf("\n");
  return -1;
}

int main() {
  FILE *file = fopen("prefs-test.ini", "r");
  if (!file) {
    return 1;
  }

  /* Expected preferences found from parsing. See prefs-test.ini for the input
   * data read */
  const Pref expected_prefs[] = {
      BOOL("allow", true),
      BOOL("deny", false),
      BOOL("underscore_valid", true),
      INT("min", 0),
      INT("max", 100),
      INT("neg", -123),
      INT("hex", 0x52fe),
      STR("word", "reckless"),
      STR("code", "ab1234cd"),
      BOOL("mIxEdCaSe", true),
      STR("spaces", "are allowed"),
      STR("many_spaces", "are allowed"),
      STR("key", "value"),
      STR("key", "duplicate keys are allowed"),
      /* Max key length of 63 chars */
      BOOL("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
           false),
      /* Max value length of 511 chars */
      STR("x",
          "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
          "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
          "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
          "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
          "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
          "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
          "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
          "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"),
  };
  const int num_expected = sizeof(expected_prefs) / sizeof(Pref);

  Pref pref;
  int current = 0;
  while (PREFS_read_prefs(file, &pref)) {
    /* Another pref to read */
    if (current == num_expected) {
      printf("found ");
      print_pref(pref);
      printf(" when not expecting more\n");
      return 1;
    }

    Pref expected = expected_prefs[current];

    if (pref.type != expected.type) {
      return print_err(pref, expected);
    }

    if (strcmp(pref.key, expected.key) != 0) {
      return print_err(pref, expected);
    }

    switch (pref.type) {
      case PREF_ERR:
        break;
      case PREF_BOOL:
        if (pref.value.b != expected.value.b) {
          return print_err(pref, expected);
        }
        break;
      case PREF_INT:
        if (pref.value.i != expected.value.i) {
          return print_err(pref, expected);
        }
        break;
      case PREF_STR:
        if (strcmp(pref.value.s, expected.value.s) != 0) {
          return print_err(pref, expected);
        }
        break;
    }

    current++;
  }

  if (current != num_expected) {
    printf("did not test all expected preferences");
    return 1;
  }

  return 0;
}
