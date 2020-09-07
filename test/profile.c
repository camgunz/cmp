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

#include "tests.h"

int main(void) {
  test_msgpack(NULL);
  test_fixedint(NULL);
  test_numbers(NULL);
  test_nil(NULL);
  test_boolean(NULL);
  test_bin(NULL);
  test_string(NULL);
  test_array(NULL);
  test_map(NULL);
  test_ext(NULL);
  test_obj(NULL);

#ifndef CMP_NO_FLOAT
  test_float_flip(NULL);
#endif

  test_skipping(NULL);
  test_deprecated_limited_skipping(NULL);
  test_errors(NULL);
  test_version(NULL);
  test_conversions(NULL);

  return EXIT_SUCCESS;
}

/* vi: set et ts=2 sw=2: */
