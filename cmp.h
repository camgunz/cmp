struct cmp_ctx_s;

typedef bool   (*cmp_reader)(struct cmp_ctx_s *ctx, void *data, size_t limit);
typedef size_t (*cmp_writer)(
  struct cmp_ctx_s *ctx, const void *data, size_t count
);

enum {
  CMP_TYPE_POSITIVE_FIXNUM, /*  0 */
  CMP_TYPE_FIXMAP,          /*  1 */
  CMP_TYPE_FIXARRAY,        /*  2 */
  CMP_TYPE_FIXSTR,          /*  3 */
  CMP_TYPE_NIL,             /*  4 */
  CMP_TYPE_BOOLEAN,         /*  5 */
  CMP_TYPE_BIN8,            /*  6 */
  CMP_TYPE_BIN16,           /*  7 */
  CMP_TYPE_BIN32,           /*  8 */
  CMP_TYPE_EXT8,            /*  9 */
  CMP_TYPE_EXT16,           /* 10 */
  CMP_TYPE_EXT32,           /* 11 */
  CMP_TYPE_FLOAT,           /* 12 */
  CMP_TYPE_DOUBLE,          /* 13 */
  CMP_TYPE_UINT8,           /* 14 */
  CMP_TYPE_UINT16,          /* 15 */
  CMP_TYPE_UINT32,          /* 16 */
  CMP_TYPE_UINT64,          /* 17 */
  CMP_TYPE_SINT8,           /* 18 */
  CMP_TYPE_SINT16,          /* 19 */
  CMP_TYPE_SINT32,          /* 20 */
  CMP_TYPE_SINT64,          /* 21 */
  CMP_TYPE_FIXEXT1,         /* 22 */
  CMP_TYPE_FIXEXT2,         /* 23 */
  CMP_TYPE_FIXEXT4,         /* 24 */
  CMP_TYPE_FIXEXT8,         /* 25 */
  CMP_TYPE_FIXEXT16,        /* 26 */
  CMP_TYPE_STR8,            /* 27 */
  CMP_TYPE_STR16,           /* 28 */
  CMP_TYPE_STR32,           /* 29 */
  CMP_TYPE_ARRAY16,         /* 30 */
  CMP_TYPE_ARRAY32,         /* 31 */
  CMP_TYPE_MAP16,           /* 32 */
  CMP_TYPE_MAP32,           /* 33 */
  CMP_TYPE_NEGATIVE_FIXNUM  /* 34 */
};

typedef struct cmp_ext_s {
  int8_t type;
  uint64_t size;
} cmp_ext_t;

union cmp_object_data_u {
  bool      boolean;
  uint8_t   u8;
  uint16_t  u16;
  uint32_t  u32;
  uint64_t  u64;
  int8_t    s8;
  int16_t   s16;
  int32_t   s32;
  int64_t   s64;
  float     flt;
  double    dbl;
  uint64_t  array_size;
  uint64_t  map_size;
  uint64_t  str_size;
  uint64_t  bin_size;
  cmp_ext_t ext;
};

typedef struct cmp_ctx_s {
  uint8_t     error;
  void       *buf;
  cmp_reader  read;
  cmp_writer  write;
} cmp_ctx_t;

typedef struct cmp_object_s {
  uint8_t type;
  union cmp_object_data_u as;
} cmp_object_t;

void cmp_init(cmp_ctx_t *ctx, void *buf, cmp_reader read, cmp_writer write);

uint32_t    cmp_version(void);
const char* cmp_strerror(cmp_ctx_t *ctx);

bool cmp_write_pfix(cmp_ctx_t *ctx, uint8_t c);
bool cmp_write_nfix(cmp_ctx_t *ctx, int8_t c);

bool cmp_write_sfix(cmp_ctx_t *ctx, int8_t c);
bool cmp_write_s8(cmp_ctx_t *ctx, int8_t c);
bool cmp_write_s16(cmp_ctx_t *ctx, int16_t s);
bool cmp_write_s32(cmp_ctx_t *ctx, int32_t i);
bool cmp_write_s64(cmp_ctx_t *ctx, int64_t l);
bool cmp_write_sint(cmp_ctx_t *ctx, int64_t d);

bool cmp_write_ufix(cmp_ctx_t *ctx, uint8_t c);
bool cmp_write_u8(cmp_ctx_t *ctx, uint8_t c);
bool cmp_write_u16(cmp_ctx_t *ctx, uint16_t s);
bool cmp_write_u32(cmp_ctx_t *ctx, uint32_t i);
bool cmp_write_u64(cmp_ctx_t *ctx, uint64_t l);
bool cmp_write_uint(cmp_ctx_t *ctx, uint64_t u);

bool cmp_write_float(cmp_ctx_t *ctx, float f);
bool cmp_write_double(cmp_ctx_t *ctx, double d);

bool cmp_write_nil(cmp_ctx_t *ctx);
bool cmp_write_true(cmp_ctx_t *ctx);
bool cmp_write_false(cmp_ctx_t *ctx);

bool cmp_write_bin8_marker(cmp_ctx_t *ctx, uint8_t size);
bool cmp_write_bin8(cmp_ctx_t *ctx, const void *data, uint8_t size);
bool cmp_write_bin16_marker(cmp_ctx_t *ctx, uint16_t size);
bool cmp_write_bin16(cmp_ctx_t *ctx, const void *data, uint16_t size);
bool cmp_write_bin32_marker(cmp_ctx_t *ctx, uint32_t size);
bool cmp_write_bin32(cmp_ctx_t *ctx, const void *data, uint32_t size);
bool cmp_write_bin(cmp_ctx_t *ctx, const void *data, uint32_t size);

