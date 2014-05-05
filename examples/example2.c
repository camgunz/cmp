#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmp.h"

static bool read_bytes(void *data, size_t sz, FILE *fh) {
    return fread(data, sizeof(uint8_t), sz, fh) == (sz * sizeof(uint8_t));
}

static bool file_reader(cmp_ctx_t *ctx, void *data, size_t limit) {
    return read_bytes(data, limit, (FILE *)ctx->buf);
}

static size_t file_writer(cmp_ctx_t *ctx, const void *data, size_t count) {
    return fwrite(data, sizeof(uint8_t), count, (FILE *)ctx->buf);
}

void error_and_exit(const char *msg) {
    fprintf(stderr, "%s\n\n", msg);
    exit(EXIT_FAILURE);
}

int main(void) {
    FILE *fh = NULL;
    cmp_ctx_t cmp;
    uint16_t year = 1983;
    uint8_t month = 5;
    uint8_t day = 28;
    int64_t sint = 0;
    uint64_t uint = 0;
    float flt = 0.0f;
    double dbl = 0.0;
    bool boolean = false;
    uint8_t fake_bool = 0;
    uint32_t string_size = 0;
    uint32_t array_size = 0;
    uint32_t binary_size = 0;
    uint32_t map_size = 0;
    int8_t ext_type = 0;
    uint32_t ext_size = 0;
    char sbuf[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    fh = fopen("cmp_data.dat", "w+b");

    if (fh == NULL)
        error_and_exit("Error opening data.dat");

    cmp_init(&cmp, fh, file_reader, file_writer);

    /*
     * When you write an array, you first specify the number of array
     * elements, then you write that many elements.
     */
    if (!cmp_write_array(&cmp, 9))
        error_and_exit(cmp_strerror(&cmp));

    if (!cmp_write_sint(&cmp, -14))
        error_and_exit(cmp_strerror(&cmp));

    if (!cmp_write_uint(&cmp, 38))
        error_and_exit(cmp_strerror(&cmp));

    if (!cmp_write_float(&cmp, 1.8f))
        error_and_exit(cmp_strerror(&cmp));

    if (!cmp_write_double(&cmp, 300.4))
        error_and_exit(cmp_strerror(&cmp));

    if (!cmp_write_nil(&cmp))
        error_and_exit(cmp_strerror(&cmp));

    if (!cmp_write_true(&cmp))
        error_and_exit(cmp_strerror(&cmp));

    if (!cmp_write_false(&cmp))
        error_and_exit(cmp_strerror(&cmp));

    if (!cmp_write_bool(&cmp, false))
        error_and_exit(cmp_strerror(&cmp));

    if (!cmp_write_u8_as_bool(&cmp, 1))
        error_and_exit(cmp_strerror(&cmp));

    /* Array full */

    /*
     * Maps work similar to arrays, but the length is in "pairs", so this
     * writes 2 pairs to the map.  Subsequently, pairs are written in key,
     * value order.
     */

    if (!cmp_write_map(&cmp, 2))
        error_and_exit(cmp_strerror(&cmp));

    /* You can write string data all at once... */

    if (!cmp_write_str(&cmp, "Greeting", 8))
        error_and_exit(cmp_strerror(&cmp));

    if (!cmp_write_str(&cmp, "Hello", 5))
        error_and_exit(cmp_strerror(&cmp));

    if (!cmp_write_str(&cmp, "Name", 4))
        error_and_exit(cmp_strerror(&cmp));

    /* ...or in chunks */

    if (!cmp_write_str_marker(&cmp, 5))
        error_and_exit(cmp_strerror(&cmp));

    if (file_writer(&cmp, "Li", 2) != 2)
        error_and_exit(strerror(errno));

    if (file_writer(&cmp, "nus", 3) != 3)
        error_and_exit(strerror(errno));

    /* Map full */

    /* Binary data functions the same as string data */

    if (!cmp_write_bin(&cmp, "MessagePack", 11))
        error_and_exit(cmp_strerror(&cmp));

    if (!cmp_write_bin_marker(&cmp, 8))
        error_and_exit(cmp_strerror(&cmp));

    if (file_writer(&cmp, "is ", 3) != 3)
        error_and_exit(strerror(errno));

    if (file_writer(&cmp, "great", 5) != 5)
        error_and_exit(strerror(errno));

    /*
     * With extended types, you can create your own custom types.  Here we
     * create a simple date type.
     */

    /* cmp_write_ext_marker(type, size) */
    if (!cmp_write_ext_marker(&cmp, 1, 4))
        error_and_exit(cmp_strerror(&cmp));

    file_writer(&cmp, &year, sizeof(uint16_t));
    file_writer(&cmp, &month, sizeof(uint8_t));
    file_writer(&cmp, &day, sizeof(uint8_t));

    /* Now we can read the data back just as easily */

    rewind(fh);

    if (!cmp_read_array(&cmp, &array_size))
        error_and_exit(cmp_strerror(&cmp));

    if (array_size != 9)
        error_and_exit("Array size was not 9");

    if (!cmp_read_sinteger(&cmp, &sint))
        error_and_exit(cmp_strerror(&cmp));

    if (sint != -14)
        error_and_exit("Signed int was not -14");

    if (!cmp_read_uinteger(&cmp, &uint))
        error_and_exit(cmp_strerror(&cmp));

    if (uint != 38)
        error_and_exit("Unsigned int was not 38");

    if (!cmp_read_float(&cmp, &flt))
        error_and_exit(cmp_strerror(&cmp));

    if (flt != 1.8f)
        error_and_exit("Float was not 1.8f");

    if (!cmp_read_double(&cmp, &dbl))
        error_and_exit(cmp_strerror(&cmp));

    if (dbl != 300.4)
        error_and_exit("Double was not 300.f");

    if (!cmp_read_nil(&cmp))
        error_and_exit(cmp_strerror(&cmp));

    if (!cmp_read_bool(&cmp, &boolean))
        error_and_exit(cmp_strerror(&cmp));

    if (boolean != true)
        error_and_exit("First boolean was not true");

    if (!cmp_read_bool(&cmp, &boolean))
        error_and_exit(cmp_strerror(&cmp));

    if (boolean != false)
        error_and_exit("Second boolean was not false");

    if (!cmp_read_bool(&cmp, &boolean))
        error_and_exit(cmp_strerror(&cmp));

    if (boolean != false)
        error_and_exit("Third boolean was not false");

    if (!cmp_read_bool_as_u8(&cmp, &fake_bool))
        error_and_exit(cmp_strerror(&cmp));

    if (fake_bool != 1) {
        fprintf(stderr, "%u.\n", fake_bool);
        error_and_exit("Third boolean (u8) was not 1");
    }

    if (!cmp_read_map(&cmp, &map_size))
        error_and_exit(cmp_strerror(&cmp));

    if (map_size != 2)
        error_and_exit("Map size was not 2");

    /*
     * String reading here.  Note that normally strings are encoded using
     * UTF-8.  I have cleverly restricted this example to ASCII, which overlaps
     * UTF-8 encoding, but this must not be assumed in real-world code.
     *
     * You can read strings in two ways.  Either you can read the string's size
     * in bytes and then read the bytes manually...
     */

    if (!cmp_read_str_size(&cmp, &string_size))
        error_and_exit(cmp_strerror(&cmp));

    if (string_size != 8)
        error_and_exit("Greeting string key size was not 8");

    if (!read_bytes(sbuf, 8, fh))
        error_and_exit(strerror(errno));

    sbuf[string_size] = 0;

    if (strncmp(sbuf, "Greeting", 8) != 0)
        error_and_exit("Greeting string key name was not 'Greeting'");

    /*
     * ...or you can set the maximum number of bytes to read and do it all in
     * one call.  cmp_read_str will write no more than "size" bytes, including
     * the terminating NULL, to the passed buffer.  If the string's size
     * exceeds the passed buffer size, the "size" input is set to the number of
     * bytes necessary, not including the terminating NULL.  Otherwise, the
     * "size" input is set to the number of bytes written, not including the
     * terminating NULL.
     */

    string_size = sizeof(sbuf);
    if (!cmp_read_str(&cmp, sbuf, &string_size))
        error_and_exit(cmp_strerror(&cmp));

    if (strncmp(sbuf, "Hello", 5) != 0)
        error_and_exit("Greeting string value was not 'Hello'");

    string_size = sizeof(sbuf);
    if (!cmp_read_str(&cmp, sbuf, &string_size))
        error_and_exit(cmp_strerror(&cmp));

    if (strncmp(sbuf, "Name", 4) != 0)
        error_and_exit("Name key name was not 'Name'");

    string_size = sizeof(sbuf);
    if (!cmp_read_str(&cmp, sbuf, &string_size))
        error_and_exit(cmp_strerror(&cmp));

    if (strncmp(sbuf, "Linus", 5) != 0)
        error_and_exit("Name key value was not 'Linus'");

    memset(sbuf, 0, sizeof(sbuf));
    binary_size = sizeof(sbuf);
    if (!cmp_read_bin(&cmp, &sbuf, &binary_size))
        error_and_exit(cmp_strerror(&cmp));

    if (memcmp(sbuf, "MessagePack", 11) != 0)
        error_and_exit("1st binary value was not 'MessagePack'");

    memset(sbuf, 0, sizeof(sbuf));
    binary_size = sizeof(sbuf);
    if (!cmp_read_bin(&cmp, &sbuf, &binary_size))
        error_and_exit(cmp_strerror(&cmp));

    if (memcmp(sbuf, "is great", 8) != 0)
        error_and_exit("2nd binary value was not 'is great'");

    if (!cmp_read_ext_marker(&cmp, &ext_type, &ext_size))
        error_and_exit(cmp_strerror(&cmp));

    if (!read_bytes(&year, sizeof(uint16_t), fh))
        error_and_exit(strerror(errno));

    if (!read_bytes(&month, sizeof(uint8_t), fh))
        error_and_exit(strerror(errno));

    if (!read_bytes(&day, sizeof(uint8_t), fh))
        error_and_exit(strerror(errno));

    if (year != 1983)
        error_and_exit("Year was not 1983");

    if (month != 5)
        error_and_exit("Month was not 5");

    if (day != 28)
        error_and_exit("Day was not 28");

    rewind(fh);

    /* Alternately, you can read objects until the stream is empty */
    while (1) {
        cmp_object_t obj;

        if (!cmp_read_object(&cmp, &obj)) {
            if (feof(fh))
                break;

            error_and_exit(cmp_strerror(&cmp));
        }

        switch (obj.type) {
            case CMP_TYPE_POSITIVE_FIXNUM:
            case CMP_TYPE_UINT8:
                printf("Unsigned Integer: %u\n", obj.as.u8);
                break;
            case CMP_TYPE_FIXMAP:
            case CMP_TYPE_MAP16:
            case CMP_TYPE_MAP32:
                printf("Map: %u\n", obj.as.map_size);
                break;
            case CMP_TYPE_FIXARRAY:
            case CMP_TYPE_ARRAY16:
            case CMP_TYPE_ARRAY32:
                printf("Array: %u\n", obj.as.array_size);
                break;
            case CMP_TYPE_FIXSTR:
            case CMP_TYPE_STR8:
            case CMP_TYPE_STR16:
            case CMP_TYPE_STR32:
                if (!read_bytes(sbuf, obj.as.str_size, fh))
                    error_and_exit(strerror(errno));
                sbuf[obj.as.str_size] = 0;
                printf("String: %s\n", sbuf);
                break;
            case CMP_TYPE_BIN8:
            case CMP_TYPE_BIN16:
            case CMP_TYPE_BIN32:
                memset(sbuf, 0, sizeof(sbuf));
                if (!read_bytes(sbuf, obj.as.bin_size, fh))
                    error_and_exit(strerror(errno));
                printf("Binary: %s\n", sbuf);
                break;
            case CMP_TYPE_NIL:
                printf("NULL\n");
                break;
            case CMP_TYPE_BOOLEAN:
                if (obj.as.boolean)
                    printf("Boolean: true\n");
                else
                    printf("Boolean: false\n");
                break;
            case CMP_TYPE_EXT8:
            case CMP_TYPE_EXT16:
            case CMP_TYPE_EXT32:
            case CMP_TYPE_FIXEXT1:
            case CMP_TYPE_FIXEXT2:
            case CMP_TYPE_FIXEXT4:
            case CMP_TYPE_FIXEXT8:
            case CMP_TYPE_FIXEXT16:
                if (obj.as.ext.type == 1) { /* Date object */
                    if (!read_bytes(&year, sizeof(uint16_t), fh))
                        error_and_exit(strerror(errno));

                    if (!read_bytes(&month, sizeof(uint8_t), fh))
                        error_and_exit(strerror(errno));

                    if (!read_bytes(&day, sizeof(uint8_t), fh))
                        error_and_exit(strerror(errno));

                    printf("Date: %u/%u/%u\n", year, month, day);
                }
                else {
                    printf("Extended type {%d, %u}: ",
                        obj.as.ext.type, obj.as.ext.size
                    );
                    while (obj.as.ext.size--) {
                        read_bytes(sbuf, sizeof(uint8_t), fh);
                        printf("%02x ", sbuf[0]);
                    }
                    printf("\n");
                }
                break;
            case CMP_TYPE_FLOAT:
                printf("Float: %f\n", obj.as.flt);
                break;
            case CMP_TYPE_DOUBLE:
                printf("Double: %f\n", obj.as.dbl);
                break;
            case CMP_TYPE_UINT16:
                printf("Unsigned Integer: %u\n", obj.as.u16);
                break;
            case CMP_TYPE_UINT32:
                printf("Unsigned Integer: %u\n", obj.as.u32);
                break;
            case CMP_TYPE_UINT64:
                printf("Unsigned Integer: %" PRIu64 "\n", obj.as.u64);
                break;
            case CMP_TYPE_NEGATIVE_FIXNUM:
            case CMP_TYPE_SINT8:
                printf("Signed Integer: %d\n", obj.as.s8);
                break;
            case CMP_TYPE_SINT16:
                printf("Signed Integer: %d\n", obj.as.s16);
                break;
            case CMP_TYPE_SINT32:
                printf("Signed Integer: %d\n", obj.as.s32);
                break;
            case CMP_TYPE_SINT64:
                printf("Signed Integer: %" PRId64 "\n", obj.as.s64);
                break;
            default:
                printf("Unrecognized object type %u\n", obj.type);
                break;
        }
    }

    fclose(fh);

    return EXIT_SUCCESS;
}

