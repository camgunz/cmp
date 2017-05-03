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

static bool force_reader_to_fail = false;
static bool force_writer_to_fail = false;
static bool force_skipper_to_fail = false;

#define assert_float_equal(f1, f2) \
  assert_memory_equal(&(f1), &(f2), sizeof(float))

#define assert_double_equal(d1, d2) \
  assert_memory_equal(&(d1), &(d2), sizeof(double))

#define test_format(wfunc, rfunc, otype, ctype, in, fmt, dlen) do {           \
  M_BufferClear(&buf);                                                        \
  assert_true(wfunc(&cmp, in));                                               \
  M_BufferSeek(&buf, 0);                                                      \
  assert_memory_equal(buf.data, fmt, dlen);                                   \
  M_BufferSeek(&buf, 0);                                                      \
  assert_true(cmp_read_object(&cmp, &obj));                                   \
  assert_true(obj.as.otype == in);                                            \
  M_BufferSeek(&buf, 0);                                                      \
  do {                                                                        \
    ctype value;                                                              \
    assert_true(rfunc(&cmp, (ctype *)&value));                                \
    assert_true(in == value);                                                 \
  } while (0);                                                                \
} while (0)

#define test_format_with_length(wfunc, rfunc, otype, in, len, fmt, dlen)      \
  M_BufferClear(&buf);                                                        \
  assert_true(wfunc(&cmp, in, len));                                          \
  M_BufferSeek(&buf, 0);                                                      \
  assert_memory_equal(buf.data, fmt, dlen);                                   \
  M_BufferSeek(&buf, 0);                                                      \
  assert_true(cmp_read_object(&cmp, &obj));                                   \
  assert_true(obj.as.otype == len);                                           \
  M_BufferSeek(&buf, 0);                                                      \
  do {                                                                        \
    char ldata[len + 1];                                                      \
    uint32_t data_length = len + 1;                                           \
    memset(ldata, 0, sizeof(ldata));                                          \
    assert_true(rfunc(&cmp, ldata, &data_length));                            \
    assert_int_equal(data_length, len);                                       \
    assert_memory_equal(ldata, in, len);                                      \
  } while (0);

#define test_int_format(wfunc, rfunc, otype, ctype, in, fmt, len) do { \
  ctype value;                                                         \
  M_BufferSeek(&buf, 0);                                               \
  assert_true(wfunc(&cmp, in));                                        \
  assert_memory_equal(buf.data, fmt, len);                             \
  M_BufferSeek(&buf, 0);                                               \
  assert_true(cmp_read_object(&cmp, &obj));                            \
  assert_int_equal(obj.as.otype, in);                                  \
  M_BufferSeek(&buf, 0);                                               \
  assert_true(rfunc(&cmp, &value));                                    \
  assert_int_equal(in, value);                                         \
} while (0)

#define test_float_format(wfunc, rfunc, otype, ctype, in, fmt, len) do { \
  ctype value;                                                           \
  M_BufferSeek(&buf, 0);                                                 \
  assert_true(wfunc(&cmp, in));                                          \
  assert_memory_equal(buf.data, fmt, len);                               \
  M_BufferSeek(&buf, 0);                                                 \
  assert_true(cmp_read_object(&cmp, &obj));                              \
  assert_true(in == obj.as.otype);                                       \
  M_BufferSeek(&buf, 0);                                                 \
  assert_true(rfunc(&cmp, &value));                                      \
  assert_true(in == value);                                              \
} while (0)

#define test_double_format(wfunc, rfunc, otype, ctype, in, fmt, len) do { \
  ctype value;                                                            \
  M_BufferSeek(&buf, 0);                                                  \
  assert_true(wfunc(&cmp, in));                                           \
  assert_memory_equal(buf.data, fmt, len);                                \
  M_BufferSeek(&buf, 0);                                                  \
  assert_true(cmp_read_object(&cmp, &obj));                               \
  assert_true(in == obj.as.otype);                                        \
  M_BufferSeek(&buf, 0);                                                  \
  assert_true(rfunc(&cmp, &value));                                       \
  assert_true(in == value);                                               \
} while (0)

#define test_format_no_input(wfunc, otype, fmt, dlen, out) do {              \
  M_BufferClear(&buf);                                                       \
  assert_true(wfunc(&cmp));                                                  \
  M_BufferSeek(&buf, 0);                                                     \
  assert_memory_equal(buf.data, fmt, dlen);                                  \
  M_BufferSeek(&buf, 0);                                                     \
  assert_true(cmp_read_object(&cmp, &obj));                                  \
  assert_int_equal(obj.as.otype, out);                                       \
} while (0)

#define obj_write(func, val)                                                  \
  M_BufferSeek(&buf, 0);                                                      \
  func(&cmp, val);                                                            \
  M_BufferSeek(&buf, 0);                                                      \
  cmp_read_object(&cmp, &obj);

#define obj_write_no_val(func)                                                \
  M_BufferSeek(&buf, 0);                                                      \
  func(&cmp);                                                                 \
  M_BufferSeek(&buf, 0);                                                      \
  cmp_read_object(&cmp, &obj);

#define obj_write_len(func, val, len)                                         \
  M_BufferSeek(&buf, 0);                                                      \
  func(&cmp, val, len);                                                       \
  M_BufferSeek(&buf, 0);                                                      \
  cmp_read_object(&cmp, &obj);

#define obj_test(func, as_func, type, ctype, val) do {                        \
  ctype var;                                                                  \
  assert_true(func(&obj));                                                    \
  assert_true(as_func(&obj, &var));                                           \
  assert_true(var == val);                                                    \
} while (0);

#define obj_str_test(val) do {                                                \
  uint32_t length;                                                            \
  assert_true(cmp_object_is_str(&obj));                                       \
  assert_true(cmp_object_as_str(&obj, &length));                              \
  assert_string_equal(M_BufferGetDataAtCursor(&buf), val);                    \
} while (0);

#define obj_to_str_test(val) do {                                             \
  uint32_t length;                                                            \
  char data[255];                                                             \
  assert_true(cmp_object_is_str(&obj));                                       \
  assert_true(cmp_object_as_str(&obj, &length));                              \
  assert_true(cmp_object_to_str(&cmp, &obj, data, 255));                      \
  assert_string_equal(data, val);                                             \
} while (0);

#define obj_bin_test(val, inlength) do {                                      \
  uint32_t length;                                                            \
  assert_true(cmp_object_is_bin(&obj));                                       \
  assert_true(cmp_object_as_bin(&obj, &length));                              \
  assert_int_equal(length, inlength);                                         \
  assert_memory_equal(M_BufferGetDataAtCursor(&buf), val, inlength);          \
} while (0);

#define obj_to_bin_test(val, inlength) do {                                   \
  uint32_t length;                                                            \
  unsigned char data[255];                                                    \
  assert_true(cmp_object_is_bin(&obj));                                       \
  assert_true(cmp_object_as_bin(&obj, &length));                              \
  assert_int_equal(length, inlength);                                         \
  assert_true(cmp_object_to_bin(&cmp, &obj, data, 255));                      \
  assert_memory_equal(data, val, length);                                     \
} while (0);

#define obj_array_test(val1, val2) do {                                       \
  uint32_t length;                                                            \
  uint64_t var1;                                                              \
  uint64_t var2;                                                              \
  assert_true(cmp_object_is_array(&obj));                                     \
  assert_true(cmp_object_as_array(&obj, &length));                            \
  assert_int_equal(length, 2);                                                \
  assert_true(cmp_read_uinteger(&cmp, &var1));                                \
  assert_true(cmp_read_uinteger(&cmp, &var2));                                \
  assert_true(var1 == val1);                                                  \
  assert_true(var2 == val2);                                                  \
} while (0);

#define obj_map_test(inkey, invalue) do {                                     \
  uint32_t length;                                                            \
  uint64_t key;                                                               \
  uint64_t value;                                                             \
  assert_true(cmp_object_is_map(&obj));                                       \
  assert_true(cmp_object_as_map(&obj, &length));                              \
  assert_int_equal(length, 1);                                                \
  assert_true(cmp_read_uinteger(&cmp, &key));                                 \
  assert_true(cmp_read_uinteger(&cmp, &value));                               \
  assert_true(key == inkey);                                                  \
  assert_true(value == invalue);                                              \
} while (0);

#define obj_ext_test(func, as_func, type, ctype, inkey, invalue)              \
  do {                                                                        \
    uint32_t length;                                                          \
    uint32_t key;                                                             \
    uint32_t value;                                                           \
    assert_true(cmp_object_is_map(&obj));                                     \
    assert_true(cmp_object_as_map(&obj, &length));                            \
    assert_int_equal(length, 1);                                              \
    assert_true(cmp_read_uinteger(&cmp, &key));                               \
    assert_true(cmp_read_uinteger(&cmp, &value));                             \
    assert_true(key == inkey);                                                \
    assert_true(value == invalue);                                            \
  } while (0);

#define obj_test_no_read(func, type)                                          \
  assert_true(func(&obj));

#define obj_test_not(func, type)                                              \
  assert_false(func(&obj));

#define test_fixext_format(wfunc, etype, esize, in, fmt, dlen)                \
  M_BufferClear(&buf);                                                        \
  assert_true(wfunc(&cmp, etype, in));                                        \
  M_BufferSeek(&buf, 0);                                                      \
  assert_memory_equal(buf.data, fmt, dlen);                                   \
  M_BufferSeek(&buf, 0);                                                      \
  assert_true(cmp_read_object(&cmp, &obj));                                   \
  assert_true(obj.as.ext.type == etype);                                      \
  assert_true(obj.as.ext.size == esize);                                      \
  M_BufferSeek(&buf, 0);                                                      \
  do {                                                                        \
    char edata[esize];                                                        \
    int8_t dummy_type = etype;                                                \
    uint32_t dummy_size = esize;                                              \
    memset(edata, 0, sizeof(edata));                                          \
    assert_true(cmp_read_ext(&cmp, &dummy_type, &dummy_size, edata));         \
    assert_true(dummy_type == etype);                                         \
    assert_true(dummy_size == esize);                                         \
    assert_memory_equal(edata, in, esize);                                    \
  } while (0)

