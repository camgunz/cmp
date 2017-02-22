#!/bin/sh

EXE_NAME='test-cmp'
CC=clang
# CC=gcc
CFLAGS="--std=c99"
CFLAGS="$CFLAGS -Werror -Wall -Wextra"
CFLAGS="$CFLAGS -funsigned-char"
CFLAGS="$CFLAGS -fwrapv"
CFLAGS="$CFLAGS -Wmissing-format-attribute"
CFLAGS="$CFLAGS -Wpointer-arith"
CFLAGS="$CFLAGS -Wformat-nonliteral"
CFLAGS="$CFLAGS -Winit-self"
CFLAGS="$CFLAGS -Wwrite-strings"
CFLAGS="$CFLAGS -Wshadow"
CFLAGS="$CFLAGS -Wenum-compare"
CFLAGS="$CFLAGS -Wempty-body"
if [ $CC = 'clang' ]
then
  CFLAGS="$CFLAGS -Wsizeof-array-argument" # GCC does not support this
  CFLAGS="$CFLAGS -Wstring-conversion"     # GCC does not support this
fi
CFLAGS="$CFLAGS -Wparentheses"
CFLAGS="$CFLAGS -Wcast-align"
CFLAGS="$CFLAGS -Werror=strict-aliasing"
CFLAGS="$CFLAGS --pedantic-errors"
CFLAGS="$CFLAGS -g"

rm -f "$EXE_NAME"

$CC $CFLAGS -I../ -o $EXE_NAME ../cmp.c buf.c utils.c test.c || exit 1

# gdb -ex run --args ./$EXE_NAME
./$EXE_NAME

