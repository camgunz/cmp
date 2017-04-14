CC ?= gcc

CFLAGS ?= -Werror -Wall -Wextra -funsigned-char -fwrapv -Wmissing-format-attribute -Wpointer-arith -Wformat-nonliteral -Winit-self -Wwrite-strings -Wshadow -Wenum-compare -Wempty-body -Wparentheses -Wcast-align -Wstrict-aliasing --pedantic-errors -g -O0 -fprofile-arcs -ftest-coverage

.PHONY: all clean

all: cmptest example1 example2

cmptest:
	$(CC) $(CFLAGS) --std=c99 -I. -o cmptest cmp.c test/test.c test/buf.c test/utils.c

example1:
	$(CC) $(CFLAGS) --std=c89 -I. -o example1 cmp.c examples/example1.c

example2:
	$(CC) $(CFLAGS) --std=c89 -I. -o example2 cmp.c examples/example2.c

clean:
	@rm -f cmptest
	@rm -f example1
	@rm -f example2
	@rm -f *.gcno
	@rm -f cmp_data.dat
