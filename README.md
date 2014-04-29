# cmp

CMP is a C implementation of the MessagePack serialization format.  It
currently implements version 5 of the [MessagePack
Spec](http://github.com/msgpack/msgpack/blob/master/spec.md).

CMP's goal is to be lightweight and straightforward, forcing nothing on the
programmer.

## License

While I'm a big believer in the GPL, I license CMP under the MIT license.

## Example Usage

The following examples use a file as the backend, and are modeled after the
examples included with the msgpack-c project.

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
        fprintf(stderr, "%s", msg);
        exit(EXIT_FAILURE);
    }

    int main(void) {
        FILE *fh = NULL;
        cmp_ctx_t cmp;
        uint32_t array_length = 0;
        uint32_t raw_length = 0;
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
         * ...or you can set the maximum number of bytes to read and do it all
         * in one call
         */

        raw_length = sizeof(message_pack) - 1;
        if (!cmp_read_raw(&cmp, message_pack, &raw_length))
            error_and_exit(cmp_strerror(&cmp));

        printf("Array Length: %zu.\n", array_length);
        printf("[\"%s\", \"%s\"]\n", hello, message_pack);

        fclose(fh);

        return EXIT_SUCCESS;
    }


## Fast, Lightweight, Flexible, and Robust

CMP uses no internal buffers; conversions, encoding and decoding are done on
the fly.

CMP's source is < 2,000 LOC, and it allocates nothing.

CMP uses standardized types rather than declaring its own, and it depends only
on `stdbool.h`, `stdint.h` and `string.h`.  It has no link-time dependencies,
not even the C Standard Library.

CMP only requires the programmer supply a read function and a write function.
In this way, the programmer can use CMP on memory, files, sockets, etc.

CMP is portable.  It uses fixed-width integer types, and checks the endianness
of the machine at runtime before swapping bytes (MessagePack is big-endian).
It also provides a fairly comprehensive error reporting mechanism modeled after
`errno` and `strerror`.  CMP is also threadsafe; while contexts cannot be
shared between threads, each thread may use its own context freely.  CMP is
written using C89 (ANSI C), aside, of course, from its use of fixed-width
integer types and `bool`.

CMP is tested on the MessagePack test suite.  Its small test program is
compiled with clang using `-Wall -Werror -Wextra ...` along with several other
flags, and generates no compilation errors.

## Building

There is no build system for CMP.  The programmer can drop `cmp.c` and `cmp.h`
in their source tree and modify as necessary.  No special compiler settings are required to build it.

