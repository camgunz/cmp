#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "utils.h"
#include "buf.h"

#include <stdbool.h>
#include "cmp.h"

#define run_test(t)             \
  printf(#t " test: ");         \
  if (!run_ ## t ## _tests()) { \
    printf("-- FAILED --\n");   \
    return EXIT_FAILURE;        \
  }                             \
  else {                        \
    printf("passed\n");         \
  }

static bool buf_reader(struct cmp_ctx_s *ctx, void *data, size_t limit) {
  buf_t *buf = (buf_t *)ctx->buf;

  return M_BufferRead(buf, data, limit);
}

static size_t buf_writer(struct cmp_ctx_s *ctx, const void *data, size_t sz) {
  buf_t *buf = (buf_t *)ctx->buf;
  size_t pos = M_BufferGetCursor(buf);

  M_BufferWrite(buf, (void *)data, sz);

  return M_BufferGetCursor(buf) - pos;
}


/*
[
  false,
  true,
  null,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  -1,
  -1,
  -1,
  -1,
  -1,
  127,
  127,
  255,
  65535,
  4294967295,
  -32,
  -32,
  -128,
  -32768,
  -2147483648,
  0.0,
  -0.0,
  1.0,
  -1.0,
  "a",
  "a",
  "a",
  "",
  "",
  "",
  [0],
  [0],
  [0],
  [],
  [],
  [],
  {},
  {},
  {},
  {"a":97},
  {"a":97},
  {"a":97},
  [[]],
  [["a"]]
]
*/

bool run_msgpack_tests(void) {
  buf_t in_buf, out_buf;
  struct cmp_ctx_s in_cmp, out_cmp;
  struct cmp_object_s obj;
  dboolean buffers_equal = false;

  M_BufferInit(&in_buf);
  M_BufferSetFile(&in_buf, "cases.mpac");
  M_BufferSeek(&in_buf, 0);
  M_BufferInitWithCapacity(&out_buf, M_BufferGetSize(&in_buf));
  cmp_init(&in_cmp, &in_buf, &buf_reader, &buf_writer);
  cmp_init(&out_cmp, &out_buf, &buf_reader, &buf_writer);

  while (M_BufferGetCursor(&in_buf) < (M_BufferGetSize(&in_buf))) {
    if (!cmp_read_object(&in_cmp, &obj)) {
      fprintf(stderr, "Error reading object: %s\n", cmp_strerror(&in_cmp));
      return false;
    }
    if (!cmp_write_object(&out_cmp, &obj)) {
      fprintf(stderr, "Error writing object: %s\n", cmp_strerror(&out_cmp));
      return false;
    }
  }

  M_BufferSeek(&in_buf, 0);
  M_BufferSeek(&out_buf, 0);
  buffers_equal = M_BufferEqualsData(
    &in_buf, M_BufferGetData(&out_buf), M_BufferGetSize(&out_buf)
  );

  if (!buffers_equal) {
    M_BufferPrint(&in_buf);
    M_BufferPrint(&out_buf);
    return false;
  }

  return true;
}

bool run_ext_type_texts(void) {

}

int main(void) {

  run_test(msgpack);

  printf("\nAll tests pass!\n\n");
  return EXIT_SUCCESS;
}

/* vi: set et ts=2 sw=2: */