#define test_ext_format(wfunc, etype, esize, in, fmt, dlen)                   \
  M_BufferClear(&buf);                                                        \
  assert_true(wfunc(&cmp, etype, esize, in));                                 \
  M_BufferSeek(&buf, 0);                                                      \
  assert_memory_equal(buf.data, fmt, dlen);                                   \
  M_BufferSeek(&buf, 0);                                                      \
  assert_true(cmp_read_object(&cmp, &obj));                                   \
  assert_true(obj.as.ext.type == etype);                                      \
  assert_true(obj.as.ext.size == esize);                                      \
  M_BufferSeek(&buf, 0);                                                      \
  do {                                                                        \
    char edata[esize];                                                        \
    int8_t dummy_type = 0;                                                    \
    uint32_t dummy_size = 0;                                                  \
    assert_true(cmp_read_ext(&cmp, &dummy_type, &dummy_size, edata));         \
    assert_true(dummy_type == etype);                                         \
    assert_true(dummy_size == esize);                                         \
    assert_memory_equal(edata, in, esize);                                    \
  } while (0)

static bool buf_reader(cmp_ctx_t *ctx, void *data, size_t limit) {
  if (force_reader_to_fail) {
    return false;
  }

  buf_t *buf = (buf_t *)ctx->buf;

  return M_BufferRead(buf, data, limit);
}

static size_t buf_writer(cmp_ctx_t *ctx, const void *data, size_t sz) {
  if (force_writer_to_fail) {
    return false;
  }

  buf_t *buf = (buf_t *)ctx->buf;
  size_t pos = M_BufferGetCursor(buf);

  M_BufferWrite(buf, (void *)data, sz);

  return M_BufferGetCursor(buf) - pos;
}

static bool buf_skipper(cmp_ctx_t *ctx, size_t count) {
  if (force_skipper_to_fail) {
    return false;
  }

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

  test_int_format(
    cmp_write_ufix, cmp_read_uinteger, u8, uint64_t, 0, "\x00", 1
  );
  test_int_format(
    cmp_write_ufix, cmp_read_uinteger, u8, uint64_t, -0, "\x00", 1
  );
  test_int_format(
    cmp_write_sfix, cmp_read_uinteger, u8, uint64_t, 0, "\x00", 1
  );
  test_int_format(
    cmp_write_sfix, cmp_read_sinteger, s8, int64_t, -0, "\x00", 1
  );
  test_int_format(
    cmp_write_sfix, cmp_read_uinteger, u8, uint64_t, 127, "\x7f", 1
  );
  test_int_format(
    cmp_write_sfix, cmp_read_sinteger, s8, int64_t, -32, "\xe0", 1
  );
  test_int_format(
    cmp_write_pfix, cmp_read_uinteger, u8, uint64_t, 0, "\x00", 1
  );
  test_int_format(
    cmp_write_pfix, cmp_read_uinteger, u8, uint64_t, 1, "\x01", 1
  );
  test_int_format(
    cmp_write_pfix, cmp_read_uinteger, u8, uint64_t, 127, "\x7f", 1
  );
  test_int_format(
    cmp_write_nfix, cmp_read_sinteger, s8, int64_t, -1, "\xff", 1
  );
  test_int_format(
    cmp_write_nfix, cmp_read_sinteger, s8, int64_t, -32, "\xe0", 1
  );
}

