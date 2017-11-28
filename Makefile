CC ?= gcc
CLANG ?= clang

CFLAGS ?= -Werror -Wall -Wextra -funsigned-char -fwrapv -Wconversion -Wno-sign-conversion -Wmissing-format-attribute -Wpointer-arith -Wformat-nonliteral -Winit-self -Wwrite-strings -Wshadow -Wenum-compare -Wempty-body -Wparentheses -Wcast-align -Wstrict-aliasing --pedantic-errors
CMPCFLAGS ?= -std=c89
TESTCFLAGS ?= -std=c99 -Wno-error=deprecated-declarations -Wno-deprecated-declarations -O0

ADDRCFLAGS ?= -fsanitize=address
MEMCFLAGS ?= -fsanitize=memory -fno-omit-frame-pointer -fno-optimize-sibling-calls
UBCFLAGS ?= -fsanitize=undefined

.PHONY: all clean test coverage

all: cmptest example1 example2

test: addrtest memtest ubtest unittest

unittest: cmptest
	@./cmptest

addrtest: cmpaddrtest
	@./cmpaddrtest

memtest: cmpmemtest
	@./cmpmemtest

ubtest: cmpubtest
	@./cmpubtest

cmp.o:
	$(CC) $(CFLAGS) $(CMPCFLAGS) -fprofile-arcs -ftest-coverage -g -I. -c cmp.c

cmptest: cmp.o
	$(CC) $(CFLAGS) $(TESTCFLAGS) -fprofile-arcs -ftest-coverage -g -I. -o cmptest cmp.o test/test.c test/buf.c test/utils.c -lcmocka

clangcmp.o:
	$(CLANG) $(CFLAGS) $(CMPCFLAGS) -fprofile-arcs -ftest-coverage -g -I. -c cmp.c -o clangcmp.o

cmpaddrtest: clangcmp.o clean
	$(CLANG) $(CFLAGS) $(TESTCFLAGS) $(ADDRCFLAGS) -I. -o cmpaddrtest cmp.c test/test.c test/buf.c test/utils.c -lcmocka

cmpmemtest: clangcmp.o clean
	$(CLANG) $(CFLAGS) $(TESTCFLAGS) $(MEMCFLAGS) -I. -o cmpmemtest cmp.c test/test.c test/buf.c test/utils.c -lcmocka

cmpubtest: clangcmp.o clean
	$(CLANG) $(CFLAGS) $(TESTCFLAGS) $(UBCFLAGS) -I. -o cmpubtest cmp.c test/test.c test/buf.c test/utils.c -lcmocka

example1:
	$(CC) $(CFLAGS) --std=c89 -O3 -I. -o example1 cmp.c examples/example1.c

example2:
	$(CC) $(CFLAGS) --std=c89 -O3 -I. -o example2 cmp.c examples/example2.c

coverage:
	@rm -f base_coverage.info test_coverage.info total_coverage.info
	@rm -rf coverage
	@lcov -q -c -i -d . -o base_coverage.info
	@lcov -q -c -d . -o test_coverage.info
	@lcov -q -a base_coverage.info -a test_coverage.info -o total_coverage.info
	@lcov -q --summary total_coverage.info
	@mkdir coverage
	@genhtml -q -o coverage total_coverage.info

clean:
	@rm -f cmptest
	@rm -f cmpaddrtest
	@rm -f cmpmemtest
	@rm -f cmpubtest
	@rm -f example1
	@rm -f example2
	@rm -f *.o
	@rm -f *.gcno *.gcda *.info
	@rm -f cmp_data.dat
