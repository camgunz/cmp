cmp
===

CMP is a C implementation of the MessagePack serialization format.  It
currently implements the spec located on the [MessagePack
Wiki](http://wiki.msgpack.org/display/MSGPACK/Format+specification), but as
that spec is outdated, CMP will soon be updated to support [the latest
version](http://github.com/msgpack/msgpack/blob/master/spec.md).

CMP's goal is to be lightweight and straightforward, forcing nothing on the
programmer.

### Fast

CMP uses no internal buffers.  Conversions, encoding and decoding are done on
the fly.

### Lightweight

CMP's source is < 1,500 LOC, and it allocates nothing.  It has no dependencies,
not even the C Standard Library.

### Flexible

CMP only requires a read function and a write function.  In this way, the
programmer can use CMP on memory, files, sockets, etc.

### Simple

CMP is straightforward and unsurprising.  It makes minimal use of the
preprocessor, and its code is written in a clear manner with descriptive
variable names.  Despite its small size, CMP provides many convenience methods
to the programmer.  Consider packing an `int64_t` as efficiently as possible:

    int counter(void) {
        int64_t c = 0;
        bool res = false;
        struct cmp_ctx_s cmp;

        /* ... */

        if (c >= -31 && c <= -1)
            res = cmp_write_nfix(&cmp, c);
        else if (c <= 0 && c <= 127)
            res = cmp_write_pfix(&cmp, c);
        else if (c >= -32767 && c <= 32767)
            res = cmp_write_s16(&cmp, c);
        else if (c >= -2147483647 && c <= 2147483647)
            res = cmp_write_s32(&cmp, c);
        else if (c >= -9223372036854775807 && c <= 9223372036854775807)
            res = cmp_write_s64(&cmp, c);

        if (!res) {
            /* handle error */
        }
    }

Now with convenience methods:

    int counter(void) {
        int64_t c = 0;
        struct cmp_ctx_s cmp;

        /* ... */

        if (!cmp_write_sint(&cmp, c)) {
            /* handle error */
        }

    }

And, of course, the type-specific writers are still available for maximum
flexibility.

### Robust

CMP is portable.  It uses fixed-width integer types, and checks the endianness
of the machine at runtime before swapping bytes (MessagePack is big-endian).
It also provides a fairly comprehensive error reporting mechanism modeled after
`errno` and `strerror`.  CMP is also threadsafe; while contexts cannot be
shared between threads, each thread may use its own context freely.  CMP is
written using C89 (ANSI C).

CMP is tested on the MessagePack test suite.  Its small test suite is compiled
with clang using with `-Wall -Werror -Wextra ...` and a whole bunch of other
flags, and generates no compilation errors.

### License

While I'm a big believer in the GPL, I license CMP under the MIT license.

### Building

There is no build system for CMP.  The programmer can drop `cmp.c` and `cmp.h`
in their source tree and modify as necessary.

CMP uses standardized types rather than declaring its own, therefore it
requires `stdint.h` and `stdbool.h`.

### Usage

The following examples use a file as the backend, and are modeled after the
examples included with the msgpack-c project.

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

### To Do

  - Add versioning
  - Implement latest spec

