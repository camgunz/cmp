CC ?= gcc
CLANG ?= clang

EXTRA_CFLAGS ?= -Werror -Wall -Wextra -funsigned-char -fwrapv -Wconversion \
				-Wno-sign-conversion -Wmissing-format-attribute \
				-Wpointer-arith -Wformat-nonliteral -Winit-self \
				-Wwrite-strings -Wshadow -Wenum-compare -Wempty-body \
				-Wparentheses -Wcast-align -Wstrict-aliasing --pedantic-errors
CMPCFLAGS ?= -std=c89 -Wno-c99-extensions
TESTCFLAGS ?= -std=c99 -Wno-error=deprecated-declarations \
			  -Wno-deprecated-declarations -O0
NOFPUTESTCFLAGS ?= $(TESTCFLAGS) -DCMP_NO_FLOAT

ADDRCFLAGS ?= -fsanitize=address
MEMCFLAGS ?= -fsanitize=memory -fno-omit-frame-pointer \
			 -fno-optimize-sibling-calls
UBCFLAGS ?= -fsanitize=undefined

.PHONY: all clean test coverage

all: cmpunittest example1 example2

profile: cmpprof
	@env LD_PRELOAD=/usr/lib/libprofiler.so CPUPROFILE=cmp.prof \
		CPUPROFILE_FREQUENCY=1000 ./cmpprof
	@pprof --web ./cmpprof cmp.prof

test: addrtest memtest nofloattest ubtest unittest

testprogs: cmpaddrtest cmpmemtest cmpnofloattest cmpubtest cmpunittest

addrtest: cmpaddrtest
	@./cmpaddrtest

memtest: cmpmemtest
	@./cmpmemtest

nofloattest: cmpnofloattest
	@./cmpnofloattest
	@rm -f *.gcno *.gcda *.info

ubtest: cmpubtest
	@./cmpubtest

unittest: cmpunittest
	@./cmpunittest

cmp.o: cmp.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(CMPCFLAGS) \
		-fprofile-arcs -ftest-coverage -g -I. -c cmp.c

cmpprof: cmp.o
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(TESTCFLAGS) $(LDFLAGS) \
		-fprofile-arcs -I. \
		-o cmpprof cmp.o test/profile.c test/tests.c test/buf.c test/utils.c \
		-lcmocka -lprofiler

cmpunittest: cmp.o
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(TESTCFLAGS) $(LDFLAGS) \
		-fprofile-arcs -ftest-coverage -g -I. \
		-o cmpunittest cmp.o test/test.c test/tests.c test/buf.c test/utils.c \
		-lcmocka

cmpnofloattest: cmp.o
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(NOFPUTESTCFLAGS) $(LDFLAGS) \
		-fprofile-arcs -ftest-coverage -g -I. \
		-o cmpnofloattest cmp.o test/test.c test/tests.c test/buf.c \
		test/utils.c \
		-lcmocka

clangcmp.o: cmp.c
	$(CLANG) $(CFLAGS) $(EXTRA_CFLAGS) $(CMPCFLAGS) \
		-fprofile-arcs -ftest-coverage -g -I. -c cmp.c -o clangcmp.o

cmpaddrtest: clangcmp.o clean
	$(CLANG) $(CFLAGS) $(EXTRA_CFLAGS) $(TESTCFLAGS) $(ADDRCFLAGS) $(LDFLAGS) \
		-I. -o cmpaddrtest \
		cmp.c test/test.c test/tests.c test/buf.c test/utils.c \
		-lcmocka

cmpmemtest: clangcmp.o clean
	$(CLANG) $(CFLAGS) $(EXTRA_CFLAGS) $(TESTCFLAGS) $(MEMCFLAGS) $(LDFLAGS) \
		-I. -o cmpmemtest \
		cmp.c test/test.c test/tests.c test/buf.c test/utils.c \
		-lcmocka

cmpubtest: clangcmp.o clean
	$(CLANG) $(CFLAGS) $(EXTRA_CFLAGS) $(TESTCFLAGS) $(UBCFLAGS) $(LDFLAGS) \
		-I. -o cmpubtest \
		cmp.c test/test.c test/tests.c test/buf.c test/utils.c \
		-lcmocka

example1:
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) --std=c89 -O3 -I. -o example1 \
		cmp.c examples/example1.c

example2:
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) --std=c89 -O3 -I. -o example2 \
		cmp.c examples/example2.c

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
	@rm -f cmp.prof
	@rm -f cmpunittest
	@rm -f cmpaddrtest
	@rm -f cmpmemtest
	@rm -f cmpubtest
	@rm -f cmpnofloattest
	@rm -f cmpprof
	@rm -f example1
	@rm -f example2
	@rm -f *.o
	@rm -f *.gcno *.gcda *.info
	@rm -f cmp_data.dat