static void test_numbers(void **state) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

  (void)state;

  setup_cmp_and_buf(&cmp, &buf);

  test_int_format(
    cmp_write_s8, cmp_read_sinteger, s8, int64_t,  0,   "\xd0\x00", 2
  );
  test_int_format(
    cmp_write_s8, cmp_read_sinteger, s8, int64_t,  1,   "\xd0\x01", 2
  );
  test_int_format(
    cmp_write_s8, cmp_read_sinteger, s8, int64_t, -1,   "\xd0\xff", 2
  );
  test_int_format(
    cmp_write_s8, cmp_read_sinteger, s8, int64_t,  127, "\xd0\x7f", 2
  );
  test_int_format(
    cmp_write_s8, cmp_read_sinteger, s8, int64_t, -128, "\xd0\x80", 2
  );

  test_int_format(
    cmp_write_s16, cmp_read_sinteger, s16, int64_t,  0,     "\xd1\x00\x00", 3
  );
  test_int_format(
    cmp_write_s16, cmp_read_sinteger, s16, int64_t,  1,     "\xd1\x00\x01", 3
  );
  test_int_format(
    cmp_write_s16, cmp_read_sinteger, s16, int64_t, -1,     "\xd1\xff\xff", 3
  );
  test_int_format(
    cmp_write_s16, cmp_read_sinteger, s16, int64_t,  127,   "\xd1\x00\x7f", 3
  );
  test_int_format(
    cmp_write_s16, cmp_read_sinteger, s16, int64_t, -128,   "\xd1\xff\x80", 3
  );
  test_int_format(
    cmp_write_s16, cmp_read_sinteger, s16, int64_t,  256,   "\xd1\x01\x00", 3
  );
  test_int_format(
    cmp_write_s16, cmp_read_sinteger, s16, int64_t,  32767, "\xd1\x7f\xff", 3
  );
  test_int_format(
    cmp_write_s16, cmp_read_sinteger, s16, int64_t, -32768, "\xd1\x80\x00", 3
  );

  test_int_format(
    cmp_write_s32, cmp_read_sinteger, s32, int64_t, 0, "\xd2\x00\x00\x00\x00", 5
  );
  test_int_format(
    cmp_write_s32, cmp_read_sinteger, s32, int64_t, 1, "\xd2\x00\x00\x00\x01", 5
  );
  test_int_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    -1,
    "\xd2\xff\xff\xff\xff",
    5
  );
  test_int_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    127,
    "\xd2\x00\x00\x00\x7f",
    5
  );
  test_int_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    -128,
    "\xd2\xff\xff\xff\x80",
    5
  );
  test_int_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    256,
    "\xd2\x00\x00\x01\x00",
    5
  );
  test_int_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    32767,
    "\xd2\x00\x00\x7f\xff",
    5
  );
  test_int_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    -32768,
    "\xd2\xff\xff\x80\x00",
    5
  );
  test_int_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    65535,
    "\xd2\x00\x00\xff\xff",
    5
  );
  test_int_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    -65536,
    "\xd2\xff\xff\x00\x00",
    5
  );
  test_int_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    8388607,
    "\xd2\x00\x7f\xff\xff",
    5
  );
  test_int_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    -8388608,
    "\xd2\xff\x80\x00\x00",
    5
  );
  test_int_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    16777215,
    "\xd2\x00\xff\xff\xff",
    5
  );
  test_int_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    -16777216,
    "\xd2\xff\x00\x00\x00",
    5
  );
  test_int_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    2147483647,
    "\xd2\x7f\xff\xff\xff",
    5
  );
  test_int_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    -2147483648,
    "\xd2\x80\x00\x00\x00",
    5
  );

  test_int_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    0,
    "\xd3\x00\x00\x00\x00\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    1,
    "\xd3\x00\x00\x00\x00\x00\x00\x00\x01",
    9
  );
  test_int_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    -1,
    "\xd3\xff\xff\xff\xff\xff\xff\xff\xff",
    9
  );
  test_int_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    127,
    "\xd3\x00\x00\x00\x00\x00\x00\x00\x7f",
    9
  );
  test_int_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    -128,
    "\xd3\xff\xff\xff\xff\xff\xff\xff\x80",
    9
  );
  test_int_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    256,
    "\xd3\x00\x00\x00\x00\x00\x00\x01\x00",
    9
  );
  test_int_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    32767,
    "\xd3\x00\x00\x00\x00\x00\x00\x7f\xff",
    9
  );
  test_int_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    -32768,
    "\xd3\xff\xff\xff\xff\xff\xff\x80\x00",
    9
  );
  test_int_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    65535,
    "\xd3\x00\x00\x00\x00\x00\x00\xff\xff",
    9
  );
  test_int_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    -65536,
    "\xd3\xff\xff\xff\xff\xff\xff\x00\x00",
    9
  );
  test_int_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    8388607,
    "\xd3\x00\x00\x00\x00\x00\x7f\xff\xff",
    9
  );
  test_int_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    -8388608,
    "\xd3\xff\xff\xff\xff\xff\x80\x00\x00",
    9
  );
  test_int_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    16777215,
    "\xd3\x00\x00\x00\x00\x00\xff\xff\xff",
    9
  );
  test_int_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    -16777216,
    "\xd3\xff\xff\xff\xff\xff\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    2147483647,
    "\xd3\x00\x00\x00\x00\x7f\xff\xff\xff",
    9
  );
  test_int_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    -2147483648,
    "\xd3\xff\xff\xff\xff\x80\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    4294967295,
    "\xd3\x00\x00\x00\x00\xff\xff\xff\xff",
    9
  );
  test_int_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    -4294967296,
    "\xd3\xff\xff\xff\xff\x00\x00\x00\x00",
    9
  );

  test_int_format(
    cmp_write_sint, cmp_read_uinteger, u8, uint64_t, 0, "\x00", 1
  );
  test_int_format(
    cmp_write_sint, cmp_read_uinteger, u8, uint64_t, 1, "\x01", 1
  );
  test_int_format(
    cmp_write_sint, cmp_read_uinteger, u8, uint64_t, 127, "\x7f", 1
  );
  test_int_format(
    cmp_write_sint, cmp_read_uinteger, u8, uint64_t, 128, "\xcc\x80", 2
  );
  test_int_format(
    cmp_write_sint, cmp_read_uinteger, u8, uint64_t, 255, "\xcc\xff", 2
  );
  test_int_format(
    cmp_write_sint, cmp_read_uinteger, u16, uint64_t, 256, "\xcd\x01\x00", 3
  );
  test_int_format(
    cmp_write_sint, cmp_read_uinteger, u16, uint64_t, 32767, "\xcd\x7f\xff", 3
  );
  test_int_format(
    cmp_write_sint, cmp_read_uinteger, u16, uint64_t, 32768, "\xcd\x80\x00", 3
  );
  test_int_format(
    cmp_write_sint, cmp_read_uinteger, u16, uint64_t, 65535, "\xcd\xff\xff", 3
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    65536,
    "\xce\x00\x01\x00\x00",
    5
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    8388607,
    "\xce\x00\x7f\xff\xff",
    5
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    8388608,
    "\xce\x00\x80\x00\x00",
    5
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    16777215,
    "\xce\x00\xff\xff\xff",
    5
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    16777216,
    "\xce\x01\x00\x00\x00",
    5
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    2147483647,
    "\xce\x7f\xff\xff\xff",
    5
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    4294967296,
    "\xcf\x00\x00\x00\x01\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    549755813887,
    "\xcf\x00\x00\x00\x7f\xff\xff\xff\xff",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    549755813888,
    "\xcf\x00\x00\x00\x80\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    1099511627775,
    "\xcf\x00\x00\x00\xff\xff\xff\xff\xff",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    1099511627776,
    "\xcf\x00\x00\x01\x00\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    140737488355327,
    "\xcf\x00\x00\x7f\xff\xff\xff\xff\xff",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    140737488355328,
    "\xcf\x00\x00\x80\x00\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    281474976710655,
    "\xcf\x00\x00\xff\xff\xff\xff\xff\xff",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    281474976710656,
    "\xcf\x00\x01\x00\x00\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    36028797018963967,
    "\xcf\x00\x7f\xff\xff\xff\xff\xff\xff",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    36028797018963968,
    "\xcf\x00\x80\x00\x00\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    72057594037927935,
    "\xcf\x00\xff\xff\xff\xff\xff\xff\xff",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    72057594037927936,
    "\xcf\x01\x00\x00\x00\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    9223372036854775807,
    "\xcf\x7f\xff\xff\xff\xff\xff\xff\xff",
    9
  );

  test_int_format(
    cmp_write_sint, cmp_read_sinteger, s8, int64_t, -1, "\xff", 1
  );
  test_int_format(
    cmp_write_sint, cmp_read_sinteger, s8, int64_t, -32, "\xe0", 1
  );
  test_int_format(
    cmp_write_sint, cmp_read_sinteger, s8, int64_t, -127, "\xd0\x81", 2
  );
  test_int_format(
    cmp_write_sint, cmp_read_sinteger, s8, int64_t, -128, "\xd0\x80", 2
  );
  test_int_format(
    cmp_write_sint, cmp_read_sinteger, s16, int64_t, -255, "\xd1\xff\x01", 3
  );
  test_int_format(
    cmp_write_sint, cmp_read_sinteger, s16, int64_t, -256, "\xd1\xff\x00", 3
  );
  test_int_format(
    cmp_write_sint, cmp_read_sinteger, s16, int64_t, -32767, "\xd1\x80\x01", 3
  );
  test_int_format(
    cmp_write_sint, cmp_read_sinteger, s16, int64_t, -32768, "\xd1\x80\x00", 3
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s32,
    int64_t,
    -65535,
    "\xd2\xff\xff\x00\x01",
    5
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s32,
    int64_t,
    -65536,
    "\xd2\xff\xff\x00\x00",
    5
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s32,
    int64_t,
    -8388607,
    "\xd2\xff\x80\x00\x01",
    5
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s32,
    int64_t,
    -8388608,
    "\xd2\xff\x80\x00\x00",
    5
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s32,
    int64_t,
    -16777215,
    "\xd2\xff\x00\x00\x01",
    5
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s32,
    int64_t,
    -16777216,
    "\xd2\xff\x00\x00\x00",
    5
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s32,
    int64_t,
    -2147483647,
    "\xd2\x80\x00\x00\x01",
    5
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s32,
    int64_t,
    -2147483648,
    "\xd2\x80\x00\x00\x00",
    5
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -4294967295,
    "\xd3\xff\xff\xff\xff\x00\x00\x00\x01",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -4294967296,
    "\xd3\xff\xff\xff\xff\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -549755813887,
    "\xd3\xff\xff\xff\x80\x00\x00\x00\x01",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -549755813888,
    "\xd3\xff\xff\xff\x80\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -1099511627775,
    "\xd3\xff\xff\xff\x00\x00\x00\x00\x01",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -1099511627776,
    "\xd3\xff\xff\xff\x00\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -140737488355327,
    "\xd3\xff\xff\x80\x00\x00\x00\x00\x01",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -140737488355328,
    "\xd3\xff\xff\x80\x00\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -281474976710655,
    "\xd3\xff\xff\x00\x00\x00\x00\x00\x01",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -281474976710656,
    "\xd3\xff\xff\x00\x00\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -36028797018963967,
    "\xd3\xff\x80\x00\x00\x00\x00\x00\x01",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -36028797018963968,
    "\xd3\xff\x80\x00\x00\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -72057594037927935,
    "\xd3\xff\x00\x00\x00\x00\x00\x00\x01",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -72057594037927936,
    "\xd3\xff\x00\x00\x00\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -9223372036854775807,
    "\xd3\x80\x00\x00\x00\x00\x00\x00\x01",
    9
  );

  test_int_format(cmp_write_u8,
    cmp_read_uinteger,
    u8,
    uint64_t,
    0,
    "\xcc\x00",
    1);
  test_int_format(cmp_write_u8,
    cmp_read_uinteger,
    u8,
    uint64_t,
    1,
    "\xcc\x01",
    1);
  test_int_format(
    cmp_write_u8,
    cmp_read_uinteger,
    u8,
    uint64_t,
    127,
    "\xcc\x7f",
    1
  );
  test_int_format(
    cmp_write_u8,
    cmp_read_uinteger,
    u8,
    uint64_t,
    255,
    "\xcc\xff",
    1
  );

  test_int_format(
    cmp_write_u16, cmp_read_uinteger, u16, uint64_t, 0, "\xcd\x00\x00", 2
  );
  test_int_format(
    cmp_write_u16, cmp_read_uinteger, u16, uint64_t, 1, "\xcd\x00\x01", 2
  );
  test_int_format(
    cmp_write_u16, cmp_read_uinteger, u16, uint64_t, 127, "\xcd\x00\x7f", 2
  );
  test_int_format(
    cmp_write_u16, cmp_read_uinteger, u16, uint64_t, 256, "\xcd\x01\x00", 2
  );
  test_int_format(
    cmp_write_u16, cmp_read_uinteger, u16, uint64_t, 32767, "\xcd\x7f\xff", 2
  );
  test_int_format(
    cmp_write_u16, cmp_read_uinteger, u16, uint64_t, 65535, "\xcd\xff\xff", 2
  );

  test_int_format(
    cmp_write_u32,
    cmp_read_uinteger,
    u32,
    uint64_t,
    0,
    "\xce\x00\x00\x00\x00",
    5
  );
  test_int_format(
    cmp_write_u32,
    cmp_read_uinteger,
    u32,
    uint64_t,
    1,
    "\xce\x00\x00\x00\x01",
    5
  );
  test_int_format(
    cmp_write_u32,
    cmp_read_uinteger,
    u32,
    uint64_t,
    127,
    "\xce\x00\x00\x00\x7f",
    5
  );
  test_int_format(
    cmp_write_u32,
    cmp_read_uinteger,
    u32,
    uint64_t,
    256,
    "\xce\x00\x00\x01\x00",
    5
  );
  test_int_format(
    cmp_write_u32,
    cmp_read_uinteger,
    u32,
    uint64_t,
    32767,
    "\xce\x00\x00\x7f\xff",
    5
  );
  test_int_format(
    cmp_write_u32,
    cmp_read_uinteger,
    u32,
    uint64_t,
    65535,
    "\xce\x00\x00\xff\xff",
    5
  );
  test_int_format(
    cmp_write_u32,
    cmp_read_uinteger,
    u32,
    uint64_t,
    8388607,
    "\xce\x00\x7f\xff\xff",
    5
  );
  test_int_format(
    cmp_write_u32,
    cmp_read_uinteger,
    u32,
    uint64_t,
    16777215,
    "\xce\x00\xff\xff\xff",
    5
  );
  test_int_format(
    cmp_write_u32,
    cmp_read_uinteger,
    u32,
    uint64_t,
    2147483647,
    "\xce\x7f\xff\xff\xff",
    5
  );
  test_int_format(
    cmp_write_u32,
    cmp_read_uinteger,
    u32,
    uint64_t,
    4294967295,
    "\xce\xff\xff\xff\xff",
    5
  );

  test_int_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    0,
    "\xcf\x00\x00\x00\x00\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    1,
    "\xcf\x00\x00\x00\x00\x00\x00\x00\x01",
    9
  );
  test_int_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    127,
    "\xcf\x00\x00\x00\x00\x00\x00\x00\x7f",
    9
  );
  test_int_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    256,
    "\xcf\x00\x00\x00\x00\x00\x00\x01\x00",
    9
  );
  test_int_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    32767,
    "\xcf\x00\x00\x00\x00\x00\x00\x7f\xff",
    9
  );
  test_int_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    65535,
    "\xcf\x00\x00\x00\x00\x00\x00\xff\xff",
    9
  );
  test_int_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    8388607,
    "\xcf\x00\x00\x00\x00\x00\x7f\xff\xff",
    9
  );
  test_int_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    16777215,
    "\xcf\x00\x00\x00\x00\x00\xff\xff\xff",
    9
  );
  test_int_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    2147483647,
    "\xcf\x00\x00\x00\x00\x7f\xff\xff\xff",
    9
  );
  test_int_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    4294967295,
    "\xcf\x00\x00\x00\x00\xff\xff\xff\xff",
    9
  );
  test_int_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    0xFFFFFFFFFFFFFFFE,
    "\xcf\xff\xff\xff\xff\xff\xff\xff\xfe",
    9
  );
  test_int_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    0xFFFFFFFFFFFFFFFF,
    "\xcf\xff\xff\xff\xff\xff\xff\xff\xff",
    9
  );

  test_int_format(
    cmp_write_uint, cmp_read_uinteger, u8, uint64_t, 0, "\x00", 1
  );
  test_int_format(
    cmp_write_uint, cmp_read_uinteger, u8, uint64_t, 1, "\x01", 1
  );
  test_int_format(
    cmp_write_uint, cmp_read_uinteger, u8, uint64_t, 127, "\x7f", 1
  );
  test_int_format(
    cmp_write_uint, cmp_read_uinteger, u8, uint64_t, 128, "\xcc\x80", 2
  );
  test_int_format(
    cmp_write_uint, cmp_read_uinteger, u8, uint64_t, 255, "\xcc\xff", 2
  );
  test_int_format(
    cmp_write_uint, cmp_read_uinteger, u16, uint64_t, 256, "\xcd\x01\x00", 3
  );
  test_int_format(
    cmp_write_uint, cmp_read_uinteger, u16, uint64_t, 32767, "\xcd\x7f\xff", 3
  );
  test_int_format(
    cmp_write_uint, cmp_read_uinteger, u16, uint64_t, 32768, "\xcd\x80\x00", 3
  );
  test_int_format(
    cmp_write_uint, cmp_read_uinteger, u16, uint64_t, 65535, "\xcd\xff\xff", 3
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    65536,
    "\xce\x00\x01\x00\x00",
    5
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    8388607,
    "\xce\x00\x7f\xff\xff",
    5
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    8388608,
    "\xce\x00\x80\x00\x00",
    5
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    16777215,
    "\xce\x00\xff\xff\xff",
    5
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    16777216,
    "\xce\x01\x00\x00\x00",
    5
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    2147483647,
    "\xce\x7f\xff\xff\xff",
    5
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    2147483648,
    "\xce\x80\x00\x00\x00",
    5
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    4294967295,
    "\xce\xff\xff\xff\xff",
    5
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    4294967296,
    "\xcf\x00\x00\x00\x01\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    549755813887,
    "\xcf\x00\x00\x00\x7f\xff\xff\xff\xff",
    9
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    549755813888,
    "\xcf\x00\x00\x00\x80\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    1099511627775,
    "\xcf\x00\x00\x00\xff\xff\xff\xff\xff",
    9
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    1099511627776,
    "\xcf\x00\x00\x01\x00\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    140737488355327,
    "\xcf\x00\x00\x7f\xff\xff\xff\xff\xff",
    9
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    140737488355328,
    "\xcf\x00\x00\x80\x00\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    281474976710655,
    "\xcf\x00\x00\xff\xff\xff\xff\xff\xff",
    9
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    281474976710656,
    "\xcf\x00\x01\x00\x00\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    36028797018963967,
    "\xcf\x00\x7f\xff\xff\xff\xff\xff\xff",
    9
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    36028797018963968,
    "\xcf\x00\x80\x00\x00\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    72057594037927935,
    "\xcf\x00\xff\xff\xff\xff\xff\xff\xff",
    9
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    72057594037927936,
    "\xcf\x01\x00\x00\x00\x00\x00\x00\x00",
    9
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    9223372036854775807,
    "\xcf\x7f\xff\xff\xff\xff\xff\xff\xff",
    9
  );
  test_int_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    0xFFFFFFFFFFFFFFFF,
    "\xcf\xff\xff\xff\xff\xff\xff\xff\xff",
    9
  );

  test_float_format(
    cmp_write_float,
    cmp_read_float,
    flt,
    float,
    0.0f,
    "\xca\x00\x00\x00\x00",
    5
  );
  test_float_format(
    cmp_write_float,
    cmp_read_float,
    flt,
    float,
    -0.0f,
    "\xca\x80\x00\x00\x00",
    5
  );
  test_float_format(
    cmp_write_float,
    cmp_read_float,
    flt,
    float,
    1.0f,
    "\xca\x3f\x80\x00\x00",
    5
  );
  test_float_format(
    cmp_write_float,
    cmp_read_float,
    flt,
    float,
    -1.0f,
    "\xca\xbf\x80\x00\x00",
    5
  );
  test_float_format(
    cmp_write_float,
    cmp_read_float,
    flt,
    float,
    65535.0f,
    "\xca\x47\x7f\xff\x00",
    5
  );
  test_float_format(
    cmp_write_float,
    cmp_read_float,
    flt,
    float,
    -65535.0f,
    "\xca\xc7\x7f\xff\x00",
    5
  );
  test_float_format(
    cmp_write_float,
    cmp_read_float,
    flt,
    float,
    32767.0f,
    "\xca\x46\xff\xfe\x00",
    5
  );
  test_float_format(
    cmp_write_float,
    cmp_read_float,
    flt,
    float,
    -32767.0f,
    "\xca\xc6\xff\xfe\x00",
    5
  );

  test_double_format(
    cmp_write_double,
    cmp_read_double,
    dbl,
    double,
    0.0,
    "\xcb\x00\x00\x00\x00\x00\x00\x00\x00",
    9
  );
  test_double_format(
    cmp_write_double,
    cmp_read_double,
    dbl,
    double,
    -0.0,
    "\xcb\x80\x00\x00\x00\x00\x00\x00\x00",
    9
  );
  test_double_format(
    cmp_write_double,
    cmp_read_double,
    dbl,
    double,
    1.0,
    "\xcb\x3f\xf0\x00\x00\x00\x00\x00\x00",
    9
  );
  test_double_format(
    cmp_write_double,
    cmp_read_double,
    dbl,
    double,
    -1.0,
    "\xcb\xbf\xf0\x00\x00\x00\x00\x00\x00",
    9
  );
  test_double_format(
    cmp_write_double,
    cmp_read_double,
    dbl,
    double,
    2147483647.0,
    "\xcb\x41\xdf\xff\xff\xff\xc0\x00\x00",
    9
  );
  test_double_format(
    cmp_write_double,
    cmp_read_double,
    dbl,
    double,
    -2147483647.0,
    "\xcb\xc1\xdf\xff\xff\xff\xc0\x00\x00",
    9
  );
  test_double_format(
    cmp_write_double,
    cmp_read_double,
    dbl,
    double,
    4294967295.0,
    "\xcb\x41\xef\xff\xff\xff\xe0\x00\x00",
    9
  );
  test_double_format(
    cmp_write_double,
    cmp_read_double,
    dbl,
    double,
    -4294967295.0,
    "\xcb\xc1\xef\xff\xff\xff\xe0\x00\x00",
    9
  );

  test_double_format(
    cmp_write_decimal,
    cmp_read_decimal,
    flt,
    double,
    2.0f,
    "\xca\x40\x00\x00\x00",
    5
  );

  test_double_format(
    cmp_write_decimal,
    cmp_read_decimal,
    dbl,
    double,
    1111111111111111.125000,
    "\xcb\x43\x0f\x94\x65\xb8\xab\x8e\x39",
    9
  );
}

