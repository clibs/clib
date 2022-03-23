CC ?= cc
VALGRIND ?= valgrind
TEST_RUNNER ?=

SRC = ../../src/common/clib-cache.c
DEPS += $(wildcard ../../deps/*/*.c)
OBJS = $(SRC:.c=.o) $(DEPS:.c=.o)
TEST_SRC = $(wildcard *.c)
TEST_OBJ = $(TEST_SRC:.c=.o)
TEST_BIN = $(TEST_SRC:.c=)

CFLAGS += -std=c99 -Wall -I../../src/common -I../../deps  -g
LDFLAGS = -lcurl
VALGRIND_OPTS ?= --leak-check=full --error-exitcode=3

.DEFAULT_GOAL := test

test: $(TEST_BIN)
	$(foreach t, $^, $(TEST_RUNNER) ./$(t) || exit 1;)

valgrind: TEST_RUNNER=$(VALGRIND) $(VALGRIND_OPTS)
valgrind: test

example: example.o $(OBJS)

cache-%: cache-%.o $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(OBJS)
	rm -f $(TEST_OBJ)
	rm -f $(TEST_BIN)
	rm -rf test/fixtures

.PHONY: test valgrind clean
