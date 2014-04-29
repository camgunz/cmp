#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "buf.h"

#include <stdbool.h>
#include "cmp.h"

#define run_test(t)                     \
  printf(#t " test: ");                 \
  if (!run_ ## t ## _tests()) {         \
    printf("-- FAILED --\n");           \
    printf("\t %s\n", failure_message); \
    return EXIT_FAILURE;                \
  }                                     \
  else {                                \
    printf("passed\n");                 \
  }

static char failure_message[4096];

static bool buf_reader(cmp_ctx_t *ctx, void *data, size_t limit) {
  buf_t *buf = (buf_t *)ctx->buf;

  return M_BufferRead(buf, data, limit);
}

static size_t buf_writer(cmp_ctx_t *ctx, const void *data, size_t sz) {
  buf_t *buf = (buf_t *)ctx->buf;
  size_t pos = M_BufferGetCursor(buf);

  M_BufferWrite(buf, (void *)data, sz);

  return M_BufferGetCursor(buf) - pos;
}

static void setup_cmp_and_buf(cmp_ctx_t *cmp, buf_t *buf) {
  M_BufferInit(buf);
  cmp_init(cmp, buf, &buf_reader, &buf_writer);
}

static void set_error(const char *msg, ...) {
  va_list args;

  va_start(args, msg);

  vsprintf(failure_message, msg, args);

  va_end(args);
}

bool run_msgpack_tests(void) {
  buf_t in_buf, out_buf;
  cmp_ctx_t in_cmp, out_cmp;
  cmp_object_t obj;
  dboolean buffers_equal = false;

  setup_cmp_and_buf(&in_cmp, &in_buf);
  M_BufferSetFile(&in_buf, "cases.mpac");
  M_BufferSeek(&in_buf, 0);

  setup_cmp_and_buf(&out_cmp, &out_buf);
  M_BufferEnsureCapacity(&out_buf, M_BufferGetSize(&in_buf));

  while (M_BufferGetCursor(&in_buf) < (M_BufferGetSize(&in_buf))) {
    if (!cmp_read_object(&in_cmp, &obj)) {
      set_error("Error reading object: %s\n", cmp_strerror(&in_cmp));
      return false;
    }
    if (!cmp_write_object(&out_cmp, &obj)) {
      set_error("Error writing object: %s\n", cmp_strerror(&out_cmp));
      return false;
    }
  }

  M_BufferSeek(&in_buf, 0);
  M_BufferSeek(&out_buf, 0);
  buffers_equal = M_BufferEqualsData(
    &in_buf, M_BufferGetData(&out_buf), M_BufferGetSize(&out_buf)
  );

  if (!buffers_equal) {
    set_error("Buffers did not match.\n");
    M_BufferPrint(&in_buf);
    M_BufferPrint(&out_buf);
    M_BufferFree(&in_buf);
    M_BufferFree(&out_buf);

    return false;
  }

  M_BufferFree(&in_buf);
  M_BufferFree(&out_buf);

  return true;
}

bool run_fixedint_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;

  setup_cmp_and_buf(&cmp, &buf);

  if (cmp_write_pfix(&cmp, 128)) {
    set_error("Wrote a positive fixed integer that was too large.\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_pfix(&cmp, 200)) {
    set_error("Wrote a positive fixed integer that was too large.\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_pfix(&cmp, -1)) {
    set_error("Wrote a negative positive fixed integer (-1).\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_pfix(&cmp, -31)) {
    set_error("Wrote a negative positive fixed integer (-31).\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_pfix(&cmp, -32)) {
    set_error("Wrote a negative positive fixed integer (-32).\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_pfix(&cmp, -127)) {
    set_error("Wrote a negative positive fixed integer (-127).\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_pfix(&cmp, -128)) {
    set_error("Wrote a negative positive fixed integer (-128).\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_ufix(&cmp, -128)) {
    set_error("Wrote a negative unsigned fixed integer (-128).\n");
    return false;
  }
  cmp.error = 0;

  if (!cmp_write_ufix(&cmp, 0)) {
    set_error("Failed to write ufix (0): %s.\n", cmp_strerror(&cmp));
    return false;
  }
  cmp.error = 0;

  if (!cmp_write_ufix(&cmp, -0)) {
    set_error("Failed to write ufix (-0): %s.\n", cmp_strerror(&cmp));
    return false;
  }
  cmp.error = 0;

  if (cmp_write_ufix(&cmp, -1)) {
    set_error("Wrote a negative unsigned fixed integer.\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_ufix(&cmp, -128)) {
    set_error("Wrote a negative unsigned fixed integer.\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_sfix(&cmp, -33)) {
    set_error("Wrote a negative signed fixed integer that was too large.\n");
    return false;
  }
  cmp.error = 0;

  if (!cmp_write_sfix(&cmp, 0)) {
    set_error("Failed to write sfix (0): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sfix(&cmp, -0)) {
    set_error("Failed to write sfix (-0): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sfix(&cmp, 127)) {
    set_error("Failed to write sfix (127): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sfix(&cmp, -32)) {
    set_error("Failed to write sfix (-32): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (cmp_write_sfix(&cmp, 128)) {
    set_error("Wrote 128 as a signed fixed integer.\n");
    return false;
  }

  if (cmp_write_nfix(&cmp, 0)) {
    set_error("Wrote 0 as a negative fixed integer.\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_nfix(&cmp, 1)) {
    set_error("Wrote 1 as a negative fixed integer.\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_nfix(&cmp, 128)) {
    set_error("Wrote 128 as a negative fixed integer.\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_nfix(&cmp, -33)) {
    set_error("Wrote a negative fixed integer that was too large.\n");
    return false;
  }
  cmp.error = 0;

  if (!cmp_write_pfix(&cmp, 0)) {
    set_error("Failed to write pfix (0): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_pfix(&cmp, 1)) {
    set_error("Failed to write pfix (1): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_pfix(&cmp, 127)) {
    set_error("Failed to write pfix (127): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_nfix(&cmp, -1)) {
    set_error("Failed to write nfix (-1): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_nfix(&cmp, -32)) {
    set_error("Failed to write nfix (-32): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  return true;
}

bool run_number_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;

  setup_cmp_and_buf(&cmp, &buf);

  if (!cmp_write_sint(&cmp, 0)) {
    set_error("Failed to write sint (0): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sint(&cmp, -0)) {
    set_error("Failed to write sint (0): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sint(&cmp, 1)) {
    set_error("Failed to write sint (1): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sint(&cmp, -1)) {
    set_error("Failed to write sint (1): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sint(&cmp, 0x7F)) {
    set_error("Failed to write sint (0x80): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sint(&cmp, 0x80)) {
    set_error("Failed to write sint (0x80): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sint(&cmp, 0xFF)) {
    set_error("Failed to write sint (0xFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sint(&cmp, 0xFFF)) {
    set_error("Failed to write sint (0xFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sint(&cmp, 0xFFFF)) {
    set_error("Failed to write sint (0xFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sint(&cmp, 0xFFFFF)) {
    set_error("Failed to write sint (0xFFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sint(&cmp, 0xFFFFFF)) {
    set_error("Failed to write sint (0xFFFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sint(&cmp, 0xFFFFFFF)) {
    set_error("Failed to write sint (0xFFFFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sint(&cmp, 0xFFFFFFFF)) {
    set_error("Failed to write sint (0xFFFFFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sint(&cmp, 0xFFFFFFFFF)) {
    set_error("Failed to write sint (0xFFFFFFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sint(&cmp, -0xFF)) {
    set_error("Failed to write sint (-0xFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sint(&cmp, -0xFFF)) {
    set_error("Failed to write sint (-0xFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sint(&cmp, -0xFFFF)) {
    set_error("Failed to write sint (-0xFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sint(&cmp, -0xFFFFF)) {
    set_error("Failed to write sint (-0xFFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sint(&cmp, -0xFFFFFF)) {
    set_error("Failed to write sint (-0xFFFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sint(&cmp, -0xFFFFFFF)) {
    set_error("Failed to write sint (-0xFFFFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sint(&cmp, -0xFFFFFFFF)) {
    set_error("Failed to write sint (-0xFFFFFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_sint(&cmp, -0xFFFFFFFFF)) {
    set_error("Failed to write sint (-0xFFFFFFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_uint(&cmp, 0)) {
    set_error("Failed to write uint (0): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_uint(&cmp, -0)) {
    set_error("Failed to write uint (0): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_uint(&cmp, 1)) {
    set_error("Failed to write uint (1): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_uint(&cmp, -1)) {
    set_error("Failed to write uint (1): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_uint(&cmp, 0x7F)) {
    set_error("Failed to write uint (0x80): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_uint(&cmp, 0x80)) {
    set_error("Failed to write uint (0x80): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_uint(&cmp, 0xFF)) {
    set_error("Failed to write uint (0xFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_uint(&cmp, 0xFFF)) {
    set_error("Failed to write uint (0xFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_uint(&cmp, 0xFFFF)) {
    set_error("Failed to write uint (0xFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_uint(&cmp, 0xFFFFF)) {
    set_error("Failed to write uint (0xFFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_uint(&cmp, 0xFFFFFF)) {
    set_error("Failed to write uint (0xFFFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_uint(&cmp, 0xFFFFFFF)) {
    set_error("Failed to write uint (0xFFFFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_uint(&cmp, 0xFFFFFFFF)) {
    set_error("Failed to write uint (0xFFFFFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_uint(&cmp, 0xFFFFFFFFF)) {
    set_error("Failed to write uint (0xFFFFFFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_uint(&cmp, -0xFF)) {
    set_error("Failed to write uint (-0xFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_uint(&cmp, -0xFFF)) {
    set_error("Failed to write uint (-0xFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_uint(&cmp, -0xFFFF)) {
    set_error("Failed to write uint (-0xFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_uint(&cmp, -0xFFFFF)) {
    set_error("Failed to write uint (-0xFFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_uint(&cmp, -0xFFFFFF)) {
    set_error("Failed to write uint (-0xFFFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_uint(&cmp, -0xFFFFFFF)) {
    set_error("Failed to write uint (-0xFFFFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_uint(&cmp, -0xFFFFFFFF)) {
    set_error("Failed to write uint (-0xFFFFFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_uint(&cmp, -0xFFFFFFFFF)) {
    set_error("Failed to write uint (-0xFFFFFFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_float(&cmp, 0.0f)) {
    set_error("Failed to write float (0.0f): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_float(&cmp, -0.0f)) {
    set_error("Failed to write float (-0.0f): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_float(&cmp, 1.0f)) {
    set_error("Failed to write float (1.0f): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_float(&cmp, -1.0f)) {
    set_error("Failed to write float (-1.0f): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_float(&cmp, 0xFFFFFFFF)) {
    set_error("Failed to write float (0xFFFFFFFF): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_float(&cmp, 0x80000000)) {
    set_error("Failed to write float (0x80000000): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_float(&cmp, -0x80000000)) {
    set_error("Failed to write float (-0x80000000): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_double(&cmp, 0.0)) {
    set_error("Failed to write double (0.0): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_double(&cmp, -0.0)) {
    set_error("Failed to write double (-0.0): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_double(&cmp, 1.0)) {
    set_error("Failed to write double (1.0): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_double(&cmp, -1.0)) {
    set_error("Failed to write double (-1.0): %s.\n", cmp_strerror(&cmp));
    return false;
  }

  if (!cmp_write_double(&cmp, 0xFFFFFFFFFFFFFFFF)) {
    set_error("Failed to write double (0xFFFFFFFFFFFFFFFF): %s.\n",
      cmp_strerror(&cmp)
    );
    return false;
  }

  if (!cmp_write_double(&cmp, -0x8000000000000000)) {
    set_error("Failed to write double (-0x8000000000000000): %s.\n",
      cmp_strerror(&cmp)
    );
    return false;
  }

  if (!cmp_write_double(&cmp, 0x8000000000000000)) {
    set_error("Failed to write double (0x8000000000000000): %s.\n",
      cmp_strerror(&cmp)
    );
    return false;
  }

  return true;
}

int main(void) {
  printf("=== Testing CMP v%u ===\n\n", cmp_version());

  run_test(msgpack);
  run_test(fixedint);
  run_test(number);

  printf("\nAll tests pass!\n\n");
  return EXIT_SUCCESS;
}

/* vi: set et ts=2 sw=2: */