static void test_nil(void **state) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

  (void)state;

  setup_cmp_and_buf(&cmp, &buf);

  test_format_no_input(cmp_write_nil, u8, "\xc0", 1, 0);
}

static void test_boolean(void **state) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

  (void)state;

  setup_cmp_and_buf(&cmp, &buf);

  test_format_no_input(cmp_write_false, boolean, "\xc2", 1, false);
  test_format_no_input(cmp_write_true,  boolean, "\xc3", 1, true);
  test_format(cmp_write_bool, cmp_read_bool, boolean, bool, false, "\xc2", 1);
  test_format(cmp_write_bool, cmp_read_bool, boolean, bool, true, "\xc3", 1);
  test_format(
    cmp_write_u8_as_bool, cmp_read_bool_as_u8, boolean, uint8_t, 0, "\xc2", 1
  );
  test_format(
    cmp_write_u8_as_bool, cmp_read_bool_as_u8, boolean, uint8_t, 1, "\xc3", 1
  );
}

static void test_bin(void **state) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

  (void)state;

  setup_cmp_and_buf(&cmp, &buf);

  test_format_with_length(
    cmp_write_bin8, cmp_read_bin, bin_size, "", 0, "\xc4\x00", 2
  );
  test_format_with_length(
    cmp_write_bin8,
    cmp_read_bin,
    bin_size,
    "Hey there\n",
    10,
    "\xc4\x0aHey there\n",
    12
  );
  test_format_with_length(
    cmp_write_bin16, cmp_read_bin, bin_size, "", 0, "\xc5\x00\x00", 3
  );
  test_format_with_length(
    cmp_write_bin16,
    cmp_read_bin,
    bin_size,
    "Hey there\n",
    10,
    "\xc5\x00\x0aHey there\n",
    13
  );
  test_format_with_length(
    cmp_write_bin32, cmp_read_bin, bin_size, "", 0, "\xc6\x00\x00\x00\x00", 5
  );
  test_format_with_length(
    cmp_write_bin32,
    cmp_read_bin,
    bin_size,
    "Hey there\n",
    10,
    "\xc6\x00\x00\x00\x0aHey there\n",
    15
  );
  test_format_with_length(
    cmp_write_bin, cmp_read_bin, bin_size, "", 0, "\xc4\x00", 2
  );
  test_format_with_length(
    cmp_write_bin,
    cmp_read_bin,
    bin_size,
    "Hey there\n",
    10,
    "\xc4\x0aHey there\n",
    12
  );
}

