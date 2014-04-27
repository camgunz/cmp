#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "cmp.h"

enum {
  MP_POSITIVE_FIXNUM,
  MP_FIXMAP,
  MP_FIXARRAY,
  MP_FIXRAW,
  MP_NIL,
  MP_BOOLEAN,
  MP_FLOAT,
  MP_DOUBLE,
  MP_U8,
  MP_U16,
  MP_U32,
  MP_U64,
  MP_S8,
  MP_S16,
  MP_S32,
  MP_S64,
  MP_RAW16,
  MP_RAW32,
  MP_ARRAY16,
  MP_ARRAY32,
  MP_MAP16,
  MP_MAP32,
  MP_NEGATIVE_FIXNUM
};

/*
 * Not Working:
 * 0xCE,
 * 0xCF,
 * 0xD2,
 * 0xD3,
 * 0xCB,
 */

enum {
  POSITIVE_FIXNUM_MARKER = 0x00,
  FIXMAP_MARKER          = 0x80,
  FIXARRAY_MARKER        = 0x90,
  FIXRAW_MARKER          = 0xA0,
  NIL_MARKER             = 0xC0,
  FALSE_MARKER           = 0xC2,
  TRUE_MARKER            = 0xC3,
  FLOAT_MARKER           = 0xCA,
  DOUBLE_MARKER          = 0xCB,
  U8_MARKER              = 0xCC,
  U16_MARKER             = 0xCD,
  U32_MARKER             = 0xCE,
  U64_MARKER             = 0xCF,
  S8_MARKER              = 0xD0,
  S16_MARKER             = 0xD1,
  S32_MARKER             = 0xD2,
  S64_MARKER             = 0xD3,
  RAW16_MARKER           = 0xDA,
  RAW32_MARKER           = 0xDB,
  ARRAY16_MARKER         = 0xDC,
  ARRAY32_MARKER         = 0xDD,
  MAP16_MARKER           = 0xDE,
  MAP32_MARKER           = 0xDF,
  NEGATIVE_FIXNUM_MARKER = 0xE0
};

enum {
  POSITIVE_FIXNUM_SIZE = 0x7F,
  FIXARRAY_SIZE        = 0xF,
  FIXMAP_SIZE          = 0xF,
  FIXRAW_SIZE          = 0x1F,
  NEGATIVE_FIXNUM_SIZE = 0x1F
};

enum {
  ERROR_NONE,
  RAW_DATA_LENGTH_TOO_LONG_ERROR,
  ARRAY_LENGTH_TOO_LONG_ERROR,
  MAP_LENGTH_TOO_LONG_ERROR,
  INPUT_VALUE_TOO_LARGE_ERROR,
  FIXED_VALUE_WRITING_ERROR,
  TYPE_MARKER_READING_ERROR,
  TYPE_MARKER_WRITING_ERROR,
  DATA_READING_ERROR,
  DATA_WRITING_ERROR,
  INVALID_TYPE_ERROR,
  LENGTH_READING_ERROR,
  LENGTH_WRITING_ERROR,
  ERROR_MAX
};

const char *cmp_error_messages[ERROR_MAX + 1] = {
  "No Error",
  "Specified raw data length is too long (> 0xFFFFFFFF)",
  "Specified array length is too long (> 0xFFFFFFFF)",
  "Specified map length is too long (> 0xFFFFFFFF)",
  "Input value is too large",
  "Error writing fixed value",
  "Error reading type marker",
  "Error writing type marker",
  "Error reading packed data",
  "Error writing packed data",
  "Invalid type",
  "Error reading length",
  "Error writing length",
  "Max Error"
};

static const int32_t _i = 1;
#define is_bigendian() ((*(char *)&_i) == 0)

static uint16_t b16(uint16_t x) {
  char *b = (char *)&x;

  if (!is_bigendian()) {
    char swap = 0;

    swap = b[0];
    b[0] = b[1];
    b[1] = swap;
  }

  return x;
}

static uint32_t b32(uint32_t x) {
  char *b = (char *)&x;

  if (!is_bigendian()) {
    char swap = 0;

    swap = b[0];
    b[0] = b[3];
    b[3] = swap;

    swap = b[1];
    b[1] = b[2];
    b[2] = swap;
  }

  return x;
}

