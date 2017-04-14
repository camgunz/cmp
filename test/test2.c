/*
The MIT License (MIT)

Copyright (c) 2017 Charles Gunyon

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

#include "buf.h"
#include "cmp.h"

#define test_int_format(wfunc, rfunc, otype, ctype, in, fmt) do { \
  ctype value;                                                    \
  M_BufferSeek(&buf, 0);                                          \
  assert_true(wfunc(&cmp, in));                                   \
  assert_memory_equal(buf.data, fmt, strlen(fmt));                \
  M_BufferSeek(&buf, 0);                                          \
  assert_true(cmp_read_object(&cmp, &obj));                       \
  assert_int_equal(obj.as.otype, in);                             \
  M_BufferSeek(&buf, 0);                                          \
  assert_true(rfunc(&cmp, &value));                               \
  assert_int_equal(in, value);                                    \
} while (0)

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

static bool buf_skipper(cmp_ctx_t *ctx, size_t count) {
  buf_t *buf = (buf_t *)ctx->buf;

  return M_BufferSeekForward(buf, count);
}

static void setup_cmp_and_buf(cmp_ctx_t *cmp, buf_t *buf) {
  M_BufferInitWithCapacity(buf, 32);
  cmp_init(cmp, buf, &buf_reader, &buf_skipper, &buf_writer);
}

static void test_msgpack(void **state) {
  buf_t in_buf;
  buf_t out_buf;
  cmp_ctx_t in_cmp;
  cmp_ctx_t out_cmp;
  cmp_object_t obj;

  (void)state;

  setup_cmp_and_buf(&in_cmp, &in_buf);
  M_BufferSetFile(&in_buf, "cases.mpac");
  M_BufferSeek(&in_buf, 0);

  setup_cmp_and_buf(&out_cmp, &out_buf);
  M_BufferEnsureCapacity(&out_buf, M_BufferGetSize(&in_buf));

  while (M_BufferGetCursor(&in_buf) < (M_BufferGetSize(&in_buf))) {
    assert_true(cmp_read_object(&in_cmp, &obj));
    assert_true(cmp_write_object(&out_cmp, &obj));
  }

  assert_memory_equal(in_buf.data, out_buf.data, out_buf.size);

  M_BufferFree(&in_buf);
  M_BufferFree(&out_buf);
}

static void test_fixedint(void **state) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

  (void)state;

  setup_cmp_and_buf(&cmp, &buf);

  assert_false(cmp_write_pfix(&cmp, 128));
  cmp.error = 0;

  assert_false(cmp_write_pfix(&cmp, 200));
  cmp.error = 0;

  assert_false(cmp_write_pfix(&cmp, -1));
  cmp.error = 0;

  assert_false(cmp_write_pfix(&cmp, -31));
  cmp.error = 0;

  assert_false(cmp_write_pfix(&cmp, -32));
  cmp.error = 0;

  assert_false(cmp_write_pfix(&cmp, -127));
  cmp.error = 0;

  assert_false(cmp_write_pfix(&cmp, -128));
  cmp.error = 0;

  assert_false(cmp_write_ufix(&cmp, -128));
  cmp.error = 0;

  assert_false(cmp_write_ufix(&cmp, -1));
  cmp.error = 0;

  assert_false(cmp_write_ufix(&cmp, -128));
  cmp.error = 0;

  assert_false(cmp_write_sfix(&cmp, -33));
  cmp.error = 0;

  assert_false(cmp_write_nfix(&cmp, 0));
  cmp.error = 0;

  assert_false(cmp_write_nfix(&cmp, 1));
  cmp.error = 0;

  assert_false(cmp_write_nfix(&cmp, -33));
  cmp.error = 0;

  test_int_format(cmp_write_ufix, cmp_read_uinteger, u8, uint64_t, 0, "\x00");
  test_int_format(cmp_write_ufix, cmp_read_uinteger, u8, uint64_t, -0, "\x00");
  test_int_format(cmp_write_sfix, cmp_read_uinteger, u8, uint64_t, 0, "\x00");
  test_int_format(cmp_write_sfix, cmp_read_sinteger, s8, int64_t, -0, "\x00");
  test_int_format(cmp_write_sfix, cmp_read_uinteger, u8, uint64_t, 127, "\x7f");
  test_int_format(cmp_write_sfix, cmp_read_sinteger, s8, int64_t, -32, "\xe0");
  test_int_format(cmp_write_pfix, cmp_read_uinteger, u8, uint64_t, 0, "\x00");
  test_int_format(cmp_write_pfix, cmp_read_uinteger, u8, uint64_t, 1, "\x01");
  test_int_format(cmp_write_pfix, cmp_read_uinteger, u8, uint64_t, 127, "\x7f");
  test_int_format(cmp_write_nfix, cmp_read_sinteger, s8, int64_t, -1, "\xff");
  test_int_format(cmp_write_nfix, cmp_read_sinteger, s8, int64_t, -32, "\xe0");
}

int main(void) {
    int failed_test_count = 0;

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_msgpack),
        cmocka_unit_test(test_fixedint),
    };

    failed_test_count = cmocka_run_group_tests(tests, NULL, NULL);

    if (failed_test_count > 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/* vi: set et ts=4 sw=4: */