static void test_string(void **state) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

  (void)state;

  setup_cmp_and_buf(&cmp, &buf);

  test_format_with_length(
    cmp_write_fixstr, cmp_read_str, str_size, "", 0, "\xa0", 1
  );
  test_format_with_length(
    cmp_write_fixstr,
    cmp_read_str,
    str_size,
    "Hey there\n",
    10,
    "\xaaHey there\n",
    11
  );
  test_format_with_length(
    cmp_write_str8, cmp_read_str, str_size, "", 0, "\xd9\x00", 2
  );
  test_format_with_length(
    cmp_write_str8,
    cmp_read_str,
    str_size,
    "Hey there\n",
    10,
    "\xd9\x0aHey there\n",
    12
  );
  test_format_with_length(
    cmp_write_str16,
    cmp_read_str,
    str_size,
    "",
    0,
    "\xda\x00\x00",
    3
  );
  test_format_with_length(
    cmp_write_str16,
    cmp_read_str,
    str_size,
    "Hey there\n",
    10,
    "\xda\x00\x0aHey there\n",
    13
  );
  test_format_with_length(
    cmp_write_str32,
    cmp_read_str,
    str_size,
    "",
    0,
    "\xdb\x00\x00\x00\x00",
    5
  );
  test_format_with_length(
    cmp_write_str32,
    cmp_read_str,
    str_size,
    "Hey there\n",
    10,
    "\xdb\x00\x00\x00\x0aHey there\n",
    15
  );
  test_format_with_length(
    cmp_write_str, cmp_read_str, str_size, "", 0, "\xa0", 1
  );
  test_format_with_length(
    cmp_write_str,
    cmp_read_str,
    str_size,
    "Hey there\n",
    10,
    "\xaaHey there\n",
    11
  );
  test_format_with_length(
    cmp_write_str_v4,
    cmp_read_str,
    str_size,
    "With your feet on the air and your head on the ground\n",
    54,
    "\xda\x00\x36With your feet on the air and your head on the ground\n",
    57
  );
}

static void test_array(void **state) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

  (void)state;

  setup_cmp_and_buf(&cmp, &buf);

  test_format(
    cmp_write_fixarray, cmp_read_array, array_size, uint32_t, 0, "\x90", 1
  );
  test_format(
    cmp_write_fixarray, cmp_read_array, array_size, uint32_t, 10, "\x9a", 1
  );
  test_format(
    cmp_write_array16,
    cmp_read_array,
    array_size,
    uint32_t,
    0,
    "\xdc\x00\x00",
    3
  );
  test_format(
    cmp_write_array16,
    cmp_read_array,
    array_size,
    uint32_t,
    10,
    "\xdc\x00\x0a",
    3
  );
  test_format(
    cmp_write_array32,
    cmp_read_array,
    array_size,
    uint32_t,
    0,
    "\xdd\x00\x00\x00\x00",
    5
  );
  test_format(
    cmp_write_array32,
    cmp_read_array,
    array_size,
    uint32_t,
    10,
    "\xdd\x00\x00\x00\x0a",
    5
  );
  test_format(
    cmp_write_array, cmp_read_array, array_size, uint32_t, 0, "\x90", 1
  );
  test_format(
    cmp_write_array, cmp_read_array, array_size, uint32_t, 10, "\x9a", 1
  );
}

static void test_map(void **state) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

  (void)state;

  setup_cmp_and_buf(&cmp, &buf);

  test_format(
    cmp_write_fixmap, cmp_read_map, map_size, uint32_t, 0, "\x80", 1
  );
  test_format(
    cmp_write_fixmap, cmp_read_map, map_size, uint32_t, 10, "\x8a", 1
  );
  test_format(
    cmp_write_map16, cmp_read_map, map_size, uint32_t, 0, "\xde\x00\x00", 3
  );
  test_format(
    cmp_write_map16, cmp_read_map, map_size, uint32_t, 10, "\xde\x00\x0a", 3
  );
  test_format(
    cmp_write_map32,
    cmp_read_map,
    map_size,
    uint32_t,
    0,
    "\xdf\x00\x00\x00\x00",
    5
  );
  test_format(
    cmp_write_map32,
    cmp_read_map,
    map_size,
    uint32_t,
    10,
    "\xdf\x00\x00\x00\x0a",
    5
  );
  test_format(
    cmp_write_map, cmp_read_map, map_size, uint32_t, 0, "\x80", 1
  );
  test_format(
    cmp_write_map, cmp_read_map, map_size, uint32_t, 10, "\x8a", 1
  );

  M_BufferSeek(&buf, 0);

  assert_true(cmp_write_map(&cmp, 3));
  assert_true(cmp_write_str(&cmp, "a", 1));
  assert_true(cmp_write_str(&cmp, "apple", 5));
  assert_true(cmp_write_str(&cmp, "b", 1));
  assert_true(cmp_write_str(&cmp, "banana", 6));
  assert_true(cmp_write_str(&cmp, "c", 1));
  assert_true(cmp_write_str(&cmp, "coconut", 7));

  M_BufferSeek(&buf, 0);

  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_FIXMAP);
  assert_int_equal(obj.as.map_size, 3);

  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_FIXSTR);
  assert_int_equal(obj.as.str_size, 1);
  assert_memory_equal(M_BufferGetDataAtCursor(&buf), "a", 1);
  M_BufferSeekForward(&buf, 1);

  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_FIXSTR);
  assert_int_equal(obj.as.str_size, 5);
  assert_memory_equal(M_BufferGetDataAtCursor(&buf), "apple", 5);
  M_BufferSeekForward(&buf, 5);

  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_FIXSTR);
  assert_int_equal(obj.as.str_size, 1);
  assert_memory_equal(M_BufferGetDataAtCursor(&buf), "b", 1);
  M_BufferSeekForward(&buf, 1);

  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_FIXSTR);
  assert_int_equal(obj.as.str_size, 6);
  assert_memory_equal(M_BufferGetDataAtCursor(&buf), "banana", 6);
  M_BufferSeekForward(&buf, 6);

  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_FIXSTR);
  assert_int_equal(obj.as.str_size, 1);
  assert_memory_equal(M_BufferGetDataAtCursor(&buf), "c", 1);
  M_BufferSeekForward(&buf, 1);

  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_FIXSTR);
  assert_int_equal(obj.as.str_size, 7);
  assert_memory_equal(M_BufferGetDataAtCursor(&buf), "coconut", 7);
  M_BufferSeekForward(&buf, 7);
}

static void test_ext(void **state) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;
  char *buf16 = malloc(0x7FFF);
  char *buf32 = malloc(0x10000);

  memset(buf16, 'C', 0x7FFF);
  memset(buf32, 'C', 0x10000);

  (void)state;

  setup_cmp_and_buf(&cmp, &buf);

  test_fixext_format(cmp_write_fixext1, 1, 1, "C", "\xd4\x01\x43", 3);
  test_fixext_format(cmp_write_fixext2, 2, 2, "CC", "\xd5\x02\x43\x43", 4);
  test_fixext_format(
    cmp_write_fixext4, 3, 4, "CCCC", "\xd6\x03\x43\x43\x43\x43", 6
  );
  test_fixext_format(
    cmp_write_fixext8,
    4,
    8,
    "CCCCCCCC",
    "\xd7\x04\x43\x43\x43\x43\x43\x43\x43\x43",
    10
  );
  test_fixext_format(
    cmp_write_fixext16,
    5,
    16,
    "CCCCCCCCCCCCCCCC",
    "\xd8\x05\x43\x43\x43\x43\x43\x43\x43\x43\x43\x43\x43\x43\x43\x43\x43\x43",
    18
  );
  test_ext_format(cmp_write_ext8, 1, 1, "C", "\xc7\x01\x01\x43", 4);
  test_ext_format(
    cmp_write_ext8, 2, 3, "CCC", "\xc7\x03\x02\x43\x43\x43", 6
  );
  test_ext_format(cmp_write_ext16, 1, 1, "C", "\xc8\x00\x01\x01\x43", 5);
  test_ext_format(
    cmp_write_ext16, 2, 3, "CCC", "\xc8\x00\x03\x02\x43\x43\x43", 7
  );
  test_ext_format(
    cmp_write_ext32, 1, 1, "C", "\xc9\x00\x00\x00\x01\x01\x43", 7
  );
  test_ext_format(
    cmp_write_ext32, 2, 3, "CCC", "\xc9\x00\x00\x00\x03\x02\x43\x43\x43", 9
  );
  test_ext_format(cmp_write_ext, 1, 1, "C", "\xd4\x01\x43", 3);
  test_ext_format(
    cmp_write_ext, 2, 3, "CCC", "\xc7\x03\x02\x43\x43\x43", 6
  );

  M_BufferSeek(&buf, 0);

  assert_true(cmp_write_ext(&cmp, 2, 1, "C"));
  assert_true(cmp_write_ext(&cmp, 3, 2, "CC"));
  assert_true(cmp_write_ext(&cmp, 4, 4, "CCCC"));
  assert_true(cmp_write_ext(&cmp, 5, 8, "CCCCCCCC"));
  assert_true(cmp_write_ext(&cmp, 6, 16, "CCCCCCCCCCCCCCCC"));
  assert_true(cmp_write_ext(&cmp, 7, 0x7FFF, buf16));
  assert_true(cmp_write_ext(&cmp, 8, 0x10000, buf32));

  M_BufferSeek(&buf, 0);

  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_FIXEXT1);
  assert_int_equal(obj.as.ext.type, 2);
  assert_int_equal(obj.as.ext.size, 1);
  assert_memory_equal(M_BufferGetDataAtCursor(&buf), "C", 1);
  M_BufferSeekForward(&buf, 1);

  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_FIXEXT2);
  assert_int_equal(obj.as.ext.type, 3);
  assert_int_equal(obj.as.ext.size, 2);
  assert_memory_equal(M_BufferGetDataAtCursor(&buf), "CC", 2);
  M_BufferSeekForward(&buf, 2);

  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_FIXEXT4);
  assert_int_equal(obj.as.ext.type, 4);
  assert_int_equal(obj.as.ext.size, 4);
  assert_memory_equal(M_BufferGetDataAtCursor(&buf), "CCCC", 4);
  M_BufferSeekForward(&buf, 4);

  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_FIXEXT8);
  assert_int_equal(obj.as.ext.type, 5);
  assert_int_equal(obj.as.ext.size, 8);
  assert_memory_equal(M_BufferGetDataAtCursor(&buf), "CCCCCCCC", 8);
  M_BufferSeekForward(&buf, 8);

  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_FIXEXT16);
  assert_int_equal(obj.as.ext.type, 6);
  assert_int_equal(obj.as.ext.size, 16);
  assert_memory_equal(M_BufferGetDataAtCursor(&buf), "CCCCCCCCCCCCCCCC", 16);
  M_BufferSeekForward(&buf, 16);

  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_EXT16);
  assert_int_equal(obj.as.ext.type, 7);
  assert_int_equal(obj.as.ext.size, 0x7FFF);
  assert_memory_equal(M_BufferGetDataAtCursor(&buf), buf16, 0x7FFF);
  M_BufferSeekForward(&buf, 0x7FFF);

  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_EXT32);
  assert_int_equal(obj.as.ext.type, 8);
  assert_int_equal(obj.as.ext.size, 0x10000);
  assert_memory_equal(M_BufferGetDataAtCursor(&buf), buf32, 0x10000);
  M_BufferSeekForward(&buf, 0x100000);

  free(buf16);
  free(buf32);
}

