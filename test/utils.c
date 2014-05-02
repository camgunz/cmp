#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

void doom_printf(const char *msg, ...) {
  va_list args;

  va_start(args, msg);

  vprintf(msg, args);

  va_end(args);
}

void I_Error(const char *msg, ...) {
  va_list args;

  va_start(args, msg);

  vfprintf(stderr, msg, args);

  va_end(args);

  exit(EXIT_FAILURE);
}

