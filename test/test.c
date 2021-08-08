/*
The MIT License (MIT)

Copyright (c) 2020 Charles Gunyon

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

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

#include <cmocka.h>

#include "tests.h"

int main(void) {
  /* Use the old CMocka API because Travis' latest Ubuntu is Trusty */
  const UnitTest tests[17] = {
    unit_test(test_msgpack),
    unit_test(test_fixedint),
    unit_test(test_numbers),
    unit_test(test_nil),
    unit_test(test_boolean),
    unit_test(test_bin),
    unit_test(test_string),
    unit_test(test_array),
    unit_test(test_map),
    unit_test(test_ext),
    unit_test(test_obj),

#ifndef CMP_NO_FLOAT
    unit_test(test_float_flip),
#endif

    unit_test(test_skipping),
    unit_test(test_deprecated_limited_skipping),
    unit_test(test_errors),
    unit_test(test_version),
    unit_test(test_conversions),
  };

  if (run_tests(tests)) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

/* vi: set et ts=2 sw=2: */
