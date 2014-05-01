
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "buf.h"

union short_bytes  { unsigned char c[2]; short s; };
union ushort_bytes { unsigned char c[2]; unsigned short s; };
union int_bytes    { unsigned char c[4]; int i; };
union uint_bytes   { unsigned char c[4]; unsigned int i; };
union float_bytes  { unsigned char c[4]; float f; };
union bool_bytes   { unsigned char c[4]; dboolean b; };
union long_bytes   { unsigned char c[8]; int64_t l; };
union ulong_bytes  { unsigned char c[8]; uint64_t l; };
union double_bytes { unsigned char c[8]; double d; };

static void check_cursor(buf_t *buf) {
  if (buf->cursor > buf->size)
    buf->size = buf->cursor;
}

buf_t* M_BufferNew(void) {
  buf_t *buf = calloc(1, sizeof(buf_t));

  if (buf == NULL)
    I_Error("M_BufferNew: Calloc returned NULL.\n");

  M_BufferInit(buf);

  return buf;
}

buf_t* M_BufferNewWithCapacity(size_t capacity) {
  buf_t *buf = calloc(1, sizeof(buf_t));

  if (buf == NULL)
    I_Error("M_BufferNew: Calloc returned NULL.\n");

  M_BufferInitWithCapacity(buf, capacity);

  return buf;
}

void M_BufferInit(buf_t *buf) {
  buf->size     = 0;
  buf->capacity = 0;
  buf->cursor   = 0;
  buf->data     = NULL;
}

void M_BufferInitWithCapacity(buf_t *buf, size_t capacity) {
  M_BufferInit(buf);
  M_BufferEnsureTotalCapacity(buf, capacity);
}

size_t M_BufferGetCapacity(buf_t *buf) {
  return buf->capacity;
}

size_t M_BufferGetSize(buf_t *buf) {
  return buf->size;
}

size_t M_BufferGetCursor(buf_t *buf) {
  return buf->cursor;
}

char* M_BufferGetData(buf_t *buf) {
  return buf->data + buf->cursor;
}

void M_BufferEnsureCapacity(buf_t *buf, size_t capacity) {
  if (buf->capacity < buf->cursor + capacity) {
    size_t old_capacity = buf->capacity;

    buf->capacity = buf->cursor + capacity;
    buf->data = realloc(buf->data, buf->capacity * sizeof(byte));

    if (buf->data == NULL)
      I_Error("M_BufferEnsureCapacity: Allocating buffer data failed");

    memset(buf->data + old_capacity, 0, buf->capacity - old_capacity);
  }
}

void M_BufferEnsureTotalCapacity(buf_t *buf, size_t capacity) {
  if (buf->capacity < capacity) {
    size_t old_capacity = buf->capacity;

    buf->capacity = capacity;
    buf->data = realloc(buf->data, buf->capacity * sizeof(byte));

    if (buf->data == NULL)
      I_Error("M_BufferEnsureCapacity: Allocating buffer data failed");

    memset(buf->data + old_capacity, 0, buf->capacity - old_capacity);
  }
}

void M_BufferCopy(buf_t *dst, buf_t *src) {
  M_BufferSetData(dst, M_BufferGetData(src), M_BufferGetSize(src));
}

void M_BufferSetData(buf_t *buf, void *data, size_t size) {
  M_BufferClear(buf);
  M_BufferEnsureTotalCapacity(buf, size);
  M_BufferWrite(buf, data, size);
}

void M_BufferSetString(buf_t *buf, char *data, size_t length) {
  M_BufferClear(buf);
  M_BufferWriteString(buf, data, length);
}

dboolean M_BufferSetFile(buf_t *buf, const char *filename) {
  FILE *fp = NULL;
  size_t length = 0;
  dboolean out = false;

  if ((fp = fopen(filename, "rb")) == NULL)
    return false;

  fseek(fp, 0, SEEK_END);
  length = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  M_BufferClear(buf);
  M_BufferEnsureTotalCapacity(buf, length);

  if (fread(buf->data, sizeof(byte), length, fp) == length) {
    buf->cursor = length;
    buf->size = length;
    out = true;
  }
  else {
    M_BufferClear(buf);
    out = false;
  }

  fclose(fp);
  return out;
}

