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


#ifndef M_BUF_H__
#define M_BUF_H__

typedef struct buf_s {
  size_t capacity;
  size_t size;
  size_t cursor;
  char *data;
} buf_t;

buf_t* M_BufferNew(void);
buf_t* M_BufferNewWithCapacity(size_t capacity);
void   M_BufferInit(buf_t *buf);
void   M_BufferInitWithCapacity(buf_t *buf, size_t capacity);

size_t M_BufferGetCapacity(buf_t *buf);
size_t M_BufferGetSize(buf_t *buf);
size_t M_BufferGetCursor(buf_t *buf);
char*  M_BufferGetData(buf_t *buf);
char*  M_BufferGetDataAtCursor(buf_t *buf);

void M_BufferEnsureCapacity(buf_t *buf, size_t capacity);
void M_BufferEnsureTotalCapacity(buf_t *buf, size_t capacity);

void M_BufferCopy(buf_t *dst, buf_t *src);
void M_BufferCursorCopy(buf_t *dst, buf_t *src);
bool M_BufferMove(buf_t *buf, size_t dpos, size_t spos, size_t count);

void M_BufferSetData(buf_t *buf, const void *data, size_t size);
void M_BufferSetString(buf_t *buf, const char *data, size_t length);
bool M_BufferSetFile(buf_t *buf, const char *filename);

bool M_BufferSeek(buf_t *buf, size_t pos);
bool M_BufferSeekBackward(buf_t *buf, size_t count);
bool M_BufferSeekForward(buf_t *buf, size_t count);

uint8_t M_BufferPeek(buf_t *buf);

void M_BufferWrite(buf_t *buf, const void *data, size_t size);
void M_BufferWriteBool(buf_t *buf, bool b);
void M_BufferWriteBools(buf_t *buf, const bool *bools, size_t count);
void M_BufferWriteChar(buf_t *buf, char c);
void M_BufferWriteChars(buf_t *buf, const char *chars, size_t count);
void M_BufferWriteUChar(buf_t *buf, unsigned char c);
void M_BufferWriteUChars(buf_t *buf, const unsigned char *uchars,
                                         size_t count);
void M_BufferWriteShort(buf_t *buf, short s);
void M_BufferWriteShorts(buf_t *buf, const short *shorts, size_t count);
void M_BufferWriteUShort(buf_t *buf, unsigned short s);
void M_BufferWriteUShorts(buf_t *buf, const unsigned short *ushorts,
                                          size_t count);
void M_BufferWriteInt(buf_t *buf, int i);
void M_BufferWriteInts(buf_t *buf, const int *ints, size_t count);
void M_BufferWriteUInt(buf_t *buf, unsigned int i);
void M_BufferWriteUInts(buf_t *buf, const unsigned int *ints,
                                        size_t count);
void M_BufferWriteLong(buf_t *buf, int64_t l);
void M_BufferWriteLongs(buf_t *buf, const int64_t *longs, size_t count);
void M_BufferWriteULong(buf_t *buf, uint64_t l);
void M_BufferWriteULongs(buf_t *buf, const uint64_t *longs, size_t count);
void M_BufferWriteFloat(buf_t *buf, float f);
void M_BufferWriteFloats(buf_t *buf, const float *floats, size_t count);
void M_BufferWriteDouble(buf_t *buf, double d);
void M_BufferWriteDoubles(buf_t *buf, const double *doubles, size_t count);
void M_BufferWriteString(buf_t *buf, const char *string, size_t length);
void M_BufferWriteZeros(buf_t *buf, size_t count);

bool M_BufferEqualsString(buf_t *buf, const char *s);
bool M_BufferEqualsData(buf_t *buf, const void *d, size_t size);

bool M_BufferRead(buf_t *buf, void *data, size_t size);
bool M_BufferReadBool(buf_t *buf, bool *b);
bool M_BufferReadBools(buf_t *buf, bool *b, size_t count);
bool M_BufferReadChar(buf_t *buf, char *c);
bool M_BufferReadChars(buf_t *buf, char *c, size_t count);
bool M_BufferReadUChar(buf_t *buf, unsigned char *c);
bool M_BufferReadUChars(buf_t *buf, unsigned char *c, size_t count);
bool M_BufferReadShort(buf_t *buf, short *s);
bool M_BufferReadShorts(buf_t *buf, short *shorts, size_t count);
bool M_BufferReadUShort(buf_t *buf, unsigned short *s);
bool M_BufferReadUShorts(buf_t *buf, unsigned short *s, size_t count);
bool M_BufferReadInt(buf_t *buf, int *i);
bool M_BufferReadInts(buf_t *buf, int *i, size_t count);
bool M_BufferReadUInt(buf_t *buf, unsigned int *i);
bool M_BufferReadUInts(buf_t *buf, unsigned int *i, size_t count);
bool M_BufferReadLong(buf_t *buf, int64_t *l);
bool M_BufferReadLongs(buf_t *buf, int64_t *l, size_t count);
bool M_BufferReadULong(buf_t *buf, uint64_t *l);
bool M_BufferReadULongs(buf_t *buf, uint64_t *l, size_t count);
bool M_BufferReadFloat(buf_t *buf, float *f);
bool M_BufferReadFloats(buf_t *buf, float *f, size_t count);
bool M_BufferReadDouble(buf_t *buf, double *d);
bool M_BufferReadDoubles(buf_t *buf, double *d, size_t count);
bool M_BufferReadString(buf_t *buf, char *s, size_t length);
bool M_BufferReadStringDup(buf_t *buf, char **s);
bool M_BufferCopyString(buf_t *dst, buf_t *src);

void M_BufferCompact(buf_t *buf);
void M_BufferTruncate(buf_t *buf, size_t new_size);
void M_BufferZero(buf_t *buf);
void M_BufferClear(buf_t *buf);
void M_BufferFree(buf_t *buf);

void M_BufferPrint(buf_t *buf);
void M_BufferPrintAll(buf_t *buf);

#endif

/* vi: set et ts=2 sw=2: */

