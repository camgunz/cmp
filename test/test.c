#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "buf.h"

#include "cmp.h"

#ifdef __GNUC__
static void error_printf(const char *msg, ...)
  __attribute__ ((format (printf, 1, 2)));
#else
static void error_printf(const char *msg, ...);
#endif

#define run_tests(t)                                                          \
  printf(#t " test: ");                                                       \
  if (!run_ ## t ## _tests()) {                                               \
    M_BufferSeek(&error_message, 0);                                          \
    printf("-- FAILED --\n");                                                 \
    printf("\t %s\n", M_BufferGetData(&error_message));                       \
    exit(EXIT_FAILURE);                                                       \
  }                                                                           \
  else {                                                                      \
    printf("passed\n");                                                       \
  }

#define test_format(wfunc, rfunc, otype, ctype, in, data, dlen)               \
  M_BufferClear(&buf);                                                        \
  error_clear();                                                              \
  if (!wfunc(&cmp, in)) {                                                     \
    error_printf("%s:%d ", __func__, __LINE__);                               \
    error_printf("%s(&cmp, %s) failed: %s\n",                                 \
      #wfunc, #in, cmp_strerror(&cmp)                                         \
    );                                                                        \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!M_BufferEqualsData(&buf, data, dlen)) {                                \
    error_printf("%s:%d ", __func__, __LINE__);                               \
    error_printf("%s(&cmp, %s) wrote invalid MessagePack data\n\n",           \
      #wfunc, #in                                                             \
    );                                                                        \
    error_printbin(data, dlen);                                               \
    error_printbin(M_BufferGetData(&buf), M_BufferGetSize(&buf));             \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!cmp_read_object(&cmp, &obj)) {                                         \
    error_printf("%s:%d ", __func__, __LINE__);                               \
    error_printf("Error reading object written by %s(&cmp, %s): %s\n",        \
      #wfunc, #in, cmp_strerror(&cmp)                                         \
    );                                                                        \
    return false;                                                             \
  }                                                                           \
  if (obj.as.otype != in) {                                                   \
    error_printf("%s:%d ", __func__, __LINE__);                               \
    error_printf("Input/Output mismatch: %s(&cmp, %s) != ",                   \
      #wfunc, #in                                                             \
    );                                                                        \
    error_print_object(&obj);                                                 \
    error_printf("\n");                                                       \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  do {                                                                        \
    ctype value;                                                              \
    if (!rfunc(&cmp, (ctype *)&value)) {                                      \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Error reading object written by %s(&cmp, %s): %s\n",      \
        #wfunc, #in, cmp_strerror(&cmp)                                       \
      );                                                                      \
      return false;                                                           \
    }                                                                         \
    if (in != value) {                                                        \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Input/Output mismatch: %s(&cmp, %s) != ",                 \
        #wfunc, #in                                                           \
      );                                                                      \
      if (strcmp(#ctype, "uint64_t") == 0)                                    \
        error_printf("%"PRIu64, (uint64_t)value);                             \
      else if (strcmp(#ctype, "int64_t") == 0)                                \
        error_printf("%"PRId64, (int64_t)value);                              \
      else if (strcmp(#ctype, "float") == 0)                                  \
        error_printf("%f", (float)value);                                     \
      else if (strcmp(#ctype, "double") == 0)                                 \
        error_printf("%f", (double)value);                                    \
      else if (strcmp(#ctype, "bool") == 0)                                   \
        error_printf("%d", (bool)value);                                      \
      else                                                                    \
        error_and_exit("Invalid ctype passed to test_format\n");              \
      error_printf("\n");                                                     \
      return false;                                                           \
    }                                                                         \
  } while (0);

#define test_format_with_length(wfunc, rfunc, otype, in, len, data, dlen)     \
  M_BufferClear(&buf);                                                        \
  error_clear();                                                              \
  if (!wfunc(&cmp, in, len)) {                                                \
    error_printf("%s:%d ", __func__, __LINE__);                               \
    error_printf("%s(&cmp, ", #wfunc);                                        \
    error_printbin(in, len);                                                  \
    error_printf(", %d) failed: %s\n", len, cmp_strerror(&cmp));              \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!M_BufferEqualsData(&buf, data, dlen)) {                                \
    error_printf("%s:%d ", __func__, __LINE__);                               \
    error_printf("%s(&cmp, ", #wfunc);                                        \
    error_printbin(in, len);                                                  \
    error_printf(", %d) wrote invalid MessagePack data.\n", len);             \
    error_printbin(data, dlen);                                               \
    error_printbin(M_BufferGetData(&buf), M_BufferGetSize(&buf));             \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!cmp_read_object(&cmp, &obj)) {                                         \
    error_printf("%s:%d ", __func__, __LINE__);                               \
    error_printf("Error reading object written by %s(&cmp, ", #wfunc);        \
    error_printbin(in, len);                                                  \
    error_printf(", %d): %s\n", len, cmp_strerror(&cmp));                     \
    return false;                                                             \
  }                                                                           \
  if (obj.as.otype != len) {                                                  \
    error_printf("%s:%d ", __func__, __LINE__);                               \
    error_printf("Input/Output mismatch: %s(&cmp, ", #wfunc);                 \
    error_printbin(in, len);                                                  \
    error_printf(", %d) != ", len);                                           \
    error_print_object(&obj);                                                 \
    error_printf("\n");                                                       \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  do {                                                                        \
    char ldata[len + 1];                                                      \
    uint32_t data_length = len + 1;                                           \
    memset(ldata, 0, sizeof(ldata));                                          \
    if (!rfunc(&cmp, ldata, &data_length)) {                                  \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Error reading object written by %s(&cmp, ", #wfunc);      \
      error_printbin(in, len);                                                \
      error_printf(", %d): %s\n", len, cmp_strerror(&cmp));                   \
      return false;                                                           \
    }                                                                         \
    if (data_length != len) {                                                 \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Error reading object written by %s(&cmp, ", #wfunc);      \
      error_printbin(in, len);                                                \
      error_printf(", %d): %u != %d\n", len, data_length, len);               \
    }                                                                         \
    if (memcmp(ldata, in, len) != 0) {                                        \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Input/Output mismatch: %s(&cmp, [ ", #wfunc);             \
      for (int i = 0; i < len; i++)                                           \
        error_printf(" %02X", data[i]);                                       \
      error_printf(" ], %s, %d) != [", #in, len);                             \
      for (int i = 0; i < len; i++)                                           \
        error_printf(" %02X", ldata[i]);                                      \
      error_printf(" ]\n");                                                   \
      return false;                                                           \
    }                                                                         \
  } while (0);

#define test_format_no_input(wfunc, otype, data, dlen, out)                   \
  M_BufferClear(&buf);                                                        \
  error_clear();                                                              \
  if (!wfunc(&cmp)) {                                                         \
    error_printf("%s(&cmp) failed: %s\n", #wfunc, cmp_strerror(&cmp));        \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!M_BufferEqualsData(&buf, data, dlen)) {                                \
    error_printf("%s(&cmp) wrote invalid MessagePack data\n\n", #wfunc);      \
    error_printbin(data, dlen);                                               \
    error_printbin(M_BufferGetData(&buf), M_BufferGetSize(&buf));             \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!cmp_read_object(&cmp, &obj)) {                                         \
    error_printf("Error reading object written by %s(&cmp): %s\n",            \
      #wfunc, cmp_strerror(&cmp)                                              \
    );                                                                        \
    return false;                                                             \
  }                                                                           \
  if (obj.as.otype != out) {                                                  \
    error_printf("Input/Output mismatch: %s(&cmp) != %s\n",                   \
      #wfunc, #out                                                            \
    );                                                                        \
    error_print_object(&obj);                                                 \
    error_printf("\n");                                                       \
    return false;                                                             \
  }

#define test_fixext_format(wfunc, etype, esize, in, data, dlen)               \
  M_BufferClear(&buf);                                                        \
  error_clear();                                                              \
  if (!wfunc(&cmp, etype, in)) {                                              \
    error_printf("%s(&cmp, %d, ", #wfunc, etype);                             \
    error_printbin(in, esize);                                                \
    error_printf(") failed: %s\n", cmp_strerror(&cmp));                       \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!M_BufferEqualsData(&buf, data, dlen)) {                                \
    error_printf("%s(&cmp, %d, ", #wfunc, etype);                             \
    error_printbin(in, esize);                                                \
    error_printf(") wrote invalid MessagePack data.\n");                      \
    error_printbin(data, dlen);                                               \
    error_printbin(M_BufferGetData(&buf), M_BufferGetSize(&buf));             \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!cmp_read_object(&cmp, &obj)) {                                         \
    error_printf("Error reading object written by %s(&cmp, %d, ",             \
      #wfunc, etype                                                           \
    );                                                                        \
    error_printbin(in, esize);                                                \
    error_printf("): %s\n", cmp_strerror(&cmp));                              \
    return false;                                                             \
  }                                                                           \
  if (obj.as.ext.type != etype || obj.as.ext.size != esize) {                 \
    error_printf("Input/Output mismatch: %s(&cmp, %d, ",                      \
      #wfunc, etype                                                           \
    );                                                                        \
    error_printbin(in, esize);                                                \
    error_printf(") != {%d, %u}\n", obj.as.ext.type, obj.as.ext.size);        \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  do {                                                                        \
    char edata[esize];                                                        \
    int8_t dummy_type = etype;                                                \
    uint32_t dummy_size = esize;                                              \
    memset(edata, 0, sizeof(edata));                                          \
    if (!cmp_read_ext(&cmp, &dummy_type, &dummy_size, edata)) {               \
      error_printf("Error reading object written by %s(&cmp, %d, ",           \
        #wfunc, etype                                                         \
      );                                                                      \
      error_printbin(in, esize);                                              \
      error_printf("): %s\n", cmp_strerror(&cmp));                            \
      return false;                                                           \
    }                                                                         \
    if (dummy_type != etype) {                                                \
      error_printf("Error reading object written by %s(&cmp, %d, %u, ",       \
        #wfunc, etype, esize                                                  \
      );                                                                      \
      error_printbin(in, esize);                                              \
      error_printf("): %d != %d.\n", dummy_type, etype);                      \
    }                                                                         \
    if (dummy_size != esize) {                                                \
      error_printf("Error reading object written by %s(&cmp, %d, %u, ",       \
        #wfunc, etype, esize                                                  \
      );                                                                      \
      error_printbin(in, esize);                                              \
      error_printf("): %u != %u.\n", dummy_size, esize);                      \
    }                                                                         \
    if (memcmp(edata, in, esize) != 0) {                                      \
      error_printf("Input/Output mismatch: %s(&cmp, %d, ",                    \
        #wfunc, etype                                                         \
      );                                                                      \
      error_printbin(in, esize);                                              \
      error_printf(") != {%d, %u}\n", obj.as.ext.type, obj.as.ext.size);      \
      return false;                                                           \
    }                                                                         \
  } while (0);

#define test_ext_format(wfunc, etype, esize, in, data, dlen)                  \
  M_BufferClear(&buf);                                                        \
  error_clear();                                                              \
  if (!wfunc(&cmp, etype, esize, in)) {                                       \
    error_printf("%s(&cmp, %d, %u, ", #wfunc, etype, esize);                  \
    error_printbin(in, esize);                                                \
    error_printf(")failed: %s\n", cmp_strerror(&cmp));                        \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!M_BufferEqualsData(&buf, data, dlen)) {                                \
    error_printf("%s(&cmp, %d, %u, ", #wfunc, etype, esize);                  \
    error_printbin(in, esize);                                                \
    error_printf(") wrote invalid MessagePack data.\n");                      \
    error_printbin(data, dlen);                                               \
    error_printf("\n");                                                       \
    error_printbin(M_BufferGetData(&buf), M_BufferGetSize(&buf));             \
    error_printf("\n");                                                       \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  if (!cmp_read_object(&cmp, &obj)) {                                         \
    error_printf("Error reading object written by %s(&cmp, %d, %u, ",         \
      #wfunc, etype, esize                                                    \
    );                                                                        \
    error_printbin(in, esize);                                                \
    error_printf("): %s\n", cmp_strerror(&cmp));                              \
    return false;                                                             \
  }                                                                           \
  if (obj.as.ext.type != etype || obj.as.ext.size != esize) {                 \
    error_printf("Input/Output mismatch: %s(&cmp, %d, %u, ",                  \
      #wfunc, etype, esize                                                    \
    );                                                                        \
    error_printbin(in, esize);                                                \
    error_printf(") != {%d, %u}\n", obj.as.ext.type, obj.as.ext.size);        \
    return false;                                                             \
  }                                                                           \
  M_BufferSeek(&buf, 0);                                                      \
  do {                                                                        \
    char edata[esize];                                                        \
    int8_t dummy_type = etype;                                                \
    uint32_t dummy_size = esize;                                              \
    if (!cmp_read_ext(&cmp, &dummy_type, &dummy_size, edata)) {               \
      error_printf("Error reading object written by %s(&cmp, %d, %u, ",       \
        #wfunc, etype, esize                                                  \
      );                                                                      \
      error_printbin(in, esize);                                              \
      error_printf("): %s\n", cmp_strerror(&cmp));                            \
      return false;                                                           \
    }                                                                         \
    if (dummy_type != etype) {                                                \
      error_printf("Error reading object written by %s(&cmp, %d, %u, ",       \
        #wfunc, etype, esize                                                  \
      );                                                                      \
      error_printbin(in, esize);                                              \
      error_printf("): %d != %d.\n", dummy_type, etype);                      \
    }                                                                         \
    if (dummy_size != esize) {                                                \
      error_printf("Error reading object written by %s(&cmp, %d, %u, ",       \
        #wfunc, etype, esize                                                  \
      );                                                                      \
      error_printbin(in, esize);                                              \
      error_printf("): %u != %u.\n", dummy_size, esize);                      \
    }                                                                         \
    if (memcmp(edata, in, esize) != 0) {                                      \
      error_printf("Input/Output mismatch: %s(&cmp, %d, %u, ",                \
        #wfunc, etype, esize                                                  \
      );                                                                      \
      error_printbin(in, esize);                                              \
      error_printf(") != {%d, %u}\n", obj.as.ext.type, obj.as.ext.size);      \
      return false;                                                           \
    }                                                                         \
  } while (0);

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

#define obj_test(func, as_func, type, ctype, val)                             \
  do {                                                                        \
    ctype var;                                                                \
    if (!func(&obj)) {                                                        \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Object type is not %s\n", type);                          \
      return false;                                                           \
    }                                                                         \
    if (!as_func(&obj, &var)) {                                               \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Error reading object as %s: %s\n",                        \
        #type, cmp_strerror(&cmp)                                             \
      );                                                                      \
      return false;                                                           \
    }                                                                         \
    if (var != val) {                                                         \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Value was not " #val " \n");                              \
      return false;                                                           \
    }                                                                         \
  } while (0);

#define obj_str_test(val)                                                     \
  do {                                                                        \
    uint32_t length;                                                          \
    if (!cmp_object_is_str(&obj)) {                                           \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Object type is not string\n");                            \
      return false;                                                           \
    }                                                                         \
    if (!cmp_object_as_str(&obj, &length)) {                                  \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Error reading object as string: %s\n",                    \
        cmp_strerror(&cmp)                                                    \
      );                                                                      \
      return false;                                                           \
    }                                                                         \
    if (strcmp(M_BufferGetDataAtCursor(&buf), val) != 0) {                    \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf(                                                           \
        "Value was not " val " (%s)\n", M_BufferGetDataAtCursor(&buf)         \
      );                                                                      \
      return false;                                                           \
    }                                                                         \
  } while (0);

#define obj_bin_test(val, inlength)                                           \
  do {                                                                        \
    uint32_t length;                                                          \
    if (!cmp_object_is_bin(&obj)) {                                           \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Object type is not binary\n");                            \
      return false;                                                           \
    }                                                                         \
    if (!cmp_object_as_bin(&obj, &length)) {                                  \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Error reading object as binary: %s\n",                    \
        cmp_strerror(&cmp)                                                    \
      );                                                                      \
      return false;                                                           \
    }                                                                         \
    if (length != inlength) {                                                 \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Mismatched lengths: %d != %d\n", inlength, length);       \
      return false;                                                           \
    }                                                                         \
    if (memcmp(M_BufferGetDataAtCursor(&buf), val, inlength) != 0) {          \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf(                                                           \
        "Value was not " val " (%s)\n", M_BufferGetDataAtCursor(&buf)         \
      );                                                                      \
      return false;                                                           \
    }                                                                         \
  } while (0);

#define obj_array_test(val1, val2)                                            \
  do {                                                                        \
    uint32_t length;                                                          \
    uint64_t var1;                                                            \
    uint64_t var2;                                                            \
    if (!cmp_object_is_array(&obj)) {                                         \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Object type is not array\n");                             \
      return false;                                                           \
    }                                                                         \
    if (!cmp_object_as_array(&obj, &length)) {                                \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Error reading object as array: %s\n",                     \
        cmp_strerror(&cmp)                                                    \
      );                                                                      \
      return false;                                                           \
    }                                                                         \
    if (length != 2) {                                                        \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Array length was not 2\n");                               \
      return false;                                                           \
    }                                                                         \
    if (!cmp_read_uinteger(&cmp, &var1)) {                                    \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Error reading first array value\n");                      \
      return false;                                                           \
    }                                                                         \
    if (!cmp_read_uinteger(&cmp, &var2)) {                                    \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Error reading second array value\n");                     \
      return false;                                                           \
    }                                                                         \
    if (var1 != val1) {                                                       \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("First array value != %d (%" PRIu64 ")\n", val1, var1);           \
      return false;                                                           \
    }                                                                         \
    if (var2 != val2) {                                                       \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Second array value != %d (%" PRIu64 ")\n", val2, var2);          \
      return false;                                                           \
    }                                                                         \
  } while (0);

#define obj_map_test(inkey, invalue)                                          \
  do {                                                                        \
    uint32_t length;                                                          \
    uint64_t key;                                                             \
    uint64_t value;                                                           \
    if (!cmp_object_is_map(&obj)) {                                           \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Object type is not map\n");                               \
      return false;                                                           \
    }                                                                         \
    if (!cmp_object_as_map(&obj, &length)) {                                  \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Error reading object as map: %s\n",                       \
        cmp_strerror(&cmp)                                                    \
      );                                                                      \
      return false;                                                           \
    }                                                                         \
    if (length != 1) {                                                        \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Map length was not 1\n");                                 \
      return false;                                                           \
    }                                                                         \
    if (!cmp_read_uinteger(&cmp, &key)) {                                     \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Error reading key\n");                                    \
      return false;                                                           \
    }                                                                         \
    if (!cmp_read_uinteger(&cmp, &value)) {                                   \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Error reading value\n");                                  \
      return false;                                                           \
    }                                                                         \
    if (key != inkey) {                                                       \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Key != %d\n", inkey);                                     \
      return false;                                                           \
    }                                                                         \
    if (value != invalue) {                                                   \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Key != %d\n", invalue);                                   \
      return false;                                                           \
    }                                                                         \
  } while (0);

#define obj_ext_test(func, as_func, type, ctype, inkey, invalue)              \
  do {                                                                        \
    uint32_t length;                                                          \
    uint32_t key;                                                             \
    uint32_t value;                                                           \
    if (!cmp_object_is_map(&obj)) {                                           \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Object type is not map\n");                               \
      return false;                                                           \
    }                                                                         \
    if (!cmp_object_as_map(&obj, &length)) {                                  \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Error reading object as map: %s\n",                       \
        cmp_strerror(&cmp)                                                    \
      );                                                                      \
      return false;                                                           \
    }                                                                         \
    if (length != 1) {                                                        \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Map length was not 1\n");                                 \
      return false;                                                           \
    }                                                                         \
    if (!cmp_read_uinteger(&cmp, &key)) {                                     \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Error reading key\n");              key                   \
      return false;                                                           \
    }                                                                         \
    if (!cmp_read_uinteger(&cmp, &value)) {                                   \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Error reading value\n");                                  \
      return false;                                                           \
    }                                                                         \
    if (key != inkey) {                                                       \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Key != %s\n", inkey);                                     \
      return false;                                                           \
    }                                                                         \
    if (value != invalue) {                                                   \
      error_printf("%s:%d ", __func__, __LINE__);                             \
      error_printf("Key != %s\n", invalue);                                   \
      return false;                                                           \
    }                                                                         \
  } while (0);

#define obj_test_no_read(func, type)                                          \
  if (!func(&obj)) {                                                          \
    error_printf("%s:%d ", __func__, __LINE__);                               \
    error_printf("Object type is not %s\n", type);                            \
    return false;                                                             \
  }                                                                           \

#define obj_test_not(func, type)                                              \
  if (func(&obj)) {                                                           \
    error_printf("%s:%d ", __func__, __LINE__);                               \
    error_printf("Object type should not be %s\n", type);                     \
    return false;                                                             \
  }                                                                           \

static buf_t error_message;

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
  M_BufferInitWithCapacity(buf, 32);
  cmp_init(cmp, buf, &buf_reader, &buf_writer);
}

static void error_clear(void) {
  M_BufferClear(&error_message);
}

static void error_printf(const char *msg, ...) {
  size_t len = 0;
  va_list args, print_args, len_test_args;

  va_start(args, msg);

  va_copy(len_test_args, args);
  va_copy(print_args, args);
  len = vsnprintf(NULL, 0, msg, len_test_args) + 1;
  M_BufferEnsureCapacity(&error_message, len);

  vfprintf(stderr, msg, print_args);

  vsnprintf(M_BufferGetData(&error_message), len, msg, args);

  va_end(args);
}

static void error_printbin(const char *data, size_t size) {
  size_t i;

  printf("[ ");
  for (i = 0; i < size; i++)
    error_printf(" %02X", data[i]);
  printf(" ]");
}

static void error_print_object(cmp_object_t *obj) {
  switch (obj->type) {
    case CMP_TYPE_POSITIVE_FIXNUM:
      error_printf("positive fixint: %u", obj->as.u8);
      break;
    case CMP_TYPE_NEGATIVE_FIXNUM:
      error_printf("negative fixnum: %d", obj->as.s8);
      break;
    case CMP_TYPE_FIXMAP:
      error_printf("fixmap: %u", obj->as.bin_size);
      break;
    case CMP_TYPE_MAP16:
      error_printf("map16: %u", obj->as.bin_size);
      break;
    case CMP_TYPE_MAP32:
      error_printf("map32: %u", obj->as.bin_size);
      break;
    case CMP_TYPE_FIXARRAY:
      error_printf("fixarray: %u", obj->as.bin_size);
      break;
    case CMP_TYPE_ARRAY16:
      error_printf("array16: %u", obj->as.bin_size);
      break;
    case CMP_TYPE_ARRAY32:
      error_printf("array32: %u", obj->as.bin_size);
      break;
    case CMP_TYPE_FIXSTR:
      error_printf("fixstr: %u", obj->as.bin_size);
      break;
    case CMP_TYPE_STR8:
      error_printf("str8: %u", obj->as.bin_size);
      break;
    case CMP_TYPE_STR16:
      error_printf("str16: %u", obj->as.bin_size);
      break;
    case CMP_TYPE_STR32:
      error_printf("str32: %u", obj->as.bin_size);
      break;
    case CMP_TYPE_BIN8:
      error_printf("bin8: %u", obj->as.bin_size);
      break;
    case CMP_TYPE_BIN16:
      error_printf("bin16: %u", obj->as.bin_size);
      break;
    case CMP_TYPE_BIN32:
      error_printf("bin32: %u", obj->as.bin_size);
      break;
    case CMP_TYPE_NIL:
      error_printf("NULL");
      break;
    case CMP_TYPE_BOOLEAN:
      if (obj->as.boolean)
        error_printf("bool: true");
      else
        error_printf("bool: false");
      break;
    case CMP_TYPE_EXT8:
      error_printf("ext8: {%d, %u}", obj->as.ext.type, obj->as.ext.size);
      break;
    case CMP_TYPE_EXT16:
      error_printf("ext16: {%d, %u}", obj->as.ext.type, obj->as.ext.size);
      break;
    case CMP_TYPE_EXT32:
      error_printf("ext32: {%d, %u}", obj->as.ext.type, obj->as.ext.size);
      break;
    case CMP_TYPE_FIXEXT1:
      error_printf("fixext1: {%d, %u}", obj->as.ext.type, obj->as.ext.size);
      break;
    case CMP_TYPE_FIXEXT2:
      error_printf("fixext2: {%d, %u}", obj->as.ext.type, obj->as.ext.size);
      break;
    case CMP_TYPE_FIXEXT4:
      error_printf("fixext4: {%d, %u}", obj->as.ext.type, obj->as.ext.size);
      break;
    case CMP_TYPE_FIXEXT8:
      error_printf("fixext8: {%d, %u}", obj->as.ext.type, obj->as.ext.size);
      break;
    case CMP_TYPE_FIXEXT16:
      error_printf("fixext16: {%d, %u}", obj->as.ext.type, obj->as.ext.size);
      break;
    case CMP_TYPE_FLOAT:
      error_printf("float: %f", obj->as.flt);
      break;
    case CMP_TYPE_DOUBLE:
      error_printf("double: %f", obj->as.dbl);
      break;
    case CMP_TYPE_UINT8:
      error_printf("uint8: %u", obj->as.u8);
      break;
    case CMP_TYPE_UINT16:
      error_printf("uint16: %u", obj->as.u16);
      break;
    case CMP_TYPE_UINT32:
      error_printf("uint32: %u", obj->as.u32);
      break;
    case CMP_TYPE_UINT64:
      error_printf("uint64: %" PRIu64, obj->as.u64);
      break;
    case CMP_TYPE_SINT8:
      error_printf("int8: %d", obj->as.s8);
      break;
    case CMP_TYPE_SINT16:
      error_printf("int16: %d", obj->as.s16);
      break;
    case CMP_TYPE_SINT32:
      error_printf("int32: %d", obj->as.s32);
      break;
    case CMP_TYPE_SINT64:
      error_printf("int64: %" PRId64, obj->as.s64);
      break;
  }
}

bool run_msgpack_tests(void) {
  buf_t in_buf, out_buf;
  cmp_ctx_t in_cmp, out_cmp;
  cmp_object_t obj;
  bool buffers_equal = false;

  setup_cmp_and_buf(&in_cmp, &in_buf);
  M_BufferSetFile(&in_buf, "cases.mpac");
  M_BufferSeek(&in_buf, 0);

  setup_cmp_and_buf(&out_cmp, &out_buf);
  M_BufferEnsureCapacity(&out_buf, M_BufferGetSize(&in_buf));

  while (M_BufferGetCursor(&in_buf) < (M_BufferGetSize(&in_buf))) {
    if (!cmp_read_object(&in_cmp, &obj)) {
      error_printf("Error reading object: %s\n", cmp_strerror(&in_cmp));
      return false;
    }
    if (!cmp_write_object(&out_cmp, &obj)) {
      error_printf("Error writing object: %s\n", cmp_strerror(&out_cmp));
      return false;
    }
  }

  M_BufferSeek(&in_buf, 0);
  M_BufferSeek(&out_buf, 0);
  buffers_equal = M_BufferEqualsData(
    &in_buf, M_BufferGetData(&out_buf), M_BufferGetSize(&out_buf)
  );

  if (!buffers_equal) {
    error_printf("Buffers did not match.\n");
    M_BufferPrint(&in_buf);
    M_BufferPrint(&out_buf);
    M_BufferFree(&in_buf);
    M_BufferFree(&out_buf);

    return false;
  }

  M_BufferFree(&in_buf);
  M_BufferFree(&out_buf);

  return true;
}

bool run_fixedint_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

  setup_cmp_and_buf(&cmp, &buf);

  error_clear();

  if (cmp_write_pfix(&cmp, 128)) {
    error_printf("Wrote a positive fixed integer that was too large.\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_pfix(&cmp, 200)) {
    error_printf("Wrote a positive fixed integer that was too large.\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_pfix(&cmp, -1)) {
    error_printf("Wrote a negative positive fixed integer (-1).\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_pfix(&cmp, -31)) {
    error_printf("Wrote a negative positive fixed integer (-31).\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_pfix(&cmp, -32)) {
    error_printf("Wrote a negative positive fixed integer (-32).\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_pfix(&cmp, -127)) {
    error_printf("Wrote a negative positive fixed integer (-127).\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_pfix(&cmp, -128)) {
    error_printf("Wrote a negative positive fixed integer (-128).\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_ufix(&cmp, -128)) {
    error_printf("Wrote a negative unsigned fixed integer (-128).\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_ufix(&cmp, -1)) {
    error_printf("Wrote a negative unsigned fixed integer.\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_ufix(&cmp, -128)) {
    error_printf("Wrote a negative unsigned fixed integer.\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_sfix(&cmp, -33)) {
    error_printf(
      "Wrote a negative signed fixed integer that was too large.\n"
    );
    return false;
  }
  cmp.error = 0;

  if (cmp_write_nfix(&cmp, 0)) {
    error_printf("Wrote 0 as a negative fixed integer.\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_nfix(&cmp, 1)) {
    error_printf("Wrote 1 as a negative fixed integer.\n");
    return false;
  }
  cmp.error = 0;

  if (cmp_write_nfix(&cmp, -33)) {
    error_printf("Wrote a negative fixed integer that was too large.\n");
    return false;
  }
  cmp.error = 0;

  test_format(cmp_write_ufix, cmp_read_uinteger, u8, uint64_t, 0, "\x00", 1);
  test_format(cmp_write_ufix, cmp_read_uinteger, u8, uint64_t, -0, "\x00", 1);
  test_format(cmp_write_sfix, cmp_read_uinteger, u8, uint64_t, 0, "\x00", 1);
  test_format(cmp_write_sfix, cmp_read_sinteger, s8, int64_t, -0, "\x00", 1);
  test_format(cmp_write_sfix, cmp_read_uinteger, u8, uint64_t, 127, "\x7f", 1);
  test_format(cmp_write_sfix, cmp_read_sinteger, s8, int64_t, -32, "\xe0", 1);
  test_format(cmp_write_pfix, cmp_read_uinteger, u8, uint64_t, 0, "\x00", 1);
  test_format(cmp_write_pfix, cmp_read_uinteger, u8, uint64_t, 1, "\x01", 1);
  test_format(cmp_write_pfix, cmp_read_uinteger, u8, uint64_t, 127, "\x7f", 1);
  test_format(cmp_write_nfix, cmp_read_sinteger, s8, int64_t, -1, "\xff", 1);
  test_format(cmp_write_nfix, cmp_read_sinteger, s8, int64_t, -32, "\xe0", 1);

  return true;
}

bool run_number_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

  setup_cmp_and_buf(&cmp, &buf);

  test_format(
    cmp_write_s8, cmp_read_sinteger, s8, int64_t,  0,   "\xd0\x00", 2
  );
  test_format(
    cmp_write_s8, cmp_read_sinteger, s8, int64_t,  1,   "\xd0\x01", 2
  );
  test_format(
    cmp_write_s8, cmp_read_sinteger, s8, int64_t, -1,   "\xd0\xff", 2
  );
  test_format(
    cmp_write_s8, cmp_read_sinteger, s8, int64_t,  127, "\xd0\x7f", 2
  );
  test_format(
    cmp_write_s8, cmp_read_sinteger, s8, int64_t, -128, "\xd0\x80", 2
  );

  test_format(
    cmp_write_s16, cmp_read_sinteger, s16, int64_t,  0,     "\xd1\x00\x00", 3
  );
  test_format(
    cmp_write_s16, cmp_read_sinteger, s16, int64_t,  1,     "\xd1\x00\x01", 3
  );
  test_format(
    cmp_write_s16, cmp_read_sinteger, s16, int64_t, -1,     "\xd1\xff\xff", 3
  );
  test_format(
    cmp_write_s16, cmp_read_sinteger, s16, int64_t,  127,   "\xd1\x00\x7f", 3
  );
  test_format(
    cmp_write_s16, cmp_read_sinteger, s16, int64_t, -128,   "\xd1\xff\x80", 3
  );
  test_format(
    cmp_write_s16, cmp_read_sinteger, s16, int64_t,  256,   "\xd1\x01\x00", 3
  );
  test_format(
    cmp_write_s16, cmp_read_sinteger, s16, int64_t,  32767, "\xd1\x7f\xff", 3
  );
  test_format(
    cmp_write_s16, cmp_read_sinteger, s16, int64_t, -32768, "\xd1\x80\x00", 3
  );

  test_format(
    cmp_write_s32, cmp_read_sinteger, s32, int64_t, 0, "\xd2\x00\x00\x00\x00", 5
  );
  test_format(
    cmp_write_s32, cmp_read_sinteger, s32, int64_t, 1, "\xd2\x00\x00\x00\x01", 5
  );
  test_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    -1,
    "\xd2\xff\xff\xff\xff",
    5
  );
  test_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    127,
    "\xd2\x00\x00\x00\x7f",
    5
  );
  test_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    -128,
    "\xd2\xff\xff\xff\x80",
    5
  );
  test_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    256,
    "\xd2\x00\x00\x01\x00",
    5
  );
  test_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    32767,
    "\xd2\x00\x00\x7f\xff",
    5
  );
  test_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    -32768,
    "\xd2\xff\xff\x80\x00",
    5
  );
  test_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    65535,
    "\xd2\x00\x00\xff\xff",
    5
  );
  test_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    -65536,
    "\xd2\xff\xff\x00\x00",
    5
  );
  test_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    8388607,
    "\xd2\x00\x7f\xff\xff",
    5
  );
  test_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    -8388608,
    "\xd2\xff\x80\x00\x00",
    5
  );
  test_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    16777215,
    "\xd2\x00\xff\xff\xff",
    5
  );
  test_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    -16777216,
    "\xd2\xff\x00\x00\x00",
    5
  );
  test_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    2147483647,
    "\xd2\x7f\xff\xff\xff",
    5
  );
  test_format(
    cmp_write_s32,
    cmp_read_sinteger,
    s32,
    int64_t,
    -2147483648,
    "\xd2\x80\x00\x00\x00",
    5
  );

  test_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    0,
    "\xd3\x00\x00\x00\x00\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    1,
    "\xd3\x00\x00\x00\x00\x00\x00\x00\x01",
    9
  );
  test_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    -1,
    "\xd3\xff\xff\xff\xff\xff\xff\xff\xff",
    9
  );
  test_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    127,
    "\xd3\x00\x00\x00\x00\x00\x00\x00\x7f",
    9
  );
  test_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    -128,
    "\xd3\xff\xff\xff\xff\xff\xff\xff\x80",
    9
  );
  test_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    256,
    "\xd3\x00\x00\x00\x00\x00\x00\x01\x00",
    9
  );
  test_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    32767,
    "\xd3\x00\x00\x00\x00\x00\x00\x7f\xff",
    9
  );
  test_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    -32768,
    "\xd3\xff\xff\xff\xff\xff\xff\x80\x00",
    9
  );
  test_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    65535,
    "\xd3\x00\x00\x00\x00\x00\x00\xff\xff",
    9
  );
  test_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    -65536,
    "\xd3\xff\xff\xff\xff\xff\xff\x00\x00",
    9
  );
  test_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    8388607,
    "\xd3\x00\x00\x00\x00\x00\x7f\xff\xff",
    9
  );
  test_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    -8388608,
    "\xd3\xff\xff\xff\xff\xff\x80\x00\x00",
    9
  );
  test_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    16777215,
    "\xd3\x00\x00\x00\x00\x00\xff\xff\xff",
    9
  );
  test_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    -16777216,
    "\xd3\xff\xff\xff\xff\xff\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    2147483647,
    "\xd3\x00\x00\x00\x00\x7f\xff\xff\xff",
    9
  );
  test_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    -2147483648,
    "\xd3\xff\xff\xff\xff\x80\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    4294967295,
    "\xd3\x00\x00\x00\x00\xff\xff\xff\xff",
    9
  );
  test_format(
    cmp_write_s64,
    cmp_read_sinteger,
    s64,
    int64_t,
    -4294967296,
    "\xd3\xff\xff\xff\xff\x00\x00\x00\x00",
    9
  );

  test_format(
    cmp_write_sint, cmp_read_uinteger, u8, uint64_t, 0, "\x00", 1
  );
  test_format(
    cmp_write_sint, cmp_read_uinteger, u8, uint64_t, 1, "\x01", 1
  );
  test_format(
    cmp_write_sint, cmp_read_uinteger, u8, uint64_t, 127, "\x7f", 1
  );
  test_format(
    cmp_write_sint, cmp_read_uinteger, u8, uint64_t, 128, "\xcc\x80", 2
  );
  test_format(
    cmp_write_sint, cmp_read_uinteger, u8, uint64_t, 255, "\xcc\xff", 2
  );
  test_format(
    cmp_write_sint, cmp_read_uinteger, u16, uint64_t, 256, "\xcd\x01\x00", 3
  );
  test_format(
    cmp_write_sint, cmp_read_uinteger, u16, uint64_t, 32767, "\xcd\x7f\xff", 3
  );
  test_format(
    cmp_write_sint, cmp_read_uinteger, u16, uint64_t, 32768, "\xcd\x80\x00", 3
  );
  test_format(
    cmp_write_sint, cmp_read_uinteger, u16, uint64_t, 65535, "\xcd\xff\xff", 3
  );
  test_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    65536,
    "\xce\x00\x01\x00\x00",
    5
  );
  test_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    8388607,
    "\xce\x00\x7f\xff\xff",
    5
  );
  test_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    8388608,
    "\xce\x00\x80\x00\x00",
    5
  );
  test_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    16777215,
    "\xce\x00\xff\xff\xff",
    5
  );
  test_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    16777216,
    "\xce\x01\x00\x00\x00",
    5
  );
  test_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    2147483647,
    "\xce\x7f\xff\xff\xff",
    5
  );
  test_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    4294967296,
    "\xcf\x00\x00\x00\x01\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    549755813887,
    "\xcf\x00\x00\x00\x7f\xff\xff\xff\xff",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    549755813888,
    "\xcf\x00\x00\x00\x80\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    1099511627775,
    "\xcf\x00\x00\x00\xff\xff\xff\xff\xff",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    1099511627776,
    "\xcf\x00\x00\x01\x00\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    140737488355327,
    "\xcf\x00\x00\x7f\xff\xff\xff\xff\xff",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    140737488355328,
    "\xcf\x00\x00\x80\x00\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    281474976710655,
    "\xcf\x00\x00\xff\xff\xff\xff\xff\xff",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    281474976710656,
    "\xcf\x00\x01\x00\x00\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    36028797018963967,
    "\xcf\x00\x7f\xff\xff\xff\xff\xff\xff",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    36028797018963968,
    "\xcf\x00\x80\x00\x00\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    72057594037927935,
    "\xcf\x00\xff\xff\xff\xff\xff\xff\xff",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    72057594037927936,
    "\xcf\x01\x00\x00\x00\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    9223372036854775807,
    "\xcf\x7f\xff\xff\xff\xff\xff\xff\xff",
    9
  );

  test_format(cmp_write_sint, cmp_read_sinteger, s8, int64_t, -1, "\xff", 1);
  test_format(cmp_write_sint, cmp_read_sinteger, s8, int64_t, -32, "\xe0", 1);
  test_format(
    cmp_write_sint, cmp_read_sinteger, s8, int64_t, -127, "\xd0\x81", 2
  );
  test_format(
    cmp_write_sint, cmp_read_sinteger, s8, int64_t, -128, "\xd0\x80", 2
  );
  test_format(
    cmp_write_sint, cmp_read_sinteger, s16, int64_t, -255, "\xd1\xff\x01", 3
  );
  test_format(
    cmp_write_sint, cmp_read_sinteger, s16, int64_t, -256, "\xd1\xff\x00", 3
  );
  test_format(
    cmp_write_sint, cmp_read_sinteger, s16, int64_t, -32767, "\xd1\x80\x01", 3
  );
  test_format(
    cmp_write_sint, cmp_read_sinteger, s16, int64_t, -32768, "\xd1\x80\x00", 3
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s32,
    int64_t,
    -65535,
    "\xd2\xff\xff\x00\x01",
    5
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s32,
    int64_t,
    -65536,
    "\xd2\xff\xff\x00\x00",
    5
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s32,
    int64_t,
    -8388607,
    "\xd2\xff\x80\x00\x01",
    5
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s32,
    int64_t,
    -8388608,
    "\xd2\xff\x80\x00\x00",
    5
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s32,
    int64_t,
    -16777215,
    "\xd2\xff\x00\x00\x01",
    5
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s32,
    int64_t,
    -16777216,
    "\xd2\xff\x00\x00\x00",
    5
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s32,
    int64_t,
    -2147483647,
    "\xd2\x80\x00\x00\x01",
    5
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s32,
    int64_t,
    -2147483648,
    "\xd2\x80\x00\x00\x00",
    5
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -4294967295,
    "\xd3\xff\xff\xff\xff\x00\x00\x00\x01",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -4294967296,
    "\xd3\xff\xff\xff\xff\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -549755813887,
    "\xd3\xff\xff\xff\x80\x00\x00\x00\x01",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -549755813888,
    "\xd3\xff\xff\xff\x80\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -1099511627775,
    "\xd3\xff\xff\xff\x00\x00\x00\x00\x01",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -1099511627776,
    "\xd3\xff\xff\xff\x00\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -140737488355327,
    "\xd3\xff\xff\x80\x00\x00\x00\x00\x01",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -140737488355328,
    "\xd3\xff\xff\x80\x00\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -281474976710655,
    "\xd3\xff\xff\x00\x00\x00\x00\x00\x01",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -281474976710656,
    "\xd3\xff\xff\x00\x00\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -36028797018963967,
    "\xd3\xff\x80\x00\x00\x00\x00\x00\x01",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -36028797018963968,
    "\xd3\xff\x80\x00\x00\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -72057594037927935,
    "\xd3\xff\x00\x00\x00\x00\x00\x00\x01",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -72057594037927936,
    "\xd3\xff\x00\x00\x00\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_sint,
    cmp_read_sinteger,
    s64,
    int64_t,
    -9223372036854775807,
    "\xd3\x80\x00\x00\x00\x00\x00\x00\x01",
    9
  );

  test_format(cmp_write_u8,
    cmp_read_uinteger,
    u8,
    uint64_t,
    0,
    "\xcc\x00",
    1);
  test_format(cmp_write_u8,
    cmp_read_uinteger,
    u8,
    uint64_t,
    1,
    "\xcc\x01",
    1);
  test_format(
    cmp_write_u8,
    cmp_read_uinteger,
    u8,
    uint64_t,
    127,
    "\xcc\x7f",
    1
  );
  test_format(
    cmp_write_u8,
    cmp_read_uinteger,
    u8,
    uint64_t,
    255,
    "\xcc\xff",
    1
  );

  test_format(
    cmp_write_u16, cmp_read_uinteger, u16, uint64_t, 0, "\xcd\x00\x00", 2
  );
  test_format(
    cmp_write_u16, cmp_read_uinteger, u16, uint64_t, 1, "\xcd\x00\x01", 2
  );
  test_format(
    cmp_write_u16, cmp_read_uinteger, u16, uint64_t, 127, "\xcd\x00\x7f", 2
  );
  test_format(
    cmp_write_u16, cmp_read_uinteger, u16, uint64_t, 256, "\xcd\x01\x00", 2
  );
  test_format(
    cmp_write_u16, cmp_read_uinteger, u16, uint64_t, 32767, "\xcd\x7f\xff", 2
  );
  test_format(
    cmp_write_u16, cmp_read_uinteger, u16, uint64_t, 65535, "\xcd\xff\xff", 2
  );

  test_format(
    cmp_write_u32,
    cmp_read_uinteger,
    u32,
    uint64_t,
    0,
    "\xce\x00\x00\x00\x00",
    5
  );
  test_format(
    cmp_write_u32,
    cmp_read_uinteger,
    u32,
    uint64_t,
    1,
    "\xce\x00\x00\x00\x01",
    5
  );
  test_format(
    cmp_write_u32,
    cmp_read_uinteger,
    u32,
    uint64_t,
    127,
    "\xce\x00\x00\x00\x7f",
    5
  );
  test_format(
    cmp_write_u32,
    cmp_read_uinteger,
    u32,
    uint64_t,
    256,
    "\xce\x00\x00\x01\x00",
    5
  );
  test_format(
    cmp_write_u32,
    cmp_read_uinteger,
    u32,
    uint64_t,
    32767,
    "\xce\x00\x00\x7f\xff",
    5
  );
  test_format(
    cmp_write_u32,
    cmp_read_uinteger,
    u32,
    uint64_t,
    65535,
    "\xce\x00\x00\xff\xff",
    5
  );
  test_format(
    cmp_write_u32,
    cmp_read_uinteger,
    u32,
    uint64_t,
    8388607,
    "\xce\x00\x7f\xff\xff",
    5
  );
  test_format(
    cmp_write_u32,
    cmp_read_uinteger,
    u32,
    uint64_t,
    16777215,
    "\xce\x00\xff\xff\xff",
    5
  );
  test_format(
    cmp_write_u32,
    cmp_read_uinteger,
    u32,
    uint64_t,
    2147483647,
    "\xce\x7f\xff\xff\xff",
    5
  );
  test_format(
    cmp_write_u32,
    cmp_read_uinteger,
    u32,
    uint64_t,
    4294967295,
    "\xce\xff\xff\xff\xff",
    5
  );

  test_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    0,
    "\xcf\x00\x00\x00\x00\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    1,
    "\xcf\x00\x00\x00\x00\x00\x00\x00\x01",
    9
  );
  test_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    127,
    "\xcf\x00\x00\x00\x00\x00\x00\x00\x7f",
    9
  );
  test_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    256,
    "\xcf\x00\x00\x00\x00\x00\x00\x01\x00",
    9
  );
  test_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    32767,
    "\xcf\x00\x00\x00\x00\x00\x00\x7f\xff",
    9
  );
  test_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    65535,
    "\xcf\x00\x00\x00\x00\x00\x00\xff\xff",
    9
  );
  test_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    8388607,
    "\xcf\x00\x00\x00\x00\x00\x7f\xff\xff",
    9
  );
  test_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    16777215,
    "\xcf\x00\x00\x00\x00\x00\xff\xff\xff",
    9
  );
  test_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    2147483647,
    "\xcf\x00\x00\x00\x00\x7f\xff\xff\xff",
    9
  );
  test_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    4294967295,
    "\xcf\x00\x00\x00\x00\xff\xff\xff\xff",
    9
  );
  test_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    0xFFFFFFFFFFFFFFFE,
    "\xcf\xff\xff\xff\xff\xff\xff\xff\xfe",
    9
  );
  test_format(
    cmp_write_u64,
    cmp_read_uinteger,
    u64,
    uint64_t,
    0xFFFFFFFFFFFFFFFF,
    "\xcf\xff\xff\xff\xff\xff\xff\xff\xff",
    9
  );

  test_format(
    cmp_write_uint, cmp_read_uinteger, u8, uint64_t, 0, "\x00", 1
  );
  test_format(
    cmp_write_uint, cmp_read_uinteger, u8, uint64_t, 1, "\x01", 1
  );
  test_format(
    cmp_write_uint, cmp_read_uinteger, u8, uint64_t, 127, "\x7f", 1
  );
  test_format(
    cmp_write_uint, cmp_read_uinteger, u8, uint64_t, 128, "\xcc\x80", 2
  );
  test_format(
    cmp_write_uint, cmp_read_uinteger, u8, uint64_t, 255, "\xcc\xff", 2
  );
  test_format(
    cmp_write_uint, cmp_read_uinteger, u16, uint64_t, 256, "\xcd\x01\x00", 3
  );
  test_format(
    cmp_write_uint, cmp_read_uinteger, u16, uint64_t, 32767, "\xcd\x7f\xff", 3
  );
  test_format(
    cmp_write_uint, cmp_read_uinteger, u16, uint64_t, 32768, "\xcd\x80\x00", 3
  );
  test_format(
    cmp_write_uint, cmp_read_uinteger, u16, uint64_t, 65535, "\xcd\xff\xff", 3
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    65536,
    "\xce\x00\x01\x00\x00",
    5
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    8388607,
    "\xce\x00\x7f\xff\xff",
    5
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    8388608,
    "\xce\x00\x80\x00\x00",
    5
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    16777215,
    "\xce\x00\xff\xff\xff",
    5
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    16777216,
    "\xce\x01\x00\x00\x00",
    5
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    2147483647,
    "\xce\x7f\xff\xff\xff",
    5
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    2147483648,
    "\xce\x80\x00\x00\x00",
    5
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u32,
    uint64_t,
    4294967295,
    "\xce\xff\xff\xff\xff",
    5
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    4294967296,
    "\xcf\x00\x00\x00\x01\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    549755813887,
    "\xcf\x00\x00\x00\x7f\xff\xff\xff\xff",
    9
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    549755813888,
    "\xcf\x00\x00\x00\x80\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    1099511627775,
    "\xcf\x00\x00\x00\xff\xff\xff\xff\xff",
    9
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    1099511627776,
    "\xcf\x00\x00\x01\x00\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    140737488355327,
    "\xcf\x00\x00\x7f\xff\xff\xff\xff\xff",
    9
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    140737488355328,
    "\xcf\x00\x00\x80\x00\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    281474976710655,
    "\xcf\x00\x00\xff\xff\xff\xff\xff\xff",
    9
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    281474976710656,
    "\xcf\x00\x01\x00\x00\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    36028797018963967,
    "\xcf\x00\x7f\xff\xff\xff\xff\xff\xff",
    9
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    36028797018963968,
    "\xcf\x00\x80\x00\x00\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    72057594037927935,
    "\xcf\x00\xff\xff\xff\xff\xff\xff\xff",
    9
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    72057594037927936,
    "\xcf\x01\x00\x00\x00\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    9223372036854775807,
    "\xcf\x7f\xff\xff\xff\xff\xff\xff\xff",
    9
  );
  test_format(
    cmp_write_uint,
    cmp_read_uinteger,
    u64,
    uint64_t,
    0xFFFFFFFFFFFFFFFF,
    "\xcf\xff\xff\xff\xff\xff\xff\xff\xff",
    9
  );

  test_format(cmp_write_float,
    cmp_read_float,
    flt,
    float,
    0.0f,
    "\xca\x00\x00\x00\x00",
    5
  );
  test_format(cmp_write_float,
    cmp_read_float,
    flt,
    float,
    -0.0f,
    "\xca\x80\x00\x00\x00",
    5
  );
  test_format(cmp_write_float,
    cmp_read_float,
    flt,
    float,
    1.0f,
    "\xca\x3f\x80\x00\x00",
    5
  );
  test_format(cmp_write_float,
    cmp_read_float,
    flt,
    float,
    -1.0f,
    "\xca\xbf\x80\x00\x00",
    5
  );
  test_format(cmp_write_float,
    cmp_read_float,
    flt,
    float,
    65535.0f,
    "\xca\x47\x7f\xff\x00",
    5
  );
  test_format(cmp_write_float,
    cmp_read_float,
    flt,
    float,
    -65535.0f,
    "\xca\xc7\x7f\xff\x00",
    5
  );
  test_format(cmp_write_float,
    cmp_read_float,
    flt,
    float,
    32767.0f,
    "\xca\x46\xff\xfe\x00",
    5
  );
  test_format(cmp_write_float,
    cmp_read_float,
    flt,
    float,
    -32767.0f,
    "\xca\xc6\xff\xfe\x00",
    5
  );

  test_format(
    cmp_write_double,
    cmp_read_double,
    dbl,
    double,
    0.0,
    "\xcb\x00\x00\x00\x00\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_double,
    cmp_read_double,
    dbl,
    double,
    -0.0,
    "\xcb\x80\x00\x00\x00\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_double,
    cmp_read_double,
    dbl,
    double,
    1.0,
    "\xcb\x3f\xf0\x00\x00\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_double,
    cmp_read_double,
    dbl,
    double,
    -1.0,
    "\xcb\xbf\xf0\x00\x00\x00\x00\x00\x00",
    9
  );
  test_format(
    cmp_write_double,
    cmp_read_double,
    dbl,
    double,
    2147483647.0,
    "\xcb\x41\xdf\xff\xff\xff\xc0\x00\x00",
    9
  );
  test_format(
    cmp_write_double,
    cmp_read_double,
    dbl,
    double,
    -2147483647.0,
    "\xcb\xc1\xdf\xff\xff\xff\xc0\x00\x00",
    9
  );
  test_format(
    cmp_write_double,
    cmp_read_double,
    dbl,
    double,
    4294967295.0,
    "\xcb\x41\xef\xff\xff\xff\xe0\x00\x00",
    9
  );
  test_format(
    cmp_write_double,
    cmp_read_double,
    dbl,
    double,
    -4294967295.0,
    "\xcb\xc1\xef\xff\xff\xff\xe0\x00\x00",
    9
  );

  return true;
}