static uint64_t b64(uint64_t x) {
  char *b = (char *)&x;

  if (!is_bigendian()) {
    char swap = 0;

    swap = b[0];
    b[0] = b[7];
    b[7] = swap;

    swap = b[1];
    b[1] = b[6];
    b[6] = swap;

    swap = b[2];
    b[2] = b[5];
    b[5] = swap;

    swap = b[3];
    b[3] = b[4];
    b[4] = swap;
  }

  return x;
}

static void set_error(struct cmp_ctx_s *ctx, uint8_t error_code) {
  ctx->error = error_code;
}

static bool read_byte(struct cmp_ctx_s *ctx, uint8_t *x) {
  return ctx->read(ctx, x, sizeof(uint8_t));
}

static bool write_byte(struct cmp_ctx_s *ctx, uint8_t x) {
  return (ctx->write(ctx, &x, sizeof(uint8_t)) == (sizeof(uint8_t)));
}

static bool read_type_marker(struct cmp_ctx_s *ctx, uint8_t *marker) {
  if (read_byte(ctx, marker))
    return true;

  set_error(ctx, TYPE_MARKER_READING_ERROR);
  return false;
}

static bool write_type_marker(struct cmp_ctx_s *ctx, uint8_t marker) {
  if (write_byte(ctx, marker))
    return true;

  set_error(ctx, TYPE_MARKER_WRITING_ERROR);
  return false;
}

static bool write_fixed_value(struct cmp_ctx_s *ctx, uint8_t value) {
  if (write_byte(ctx, value))
    return true;

  set_error(ctx, FIXED_VALUE_WRITING_ERROR);
  return false;
}

void cmp_init(struct cmp_ctx_s *ctx, void *buf, cmp_reader read,
                                                cmp_writer write) {
  ctx->error = ERROR_NONE;
  ctx->buf = buf;
  ctx->read = read;
  ctx->write = write;
}

const char* cmp_strerror(struct cmp_ctx_s *ctx) {
  if (ctx->error > ERROR_NONE && ctx->error < ERROR_MAX)
    return cmp_error_messages[ctx->error];

  return "";
}

bool cmp_write_pfix(struct cmp_ctx_s *ctx, uint8_t c) {
  return write_fixed_value(ctx, c & POSITIVE_FIXNUM_SIZE);
}

bool cmp_write_nfix(struct cmp_ctx_s *ctx, int8_t c) {
  return write_fixed_value(
    ctx, NEGATIVE_FIXNUM_MARKER | (c & NEGATIVE_FIXNUM_SIZE)
  );
}

bool cmp_write_sfix(struct cmp_ctx_s *ctx, int8_t c) {
  if (c >= 0 && c <= POSITIVE_FIXNUM_SIZE)
    return cmp_write_pfix(ctx, c);

  if (c >= -32 && c <= -1)
    return cmp_write_nfix(ctx, c);

  set_error(ctx, INPUT_VALUE_TOO_LARGE_ERROR);
  return false;
}

bool cmp_write_s8(struct cmp_ctx_s *ctx, int8_t c) {
  if (!write_type_marker(ctx, S8_MARKER))
    return false;

  return ctx->write(ctx, &c, sizeof(int8_t));
}

bool cmp_write_s16(struct cmp_ctx_s *ctx, int16_t s) {
  if (!write_type_marker(ctx, S16_MARKER))
    return false;

  s = b16(s);

  return ctx->write(ctx, &s, sizeof(int16_t));
}

bool cmp_write_s32(struct cmp_ctx_s *ctx, int32_t i) {
  if (!write_type_marker(ctx, S32_MARKER))
    return false;

  i = b32(i);

  return ctx->write(ctx, &i, sizeof(int32_t));
}

bool cmp_write_s64(struct cmp_ctx_s *ctx, int64_t l) {
  if (!write_type_marker(ctx, S64_MARKER))
    return false;

  l = b64(l);

  return ctx->write(ctx, &l, sizeof(int64_t));
}

bool cmp_write_int(struct cmp_ctx_s *ctx, int64_t d) {
  uint64_t b = d;

  if (d >= -32 && d <= -1)
    return cmp_write_nfix(ctx, d);

  if (d >= 0 && d <= POSITIVE_FIXNUM_SIZE)
    return cmp_write_pfix(ctx, d);

  if (b <= 0xFF)
    return cmp_write_s8(ctx, d);

  if (b <= 0xFFFF)
    return cmp_write_s16(ctx, d);

  if (b <= 0xFFFFFFFF)
    return cmp_write_s32(ctx, d);

  if (b <= 0xFFFFFFFFFFFFFFFF)
    return cmp_write_s64(ctx, d);

  set_error(ctx, INPUT_VALUE_TOO_LARGE_ERROR);
  return false;
}

