#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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
    fprintf(stderr, "%s", msg);
    exit(EXIT_FAILURE);
}

int main(void) {
    FILE *fh = NULL;
    cmp_ctx_t cmp;
    size_t array_length = 0;
    size_t raw_length = 0;
    char hello[6] = {0, 0, 0, 0, 0, 0};
    char message_pack[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    fh = fopen("cmp_data.dat", "w+b");

    if (fh == NULL)
        error_and_exit("Error opening data.dat\n");

    cmp_init(&cmp, fh, file_reader, file_writer);

    if (!cmp_write_array(&cmp, 2))
        error_and_exit(cmp_strerror(&cmp));

    if (!cmp_write_raw(&cmp, "Hello", 5))
        error_and_exit(cmp_strerror(&cmp));

    if (!cmp_write_raw(&cmp, "MessagePack", 11))
        error_and_exit(cmp_strerror(&cmp));

    rewind(fh);

    if (!cmp_read_array(&cmp, &array_length))
        error_and_exit(cmp_strerror(&cmp));

    /* You can read the raw byte size and then read raw bytes... */

    if (!cmp_read_raw_length(&cmp, &raw_length))
        error_and_exit(cmp_strerror(&cmp));

    if (raw_length > (sizeof(hello) - 1))
        error_and_exit("Packed 'hello' length too long\n");

    if (!read_bytes(hello, raw_length, fh))
        error_and_exit(cmp_strerror(&cmp));

    /*
     * ...or you can set the maximum number of bytes to read and do it all in
     * one call
     */

    raw_length = sizeof(message_pack) - 1;
    if (!cmp_read_raw(&cmp, message_pack, &raw_length))
        error_and_exit(cmp_strerror(&cmp));

    printf("Array Length: %zu.\n", array_length);
    printf("[\"%s\", \"%s\"]\n", hello, message_pack);

    fclose(fh);

    return EXIT_SUCCESS;
}