static void test_obj(void **state) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

  (void)state;

  setup_cmp_and_buf(&cmp, &buf);

  obj_write(cmp_write_sint, -1);
  obj_test(cmp_object_is_char, cmp_object_as_char, "char", int8_t, -1);
  obj_test(cmp_object_is_short, cmp_object_as_short, "short", int16_t, -1);
  obj_test(cmp_object_is_int, cmp_object_as_int, "int", int32_t, -1);
  obj_test(cmp_object_is_long, cmp_object_as_long, "long", int64_t, -1);
  obj_test(
    cmp_object_is_sinteger, cmp_object_as_sinteger, "sinteger", int64_t, -1
  );
  obj_test_not(cmp_object_is_uchar, "uchar");
  obj_test_not(cmp_object_is_ushort, "ushort");
  obj_test_not(cmp_object_is_uint, "uint");
  obj_test_not(cmp_object_is_ulong, "ulong");
  obj_test_not(cmp_object_is_uinteger, "uinteger");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_bool, "bool");
  obj_test_not(cmp_object_is_str, "str");
  obj_test_not(cmp_object_is_bin, "bin");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");

  obj_write(cmp_write_sint, -129);
  obj_test(cmp_object_is_short, cmp_object_as_short, "short", int16_t, -129);
  obj_test(cmp_object_is_int, cmp_object_as_int, "int", int32_t, -129);
  obj_test(cmp_object_is_long, cmp_object_as_long, "long", int64_t, -129);
  obj_test(
    cmp_object_is_sinteger, cmp_object_as_sinteger, "sinteger", int64_t, -129
  );
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_uchar, "uchar");
  obj_test_not(cmp_object_is_ushort, "ushort");
  obj_test_not(cmp_object_is_uint, "uint");
  obj_test_not(cmp_object_is_ulong, "ulong");
  obj_test_not(cmp_object_is_uinteger, "uinteger");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_bool, "bool");
  obj_test_not(cmp_object_is_str, "str");
  obj_test_not(cmp_object_is_bin, "bin");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");

  obj_write(cmp_write_sint, -32769);
  obj_test(cmp_object_is_int, cmp_object_as_int, "int", int32_t, -32769);
  obj_test(cmp_object_is_long, cmp_object_as_long, "long", int64_t, -32769);
  obj_test(
    cmp_object_is_sinteger, cmp_object_as_sinteger, "sinteger", int64_t, -32769
  );
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_short, "short");
  obj_test_not(cmp_object_is_uchar, "uchar");
  obj_test_not(cmp_object_is_ushort, "ushort");
  obj_test_not(cmp_object_is_uint, "uint");
  obj_test_not(cmp_object_is_ulong, "ulong");
  obj_test_not(cmp_object_is_uinteger, "uinteger");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_bool, "bool");
  obj_test_not(cmp_object_is_str, "str");
  obj_test_not(cmp_object_is_bin, "bin");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");

  obj_write(cmp_write_sint, -2147483649);
  obj_test(
    cmp_object_is_long, cmp_object_as_long, "long", int64_t, -2147483649
  );
  obj_test(
    cmp_object_is_sinteger,
    cmp_object_as_sinteger,
    "sinteger",
    int64_t, -2147483649
  );
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_short, "short");
  obj_test_not(cmp_object_is_int, "int");
  obj_test_not(cmp_object_is_uchar, "uchar");
  obj_test_not(cmp_object_is_ushort, "ushort");
  obj_test_not(cmp_object_is_uint, "uint");
  obj_test_not(cmp_object_is_ulong, "ulong");
  obj_test_not(cmp_object_is_uinteger, "uinteger");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_bool, "bool");
  obj_test_not(cmp_object_is_str, "str");
  obj_test_not(cmp_object_is_bin, "bin");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");

  obj_write(cmp_write_uint, 1);
  obj_test(cmp_object_is_uchar, cmp_object_as_uchar, "uchar", uint8_t, 1);
  obj_test(cmp_object_is_ushort, cmp_object_as_ushort, "ushort", uint16_t, 1);
  obj_test(cmp_object_is_uint, cmp_object_as_uint, "uint", uint32_t, 1);
  obj_test(cmp_object_is_ulong, cmp_object_as_ulong, "ulong", uint64_t, 1);
  obj_test(
    cmp_object_is_uinteger, cmp_object_as_uinteger, "uinteger", uint64_t, 1
  );
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_short, "short");
  obj_test_not(cmp_object_is_int, "int");
  obj_test_not(cmp_object_is_long, "long");
  obj_test_not(cmp_object_is_sinteger, "sinteger");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_bool, "bool");
  obj_test_not(cmp_object_is_str, "str");
  obj_test_not(cmp_object_is_bin, "bin");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");

  obj_write(cmp_write_uint, 255);
  obj_test(cmp_object_is_uchar, cmp_object_as_uchar, "uchar", uint8_t, 255);
  obj_test(
    cmp_object_is_ushort, cmp_object_as_ushort, "ushort", uint16_t, 255
  );
  obj_test(cmp_object_is_uint, cmp_object_as_uint, "uint", uint32_t, 255);
  obj_test(cmp_object_is_ulong, cmp_object_as_ulong, "ulong", uint64_t, 255);
  obj_test(
    cmp_object_is_uinteger, cmp_object_as_uinteger, "uinteger", uint64_t, 255
  );
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_short, "short");
  obj_test_not(cmp_object_is_int, "int");
  obj_test_not(cmp_object_is_long, "long");
  obj_test_not(cmp_object_is_sinteger, "sinteger");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_bool, "bool");
  obj_test_not(cmp_object_is_str, "str");
  obj_test_not(cmp_object_is_bin, "bin");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");

  obj_write(cmp_write_uint, 256);
  obj_test(
    cmp_object_is_ushort, cmp_object_as_ushort, "ushort", uint16_t, 256
  );
  obj_test(cmp_object_is_uint, cmp_object_as_uint, "uint", uint32_t, 256);
  obj_test(cmp_object_is_ulong, cmp_object_as_ulong, "ulong", uint64_t, 256);
  obj_test(
    cmp_object_is_uinteger, cmp_object_as_uinteger, "uinteger", uint64_t, 256
  );
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_short, "short");
  obj_test_not(cmp_object_is_int, "int");
  obj_test_not(cmp_object_is_long, "long");
  obj_test_not(cmp_object_is_sinteger, "sinteger");
  obj_test_not(cmp_object_is_uchar, "uchar");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_bool, "bool");
  obj_test_not(cmp_object_is_str, "str");
  obj_test_not(cmp_object_is_bin, "bin");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");

  obj_write(cmp_write_uint, 65535);
  obj_test(
    cmp_object_is_ushort, cmp_object_as_ushort, "ushort", uint16_t, 65535
  );
  obj_test(cmp_object_is_uint, cmp_object_as_uint, "uint", uint32_t, 65535);
  obj_test(cmp_object_is_ulong, cmp_object_as_ulong, "ulong", uint64_t, 65535);
  obj_test(
    cmp_object_is_uinteger,
    cmp_object_as_uinteger,
    "uinteger",
    uint64_t,
    65535
  );
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_int, "int");
  obj_test_not(cmp_object_is_long, "long");
  obj_test_not(cmp_object_is_sinteger, "sinteger");
  obj_test_not(cmp_object_is_uchar, "uchar");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_bool, "bool");
  obj_test_not(cmp_object_is_str, "str");
  obj_test_not(cmp_object_is_bin, "bin");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");

  obj_write(cmp_write_uint, 65536);
  obj_test(cmp_object_is_uint, cmp_object_as_uint, "uint", uint32_t, 65536);
  obj_test(cmp_object_is_ulong, cmp_object_as_ulong, "ulong", uint64_t, 65536);
  obj_test(
    cmp_object_is_uinteger,
    cmp_object_as_uinteger,
    "uinteger",
    uint64_t,
    65536
  );
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_short, "short");
  obj_test_not(cmp_object_is_int, "int");
  obj_test_not(cmp_object_is_long, "long");
  obj_test_not(cmp_object_is_sinteger, "sinteger");
  obj_test_not(cmp_object_is_uchar, "uchar");
  obj_test_not(cmp_object_is_ushort, "ushort");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_bool, "bool");
  obj_test_not(cmp_object_is_str, "str");
  obj_test_not(cmp_object_is_bin, "bin");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");

  obj_write(cmp_write_uint, 4294967295);
  obj_test(
    cmp_object_is_uint, cmp_object_as_uint, "uint", uint32_t, 4294967295
  );
  obj_test(
    cmp_object_is_ulong, cmp_object_as_ulong, "ulong", uint64_t, 4294967295
  );
  obj_test(
    cmp_object_is_uinteger,
    cmp_object_as_uinteger,
    "uinteger",
    uint64_t,
    4294967295
  );
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_short, "short");
  obj_test_not(cmp_object_is_int, "int");
  obj_test_not(cmp_object_is_long, "long");
  obj_test_not(cmp_object_is_sinteger, "sinteger");
  obj_test_not(cmp_object_is_uchar, "uchar");
  obj_test_not(cmp_object_is_ushort, "ushort");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_bool, "bool");
  obj_test_not(cmp_object_is_str, "str");
  obj_test_not(cmp_object_is_bin, "bin");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");

  obj_write(cmp_write_uint, 4294967296);
  obj_test(
    cmp_object_is_ulong, cmp_object_as_ulong, "ulong", uint64_t, 4294967296
  );
  obj_test(
    cmp_object_is_uinteger,
    cmp_object_as_uinteger,
    "uinteger",
    uint64_t,
    4294967296
  );
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_short, "short");
  obj_test_not(cmp_object_is_int, "int");
  obj_test_not(cmp_object_is_long, "long");
  obj_test_not(cmp_object_is_sinteger, "sinteger");
  obj_test_not(cmp_object_is_uchar, "uchar");
  obj_test_not(cmp_object_is_ushort, "ushort");
  obj_test_not(cmp_object_is_uint, "uint");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_bool, "bool");
  obj_test_not(cmp_object_is_str, "str");
  obj_test_not(cmp_object_is_bin, "bin");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");

  obj_write(cmp_write_float, 1.f);
  obj_test(cmp_object_is_float, cmp_object_as_float, "float", float, 1.f);
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_short, "short");
  obj_test_not(cmp_object_is_int, "int");
  obj_test_not(cmp_object_is_long, "long");
  obj_test_not(cmp_object_is_sinteger, "sinteger");
  obj_test_not(cmp_object_is_uchar, "uchar");
  obj_test_not(cmp_object_is_ushort, "ushort");
  obj_test_not(cmp_object_is_uint, "uint");
  obj_test_not(cmp_object_is_ulong, "ulong");
  obj_test_not(cmp_object_is_uinteger, "uinteger");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_bool, "bool");
  obj_test_not(cmp_object_is_str, "str");
  obj_test_not(cmp_object_is_bin, "bin");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");

  obj_write(cmp_write_double, 1.0);
  obj_test(cmp_object_is_double, cmp_object_as_double, "double", double, 1.0);
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_short, "short");
  obj_test_not(cmp_object_is_int, "int");
  obj_test_not(cmp_object_is_long, "long");
  obj_test_not(cmp_object_is_sinteger, "sinteger");
  obj_test_not(cmp_object_is_uchar, "uchar");
  obj_test_not(cmp_object_is_ushort, "ushort");
  obj_test_not(cmp_object_is_uint, "uint");
  obj_test_not(cmp_object_is_ulong, "ulong");
  obj_test_not(cmp_object_is_uinteger, "uinteger");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_bool, "bool");
  obj_test_not(cmp_object_is_str, "str");
  obj_test_not(cmp_object_is_bin, "bin");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");

  obj_write_no_val(cmp_write_nil);
  obj_test_no_read(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_short, "short");
  obj_test_not(cmp_object_is_int, "int");
  obj_test_not(cmp_object_is_long, "long");
  obj_test_not(cmp_object_is_sinteger, "sinteger");
  obj_test_not(cmp_object_is_uchar, "uchar");
  obj_test_not(cmp_object_is_ushort, "ushort");
  obj_test_not(cmp_object_is_uint, "uint");
  obj_test_not(cmp_object_is_ulong, "ulong");
  obj_test_not(cmp_object_is_uinteger, "uinteger");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_bool, "bool");
  obj_test_not(cmp_object_is_str, "str");
  obj_test_not(cmp_object_is_bin, "bin");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");

  obj_write_no_val(cmp_write_true);
  obj_test(cmp_object_is_bool, cmp_object_as_bool, "bool", bool, true);
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_short, "short");
  obj_test_not(cmp_object_is_int, "int");
  obj_test_not(cmp_object_is_long, "long");
  obj_test_not(cmp_object_is_sinteger, "sinteger");
  obj_test_not(cmp_object_is_uchar, "uchar");
  obj_test_not(cmp_object_is_ushort, "ushort");
  obj_test_not(cmp_object_is_uint, "uint");
  obj_test_not(cmp_object_is_ulong, "ulong");
  obj_test_not(cmp_object_is_uinteger, "uinteger");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_str, "str");
  obj_test_not(cmp_object_is_bin, "bin");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");

  obj_write_no_val(cmp_write_false);
  obj_test(cmp_object_is_bool, cmp_object_as_bool, "bool", bool, false);
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_short, "short");
  obj_test_not(cmp_object_is_int, "int");
  obj_test_not(cmp_object_is_long, "long");
  obj_test_not(cmp_object_is_sinteger, "sinteger");
  obj_test_not(cmp_object_is_uchar, "uchar");
  obj_test_not(cmp_object_is_ushort, "ushort");
  obj_test_not(cmp_object_is_uint, "uint");
  obj_test_not(cmp_object_is_ulong, "ulong");
  obj_test_not(cmp_object_is_uinteger, "uinteger");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_str, "str");
  obj_test_not(cmp_object_is_bin, "bin");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");

  obj_write(cmp_write_bool, true);
  obj_test(cmp_object_is_bool, cmp_object_as_bool, "bool", bool, true);
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_short, "short");
  obj_test_not(cmp_object_is_int, "int");
  obj_test_not(cmp_object_is_long, "long");
  obj_test_not(cmp_object_is_sinteger, "sinteger");
  obj_test_not(cmp_object_is_uchar, "uchar");
  obj_test_not(cmp_object_is_ushort, "ushort");
  obj_test_not(cmp_object_is_uint, "uint");
  obj_test_not(cmp_object_is_ulong, "ulong");
  obj_test_not(cmp_object_is_uinteger, "uinteger");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_str, "str");
  obj_test_not(cmp_object_is_bin, "bin");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");

  obj_write(cmp_write_bool, false);
  obj_test(cmp_object_is_bool, cmp_object_as_bool, "bool", bool, false);
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_short, "short");
  obj_test_not(cmp_object_is_int, "int");
  obj_test_not(cmp_object_is_long, "long");
  obj_test_not(cmp_object_is_sinteger, "sinteger");
  obj_test_not(cmp_object_is_uchar, "uchar");
  obj_test_not(cmp_object_is_ushort, "ushort");
  obj_test_not(cmp_object_is_uint, "uint");
  obj_test_not(cmp_object_is_ulong, "ulong");
  obj_test_not(cmp_object_is_uinteger, "uinteger");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_str, "str");
  obj_test_not(cmp_object_is_bin, "bin");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");

  obj_write_len(cmp_write_str, "Hey there", 9);
  obj_str_test("Hey there");
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_short, "short");
  obj_test_not(cmp_object_is_int, "int");
  obj_test_not(cmp_object_is_long, "long");
  obj_test_not(cmp_object_is_sinteger, "sinteger");
  obj_test_not(cmp_object_is_uchar, "uchar");
  obj_test_not(cmp_object_is_ushort, "ushort");
  obj_test_not(cmp_object_is_uint, "uint");
  obj_test_not(cmp_object_is_ulong, "ulong");
  obj_test_not(cmp_object_is_uinteger, "uinteger");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_bool, "bool");
  obj_test_not(cmp_object_is_bin, "bin");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");

  // Test new cmp_object_to_str
  obj_write_len(cmp_write_str, "Hey there", 9);
  obj_to_str_test("Hey there");
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_short, "short");
  obj_test_not(cmp_object_is_int, "int");
  obj_test_not(cmp_object_is_long, "long");
  obj_test_not(cmp_object_is_sinteger, "sinteger");
  obj_test_not(cmp_object_is_uchar, "uchar");
  obj_test_not(cmp_object_is_ushort, "ushort");
  obj_test_not(cmp_object_is_uint, "uint");
  obj_test_not(cmp_object_is_ulong, "ulong");
  obj_test_not(cmp_object_is_uinteger, "uinteger");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_bool, "bool");
  obj_test_not(cmp_object_is_bin, "bin");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");


  obj_write_len(cmp_write_bin, "Hey there", 9);
  obj_bin_test("Hey there", 9);
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_short, "short");
  obj_test_not(cmp_object_is_int, "int");
  obj_test_not(cmp_object_is_long, "long");
  obj_test_not(cmp_object_is_sinteger, "sinteger");
  obj_test_not(cmp_object_is_uchar, "uchar");
  obj_test_not(cmp_object_is_ushort, "ushort");
  obj_test_not(cmp_object_is_uint, "uint");
  obj_test_not(cmp_object_is_ulong, "ulong");
  obj_test_not(cmp_object_is_uinteger, "uinteger");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_bool, "bool");
  obj_test_not(cmp_object_is_str, "string");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");

  obj_write_len(cmp_write_bin, "Hey there", 9);
  obj_to_bin_test("Hey there", 9);
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_short, "short");
  obj_test_not(cmp_object_is_int, "int");
  obj_test_not(cmp_object_is_long, "long");
  obj_test_not(cmp_object_is_sinteger, "sinteger");
  obj_test_not(cmp_object_is_uchar, "uchar");
  obj_test_not(cmp_object_is_ushort, "ushort");
  obj_test_not(cmp_object_is_uint, "uint");
  obj_test_not(cmp_object_is_ulong, "ulong");
  obj_test_not(cmp_object_is_uinteger, "uinteger");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_bool, "bool");
  obj_test_not(cmp_object_is_str, "string");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");




  M_BufferSeek(&buf, 0);
  cmp_write_array(&cmp, 2);
  cmp_write_uint(&cmp, 1);
  cmp_write_uint(&cmp, 2);
  M_BufferSeek(&buf, 0);
  cmp_read_object(&cmp, &obj);
  obj_array_test(1, 2);
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_short, "short");
  obj_test_not(cmp_object_is_int, "int");
  obj_test_not(cmp_object_is_long, "long");
  obj_test_not(cmp_object_is_sinteger, "sinteger");
  obj_test_not(cmp_object_is_uchar, "uchar");
  obj_test_not(cmp_object_is_ushort, "ushort");
  obj_test_not(cmp_object_is_uint, "uint");
  obj_test_not(cmp_object_is_ulong, "ulong");
  obj_test_not(cmp_object_is_uinteger, "uinteger");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_bool, "bool");
  obj_test_not(cmp_object_is_str, "string");
  obj_test_not(cmp_object_is_bin, "bin");
  obj_test_not(cmp_object_is_map, "map");
  obj_test_not(cmp_object_is_ext, "ext");

  M_BufferSeek(&buf, 0);
  cmp_write_map(&cmp, 1);
  cmp_write_uint(&cmp, 1);
  cmp_write_uint(&cmp, 2);
  M_BufferSeek(&buf, 0);
  cmp_read_object(&cmp, &obj);
  obj_map_test(1, 2);
  obj_test_not(cmp_object_is_char, "char");
  obj_test_not(cmp_object_is_short, "short");
  obj_test_not(cmp_object_is_int, "int");
  obj_test_not(cmp_object_is_long, "long");
  obj_test_not(cmp_object_is_sinteger, "sinteger");
  obj_test_not(cmp_object_is_uchar, "uchar");
  obj_test_not(cmp_object_is_ushort, "ushort");
  obj_test_not(cmp_object_is_uint, "uint");
  obj_test_not(cmp_object_is_ulong, "ulong");
  obj_test_not(cmp_object_is_uinteger, "uinteger");
  obj_test_not(cmp_object_is_float, "float");
  obj_test_not(cmp_object_is_double, "double");
  obj_test_not(cmp_object_is_nil, "nil");
  obj_test_not(cmp_object_is_bool, "bool");
  obj_test_not(cmp_object_is_str, "string");
  obj_test_not(cmp_object_is_bin, "bin");
  obj_test_not(cmp_object_is_array, "array");
  obj_test_not(cmp_object_is_ext, "ext");
}

