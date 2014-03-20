CC     ?= cc
PREFIX ?= /usr/local

BINS = clib clib-install clib-search

SRC  = $(wildcard src/*.c)
DEPS = $(wildcard deps/*/*.c)
OBJS = $(DEPS:.c=.o)

CFLAGS  = -std=c99 -Ideps -Wall -Wno-unused-function -U__STRICT_ANSI__
LDFLAGS = -lcurl

all: $(BINS)

$(BINS): $(SRC) $(OBJS)
	$(CC) $(CFLAGS) -o $@ src/$@.c $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $< -c -o $@ $(CFLAGS)

clean:
	$(foreach c, $(BINS), rm -f $(c);)
	rm -f $(OBJS)

install: $(BINS)
	mkdir -p $(PREFIX)/bin
	$(foreach c, $(BINS), cp -f $(c) $(PREFIX)/bin/$(c);)

uninstall:
	$(foreach c, $(BINS), rm -f $(PREFIX)/bin/$(c);)

test:
	@./test.sh

.PHONY: test all clean install uninstall
