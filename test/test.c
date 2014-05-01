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

#define run_tests(t)                    \
  printf(#t " test: ");                 \
  if (!run_ ## t ## _tests()) {         \
    printf("-- FAILED --\n");           \
    printf("\t %s\n", failure_message); \
    return EXIT_FAILURE;                \
  }                                     \
  else {                                \
    printf("passed\n");                 \
  }

#define test_format(func, in, data, data_length)                              \
  M_BufferClear(&buf);                                                        \
  if (!func(&cmp, in)) {                                                      \
    set_error("%s(&cmp, %s) failed: %s\n", #func, #in, cmp_strerror(&cmp));   \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!M_BufferEqualsData(&buf, data, data_length)) {                         \
    set_error("Wrote invalid MessagePack data.\n");                           \
    printf("\n");                                                             \
    printf("%s(&cmp, %s)\n", #func, #in);                                     \
    print_bin(data, data_length);                                             \
    print_bin(M_BufferGetData(&buf), M_BufferGetSize(&buf));                  \
    return false;                                                             \
  }

#define test_str_format(func, in, length, data, data_length)                  \
  M_BufferClear(&buf);                                                        \
  if (!func(&cmp, in, length)) {                                              \
    set_error(                                                                \
      "%s(&cmp, %s, %d) failed: %s\n",                                        \
      #func, #in, length, cmp_strerror(&cmp)                                  \
    );                                                                        \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!M_BufferEqualsData(&buf, data, data_length)) {                         \
    set_error("Wrote invalid MessagePack data.\n");                           \
    printf("\n");                                                             \
    printf("%s(&cmp, %s)\n", #func, #in);                                     \
    print_bin(data, data_length);                                             \
    print_bin(M_BufferGetData(&buf), M_BufferGetSize(&buf));                  \
    return false;                                                             \
  }

#define test_format_no_input(func, data, data_length)                         \
  M_BufferClear(&buf);                                                        \
  if (!func(&cmp)) {                                                          \
    set_error("%s(&cmp) failed: %s\n", #func, cmp_strerror(&cmp));            \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!M_BufferEqualsData(&buf, data, data_length)) {                         \
    set_error("Wrote invalid MessagePack data.\n");                           \
    printf("%s(&cmp):\n", #func);                                             \
    print_bin(data, data_length);                                             \
    print_bin(M_BufferGetData(&buf), M_BufferGetSize(&buf));                  \
    return false;                                                             \
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
  M_BufferInitWithCapacity(buf, 32);
  cmp_init(cmp, buf, &buf_reader, &buf_writer);
}

static void set_error(const char *msg, ...) {
  va_list args;

  va_start(args, msg);

  vsprintf(failure_message, msg, args);

  va_end(args);
}

static void print_bin(const char *data, size_t length) {
  size_t i;

  for (i = 0; i < length; i++) {
    printf("%02x ", data[i]);

    if ((i != 0) && ((i % 26) == 0))
      printf("\n");
  }
  printf("\n");
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

  test_format(cmp_write_ufix, 0, "\x00", 1);
  test_format(cmp_write_ufix, -0, "\x00", 1);
  test_format(cmp_write_sfix, 0, "\x00", 1);
  test_format(cmp_write_sfix, -0, "\x00", 1);
  test_format(cmp_write_sfix, 127, "\x7f", 1);
  test_format(cmp_write_sfix, -32, "\xe0", 1);
  test_format(cmp_write_pfix, 0, "\x00", 1);
  test_format(cmp_write_pfix, 1, "\x01", 1);
  test_format(cmp_write_pfix, 127, "\x7f", 1);
  test_format(cmp_write_nfix, -1, "\xff", 1);
  test_format(cmp_write_nfix, -32, "\xe0", 1);

  return true;
}

bool run_number_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;

  setup_cmp_and_buf(&cmp, &buf);

  test_format(cmp_write_s8,  0,   "\xd0\x00", 2);
  test_format(cmp_write_s8,  1,   "\xd0\x01", 2);
  test_format(cmp_write_s8, -1,   "\xd0\xff", 2);
  test_format(cmp_write_s8,  127, "\xd0\x7f", 2);
  test_format(cmp_write_s8, -128, "\xd0\x80", 2);

  test_format(cmp_write_s16,  0,     "\xd1\x00\x00", 3);
  test_format(cmp_write_s16,  1,     "\xd1\x00\x01", 3);
  test_format(cmp_write_s16, -1,     "\xd1\xff\xff", 3);
  test_format(cmp_write_s16,  127,   "\xd1\x00\x7f", 3);
  test_format(cmp_write_s16, -128,   "\xd1\xff\x80", 3);
  test_format(cmp_write_s16,  256,   "\xd1\x01\x00", 3);
  test_format(cmp_write_s16,  32767, "\xd1\x7f\xff", 3);
  test_format(cmp_write_s16, -32768, "\xd1\x80\x00", 3);

  test_format(cmp_write_s32,  0,          "\xd2\x00\x00\x00\x00", 5);
  test_format(cmp_write_s32,  1,          "\xd2\x00\x00\x00\x01", 5);
  test_format(cmp_write_s32, -1,          "\xd2\xff\xff\xff\xff", 5);
  test_format(cmp_write_s32,  127,        "\xd2\x00\x00\x00\x7f", 5);
  test_format(cmp_write_s32, -128,        "\xd2\xff\xff\xff\x80", 5);
  test_format(cmp_write_s32,  256,        "\xd2\x00\x00\x01\x00", 5);
  test_format(cmp_write_s32,  32767,      "\xd2\x00\x00\x7f\xff", 5);
  test_format(cmp_write_s32, -32768,      "\xd2\xff\xff\x80\x00", 5);
  test_format(cmp_write_s32,  65535,      "\xd2\x00\x00\xff\xff", 5);
  test_format(cmp_write_s32, -65536,      "\xd2\xff\xff\x00\x00", 5);
  test_format(cmp_write_s32,  8388607,    "\xd2\x00\x7f\xff\xff", 5);
  test_format(cmp_write_s32, -8388608,    "\xd2\xff\x80\x00\x00", 5);
  test_format(cmp_write_s32,  16777215,   "\xd2\x00\xff\xff\xff", 5);
  test_format(cmp_write_s32, -16777216,   "\xd2\xff\x00\x00\x00", 5);
  test_format(cmp_write_s32,  2147483647, "\xd2\x7f\xff\xff\xff", 5);
  test_format(cmp_write_s32, -2147483648, "\xd2\x80\x00\x00\x00", 5);

  test_format(cmp_write_s64,  0,          "\xd3\x00\x00\x00\x00\x00\x00\x00\x00", 9);
  test_format(cmp_write_s64,  1,          "\xd3\x00\x00\x00\x00\x00\x00\x00\x01", 9);
  test_format(cmp_write_s64, -1,          "\xd3\xff\xff\xff\xff\xff\xff\xff\xff", 9);
  test_format(cmp_write_s64,  127,        "\xd3\x00\x00\x00\x00\x00\x00\x00\x7f", 9);
  test_format(cmp_write_s64, -128,        "\xd3\xff\xff\xff\xff\xff\xff\xff\x80", 9);
  test_format(cmp_write_s64,  256,        "\xd3\x00\x00\x00\x00\x00\x00\x01\x00", 9);
  test_format(cmp_write_s64,  32767,      "\xd3\x00\x00\x00\x00\x00\x00\x7f\xff", 9);
  test_format(cmp_write_s64, -32768,      "\xd3\xff\xff\xff\xff\xff\xff\x80\x00", 9);
  test_format(cmp_write_s64,  65535,      "\xd3\x00\x00\x00\x00\x00\x00\xff\xff", 9);
  test_format(cmp_write_s64, -65536,      "\xd3\xff\xff\xff\xff\xff\xff\x00\x00", 9);
  test_format(cmp_write_s64,  8388607,    "\xd3\x00\x00\x00\x00\x00\x7f\xff\xff", 9);
  test_format(cmp_write_s64, -8388608,    "\xd3\xff\xff\xff\xff\xff\x80\x00\x00", 9);
  test_format(cmp_write_s64,  16777215,   "\xd3\x00\x00\x00\x00\x00\xff\xff\xff", 9);
  test_format(cmp_write_s64, -16777216,   "\xd3\xff\xff\xff\xff\xff\x00\x00\x00", 9);
  test_format(cmp_write_s64,  2147483647, "\xd3\x00\x00\x00\x00\x7f\xff\xff\xff", 9);
  test_format(cmp_write_s64, -2147483648, "\xd3\xff\xff\xff\xff\x80\x00\x00\x00", 9);
  test_format(cmp_write_s64,  4294967295, "\xd3\x00\x00\x00\x00\xff\xff\xff\xff", 9);
  test_format(cmp_write_s64, -4294967296, "\xd3\xff\xff\xff\xff\x00\x00\x00\x00", 9);

  test_format(cmp_write_sint, 0,           "\x00", 1);
  test_format(cmp_write_sint, 1,           "\x01", 1);
  test_format(cmp_write_sint, 127,         "\x7f", 1);
  test_format(cmp_write_sint, 128,         "\xcc\x80", 2);
  test_format(cmp_write_sint, 255,         "\xcc\xff", 2);
  test_format(cmp_write_sint, 256,         "\xcd\x01\x00", 3);
  test_format(cmp_write_sint, 32767,       "\xcd\x7f\xff", 3);
  test_format(cmp_write_sint, 32768,       "\xcd\x80\x00", 3);
  test_format(cmp_write_sint, 65535,       "\xcd\xff\xff", 3);
  test_format(cmp_write_sint, 65536,       "\xce\x00\x01\x00\x00", 5);
  test_format(cmp_write_sint, 8388607,     "\xce\x00\x7f\xff\xff", 5);
  test_format(cmp_write_sint, 8388608,     "\xce\x00\x80\x00\x00", 5);
  test_format(cmp_write_sint, 16777215,    "\xce\x00\xff\xff\xff", 5);
  test_format(cmp_write_sint, 16777216,    "\xce\x01\x00\x00\x00", 5);
  test_format(cmp_write_sint, 2147483647,  "\xce\x7f\xff\xff\xff", 5);
  test_format(cmp_write_sint, 2147483648,  "\xce\x80\x00\x00\x00", 5);
  test_format(cmp_write_sint, 4294967295,  "\xce\xff\xff\xff\xff", 5);
  test_format(
    cmp_write_sint, 4294967296, "\xcf\x00\x00\x00\x01\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, 549755813887, "\xcf\x00\x00\x00\x7f\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_sint, 549755813888, "\xcf\x00\x00\x00\x80\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, 1099511627775, "\xcf\x00\x00\x00\xff\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_sint, 1099511627776, "\xcf\x00\x00\x01\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, 140737488355327, "\xcf\x00\x00\x7f\xff\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_sint, 140737488355328, "\xcf\x00\x00\x80\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, 281474976710655, "\xcf\x00\x00\xff\xff\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_sint, 281474976710656, "\xcf\x00\x01\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, 36028797018963967, "\xcf\x00\x7f\xff\xff\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_sint, 36028797018963968, "\xcf\x00\x80\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, 72057594037927935, "\xcf\x00\xff\xff\xff\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_sint, 72057594037927936, "\xcf\x01\x00\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, 9223372036854775807, "\xcf\x7f\xff\xff\xff\xff\xff\xff\xff", 9
  );

  test_format(cmp_write_sint, -1,          "\xff", 1);
  test_format(cmp_write_sint, -32,         "\xe0", 1);
  test_format(cmp_write_sint, -127,        "\xd0\x81", 2);
  test_format(cmp_write_sint, -128,        "\xd0\x80", 2);
  test_format(cmp_write_sint, -255,        "\xd1\xff\x01", 3);
  test_format(cmp_write_sint, -256,        "\xd1\xff\x00", 3);
  test_format(cmp_write_sint, -32767,      "\xd1\x80\x01", 3);
  test_format(cmp_write_sint, -32768,      "\xd1\x80\x00", 3);
  test_format(cmp_write_sint, -65535,      "\xd2\xff\xff\x00\x01", 5);
  test_format(cmp_write_sint, -65536,      "\xd2\xff\xff\x00\x00", 5);
  test_format(cmp_write_sint, -8388607,    "\xd2\xff\x80\x00\x01", 5);
  test_format(cmp_write_sint, -8388608,    "\xd2\xff\x80\x00\x00", 5);
  test_format(cmp_write_sint, -16777215,   "\xd2\xff\x00\x00\x01", 5);
  test_format(cmp_write_sint, -16777216,   "\xd2\xff\x00\x00\x00", 5);
  test_format(cmp_write_sint, -2147483647, "\xd2\x80\x00\x00\x01", 5);
  test_format(cmp_write_sint, -2147483648, "\xd2\x80\x00\x00\x00", 5);
  test_format(
    cmp_write_sint, -4294967295, "\xd3\xff\xff\xff\xff\x00\x00\x00\x01", 9
  );
  test_format(
    cmp_write_sint, -4294967296, "\xd3\xff\xff\xff\xff\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, -549755813887, "\xd3\xff\xff\xff\x80\x00\x00\x00\x01", 9
  );
  test_format(
    cmp_write_sint, -549755813888, "\xd3\xff\xff\xff\x80\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, -1099511627775, "\xd3\xff\xff\xff\x00\x00\x00\x00\x01", 9
  );
  test_format(
    cmp_write_sint, -1099511627776, "\xd3\xff\xff\xff\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, -140737488355327, "\xd3\xff\xff\x80\x00\x00\x00\x00\x01", 9
  );
  test_format(
    cmp_write_sint, -140737488355328, "\xd3\xff\xff\x80\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, -281474976710655, "\xd3\xff\xff\x00\x00\x00\x00\x00\x01", 9
  );
  test_format(
    cmp_write_sint, -281474976710656, "\xd3\xff\xff\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, -36028797018963967, "\xd3\xff\x80\x00\x00\x00\x00\x00\x01", 9
  );
  test_format(
    cmp_write_sint, -36028797018963968, "\xd3\xff\x80\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, -72057594037927935, "\xd3\xff\x00\x00\x00\x00\x00\x00\x01", 9
  );
  test_format(
    cmp_write_sint, -72057594037927936, "\xd3\xff\x00\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_sint, -9223372036854775807, "\xd3\x80\x00\x00\x00\x00\x00\x00\x01", 9
  );

  test_format(cmp_write_u8, 0,   "\xcc\x00", 1);
  test_format(cmp_write_u8, 1,   "\xcc\x01", 1);
  test_format(cmp_write_u8, 127, "\xcc\x7f", 1);
  test_format(cmp_write_u8, 255, "\xcc\xff", 1);

  test_format(cmp_write_u16, 0,     "\xcd\x00\x00", 2);
  test_format(cmp_write_u16, 1,     "\xcd\x00\x01", 2);
  test_format(cmp_write_u16, 127,   "\xcd\x00\x7f", 2);
  test_format(cmp_write_u16, 256,   "\xcd\x01\x00", 2);
  test_format(cmp_write_u16, 32767, "\xcd\x7f\xff", 2);
  test_format(cmp_write_u16, 65535, "\xcd\xff\xff", 2);

  test_format(cmp_write_u32, 0,          "\xce\x00\x00\x00\x00", 5);
  test_format(cmp_write_u32, 1,          "\xce\x00\x00\x00\x01", 5);
  test_format(cmp_write_u32, 127,        "\xce\x00\x00\x00\x7f", 5);
  test_format(cmp_write_u32, 256,        "\xce\x00\x00\x01\x00", 5);
  test_format(cmp_write_u32, 32767,      "\xce\x00\x00\x7f\xff", 5);
  test_format(cmp_write_u32, 65535,      "\xce\x00\x00\xff\xff", 5);
  test_format(cmp_write_u32, 8388607,    "\xce\x00\x7f\xff\xff", 5);
  test_format(cmp_write_u32, 16777215,   "\xce\x00\xff\xff\xff", 5);
  test_format(cmp_write_u32, 2147483647, "\xce\x7f\xff\xff\xff", 5);
  test_format(cmp_write_u32, 4294967295, "\xce\xff\xff\xff\xff", 5);

  test_format(cmp_write_u64, 0, "\xcf\x00\x00\x00\x00\x00\x00\x00\x00", 9);
  test_format(cmp_write_u64, 1, "\xcf\x00\x00\x00\x00\x00\x00\x00\x01", 9);
  test_format(cmp_write_u64, 127, "\xcf\x00\x00\x00\x00\x00\x00\x00\x7f", 9);
  test_format(cmp_write_u64, 256, "\xcf\x00\x00\x00\x00\x00\x00\x01\x00", 9);
  test_format(cmp_write_u64, 32767, "\xcf\x00\x00\x00\x00\x00\x00\x7f\xff", 9);
  test_format(cmp_write_u64, 65535, "\xcf\x00\x00\x00\x00\x00\x00\xff\xff", 9);
  test_format(
    cmp_write_u64, 8388607, "\xcf\x00\x00\x00\x00\x00\x7f\xff\xff", 9
  );
  test_format(
    cmp_write_u64, 16777215, "\xcf\x00\x00\x00\x00\x00\xff\xff\xff", 9
  );
  test_format(
    cmp_write_u64, 2147483647, "\xcf\x00\x00\x00\x00\x7f\xff\xff\xff", 9
  );
  test_format(
    cmp_write_u64, 4294967295, "\xcf\x00\x00\x00\x00\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_u64,
    (9223372036854775807 * 2) + 1,
    "\xcf\xff\xff\xff\xff\xff\xff\xff\xff",
    9
  );

  test_format(
    cmp_write_uint, 0, "\x00", 1
  );
  test_format(
    cmp_write_uint, 1, "\x01", 1
  );
  test_format(
    cmp_write_uint, 127, "\x7f", 1
  );
  test_format(
    cmp_write_uint, 128, "\xcc\x80", 2
  );
  test_format(
    cmp_write_uint, 255, "\xcc\xff", 2
  );
  test_format(
    cmp_write_uint, 256, "\xcd\x01\x00", 3
  );
  test_format(
    cmp_write_uint, 32767, "\xcd\x7f\xff", 3
  );
  test_format(
    cmp_write_uint, 32768, "\xcd\x80\x00", 3
  );
  test_format(
    cmp_write_uint, 65535, "\xcd\xff\xff", 3
  );
  test_format(
    cmp_write_uint, 65536, "\xce\x00\x01\x00\x00", 5
  );
  test_format(
    cmp_write_uint, 8388607, "\xce\x00\x7f\xff\xff", 5
  );
  test_format(
    cmp_write_uint, 8388608, "\xce\x00\x80\x00\x00", 5
  );
  test_format(
    cmp_write_uint, 16777215, "\xce\x00\xff\xff\xff", 5
  );
  test_format(
    cmp_write_uint, 16777216, "\xce\x01\x00\x00\x00", 5
  );
  test_format(
    cmp_write_uint, 2147483647, "\xce\x7f\xff\xff\xff", 5
  );
  test_format(
    cmp_write_uint, 2147483648, "\xce\x80\x00\x00\x00", 5
  );
  test_format(
    cmp_write_uint, 4294967295, "\xce\xff\xff\xff\xff", 5
  );
  test_format(
    cmp_write_uint, 4294967296, "\xcf\x00\x00\x00\x01\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_uint, 549755813887, "\xcf\x00\x00\x00\x7f\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_uint, 549755813888, "\xcf\x00\x00\x00\x80\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_uint, 1099511627775, "\xcf\x00\x00\x00\xff\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_uint, 1099511627776, "\xcf\x00\x00\x01\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_uint, 140737488355327, "\xcf\x00\x00\x7f\xff\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_uint, 140737488355328, "\xcf\x00\x00\x80\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_uint, 281474976710655, "\xcf\x00\x00\xff\xff\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_uint, 281474976710656, "\xcf\x00\x01\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_uint, 36028797018963967, "\xcf\x00\x7f\xff\xff\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_uint, 36028797018963968, "\xcf\x00\x80\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_uint, 72057594037927935, "\xcf\x00\xff\xff\xff\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_uint, 72057594037927936, "\xcf\x01\x00\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_uint, 9223372036854775807, "\xcf\x7f\xff\xff\xff\xff\xff\xff\xff", 9
  );
  test_format(
    cmp_write_uint, 0xFFFFFFFFFFFFFFFF, "\xcf\xff\xff\xff\xff\xff\xff\xff\xff", 9
  );

  test_format(cmp_write_float, 0.0f,      "\xca\x00\x00\x00\x00", 5);
  test_format(cmp_write_float, -0.0f,     "\xca\x80\x00\x00\x00", 5);
  test_format(cmp_write_float, 1.0f,      "\xca\x3f\x80\x00\x00", 5);
  test_format(cmp_write_float, -1.0f,     "\xca\xbf\x80\x00\x00", 5);
  test_format(cmp_write_float, 65535.0f,  "\xca\x47\x7f\xff\x00", 5);
  test_format(cmp_write_float, -65535.0f, "\xca\xc7\x7f\xff\x00", 5);
  test_format(cmp_write_float, 32767.0f,  "\xca\x46\xff\xfe\x00", 5);
  test_format(cmp_write_float, -32767.0f, "\xca\xc6\xff\xfe\x00", 5);

  test_format(
    cmp_write_double, 0.0, "\xcb\x00\x00\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_double, -0.0, "\xcb\x80\x00\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_double, 1.0, "\xcb\x3f\xf0\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_double, -1.0, "\xcb\xbf\xf0\x00\x00\x00\x00\x00\x00", 9
  );
  test_format(
    cmp_write_double, 2147483647.0, "\xcb\x41\xdf\xff\xff\xff\xc0\x00\x00", 9
  );
  test_format(
    cmp_write_double, -2147483647.0, "\xcb\xc1\xdf\xff\xff\xff\xc0\x00\x00", 9
  );
  test_format(
    cmp_write_double, 4294967295.0, "\xcb\x41\xef\xff\xff\xff\xe0\x00\x00", 9
  );
  test_format(
    cmp_write_double, -4294967295.0, "\xcb\xc1\xef\xff\xff\xff\xe0\x00\x00", 9
  );

  return true;
}

bool run_nil_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;

  setup_cmp_and_buf(&cmp, &buf);

  test_format_no_input(cmp_write_nil, "\xc0", 1)

  return true;
}

bool run_boolean_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;

  setup_cmp_and_buf(&cmp, &buf);

  test_format_no_input(cmp_write_false, "\xc2", 1)
  test_format_no_input(cmp_write_true,  "\xc3", 1)

  return true;
}

bool run_binary_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;

  setup_cmp_and_buf(&cmp, &buf);

  test_str_format(cmp_write_bin8, "Hey there\n", 10, "\xc4\x0aHey there\n", 12);
  test_str_format(
    cmp_write_bin16, "Hey there\n", 10, "\xc5\x00\x0aHey there\n", 13
  );
  test_str_format(
    cmp_write_bin32, "Hey there\n", 10, "\xc6\x00\x00\x00\x0aHey there\n", 15
  );
  test_str_format(cmp_write_bin, "Hey there\n", 10, "\xc4\x0aHey there\n", 12);

  return true;
}

bool run_string_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;

  setup_cmp_and_buf(&cmp, &buf);

  test_str_format(cmp_write_fixstr, "Hey there\n", 10, "\xaaHey there\n", 11);
  test_str_format(
    cmp_write_str8, "Hey there\n", 10, "\xd9\x0aHey there\n", 12
  );
  test_str_format(
    cmp_write_str16, "Hey there\n", 10, "\xda\x00\x0aHey there\n", 13
  );
  test_str_format(
    cmp_write_str32, "Hey there\n", 10, "\xdb\x00\x00\x00\x0aHey there\n", 15
  );
  test_str_format(cmp_write_str, "Hey there\n", 10, "\xaaHey there\n", 11);

  return true;
}

bool run_array_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;

  setup_cmp_and_buf(&cmp, &buf);

  test_format(cmp_write_fixarray, 10, "\x9a", 1);
  test_format(cmp_write_array16, 10, "\xdc\x00\x0a", 3);
  test_format(cmp_write_array32, 10, "\xdd\x00\x00\x00\x0a", 5);
  test_format(cmp_write_array, 10, "\x9a", 1);

  return true;
}

bool run_map_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;

  setup_cmp_and_buf(&cmp, &buf);

  test_format(cmp_write_fixmap, 10, "\x8a", 1);
  test_format(cmp_write_map16, 10, "\xde\x00\x0a", 3);
  test_format(cmp_write_map32, 10, "\xdf\x00\x00\x00\x0a", 5);
  test_format(cmp_write_map, 10, "\x8a", 1);

  return true;
}

int main(void) {
  printf("=== Testing CMP v%u ===\n\n", cmp_version());

  run_tests(msgpack);
  run_tests(fixedint);
  run_tests(number);
  run_tests(nil);
  run_tests(boolean);
  run_tests(binary);
  run_tests(string);
  run_tests(array);
  run_tests(map);

  printf("\nAll tests pass!\n\n");
  return EXIT_SUCCESS;
}

/* vi: set et ts=2 sw=2: */

