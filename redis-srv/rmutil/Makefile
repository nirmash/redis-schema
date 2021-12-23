# set environment variable RM_INCLUDE_DIR to the location of redismodule.h
ifndef RM_INCLUDE_DIR
	RM_INCLUDE_DIR=../
endif

CFLAGS ?= -g -fPIC -O3 -std=gnu99 -Wall -Wno-unused-function
CFLAGS += -I$(RM_INCLUDE_DIR)
CC=gcc

OBJS=util.o strings.o sds.o vector.o alloc.o periodic.o ccronexpr.o lapi.o lcorolib.o ldump.o llex.o lopcodes.o lstrlib.o lutf8lib.o lauxlib.o lctype.o lfunc.o lmathlib.o loslib.o  ltable.o lvm.o lbaselib.o ldblib.o lgc.o lmem.o lparser.o ltablib.o lzio.o lbitlib.o ldebug.o linit.o loadlib.o lstate.o ltm.o lcode.o ldo.o liolib.o lobject.o lstring.o lundump.o

all: librmutil.a

clean:
	rm -rf *.o *.a

librmutil.a: $(OBJS)
	ar rcs $@ $^

test_vector: test_vector.o vector.o
	$(CC) -Wall -o $@ $^ -lc -lpthread -O0
	@(sh -c ./$@)
.PHONY: test_vector

test_periodic: test_periodic.o periodic.o
	$(CC) -Wall -o $@ $^ -lc -lpthread -O0
	@(sh -c ./$@)
.PHONY: test_periodic
	
test: test_periodic test_vector
.PHONY: test