bool cmp_write_ufix(struct cmp_ctx_s *ctx, uint8_t c) {
  return cmp_write_pfix(ctx, c);
}

bool cmp_write_u8(struct cmp_ctx_s *ctx, uint8_t c) {
  if (!write_type_marker(ctx, U8_MARKER))
    return false;

  return ctx->write(ctx, &c, sizeof(uint8_t));
}

bool cmp_write_u16(struct cmp_ctx_s *ctx, uint16_t s) {
  if (!write_type_marker(ctx, U16_MARKER))
    return false;

  s = b16(s);

  return ctx->write(ctx, &s, sizeof(uint16_t));
}

bool cmp_write_u32(struct cmp_ctx_s *ctx, uint32_t i) {
  if (!write_type_marker(ctx, U32_MARKER))
    return false;

  i = b32(i);

  return ctx->write(ctx, &i, sizeof(uint32_t));
}

bool cmp_write_u64(struct cmp_ctx_s *ctx, uint64_t l) {
  if (!write_type_marker(ctx, U64_MARKER))
    return false;

  l = b64(l);

  return ctx->write(ctx, &l, sizeof(uint64_t));
}

bool cmp_write_uint(struct cmp_ctx_s *ctx, uint64_t u) {
  if (u <= POSITIVE_FIXNUM_SIZE)
    return cmp_write_pfix(ctx, u);

  if (u <= 0xFF)
    return cmp_write_s8(ctx, u);

  if (u <= 0xFFFF)
    return cmp_write_s16(ctx, u);

  if (u <= 0xFFFFFFFF)
    return cmp_write_s32(ctx, u);

  if (u <= 0xFFFFFFFFFFFFFFFF)
    return cmp_write_s64(ctx, u);

  set_error(ctx, INPUT_VALUE_TOO_LARGE_ERROR);
  return false;
}

bool cmp_write_float(struct cmp_ctx_s *ctx, float f) {
  if (!write_type_marker(ctx, FLOAT_MARKER))
    return false;

  return ctx->write(ctx, &f, sizeof(float));
}

bool cmp_write_double(struct cmp_ctx_s *ctx, double d) {
  if (!write_type_marker(ctx, DOUBLE_MARKER))
    return false;

  return ctx->write(ctx, &d, sizeof(double));
}

bool cmp_write_nil(struct cmp_ctx_s *ctx) {
  return write_type_marker(ctx, NIL_MARKER);
}

bool cmp_write_true(struct cmp_ctx_s *ctx) {
  return write_type_marker(ctx, TRUE_MARKER);
}

bool cmp_write_false(struct cmp_ctx_s *ctx) {
  return write_type_marker(ctx, FALSE_MARKER);
}

bool cmp_write_fixraw_marker(struct cmp_ctx_s *ctx, size_t length) {
  return write_fixed_value(ctx, FIXRAW_MARKER | (length & FIXRAW_SIZE));
}

bool cmp_write_fixraw(struct cmp_ctx_s *ctx, void *data, size_t length) {
  if (!cmp_write_fixraw_marker(ctx, length))
    return false;

  if (ctx->write(ctx, data, length))
    return true;

  set_error(ctx, DATA_WRITING_ERROR);
  return false;
}

bool cmp_write_raw16_marker(struct cmp_ctx_s *ctx, size_t length) {
  if (!write_type_marker(ctx, RAW16_MARKER))
    return false;

  length = b16(length);

  if (ctx->write(ctx, &length, sizeof(uint16_t)))
    return true;

  set_error(ctx, LENGTH_WRITING_ERROR);
  return false;
}

bool cmp_write_raw16(struct cmp_ctx_s *ctx, void *data, size_t length) {
  if (!cmp_write_raw16_marker(ctx, length))
    return false;

  if (ctx->write(ctx, data, length))
    return true;

  set_error(ctx, DATA_WRITING_ERROR);
  return false;
}

bool cmp_write_raw32_marker(struct cmp_ctx_s *ctx, size_t length) {
  if (!write_type_marker(ctx, RAW32_MARKER))
    return false;

  length = b32(length);

  if (ctx->write(ctx, &length, sizeof(uint32_t)))
    return true;

  set_error(ctx, LENGTH_WRITING_ERROR);
  return false;
}

