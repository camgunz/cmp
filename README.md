cmp
===

CMP is a C implementation of the MessagePack serialization format.  It
currently implements the spec located on the [MessagePack
Wiki](http://wiki.msgpack.org/display/MSGPACK/Format+specification), but as
that spec is outdated, CMP will soon be updated to support the latest version.

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
errno and strerror.  CMP is also threadsafe; while contexts cannot be shared
between threads, each thread may use its own context freely.  CMP is written
using C89 (ANSI C).

CMP is tested on the MessagePack test suite.  Its small test suite is compiled
with clang using with `-Wall -Werror -Wextra ...` and a whole bunch of other
flags, and generates no compilation errors.

### License

In line with [msgpack-c](http://github.com/msgpack/msgpack-c) CMP is released
under the Apache License 2.0.

### Usage

There is no build system for CMP.  The programmer can drop `cmp.c` and `cmp.h`
in their source tree and modify as necessary.

