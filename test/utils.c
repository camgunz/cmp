#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

void error_and_exit(const char *msg) {
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}

void errorf_and_exit(const char *msg, ...) {
  va_list args;

  va_start(args, msg);
  vfprintf(stderr, msg, args);
  va_end(args);

  exit(EXIT_FAILURE);
}

char* _strdup(const char *s) {
  char *out = calloc(strlen(s) + 1, sizeof(char));

  strcpy(out, s);

  return out;
}

/* vi: set et ts=2 sw=2: */