bool cmp_write_raw32(struct cmp_ctx_s *ctx, void *data, size_t length) {
  if (!cmp_write_raw32_marker(ctx, length))
    return false;

  if (ctx->write(ctx, data, length))
    return true;

  set_error(ctx, DATA_WRITING_ERROR);
  return false;
}

bool cmp_write_raw_marker(struct cmp_ctx_s *ctx, size_t length) {
  if (length <= FIXRAW_SIZE)
    return cmp_write_fixraw_marker(ctx, length);

  if (length <= 0xFFFF)
    return cmp_write_raw16_marker(ctx, length);

  if (length <= 0xFFFFFFFF)
    return cmp_write_raw16_marker(ctx, length);

  set_error(ctx, INPUT_VALUE_TOO_LARGE_ERROR);
  return false;
}

bool cmp_write_raw(struct cmp_ctx_s *ctx, void *data, size_t length) {
  if (length <= FIXRAW_SIZE)
    return cmp_write_fixraw(ctx, data, length);

  if (length <= 0xFFFF)
    return cmp_write_raw16(ctx, data, length);

  if (length <= 0xFFFFFFFF)
    return cmp_write_raw16(ctx, data, length);

  set_error(ctx, INPUT_VALUE_TOO_LARGE_ERROR);
  return false;
}

bool cmp_write_fixarray(struct cmp_ctx_s *ctx, size_t length) {
  return write_fixed_value(ctx, FIXARRAY_MARKER | (length & FIXARRAY_SIZE));
}

bool cmp_write_array16(struct cmp_ctx_s *ctx, size_t length) {
  if (!write_type_marker(ctx, ARRAY16_MARKER))
    return false;

  length = b16(length);

  if (ctx->write(ctx, &length, sizeof(uint16_t)))
    return true;

  set_error(ctx, LENGTH_WRITING_ERROR);
  return false;
}

bool cmp_write_array32(struct cmp_ctx_s *ctx, size_t length) {
  if (!write_type_marker(ctx, ARRAY32_MARKER))
    return false;

  length = b32(length);

  if (ctx->write(ctx, &length, sizeof(uint32_t)))
    return true;

  set_error(ctx, LENGTH_WRITING_ERROR);
  return false;
}

bool cmp_write_array(struct cmp_ctx_s *ctx, size_t length) {
  if (length <= FIXARRAY_SIZE)
    return cmp_write_fixarray(ctx, length);

  if (length <= 0xFFFF)
    return cmp_write_array16(ctx, length);

  if (length <= 0xFFFFFFFF)
    return cmp_write_array32(ctx, length);

  set_error(ctx, INPUT_VALUE_TOO_LARGE_ERROR);
  return false;
}

bool cmp_write_fixmap(struct cmp_ctx_s *ctx, size_t length) {
  return write_fixed_value(ctx, FIXMAP_MARKER | (length & FIXMAP_SIZE));
}

bool cmp_write_map16(struct cmp_ctx_s *ctx, size_t length) {
  if (!write_type_marker(ctx, MAP16_MARKER))
    return false;

  length = b16(length);

  if (ctx->write(ctx, &length, sizeof(uint16_t)))
    return true;

  set_error(ctx, LENGTH_WRITING_ERROR);
  return false;
}

bool cmp_write_map32(struct cmp_ctx_s *ctx, size_t length) {
  if (!write_type_marker(ctx, MAP32_MARKER))
    return false;

  length = b32(length);

  if (ctx->write(ctx, &length, sizeof(uint32_t)))
    return true;

  set_error(ctx, LENGTH_WRITING_ERROR);
  return false;
}

bool cmp_write_map(struct cmp_ctx_s *ctx, size_t length) {
  if (length <= FIXMAP_SIZE)
    return cmp_write_fixmap(ctx, length);

  if (length <= 0xFFFF)
    return cmp_write_map16(ctx, length);

  if (length <= 0xFFFFFFFF)
    return cmp_write_map32(ctx, length);

  set_error(ctx, INPUT_VALUE_TOO_LARGE_ERROR);
  return false;
}

