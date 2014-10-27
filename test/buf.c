/*****************************************************************************/
/* D2K: A Doom Source Port for the 21st Century                              */
/*                                                                           */
/* Copyright (C) 2014: See COPYRIGHT file                                    */
/*                                                                           */
/* This file is part of D2K.                                                 */
/*                                                                           */
/* D2K is free software: you can redistribute it and/or modify it under the  */
/* terms of the GNU General Public License as published by the Free Software */
/* Foundation, either version 2 of the License, or (at your option) any      */
/* later version.                                                            */
/*                                                                           */
/* D2K is distributed in the hope that it will be useful, but WITHOUT ANY    */
/* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS */
/* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more    */
/* details.                                                                  */
/*                                                                           */
/* You should have received a copy of the GNU General Public License along   */
/* with D2K.  If not, see <http://www.gnu.org/licenses/>.                    */
/*                                                                           */
/*****************************************************************************/

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "buf.h"


static void check_cursor(buf_t *buf) {
  if (buf->cursor > buf->size)
    buf->size = buf->cursor;
}

buf_t* M_BufferNew(void) {
  buf_t *buf = calloc(1, sizeof(buf_t));

  if (buf == NULL)
    error_and_exit("M_BufferNew: Calloc returned NULL.");

  M_BufferInit(buf);

  return buf;
}

buf_t* M_BufferNewWithCapacity(size_t capacity) {
  buf_t *buf = calloc(1, sizeof(buf_t));

  if (buf == NULL)
    error_and_exit("M_BufferNew: calloc returned NULL");

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
  return buf->data;
}

char* M_BufferGetDataAtCursor(buf_t *buf) {
  return buf->data + buf->cursor;
}

void M_BufferEnsureCapacity(buf_t *buf, size_t capacity) {
  size_t needed_capacity = buf->cursor + capacity;

  if (buf->capacity < needed_capacity) {
    buf->data = realloc(buf->data, needed_capacity * sizeof(uint8_t));

    if (buf->data == NULL) {
      error_and_exit(
        "M_BufferEnsureCapacity: Reallocating buffer data failed"
      );
    }

    memset(buf->data + buf->capacity, 0, needed_capacity - buf->capacity);
    buf->capacity = needed_capacity;
  }
}

void M_BufferEnsureTotalCapacity(buf_t *buf, size_t capacity) {
  if (buf->capacity < capacity) {
    size_t old_capacity = buf->capacity;

    buf->capacity = capacity;
    buf->data = realloc(buf->data, buf->capacity * sizeof(uint8_t));

    if (buf->data == NULL)
      error_and_exit("M_BufferEnsureCapacity: Allocating buffer data failed");

    memset(buf->data + old_capacity, 0, buf->capacity - old_capacity);
  }
}

void M_BufferCopy(buf_t *dst, buf_t *src) {
  M_BufferSetData(dst, M_BufferGetData(src), M_BufferGetSize(src));
}

void M_BufferCursorCopy(buf_t *dst, buf_t *src) {
  M_BufferWrite(
    dst,
    M_BufferGetDataAtCursor(src),
    M_BufferGetSize(src) - (M_BufferGetCursor(src) - 1)
  );
}

bool M_BufferMove(buf_t *buf, size_t dpos, size_t spos, size_t count) {
  if ((spos + count) > M_BufferGetSize(buf))
    return false;

  M_BufferEnsureTotalCapacity(buf, dpos + count);

  memmove(M_BufferGetData(buf) + dpos, M_BufferGetData(buf) + spos, count);

  return true;
}

void M_BufferSetData(buf_t *buf, const void *data, size_t size) {
  M_BufferClear(buf);
  M_BufferEnsureTotalCapacity(buf, size);
  M_BufferWrite(buf, data, size);
}

void M_BufferSetString(buf_t *buf, const char *data, size_t length) {
  M_BufferClear(buf);
  M_BufferWriteString(buf, data, length);
}

