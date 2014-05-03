#!/bin/sh

CC=clang
CFLAGS="--std=c89 -Werror -Wall -Wextra"
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
CFLAGS="$CFLAGS -Wsizeof-array-argument"
CFLAGS="$CFLAGS -Wstring-conversion"
CFLAGS="$CFLAGS -Wparentheses"
CFLAGS="$CFLAGS -Wcast-align"
CFLAGS="$CFLAGS -Wstrict-aliasing"
CFLAGS="$CFLAGS --pedantic-errors"
CFLAGS="$CFLAGS -g"

$CC $CFLAGS -I../ -o example1 ../cmp.c example1.c
$CC $CFLAGS -I../ -o example2 ../cmp.c example2.c