bool cmp_write_object(struct cmp_ctx_s *ctx, struct cmp_object_s *obj) {
  switch(obj->type) {
    case MP_POSITIVE_FIXNUM:
      return cmp_write_pfix(ctx, obj->data.u8);
    case MP_FIXMAP:
      return cmp_write_fixmap(ctx, obj->data.map_size);
    case MP_FIXARRAY:
      return cmp_write_fixarray(ctx, obj->data.array_size);
    case MP_FIXRAW:
      return cmp_write_fixraw_marker(ctx, obj->data.raw_size);
    case MP_NIL:
      return cmp_write_nil(ctx);
    case MP_BOOLEAN:
      if (obj->data.boolean)
        return cmp_write_true(ctx);

      return cmp_write_false(ctx);
    case MP_FLOAT:
      return cmp_write_float(ctx, obj->data.flt);
    case MP_DOUBLE:
      return cmp_write_double(ctx, obj->data.dbl);
    case MP_U8:
      return cmp_write_u8(ctx, obj->data.u8);
    case MP_U16:
      return cmp_write_u16(ctx, obj->data.u16);
    case MP_U32:
      return cmp_write_u32(ctx, obj->data.u32);
    case MP_U64:
      return cmp_write_u64(ctx, obj->data.u64);
    case MP_S8:
      return cmp_write_s8(ctx, obj->data.s8);
    case MP_S16:
      return cmp_write_s16(ctx, obj->data.s16);
    case MP_S32:
      return cmp_write_s32(ctx, obj->data.s32);
    case MP_S64:
      return cmp_write_s64(ctx, obj->data.s64);
    case MP_RAW16:
      return cmp_write_raw16_marker(ctx, obj->data.raw_size);
    case MP_RAW32:
      return cmp_write_raw32_marker(ctx, obj->data.raw_size);
    case MP_ARRAY16:
      return cmp_write_array16(ctx, obj->data.array_size);
    case MP_ARRAY32:
      return cmp_write_array32(ctx, obj->data.array_size);
    case MP_MAP16:
      return cmp_write_map16(ctx, obj->data.map_size);
    case MP_MAP32:
      return cmp_write_map32(ctx, obj->data.map_size);
    case MP_NEGATIVE_FIXNUM:
      return cmp_write_nfix(ctx, obj->data.s8);
    default:
      set_error(ctx, INVALID_TYPE_ERROR);
      return false;
  }
}

bool cmp_read_s8(struct cmp_ctx_s *ctx, int8_t *c) {
  uint8_t type_marker = 0;

  if (!read_type_marker(ctx, &type_marker))
    return false;

  if (type_marker != MP_S8) {
    set_error(ctx, INVALID_TYPE_ERROR);
    return false;
  }

  if (ctx->read(ctx, c, sizeof(int8_t)))
    return true;

  set_error(ctx, DATA_READING_ERROR);
  return false;
}

bool cmp_read_s16(struct cmp_ctx_s *ctx, int16_t *s) {
  uint8_t type_marker = 0;

  if (!read_type_marker(ctx, &type_marker))
    return false;

  if (type_marker != MP_S16) {
    set_error(ctx, INVALID_TYPE_ERROR);
    return false;
  }

  if (!ctx->read(ctx, s, sizeof(int16_t))) {
    set_error(ctx, DATA_READING_ERROR);
    return false;
  }

  *s = b16(*s);
  return true;
}

bool cmp_read_s32(struct cmp_ctx_s *ctx, int32_t *i) {
  uint8_t type_marker = 0;

  if (!read_type_marker(ctx, &type_marker))
    return false;

  if (type_marker != MP_S32) {
    set_error(ctx, INVALID_TYPE_ERROR);
    return false;
  }

  if (!ctx->read(ctx, i, sizeof(int32_t))) {
    set_error(ctx, DATA_READING_ERROR);
    return false;
  }

  *i = b32(*i);
  return true;
}

bool cmp_read_s64(struct cmp_ctx_s *ctx, int64_t *l) {
  uint8_t type_marker = 0;

  if (!read_type_marker(ctx, &type_marker))
    return false;

  if (type_marker != MP_S64) {
    set_error(ctx, INVALID_TYPE_ERROR);
    return false;
  }

  if (!ctx->read(ctx, l, sizeof(int64_t))) {
    set_error(ctx, DATA_READING_ERROR);
    return false;
  }

  *l = b64(*l);
  return true;
}

bool cmp_read_u8(struct cmp_ctx_s *ctx, uint8_t *c) {
  uint8_t type_marker = 0;

  if (!read_type_marker(ctx, &type_marker))
    return false;

  if (type_marker != MP_U8) {
    set_error(ctx, INVALID_TYPE_ERROR);
    return false;
  }

  if (ctx->read(ctx, c, sizeof(uint8_t)))
    return true;

  set_error(ctx, DATA_READING_ERROR);
  return false;
}