bool run_nil_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

  setup_cmp_and_buf(&cmp, &buf);

  test_format_no_input(cmp_write_nil, u8, "\xc0", 1, 0)

  return true;
}

bool run_boolean_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

  setup_cmp_and_buf(&cmp, &buf);

  test_format_no_input(cmp_write_false, boolean, "\xc2", 1, false)
  test_format_no_input(cmp_write_true,  boolean, "\xc3", 1, true)
  test_format(cmp_write_bool, cmp_read_bool, boolean, bool, false, "\xc2", 1);
  test_format(cmp_write_bool, cmp_read_bool, boolean, bool, true, "\xc3", 1);
  test_format(
    cmp_write_u8_as_bool, cmp_read_bool_as_u8, boolean, uint8_t, 0, "\xc2", 1
  );
  test_format(
    cmp_write_u8_as_bool, cmp_read_bool_as_u8, boolean, uint8_t, 1, "\xc3", 1
  );

  return true;
}

bool run_binary_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

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

  return true;
}

bool run_string_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

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

  return true;
}

bool run_array_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

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

  return true;
}

bool run_map_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

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

  return true;
}

bool run_ext_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

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

  return true;
}

bool run_obj_tests(void) {
  buf_t buf;
  cmp_ctx_t cmp;
  cmp_object_t obj;

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

  return true;
}

int main(void) {
  printf("\n=== Testing CMP v%u (MessagePack v%u) ===\n\n",
    cmp_version(), cmp_mp_version()
  );

  M_BufferInitWithCapacity(&error_message, 4096);

  run_tests(msgpack);
  run_tests(fixedint);
  run_tests(number);
  run_tests(nil);
  run_tests(boolean);
  run_tests(binary);
  run_tests(string);
  run_tests(array);
  run_tests(map);
  run_tests(ext);
  run_tests(obj);

  puts("\nAll tests pass!\n");

  return EXIT_SUCCESS;
}

/* vi: set et ts=2 sw=2: */

