CC ?= gcc

CFLAGS ?= -Werror -Wall -Wextra -funsigned-char -fwrapv -Wmissing-format-attribute -Wpointer-arith -Wformat-nonliteral -Winit-self -Wwrite-strings -Wshadow -Wenum-compare -Wempty-body -Wparentheses -Wcast-align -Wstrict-aliasing --pedantic-errors

.PHONY: all clean test

all: cmptest example1 example2

test: cmptest
	@./cmptest

test2: cmptest2
	@./cmptest2

cmp.o:
	$(CC) $(CFLAGS) --std=c89 -fprofile-arcs -ftest-coverage -g -O0 -I. -c cmp.c

cmptest: cmp.o
	$(CC) $(CFLAGS) --std=c99 -I. -fprofile-arcs -ftest-coverage -g -O0 -o cmptest cmp.o test/test.c test/buf.c test/utils.c

cmptest2: cmp.o
	$(CC) $(CFLAGS) -Wno-error=deprecated-declarations --std=c99 -I. -fprofile-arcs -ftest-coverage -g -O0 -o cmptest2 cmp.o test/test2.c test/buf.c test/utils.c -lcmocka

example1:
	$(CC) $(CFLAGS) --std=c89 -O3 -I. -o example1 cmp.c examples/example1.c

example2:
	$(CC) $(CFLAGS) --std=c89 -O3 -I. -o example2 cmp.c examples/example2.c

clean:
	@rm -f cmptest
	@rm -f cmptest2
	@rm -f example1
	@rm -f example2
	@rm -f *.o
	@rm -f *.gcno *.gcda *.info
	@rm -f cmp_data.dat