bool M_BufferSetFile(buf_t *buf, const char *filename) {
  FILE *fp = NULL;
  size_t length = 0;
  bool out = false;

  if ((fp = fopen(filename, "rb")) == NULL)
    return false;

  fseek(fp, 0, SEEK_END);
  length = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  M_BufferClear(buf);
  M_BufferEnsureTotalCapacity(buf, length);

  if (fread(buf->data, sizeof(uint8_t), length, fp) == length) {
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

bool M_BufferSeek(buf_t *buf, size_t pos) {
  if (pos > buf->size)
    return false;

  buf->cursor = pos;
  return true;
}

bool M_BufferSeekBackward(buf_t *buf, size_t count) {
  if (count > buf->cursor)
    return false;

  buf->cursor -= count;
  return true;
}

bool M_BufferSeekForward(buf_t *buf, size_t count) {
  if (buf->cursor + count > buf->size)
    return false;

  buf->cursor += count;
  return true;
}

uint8_t M_BufferPeek(buf_t *buf) {
  return *(buf->data + buf->cursor);
}

void M_BufferWrite(buf_t *buf, const void *data, size_t size) {
  M_BufferEnsureCapacity(buf, size);
  memcpy(buf->data + buf->cursor, data, size);
  buf->cursor += size;

  check_cursor(buf);
}

void M_BufferWriteBool(buf_t *buf, bool b) {
  M_BufferWriteBools(buf, &b, 1);
}

void M_BufferWriteBools(buf_t *buf, const bool *bools, size_t count) {
  M_BufferEnsureCapacity(buf, count * sizeof(bool));
  M_BufferWriteChars(buf, (char *)bools, count * sizeof(bool));
}

void M_BufferWriteChar(buf_t *buf, char c) {
  M_BufferWriteChars(buf, &c, 1);
}

void M_BufferWriteChars(buf_t *buf, const char *chars, size_t count) {
  M_BufferWrite(buf, chars, count * sizeof(char));
}

void M_BufferWriteUChar(buf_t *buf, unsigned char c) {
  M_BufferWriteUChars(buf, &c, 1);
}

void M_BufferWriteUChars(buf_t *buf, const unsigned char *uchars, size_t count) {
  M_BufferWrite(buf, uchars, count * sizeof(unsigned char));
}

void M_BufferWriteShort(buf_t *buf, short s) {
  M_BufferWriteShorts(buf, &s, 1);
}

void M_BufferWriteShorts(buf_t *buf, const short *shorts, size_t count) {
  M_BufferEnsureCapacity(buf, count * sizeof(short));
  M_BufferWriteChars(buf, (char *)shorts, count * sizeof(short));
}

void M_BufferWriteUShort(buf_t *buf, unsigned short s) {
  M_BufferWriteUShorts(buf, &s, 1);
}

void M_BufferWriteUShorts(buf_t *buf, const unsigned short *ushorts,
                                      size_t count) {
  M_BufferEnsureCapacity(buf, count * sizeof(unsigned short));
  M_BufferWriteChars(buf, (char *)ushorts, count * sizeof(unsigned short));
}

void M_BufferWriteInt(buf_t *buf, int i) {
  M_BufferWriteInts(buf, &i, 1);
}

void M_BufferWriteInts(buf_t *buf, const int *ints, size_t count) {
  M_BufferEnsureCapacity(buf, count * sizeof(int));
  M_BufferWriteChars(buf, (char *)ints, count * sizeof(int));
}

void M_BufferWriteUInt(buf_t *buf, unsigned int s) {
  M_BufferWriteUInts(buf, &s, 1);
}

void M_BufferWriteUInts(buf_t *buf, const unsigned int *uints, size_t count) {
  M_BufferEnsureCapacity(buf, count * sizeof(unsigned int));
  M_BufferWriteChars(buf, (char *)uints, count * sizeof(unsigned int));
}

void M_BufferWriteLong(buf_t *buf, int64_t l) {
  M_BufferWriteLongs(buf, &l, 1);
}

void M_BufferWriteLongs(buf_t *buf, const int64_t *longs, size_t count) {
  M_BufferEnsureCapacity(buf, count * sizeof(int64_t));
  M_BufferWriteChars(buf, (char *)longs, count * sizeof(int64_t));
}

void M_BufferWriteULong(buf_t *buf, uint64_t l) {
  M_BufferWriteULongs(buf, &l, 1);
}

void M_BufferWriteULongs(buf_t *buf, const uint64_t *ulongs, size_t count) {
  M_BufferEnsureCapacity(buf, count * sizeof(int64_t));
  M_BufferWriteChars(buf, (char *)ulongs, count * sizeof(uint64_t));
}

void M_BufferWriteFloat(buf_t *buf, float f) {
  M_BufferWriteFloats(buf, &f, 1);
}

void M_BufferWriteFloats(buf_t *buf, const float *floats, size_t count) {
  M_BufferEnsureCapacity(buf, count * sizeof(float));
  M_BufferWriteChars(buf, (char *)floats, count * sizeof(floats));
}

void M_BufferWriteDouble(buf_t *buf, double d) {
  M_BufferWriteDoubles(buf, &d, 1);
}

void M_BufferWriteDoubles(buf_t *buf, const double *doubles, size_t count) {
  M_BufferEnsureCapacity(buf, count * sizeof(double));
  M_BufferWriteChars(buf, (char *)doubles, count * sizeof(doubles));
}

void M_BufferWriteString(buf_t *buf, const char *string, size_t length) {
  M_BufferEnsureCapacity(buf, length + 1);
  strncpy(buf->data + buf->cursor, string, length + 1);
  buf->cursor += (length + 1);

  check_cursor(buf);
}

void M_BufferWriteZeros(buf_t *buf, size_t count) {
  M_BufferEnsureCapacity(buf, count);

  for (size_t i = 0; i < count; i++)
    buf->data[buf->cursor++] = 0;

  check_cursor(buf);
}

bool M_BufferEqualsString(buf_t *buf, const char *s) {
  if (strncmp(buf->data + buf->cursor, s, buf->size - buf->cursor) == 0)
    return true;

  return false;
}

bool M_BufferEqualsData(buf_t *buf, const void *d, size_t size) {
  if (buf->cursor + size > buf->size)
    return false;

  if (memcmp(buf->data + buf->cursor, d, size) == 0)
    return true;

  return false;
}

bool M_BufferRead(buf_t *buf, void *data, size_t size) {
  if (buf->cursor + size > buf->size)
    return false;

  if (size == 1)
    *((char *)data) = *(buf->data + buf->cursor);
  else
    memcpy(data, buf->data + buf->cursor, size);

  buf->cursor += size;

  return true;
}

bool M_BufferReadBool(buf_t *buf, bool *b) {
  return M_BufferReadBools(buf, b, 1);
}

bool M_BufferReadBools(buf_t *buf, bool *b, size_t count) {
  return M_BufferRead(buf, b, count * sizeof(bool));
}

bool M_BufferReadChar(buf_t *buf, char *c) {
  return M_BufferReadChars(buf, c, 1);
}

bool M_BufferReadChars(buf_t *buf, char *c, size_t count) {
  return M_BufferRead(buf, c, count * sizeof(char));
}

bool M_BufferReadUChar(buf_t *buf, unsigned char *c) {
  return M_BufferReadUChars(buf, c, 1);
}

bool M_BufferReadUChars(buf_t *buf, unsigned char *c, size_t count) {
  return M_BufferRead(buf, c, count * sizeof(unsigned char));
}

bool M_BufferReadShort(buf_t *buf, short *s) {
  return M_BufferReadShorts(buf, s, 1);
}

bool M_BufferReadShorts(buf_t *buf, short *s, size_t count) {
  return M_BufferRead(buf, s, count * sizeof(short));
}

bool M_BufferReadUShort(buf_t *buf, unsigned short *s) {
  return M_BufferReadUShorts(buf, s, 1);
}

bool M_BufferReadUShorts(buf_t *buf, unsigned short *s, size_t count) {
  return M_BufferRead(buf, s, count * sizeof(unsigned short));
}

bool M_BufferReadInt(buf_t *buf, int *i) {
  return M_BufferReadInts(buf, i, 1);
}

bool M_BufferReadInts(buf_t *buf, int *i, size_t count) {
  return M_BufferRead(buf, i, count * sizeof(int));
}

bool M_BufferReadUInt(buf_t *buf, unsigned int *i) {
  return M_BufferReadUInts(buf, i, 1);
}

bool M_BufferReadUInts(buf_t *buf, unsigned int *i, size_t count) {
  return M_BufferRead(buf, i, count * sizeof(unsigned int));
}

bool M_BufferReadLong(buf_t *buf, int64_t *l) {
  return M_BufferReadLongs(buf, l, 1);
}

bool M_BufferReadLongs(buf_t *buf, int64_t *l, size_t count) {
  return M_BufferRead(buf, l, count * sizeof(int64_t));
}

bool M_BufferReadULong(buf_t *buf, uint64_t *l) {
  return M_BufferReadULongs(buf, l, 1);
}

bool M_BufferReadULongs(buf_t *buf, uint64_t *l, size_t count) {
  return M_BufferRead(buf, l, count * sizeof(uint64_t));
}

bool M_BufferReadFloat(buf_t *buf, float *f) {
  return M_BufferReadFloats(buf, f, 1);
}

bool M_BufferReadFloats(buf_t *buf, float *f, size_t count) {
  return M_BufferRead(buf, f, count * sizeof(float));
}

bool M_BufferReadDouble(buf_t *buf, double *d) {
  return M_BufferReadDoubles(buf, d, 1);
}

bool M_BufferReadDoubles(buf_t *buf, double *d, size_t count) {
  return M_BufferRead(buf, d, count * sizeof(double));
}

bool M_BufferReadString(buf_t *buf, char *s, size_t length) {
  return M_BufferRead(buf, s, length);
}

bool M_BufferReadStringDup(buf_t *buf, char **s) {
  char *d = buf->data + buf->cursor;
  size_t length = strlen(d);

  if (buf->cursor + length > buf->size)
    return false;

  (*s) = strdup(buf->data + buf->cursor);
  return true;
}

bool M_BufferCopyString(buf_t *dst, buf_t *src) {
  char *s = src->data + src->cursor;
  size_t length = strlen(s);

  if (src->cursor + length >= src->size)
    return false;

  M_BufferWriteString(dst, s, length);
  return true;
}

void M_BufferCompact(buf_t *buf) {
  if (buf->size < buf->capacity) {
    char *new_buf = calloc(buf->size, sizeof(uint8_t));

    if (buf->data == NULL)
      error_and_exit("M_BufferCompact: Allocating new buffer data failed");

    memcpy(new_buf, buf->data, buf->size);
    free(buf->data);
    buf->data = new_buf;
    buf->capacity = buf->size;
    if (buf->cursor > buf->size)
      buf->cursor = buf->size;
  }
}

void M_BufferTruncate(buf_t *buf, size_t new_size) {
  size_t old_size = buf->size;

  if (new_size >= buf->size)
    errorf_and_exit("M_BufferTruncate: %zu >= %zu.", new_size, buf->size);

  memset(buf->data + new_size, 0, old_size - new_size);
  buf->size = new_size;

  if (buf->cursor >= buf->size)
    buf->cursor = buf->size - 1;
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
  printf("Buffer capacity, size and cursor: [%zu, %zu, %zu].\n",
    buf->capacity,
    buf->size,
    buf->cursor
  );

  for (size_t i = 0; i < MIN(64, buf->size); i++) {
    printf("%02X ", (unsigned char)buf->data[i]);

    if ((i > 0) && (((i + 1) % 25) == 0))
      printf("\n");
  }

  printf("\n");
}

void M_BufferPrintAll(buf_t *buf) {
  printf("Buffer capacity, size and cursor: [%zu, %zu, %zu].\n",
    buf->capacity,
    buf->size,
    buf->cursor
  );

  for (size_t i = 0; i < buf->size; i++) {
    printf("%02X ", (unsigned char)buf->data[i]);

    if ((i > 0) && (((i + 1) % 25) == 0))
      printf("\n");
  }

  printf("\n");
}

/* vi: set et ts=2 sw=2: */