dboolean M_BufferSeek(buf_t *buf, size_t pos) {
  if (pos > buf->size)
    return false;

  buf->cursor = pos;
  return true;
}

dboolean M_BufferSeekBackward(buf_t *buf, size_t count) {
  if (count > buf->cursor)
    return false;

  buf->cursor -= count;
  return true;
}

dboolean M_BufferSeekForward(buf_t *buf, size_t count) {
  if (buf->cursor + count > buf->size)
    return false;

  buf->cursor += count;
  return true;
}

byte M_BufferPeek(buf_t *buf) {
  return *(buf->data + buf->cursor);
}

void M_BufferWrite(buf_t *buf, void *data, size_t size) {
  M_BufferEnsureCapacity(buf, size);
  memcpy(buf->data + buf->cursor, data, size);
  buf->cursor += size;

  check_cursor(buf);
}

void M_BufferWriteBool(buf_t *buf, dboolean b) {
  M_BufferWriteBools(buf, &b, 1);
}

void M_BufferWriteBools(buf_t *buf, dboolean *bools, size_t count) {
  size_t i;
  M_BufferEnsureCapacity(buf, count * sizeof(dboolean));

  for (i = 0; i < count; i++) {
    union bool_bytes bb;

    bb.b = bools[i];
    M_BufferWriteUChars(buf, bb.c, 4);
  }
}

void M_BufferWriteChar(buf_t *buf, char c) {
  M_BufferWriteChars(buf, &c, 1);
}

void M_BufferWriteChars(buf_t *buf, char *chars, size_t count) {
  M_BufferWrite(buf, chars, count * sizeof(char));
}

void M_BufferWriteUChar(buf_t *buf, unsigned char c) {
  M_BufferWriteUChars(buf, &c, 1);
}

void M_BufferWriteUChars(buf_t *buf, unsigned char *uchars, size_t count) {
  M_BufferWrite(buf, uchars, count * sizeof(unsigned char));
}

void M_BufferWriteShort(buf_t *buf, short s) {
  M_BufferWriteShorts(buf, &s, 1);
}

void M_BufferWriteShorts(buf_t *buf, short *shorts, size_t count) {
  size_t i;

  M_BufferEnsureCapacity(buf, count * sizeof(short));

  for (i = 0; i < count; i++) {
    union short_bytes sb;

    sb.s = shorts[i];
    M_BufferWriteUChars(buf, sb.c, 2);
  }
}

void M_BufferWriteUShort(buf_t *buf, unsigned short s) {
  M_BufferWriteUShorts(buf, &s, 1);
}

void M_BufferWriteUShorts(buf_t *buf, unsigned short *ushorts, size_t count) {
  size_t i;

  M_BufferEnsureCapacity(buf, count * sizeof(unsigned short));

  for (i = 0; i < count; i++) {
    union ushort_bytes sb;

    sb.s = ushorts[i];
    M_BufferWriteUChars(buf, sb.c, 2);
  }
}

void M_BufferWriteInt(buf_t *buf, int i) {
  M_BufferWriteInts(buf, &i, 1);
}

void M_BufferWriteInts(buf_t *buf, int *ints, size_t count) {
  size_t i;

  M_BufferEnsureCapacity(buf, count * sizeof(int));

  for (i = 0; i < count; i++) {
    union int_bytes ib;

    ib.i = ints[i];
    M_BufferWriteUChars(buf, ib.c, 4);
  }
}

void M_BufferWriteUInt(buf_t *buf, unsigned int s) {
  M_BufferWriteUInts(buf, &s, 1);
}

void M_BufferWriteUInts(buf_t *buf, unsigned int *uints, size_t count) {
  size_t i;

  M_BufferEnsureCapacity(buf, count * sizeof(unsigned int));

  for (i = 0; i < count; i++) {
    union uint_bytes ib;

    ib.i = uints[i];
    M_BufferWriteUChars(buf, ib.c, 4);
  }
}

void M_BufferWriteLong(buf_t *buf, int64_t l) {
  M_BufferWriteLongs(buf, &l, 1);
}