bool cmp_read_u16(struct cmp_ctx_s *ctx, uint16_t *s) {
  uint8_t type_marker = 0;

  if (!read_type_marker(ctx, &type_marker))
    return false;

  if (type_marker != MP_U16) {
    set_error(ctx, INVALID_TYPE_ERROR);
    return false;
  }

  if (!ctx->read(ctx, s, sizeof(uint16_t))) {
    set_error(ctx, DATA_READING_ERROR);
    return false;
  }

  *s = b16(*s);
  return true;
}

bool cmp_read_u32(struct cmp_ctx_s *ctx, uint32_t *i) {
  uint8_t type_marker = 0;

  if (!read_type_marker(ctx, &type_marker))
    return false;

  if (type_marker != MP_U32) {
    set_error(ctx, INVALID_TYPE_ERROR);
    return false;
  }

  if (!ctx->read(ctx, i, sizeof(uint32_t))) {
    set_error(ctx, DATA_READING_ERROR);
    return false;
  }

  *i = b32(*i);
  return true;
}

bool cmp_read_u64(struct cmp_ctx_s *ctx, uint64_t *l) {
  uint8_t type_marker = 0;

  if (!read_type_marker(ctx, &type_marker))
    return false;

  if (type_marker != MP_U64) {
    set_error(ctx, INVALID_TYPE_ERROR);
    return false;
  }

  if (!ctx->read(ctx, l, sizeof(uint64_t))) {
    set_error(ctx, DATA_READING_ERROR);
    return false;
  }

  *l = b64(*l);
  return true;
}

bool cmp_read_float(struct cmp_ctx_s *ctx, float *f) {
  uint8_t type_marker = 0;

  if (!read_type_marker(ctx, &type_marker))
    return false;

  if (type_marker != MP_FLOAT) {
    set_error(ctx, INVALID_TYPE_ERROR);
    return false;
  }

  if (!ctx->read(ctx, f, sizeof(float))) {
    set_error(ctx, DATA_READING_ERROR);
    return false;
  }

  *f = b32(*f);
  return true;
}

bool cmp_read_double(struct cmp_ctx_s *ctx, double *d) {
  uint8_t type_marker = 0;

  if (!read_type_marker(ctx, &type_marker))
    return false;

  if (type_marker != MP_DOUBLE) {
    set_error(ctx, INVALID_TYPE_ERROR);
    return false;
  }

  if (!ctx->read(ctx, d, sizeof(double))) {
    set_error(ctx, DATA_READING_ERROR);
    return false;
  }

  *d = b64(*d);
  return true;
}

bool cmp_read_nil(struct cmp_ctx_s *ctx) {
  uint8_t type_marker = 0;

  if (!read_type_marker(ctx, &type_marker))
    return false;

  if (type_marker != NIL_MARKER) {
    set_error(ctx, INVALID_TYPE_ERROR);
    return false;
  }

  return true;
}

bool cmp_read_bool(struct cmp_ctx_s *ctx, bool *b) {
  uint8_t type_marker = 0;

  if (!read_type_marker(ctx, &type_marker))
    return false;

  switch (type_marker) {
    case TRUE_MARKER:
      *b = true;
      return true;
    case FALSE_MARKER:
      *b = false;
      return false;
    default:
      set_error(ctx, INVALID_TYPE_ERROR);
      return false;
  }
}

bool cmp_read_bool_as_u8(struct cmp_ctx_s *ctx, uint8_t *b) {
  uint8_t type_marker = 0;

  if (!read_type_marker(ctx, &type_marker))
    return false;

  switch (type_marker) {
    case TRUE_MARKER:
      *b = 1;
      return true;
    case FALSE_MARKER:
      *b = 2;
      return false;
    default:
      set_error(ctx, INVALID_TYPE_ERROR);
      return false;
  }

  return true;
}

bool cmp_read_raw(struct cmp_ctx_s *ctx, void *data, size_t *length) {
  uint8_t type_marker = 0;
  size_t raw_size = 0;
  uint16_t raw_size_16 = 0;
  uint32_t raw_size_32 = 0;

  if (!read_type_marker(ctx, &type_marker))
    return false;

  switch (type_marker) {
    case RAW16_MARKER:
      if (!ctx->read(ctx, &raw_size_16, sizeof(uint16_t))) {
        set_error(ctx, LENGTH_READING_ERROR);
        return false;
      }
      raw_size = b16(raw_size_16);
      break;
    case RAW32_MARKER:
      if (!ctx->read(ctx, &raw_size_32, sizeof(uint32_t))) {
        set_error(ctx, LENGTH_READING_ERROR);
        return false;
      }
      raw_size = b32(raw_size_32);
      break;
    default:
      set_error(ctx, INVALID_TYPE_ERROR);
      return false;
  }

  if (!ctx->read(ctx, data, raw_size)) {
    set_error(ctx, DATA_READING_ERROR);
    return false;
  }

  *length = raw_size;
  return true;
}

