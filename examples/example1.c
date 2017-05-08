/*
The MIT License (MIT)

Copyright (c) 2017 Charles Gunyon

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

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

static bool file_skipper(cmp_ctx_t *ctx, size_t count) {
    return fseek((FILE *)ctx->buf, count, SEEK_CUR);
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
    uint32_t array_size = 0;
    uint32_t str_size = 0;
    char hello[6] = {0, 0, 0, 0, 0, 0};
    char message_pack[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    fh = fopen("cmp_data.dat", "w+b");

    if (fh == NULL)
        error_and_exit("Error opening data.dat");

    cmp_init(&cmp, fh, file_reader, file_skipper, file_writer);

    if (!cmp_write_array(&cmp, 2))
        error_and_exit(cmp_strerror(&cmp));

    if (!cmp_write_str(&cmp, "Hello", 5))
        error_and_exit(cmp_strerror(&cmp));

    if (!cmp_write_str(&cmp, "MessagePack", 11))
        error_and_exit(cmp_strerror(&cmp));

    rewind(fh);

    if (!cmp_read_array(&cmp, &array_size))
        error_and_exit(cmp_strerror(&cmp));

    /* You can read the str byte size and then read str bytes... */

    if (!cmp_read_str_size(&cmp, &str_size))
        error_and_exit(cmp_strerror(&cmp));

    if (str_size > (sizeof(hello) - 1))
        error_and_exit("Packed 'hello' length too long\n");

    if (!read_bytes(hello, str_size, fh))
        error_and_exit(cmp_strerror(&cmp));

    /*
     * ...or you can set the maximum number of bytes to read and do it all in
     * one call
     */

    str_size = sizeof(message_pack);
    if (!cmp_read_str(&cmp, message_pack, &str_size))
        error_and_exit(cmp_strerror(&cmp));

    printf("Array Length: %u.\n", array_size);
    printf("[\"%s\", \"%s\"]\n", hello, message_pack);

    fclose(fh);

    return EXIT_SUCCESS;
}