/* Thanks to andreyvps for this test */
void test_float_flip(void **state) {
  buf_t buf;
  cmp_ctx_t cmp;
  float in;
  float out;
  char init[4];
  char outnit[4];

  (void)state;

  setup_cmp_and_buf(&cmp, &buf);

  /*
   * Writing and reading a float's bytes using cmp mangles one of the bytes for
   * certain floats.  This is one of them.
   */

  /* Specify the binary representation of a problematic float */
  init[0] = -1;
  init[1] = -121;
  init[2] = -95;
  init[3] = -66;

  /* construct the float from the memory, should be -0.315490693 */
  memcpy(&in, init, sizeof(in));

  assert_true(cmp_write_float(&cmp, in));

  /*
   * cmp writes the float header, then the bytes of the float in reversed order
   * (endianness)
   */

  assert_int_equal(buf.data[1], init[3]);
  assert_int_equal(buf.data[2], init[2]);
  assert_int_equal(buf.data[3], init[1]);
  assert_int_equal(buf.data[4], init[0]);

  M_BufferSeek(&buf, 0);

  /* read in the float using cmp. */
  assert_true(cmp_read_float(&cmp, &out));

  memcpy(outnit, &out, sizeof(out));

  /* The reader reads in exactly what was in the buffer */
  assert_int_equal(buf.data[1], outnit[3]);
  assert_int_equal(buf.data[2], outnit[2]);
  assert_int_equal(buf.data[3], outnit[1]);
  assert_int_equal(buf.data[4], outnit[0]);

  /*
   * The reader only seems ok.  The issue happens when you fiddle with the
   * float's bits in the first place.  By the time you write, it's a "valid"
   * float (though has the wrong contents), so when you read it, you're reading
   * a seemingly ok float.  When you fix the writer to write out the correct
   * bytes, the reader then makes the same mistake. The fix is to write the
   * flipped bytes to a buffer and then write that buffer out.  On the read
   * end, you read into a buffer, then write the flipped bytes into a float.
   * This way, the float is never populated with invalid bytes.
   */

  assert_true(in == out);
}

