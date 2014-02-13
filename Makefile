
CC     ?= cc
PREFIX ?= /usr/local
SRC     = $(wildcard src/*.c)
DEPS    = $(wildcard deps/*/*.c)
CFLAGS  = -std=c99 -Ideps -Wall -Wno-unused-function
ifeq ($(OS),Windows_NT)
BINS    = clib.exe clib-install.exe clib-search.exe
LDFLAGS = -lcurldll
CP_F    = copy /Y
RM_F    = del /Q /S
MKDIR_P = mkdir
else
BINS    = clib clib-install clib-search
LDFLAGS = -lcurl
CP_F    = cp -f
RM_F    = rm -f
MKDIR_P = mkdir -p
endif

all: $(BINS)

$(BINS): $(SRC)
	$(CC) $(CFLAGS) -o $@ src/$@.c $(DEPS) $(LDFLAGS)

clean:
	$(foreach c, $(BINS), $(RM_F) $(c);)

install: $(BINS)
	$(MKDIR_P) $(PREFIX)/bin
	$(foreach c, $(BINS), $(CP_F) $(c) $(PREFIX)/bin/$(c);)

uninstall:
	$(foreach c, $(BINS), $(RM_F) $(PREFIX)/bin/$(c);)

.PHONY: all clean install uninstall