bool cmp_read_array(struct cmp_ctx_s *ctx, size_t *length) {
  uint8_t type_marker = 0;
  uint16_t array_size_16 = 0;
  uint32_t array_size_32 = 0;
  size_t array_size = 0;

  if (!read_type_marker(ctx, &type_marker))
    return false;

  switch (type_marker) {
    case ARRAY16_MARKER:
      if (!ctx->read(ctx, &array_size_16, sizeof(uint16_t))) {
        set_error(ctx, LENGTH_READING_ERROR);
        return false;
      }
      array_size = b16(array_size_16);
      break;
    case ARRAY32_MARKER:
      if (!ctx->read(ctx, &array_size_32, sizeof(uint32_t))) {
        set_error(ctx, LENGTH_READING_ERROR);
        return false;
      }
      array_size = b32(array_size_32);
      break;
    default:
      set_error(ctx, INVALID_TYPE_ERROR);
      return false;
  }

  *length = array_size;
  return true;
}

bool cmp_read_map(struct cmp_ctx_s *ctx, size_t *length) {
  uint8_t type_marker = 0;
  uint16_t map_size_16 = 0;
  uint32_t map_size_32 = 0;
  size_t map_size = 0;

  if (!read_type_marker(ctx, &type_marker))
    return false;

  switch (type_marker) {
    case MAP16_MARKER:
      if (!ctx->read(ctx, &map_size_16, sizeof(uint16_t))) {
        set_error(ctx, LENGTH_READING_ERROR);
        return false;
      }
      map_size = b16(map_size_16);
      break;
    case MAP32_MARKER:
      if (!ctx->read(ctx, &map_size_32, sizeof(uint32_t))) {
        set_error(ctx, LENGTH_READING_ERROR);
        return false;
      }
      map_size = b32(map_size_32);
      break;
    default:
      set_error(ctx, INVALID_TYPE_ERROR);
      return false;
  }

  *length = map_size;
  return true;
}