#define MAP_SIZE 66000

void test_skipping(void **state) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;
  cmp_skipper skip;

  (void)state;

  setup_cmp_and_buf(&cmp, &buf);

  skip = cmp.skip;

  /*
   * Essentially, write one of everything to a buffer and then skip it.
   * Probably have to do some permutations of nested types too.
   */

  /*
   *  -8,
   *  [
   *    8,
   *    -200,
   *    200,
   *    -32000,
   *    64000,
   *    -33000,
   *    66000,
   *    -2150000000,
   *    4130000000,
   *    -9223372036854775800,
   *    18446744073709551616,
   *    {
   *      "a": "apple",
   *      "b": ["banana", "blackberry"],
   *      "c": "coconut"
   *    },
   *  ],
   *  { ... big map ... }
   */

  M_BufferEnsureCapacity(&buf, (MAP_SIZE * 2) + 32);

  assert_true(cmp_write_integer(&cmp, -8));
  assert_true(cmp_write_array(&cmp, 10));
    assert_true(cmp_write_uinteger(&cmp, 8));
    assert_true(cmp_write_integer(&cmp, -120));
    assert_true(cmp_write_uinteger(&cmp, 200));
    assert_true(cmp_write_integer(&cmp, -32000));
    assert_true(cmp_write_uinteger(&cmp, 64000));
    assert_true(cmp_write_integer(&cmp, -33000));
    assert_true(cmp_write_uinteger(&cmp, 66000));
    assert_true(cmp_write_integer(&cmp, -2150000000));
    assert_true(cmp_write_uinteger(&cmp, 4300000000));
    assert_true(cmp_write_map(&cmp, 3));
      assert_true(cmp_write_str(&cmp, "a", 1));
      assert_true(cmp_write_str(&cmp, "apple", 5));
      assert_true(cmp_write_str(&cmp, "b", 1));
      assert_true(cmp_write_array(&cmp, 2));
        assert_true(cmp_write_str(&cmp, "banana", 6));
        assert_true(cmp_write_str(&cmp, "blackberry", 10));
      assert_true(cmp_write_str(&cmp, "c", 1));
      assert_true(cmp_write_str(&cmp, "coconut", 7));
  assert_true(cmp_write_map(&cmp, MAP_SIZE));

  for (uint32_t i = 0; i < MAP_SIZE; i++) {
    cmp_write_integer(&cmp, 1);
    cmp_write_integer(&cmp, 1);
  }

  M_BufferSeek(&buf, 0);
  assert_true(cmp_skip_object(&cmp, &obj));

  cmp.skip = NULL;
  M_BufferSeek(&buf, 0);
  assert_true(cmp_skip_object(&cmp, &obj));
  cmp.skip = skip;

  M_BufferSeek(&buf, 0);
  assert_true(cmp_skip_object_no_limit(&cmp));
  assert_false(cmp_skip_object_limit(&cmp, &obj, 1));

  M_BufferSeek(&buf, 0);
  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_NEGATIVE_FIXNUM);
  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_FIXARRAY);
  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_POSITIVE_FIXNUM); /* 8 */
  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_SINT8); /* -120 */
  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_UINT8); /* 200 */
  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_SINT16); /* -32000 */
  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_UINT16); /* 64000 */
  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_SINT32); /* -33000 */
  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_UINT32); /* 66000 */
  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_SINT64); /* -2150000000 */
  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_UINT64); /* 41300000000 */
  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_FIXMAP);
  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_FIXSTR); /* "a" */
  M_BufferSeekForward(&buf, 1);
  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_FIXSTR); /* "apple" */
  M_BufferSeekForward(&buf, 5);
  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_FIXSTR); /* "b" */
  M_BufferSeekForward(&buf, 1);
  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_FIXARRAY);
  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_FIXSTR); /* "banana" */
  M_BufferSeekForward(&buf, 6);
  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_FIXSTR); /* "blackberry" */
  M_BufferSeekForward(&buf, 10);
  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_FIXSTR); /* "c" */
  M_BufferSeekForward(&buf, 1);
  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_FIXSTR); /* "coconut" */
  M_BufferSeekForward(&buf, 7);
  assert_true(cmp_read_object(&cmp, &obj));
  assert_int_equal(obj.type, CMP_TYPE_MAP32);

  M_BufferSeek(&buf, 0);
  assert_true(cmp_skip_object_no_limit(&cmp));
  assert_true(cmp_skip_object_no_limit(&cmp));
  assert_true(cmp_skip_object_no_limit(&cmp));
  assert_false(cmp_skip_object_no_limit(&cmp));
}

int main(void) {
  /* Use the old CMocka API because Travis' latest Ubuntu is Trusty */
  const UnitTest tests[13] = {
    unit_test(test_msgpack),
    unit_test(test_fixedint),
    unit_test(test_numbers),
    unit_test(test_nil),
    unit_test(test_boolean),
    unit_test(test_bin),
    unit_test(test_string),
    unit_test(test_array),
    unit_test(test_map),
    unit_test(test_ext),
    unit_test(test_obj),
    unit_test(test_float_flip),
    unit_test(test_skipping)
  };

  if (run_tests(tests)) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/* vi: set et ts=2 sw=2: */
