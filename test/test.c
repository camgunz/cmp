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

bool run_fixedint_tests(void) {
  return true;
}

int main(void) {

  run_test(msgpack);
  run_test(fixedint);

  printf("\nAll tests pass!\n\n");
  return EXIT_SUCCESS;
}

/* vi: set et ts=2 sw=2: */

