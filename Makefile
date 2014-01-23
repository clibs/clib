
CC     ?= cc
BINS    = clib clib-install clib-search
PREFIX ?= /usr/local
SRC     = $(wildcard src/*.c)
DEPS    = $(wildcard deps/*/*.c)
CFLAGS  = -std=c99 -Ideps -Wall -Wno-unused-function
LDFLAGS = -lcurl

all: $(BINS)

$(BINS): $(SRC)
	$(CC) $(CFLAGS) -o $@ src/$@.c $(DEPS) $(LDFLAGS)

clean:
	$(foreach c, $(BINS), rm -f $(c);)

install: $(BINS)
	mkdir -p $(PREFIX)/bin
	$(foreach c, $(BINS), cp -f $(c) $(PREFIX)/bin/$(c);)

uninstall:
	$(foreach c, $(BINS), rm -f $(PREFIX)/bin/$(c);)

.PHONY: all clean install uninstall