void M_BufferWriteLongs(buf_t *buf, int64_t *longs, size_t count) {
  size_t i;

  M_BufferEnsureCapacity(buf, count * sizeof(int64_t));

  for (i = 0; i < count; i++) {
    union long_bytes lb;

    lb.l = longs[i];
    M_BufferWriteUChars(buf, lb.c, 8);
  }
}

void M_BufferWriteULong(buf_t *buf, uint64_t l) {
  M_BufferWriteULongs(buf, &l, 1);
}

void M_BufferWriteULongs(buf_t *buf, uint64_t *ulongs, size_t count) {
  size_t i;

  M_BufferEnsureCapacity(buf, count * sizeof(uint64_t));

  for (i = 0; i < count; i++) {
    union ulong_bytes lb;

    lb.l = ulongs[i];
    M_BufferWriteUChars(buf, lb.c, 8);
  }
}

void M_BufferWriteFloat(buf_t *buf, float f) {
  M_BufferWriteFloats(buf, &f, 1);
}

void M_BufferWriteFloats(buf_t *buf, float *floats, size_t count) {
  size_t i;

  M_BufferEnsureCapacity(buf, count * sizeof(float));

  for (i = 0; i < count; i++) {
    union float_bytes fb;

    fb.f = floats[i];
    M_BufferWriteUChars(buf, fb.c, 4);
  }
}

void M_BufferWriteDouble(buf_t *buf, double d) {
  M_BufferWriteDoubles(buf, &d, 1);
}

void M_BufferWriteDoubles(buf_t *buf, double *doubles, size_t count) {
  size_t i;

  M_BufferEnsureCapacity(buf, count * sizeof(double));

  for (i = 0; i < count; i++) {
    union double_bytes db;

    db.d = doubles[i];
    M_BufferWriteUChars(buf, db.c, 8);
  }
}

void M_BufferWriteString(buf_t *buf, char *string, size_t length) {
  M_BufferEnsureCapacity(buf, length + 1);
  strncpy(buf->data + buf->cursor, string, length + 1);
  buf->cursor += (length + 1);

  check_cursor(buf);
}

void M_BufferWriteZeros(buf_t *buf, size_t count) {
  size_t i;

  M_BufferEnsureCapacity(buf, count);

  for (i = 0; i < count; i++)
    buf->data[buf->cursor++] = 0;

  check_cursor(buf);
}

dboolean M_BufferEqualsString(buf_t *buf, const char *s) {
  if (strncmp(buf->data + buf->cursor, s, buf->size - buf->cursor) == 0)
    return true;

  return false;
}

dboolean M_BufferEqualsData(buf_t *buf, const void *d, size_t size) {
  if (buf->cursor + size > buf->size)
    return false;

  if (memcmp(buf->data + buf->cursor, d, size) == 0)
    return true;

  return false;
}

dboolean M_BufferRead(buf_t *buf, void *data, size_t size) {
  if (buf->cursor + size > buf->size)
    return false;

  memcpy(data, buf->data + buf->cursor, size);

  buf->cursor += size;

  return true;
}

dboolean M_BufferReadBool(buf_t *buf, dboolean *b) {
  return M_BufferReadBools(buf, b, 1);
}

dboolean M_BufferReadBools(buf_t *buf, dboolean *b, size_t count) {
  return M_BufferRead(buf, b, count * sizeof(dboolean));
}

dboolean M_BufferReadChar(buf_t *buf, char *c) {
  return M_BufferReadChars(buf, c, 1);
}

dboolean M_BufferReadChars(buf_t *buf, char *c, size_t count) {
  return M_BufferRead(buf, c, count * sizeof(char));
}

dboolean M_BufferReadUChar(buf_t *buf, unsigned char *c) {
  return M_BufferReadUChars(buf, c, 1);
}

dboolean M_BufferReadUChars(buf_t *buf, unsigned char *c, size_t count) {
  return M_BufferRead(buf, c, count * sizeof(unsigned char));
}

dboolean M_BufferReadShort(buf_t *buf, short *s) {
  return M_BufferReadShorts(buf, s, 1);
}

dboolean M_BufferReadShorts(buf_t *buf, short *s, size_t count) {
  return M_BufferRead(buf, s, count * sizeof(short));
}

