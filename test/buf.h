#ifndef BUF_H__
#define BUF_H__

typedef struct buf_s {
  size_t capacity;
  size_t size;
  size_t cursor;
  char *data;
} buf_t;

buf_t*   M_BufferNew(void);
buf_t*   M_BufferNewWithCapacity(size_t capacity);
void     M_BufferInit(buf_t *buf);
void     M_BufferInitWithCapacity(buf_t *buf, size_t capacity);

size_t   M_BufferGetCapacity(buf_t *buf);
size_t   M_BufferGetSize(buf_t *buf);
size_t   M_BufferGetCursor(buf_t *buf);
char*    M_BufferGetData(buf_t *buf);

void     M_BufferEnsureCapacity(buf_t *buf, size_t capacity);
void     M_BufferEnsureTotalCapacity(buf_t *buf, size_t capacity);

void     M_BufferCopy(buf_t *dst, buf_t *src);

void     M_BufferSetData(buf_t *buf, void *data, size_t size);
void     M_BufferSetString(buf_t *buf, char *data, size_t length);
dboolean M_BufferSetFile(buf_t *buf, const char *filename);

dboolean M_BufferSeek(buf_t *buf, size_t pos);
dboolean M_BufferSeekBackward(buf_t *buf, size_t count);
dboolean M_BufferSeekForward(buf_t *buf, size_t count);
byte     M_BufferPeek(buf_t *buf);

void     M_BufferWrite(buf_t *buf, void *data, size_t size);
void     M_BufferWriteBool(buf_t *buf, dboolean b);
void     M_BufferWriteBools(buf_t *buf, dboolean *bools, size_t count);
void     M_BufferWriteChar(buf_t *buf, char c);
void     M_BufferWriteChars(buf_t *buf, char *chars, size_t count);
void     M_BufferWriteUChar(buf_t *buf, unsigned char c);
void     M_BufferWriteUChars(buf_t *buf, unsigned char *uchars, size_t count);
void     M_BufferWriteShort(buf_t *buf, short s);
void     M_BufferWriteShorts(buf_t *buf, short *shorts, size_t count);
void     M_BufferWriteUShort(buf_t *buf, unsigned short s);
void     M_BufferWriteUShorts(buf_t *buf, unsigned short *ushorts,
                                          size_t count);
void     M_BufferWriteInt(buf_t *buf, int i);
void     M_BufferWriteInts(buf_t *buf, int *ints, size_t count);
void     M_BufferWriteUInt(buf_t *buf, unsigned int i);
void     M_BufferWriteUInts(buf_t *buf, unsigned int *ints, size_t count);
void     M_BufferWriteLong(buf_t *buf, int64_t l);
void     M_BufferWriteLongs(buf_t *buf, int64_t *longs, size_t count);
void     M_BufferWriteULong(buf_t *buf, uint64_t l);
void     M_BufferWriteULongs(buf_t *buf, uint64_t *longs, size_t count);
void     M_BufferWriteFloat(buf_t *buf, float f);
void     M_BufferWriteFloats(buf_t *buf, float *floats, size_t count);
void     M_BufferWriteDouble(buf_t *buf, double d);
void     M_BufferWriteDoubles(buf_t *buf, double *doubles, size_t count);
void     M_BufferWriteString(buf_t *buf, char *string, size_t length);
void     M_BufferWriteZeros(buf_t *buf, size_t count);

dboolean M_BufferEqualsString(buf_t *buf, const char *s);
dboolean M_BufferEqualsData(buf_t *buf, const void *d, size_t size);

dboolean M_BufferRead(buf_t *buf, void *data, size_t size);
dboolean M_BufferReadBool(buf_t *buf, dboolean *b);
dboolean M_BufferReadBools(buf_t *buf, dboolean *b, size_t count);
dboolean M_BufferReadChar(buf_t *buf, char *c);
dboolean M_BufferReadChars(buf_t *buf, char *c, size_t count);
dboolean M_BufferReadUChar(buf_t *buf, unsigned char *c);
dboolean M_BufferReadUChars(buf_t *buf, unsigned char *c, size_t count);
dboolean M_BufferReadShort(buf_t *buf, short *s);
dboolean M_BufferReadShorts(buf_t *buf, short *shorts, size_t count);
dboolean M_BufferReadUShort(buf_t *buf, unsigned short *s);
dboolean M_BufferReadUShorts(buf_t *buf, unsigned short *s, size_t count);
dboolean M_BufferReadInt(buf_t *buf, int *i);
dboolean M_BufferReadInts(buf_t *buf, int *i, size_t count);
dboolean M_BufferReadUInt(buf_t *buf, unsigned int *i);
dboolean M_BufferReadUInts(buf_t *buf, unsigned int *i, size_t count);
dboolean M_BufferReadLong(buf_t *buf, int64_t *l);
dboolean M_BufferReadLongs(buf_t *buf, int64_t *l, size_t count);
dboolean M_BufferReadULong(buf_t *buf, uint64_t *l);
dboolean M_BufferReadULongs(buf_t *buf, uint64_t *l, size_t count);
dboolean M_BufferReadFloat(buf_t *buf, float *f);
dboolean M_BufferReadFloats(buf_t *buf, float *f, size_t count);
dboolean M_BufferReadDouble(buf_t *buf, double *d);
dboolean M_BufferReadDoubles(buf_t *buf, double *d, size_t count);
dboolean M_BufferReadString(buf_t *buf, char *s, size_t length);
dboolean M_BufferReadStringDup(buf_t *buf, char **s);
dboolean M_BufferCopyString(buf_t *dst, buf_t *src);

void     M_BufferCompact(buf_t *buf);
void     M_BufferZero(buf_t *buf);
void     M_BufferClear(buf_t *buf);
void     M_BufferFree(buf_t *buf);

void     M_BufferPrint(buf_t *buf);

#endif

/* vi: set et ts=2 sw=2: */