bool cmp_write_fixstr_marker(cmp_ctx_t *ctx, uint8_t size);
bool cmp_write_fixstr(cmp_ctx_t *ctx, const void *data, uint8_t size);
bool cmp_write_str8_marker(cmp_ctx_t *ctx, uint8_t size);
bool cmp_write_str8(cmp_ctx_t *ctx, const void *data, uint8_t size);
bool cmp_write_str16_marker(cmp_ctx_t *ctx, uint16_t size);
bool cmp_write_str16(cmp_ctx_t *ctx, const void *data, uint16_t size);
bool cmp_write_str32_marker(cmp_ctx_t *ctx, uint32_t size);
bool cmp_write_str32(cmp_ctx_t *ctx, const void *data, uint32_t size);
bool cmp_write_str(cmp_ctx_t *ctx, const void *data, uint32_t size);

bool cmp_write_fixarray(cmp_ctx_t *ctx, uint8_t size);
bool cmp_write_array16(cmp_ctx_t *ctx, uint16_t size);
bool cmp_write_array32(cmp_ctx_t *ctx, uint32_t size);
bool cmp_write_array(cmp_ctx_t *ctx, uint32_t size);

bool cmp_write_fixmap(cmp_ctx_t *ctx, uint8_t size);
bool cmp_write_map16(cmp_ctx_t *ctx, uint16_t size);
bool cmp_write_map32(cmp_ctx_t *ctx, uint32_t size);
bool cmp_write_map(cmp_ctx_t *ctx, uint32_t size);

bool cmp_write_fixext1_marker(cmp_ctx_t *ctx, int8_t type);
bool cmp_write_fixext1(cmp_ctx_t *ctx, int8_t type, void *data);
bool cmp_write_fixext2_marker(cmp_ctx_t *ctx, int8_t type);
bool cmp_write_fixext2(cmp_ctx_t *ctx, int8_t type, void *data);
bool cmp_write_fixext4_marker(cmp_ctx_t *ctx, int8_t type);
bool cmp_write_fixext4(cmp_ctx_t *ctx, int8_t type, void *data);
bool cmp_write_fixext8_marker(cmp_ctx_t *ctx, int8_t type);
bool cmp_write_fixext8(cmp_ctx_t *ctx, int8_t type, void *data);
bool cmp_write_fixext16_marker(cmp_ctx_t *ctx, int8_t type);
bool cmp_write_fixext16(cmp_ctx_t *ctx, int8_t type, void *data);

bool cmp_write_ext8_marker(cmp_ctx_t *ctx, uint8_t size, int8_t type);
bool cmp_write_ext8(cmp_ctx_t *ctx, uint8_t size, int8_t type, void *data);
bool cmp_write_ext16_marker(cmp_ctx_t *ctx, uint16_t size, int8_t type);
bool cmp_write_ext16(cmp_ctx_t *ctx, uint16_t size, int8_t type, void *data);
bool cmp_write_ext32_marker(cmp_ctx_t *ctx, uint32_t size, int8_t type);
bool cmp_write_ext32(cmp_ctx_t *ctx, uint32_t size, int8_t type, void *data);
bool cmp_write_ext(cmp_ctx_t *ctx, uint32_t size, int8_t type, void *data);

bool cmp_write_object(cmp_ctx_t *ctx, cmp_object_t *obj);

bool cmp_read_pfix(cmp_ctx_t *ctx, uint8_t *c);
bool cmp_read_nfix(cmp_ctx_t *ctx, int8_t *c);

bool cmp_read_sfix(cmp_ctx_t *ctx, int8_t *c);
bool cmp_read_s8(cmp_ctx_t *ctx, int8_t *c);
bool cmp_read_s16(cmp_ctx_t *ctx, int16_t *s);
bool cmp_read_s32(cmp_ctx_t *ctx, int32_t *i);
bool cmp_read_s64(cmp_ctx_t *ctx, int64_t *l);
bool cmp_read_sint(cmp_ctx_t *ctx, int64_t *l);

bool cmp_read_ufix(cmp_ctx_t *ctx, uint8_t *c);
bool cmp_read_u8(cmp_ctx_t *ctx, uint8_t *c);
bool cmp_read_u16(cmp_ctx_t *ctx, uint16_t *s);
bool cmp_read_u32(cmp_ctx_t *ctx, uint32_t *i);
bool cmp_read_u64(cmp_ctx_t *ctx, uint64_t *l);

bool cmp_read_float(cmp_ctx_t *ctx, float *f);
bool cmp_read_double(cmp_ctx_t *ctx, double *d);

bool cmp_read_nil(cmp_ctx_t *ctx);
bool cmp_read_bool(cmp_ctx_t *ctx, bool *b);
bool cmp_read_bool_as_u8(cmp_ctx_t *ctx, uint8_t *b);

bool cmp_read_str_size(cmp_ctx_t *ctx, uint32_t *size);
bool cmp_read_str(cmp_ctx_t *ctx, void *data, uint32_t *size);

bool cmp_read_bin_size(cmp_ctx_t *ctx, uint32_t *size);
bool cmp_read_bin(cmp_ctx_t *ctx, void *data, uint32_t *size);

bool cmp_read_array(cmp_ctx_t *ctx, uint32_t *size);

bool cmp_read_map(cmp_ctx_t *ctx, uint32_t *size);

bool cmp_read_ext_size(cmp_ctx_t *ctx, uint32_t *size);
bool cmp_read_ext_type(cmp_ctx_t *ctx, int8_t *type);
bool cmp_read_ext(cmp_ctx_t *ctx, uint32_t *size, int8_t *type);

bool cmp_read_object(cmp_ctx_t *ctx, cmp_object_t *obj);

/* vi: set et ts=2 sw=2: */