dboolean M_BufferReadUShort(buf_t *buf, unsigned short *s) {
  return M_BufferReadUShorts(buf, s, 1);
}

dboolean M_BufferReadUShorts(buf_t *buf, unsigned short *s, size_t count) {
  return M_BufferRead(buf, s, count * sizeof(unsigned short));
}

dboolean M_BufferReadInt(buf_t *buf, int *i) {
  return M_BufferReadInts(buf, i, 1);
}

dboolean M_BufferReadInts(buf_t *buf, int *i, size_t count) {
  return M_BufferRead(buf, i, count * sizeof(int));
}

dboolean M_BufferReadUInt(buf_t *buf, unsigned int *i) {
  return M_BufferReadUInts(buf, i, 1);
}

dboolean M_BufferReadUInts(buf_t *buf, unsigned int *i, size_t count) {
  return M_BufferRead(buf, i, count * sizeof(unsigned int));
}

dboolean M_BufferReadLong(buf_t *buf, int64_t *l) {
  return M_BufferReadLongs(buf, l, 1);
}

dboolean M_BufferReadLongs(buf_t *buf, int64_t *l, size_t count) {
  return M_BufferRead(buf, l, count * sizeof(int64_t));
}

dboolean M_BufferReadULong(buf_t *buf, uint64_t *l) {
  return M_BufferReadULongs(buf, l, 1);
}

dboolean M_BufferReadULongs(buf_t *buf, uint64_t *l, size_t count) {
  return M_BufferRead(buf, l, count * sizeof(uint64_t));
}

dboolean M_BufferReadFloat(buf_t *buf, float *f) {
  return M_BufferReadFloats(buf, f, 1);
}

dboolean M_BufferReadFloats(buf_t *buf, float *f, size_t count) {
  return M_BufferRead(buf, f, count * sizeof(float));
}

dboolean M_BufferReadDouble(buf_t *buf, double *d) {
  return M_BufferReadDoubles(buf, d, 1);
}

dboolean M_BufferReadDoubles(buf_t *buf, double *d, size_t count) {
  return M_BufferRead(buf, d, count * sizeof(double));
}

dboolean M_BufferReadString(buf_t *buf, char *s, size_t length) {
  return M_BufferRead(buf, s, length);
}

#if 0
dboolean M_BufferReadStringDup(buf_t *buf, char **s) {
  char *d = buf->data + buf->cursor;
  size_t length = strlen(d);

  if (buf->cursor + length > buf->size)
    return false;

  (*s) = strdup(buf->data + buf->cursor);
  return true;
}
#endif

dboolean M_BufferCopyString(buf_t *dst, buf_t *src) {
  char *s = src->data + src->cursor;
  size_t length = strlen(s);

  if (src->cursor + length >= src->size)
    return false;

  M_BufferWriteString(dst, s, length);
  return true;
}

void M_BufferCompact(buf_t *buf) {
  if (buf->size < buf->capacity) {
    char *new_buf = calloc(buf->size, sizeof(byte));

    if (buf->data == NULL)
      I_Error("M_BufferCompact: Allocating new buffer data failed");

    memcpy(new_buf, buf->data, buf->size);
    free(buf->data);
    buf->data = new_buf;
    buf->capacity = buf->size;
    if (buf->cursor > buf->size)
      buf->cursor = buf->size;
  }
}

void M_BufferZero(buf_t *buf) {
  memset(buf->data, 0, buf->capacity);
}

void M_BufferClear(buf_t *buf) {
  buf->size = 0;
  buf->cursor = 0;
  M_BufferZero(buf);
}

void M_BufferFree(buf_t *buf) {
  free(buf->data);
  memset(buf, 0, sizeof(buf_t));
  buf->data = NULL;
}

void M_BufferPrint(buf_t *buf) {
  size_t i;

  printf("Buffer capacity, size and cursor: [%zu, %zu, %zu].\n",
    buf->capacity,
    buf->size,
    buf->cursor
  );

  for (i = 0; i < MIN(256, buf->size); i++) {
    printf("%02x ", (unsigned char)buf->data[i]);

    if ((i > 0) && (((i + 1) % 25) == 0))
      printf("\n");
  }

  printf("\n");
}

/* vi: set et ts=2 sw=2: */