bool cmp_read_object(struct cmp_ctx_s *ctx, struct cmp_object_s *obj) {
  uint8_t type_marker = 0;

  if (!read_type_marker(ctx, &type_marker))
    return false;

  if (type_marker <= 0x7F) {
    obj->type = MP_POSITIVE_FIXNUM;
    obj->data.u8 = type_marker;
  }
  else if (type_marker <= 0x8F) {
    obj->type = MP_FIXMAP;
    obj->data.map_size = type_marker & FIXMAP_SIZE;
  }
  else if (type_marker <= 0x9F) {
    obj->type = MP_FIXARRAY;
    obj->data.array_size = type_marker & FIXARRAY_SIZE;
  }
  else if (type_marker <= 0xBF) {
    obj->type = MP_FIXRAW;
    obj->data.raw_size = type_marker & FIXRAW_SIZE;
  }
  else if (type_marker == NIL_MARKER) {
    obj->type = MP_NIL;
    obj->data.u8 = 0;
  }
  else if (type_marker == FALSE_MARKER) {
    obj->type = MP_BOOLEAN;
    obj->data.boolean = false;
  }
  else if (type_marker == TRUE_MARKER) {
    obj->type = MP_BOOLEAN;
    obj->data.boolean = true;
  }
  else if (type_marker == FLOAT_MARKER) {
    obj->type = MP_FLOAT;
    if (!ctx->read(ctx, &obj->data.flt, sizeof(float))) {
      set_error(ctx, DATA_READING_ERROR);
      return false;
    }
    obj->data.flt = obj->data.flt;
  }
  else if (type_marker == DOUBLE_MARKER) {
    obj->type = MP_DOUBLE;
    if (!ctx->read(ctx, &obj->data.dbl, sizeof(double))) {
      set_error(ctx, DATA_READING_ERROR);
      return false;
    }
    obj->data.flt = obj->data.flt;
  }
  else if (type_marker == U8_MARKER) {
    obj->type = MP_U8;
    if (!ctx->read(ctx, &obj->data.u8, sizeof(uint8_t))) {
      set_error(ctx, DATA_READING_ERROR);
      return false;
    }
  }
  else if (type_marker == U16_MARKER) {
    obj->type = MP_U16;
    if (!ctx->read(ctx, &obj->data.u16, sizeof(uint16_t))) {
      set_error(ctx, DATA_READING_ERROR);
      return false;
    }
    obj->data.u16 = b16(obj->data.u16);
  }
  else if (type_marker == U32_MARKER) {
    obj->type = MP_U32;
    if (!ctx->read(ctx, &obj->data.u32, sizeof(uint32_t))) {
      set_error(ctx, DATA_READING_ERROR);
      return false;
    }
    obj->data.u32 = b32(obj->data.u32);
  }
  else if (type_marker == U64_MARKER) {
    obj->type = MP_U64;
    if (!ctx->read(ctx, &obj->data.u64, sizeof(uint64_t))) {
      set_error(ctx, DATA_READING_ERROR);
      return false;
    }
    obj->data.u64 = b64(obj->data.u64);
  }
  else if (type_marker == S8_MARKER) {
    obj->type = MP_S8;
    if (!ctx->read(ctx, &obj->data.s8, sizeof(int8_t))) {
      set_error(ctx, DATA_READING_ERROR);
      return false;
    }
  }
  else if (type_marker == S16_MARKER) {
    obj->type = MP_S16;
    if (!ctx->read(ctx, &obj->data.s16, sizeof(int16_t))) {
      set_error(ctx, DATA_READING_ERROR);
      return false;
    }
    obj->data.u16 = b16(obj->data.u16);
  }
  else if (type_marker == S32_MARKER) {
    obj->type = MP_S32;
    if (!ctx->read(ctx, &obj->data.s32, sizeof(int32_t))) {
      set_error(ctx, DATA_READING_ERROR);
      return false;
    }
    obj->data.u32 = b32(obj->data.u32);
  }
  else if (type_marker == S64_MARKER) {
    obj->type = MP_S64;
    if (!ctx->read(ctx, &obj->data.s64, sizeof(int64_t))) {
      set_error(ctx, DATA_READING_ERROR);
      return false;
    }
    obj->data.u64 = b64(obj->data.u64);
  }
  else if (type_marker == RAW16_MARKER) {
    obj->type = MP_RAW16;
    if (!ctx->read(ctx, &obj->data.u16, sizeof(uint16_t))) {
      set_error(ctx, DATA_READING_ERROR);
      return false;
    }
    obj->data.raw_size = b16(obj->data.u16);
  }
  else if (type_marker == RAW32_MARKER) {
    obj->type = MP_RAW32;
    if (!ctx->read(ctx, &obj->data.u32, sizeof(uint32_t))) {
      set_error(ctx, DATA_READING_ERROR);
      return false;
    }
    obj->data.raw_size = b32(obj->data.u32);
  }
  else if (type_marker == ARRAY16_MARKER) {
    obj->type = MP_ARRAY16;
    if (!ctx->read(ctx, &obj->data.u16, sizeof(uint16_t))) {
      set_error(ctx, DATA_READING_ERROR);
      return false;
    }
    obj->data.raw_size = b16(obj->data.u16);
  }
  else if (type_marker == ARRAY32_MARKER) {
    obj->type = MP_ARRAY32;
    if (!ctx->read(ctx, &obj->data.u32, sizeof(uint32_t))) {
      set_error(ctx, DATA_READING_ERROR);
      return false;
    }
    obj->data.raw_size = b32(obj->data.u32);
  }
  else if (type_marker == MAP16_MARKER) {
    obj->type = MP_MAP16;
    if (!ctx->read(ctx, &obj->data.u16, sizeof(uint16_t))) {
      set_error(ctx, DATA_READING_ERROR);
      return false;
    }
    obj->data.raw_size = b16(obj->data.u16);
  }
  else if (type_marker == MAP32_MARKER) {
    obj->type = MP_MAP32;
    if (!ctx->read(ctx, &obj->data.u32, sizeof(uint32_t))) {
      set_error(ctx, DATA_READING_ERROR);
      return false;
    }
    obj->data.raw_size = b32(obj->data.u32);
  }
  else if (type_marker >= NEGATIVE_FIXNUM_MARKER) {
    obj->type = MP_NEGATIVE_FIXNUM;
    obj->data.s8 = type_marker & NEGATIVE_FIXNUM_SIZE;
  }
  else {
    set_error(ctx, INVALID_TYPE_ERROR);
    return false;
  }

  return true;
}

/* vi: set et ts=2 sw=2: */

