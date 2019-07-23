CC     ?= cc
PREFIX ?= /usr/local

ifdef EXE
	BINS = clib.exe clib-install.exe clib-search.exe
else
	BINS = clib clib-install clib-search
endif
CP      = cp -f
RM      = rm -f
MKDIR   = mkdir -p

SRC  = $(wildcard src/*.c)
DEPS = $(wildcard deps/*/*.c)
OBJS = $(DEPS:.c=.o)

ifdef STATIC
	CFLAGS  = -DCURL_STATICLIB -std=c99 -Ideps -Wall -Wno-unused-function -U__STRICT_ANSI__ $(shell deps/curl/bin/curl-config --cflags)
	LDFLAGS =  -static $(shell deps/curl/bin/curl-config --static-libs)
else
	CFLAGS  = -std=c99 -Ideps -Wall -Wno-unused-function -U__STRICT_ANSI__ $(shell curl-config --cflags)
	LDFLAGS = $(shell curl-config --libs)
endif

all: $(BINS)

$(BINS): $(SRC) $(OBJS)
	$(CC) $(CFLAGS) -o $@ src/$(@:.exe=).c $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $< -c -o $@ $(CFLAGS)

deps/list/list_iterator.o deps/list/list.o deps/list/list_node.o: deps/list/list.h
deps/substr/substr.o: deps/substr/substr.h deps/strdup/strdup.h
deps/parson/parson.o: deps/parson/parson.h
deps/str-replace/str-replace.o: deps/str-replace/str-replace.h deps/occurrences/occurrences.h deps/strdup/strdup.h
deps/path-join/path-join.o: deps/str-ends-with/str-ends-with.h deps/str-starts-with/str-starts-with.h deps/path-join/path-join.h deps/strdup/strdup.h
deps/str-flatten/str-flatten.o: deps/str-flatten/str-flatten.h
deps/http-get/http-get.o: deps/http-get/http-get.h

clean:
	$(foreach c, $(BINS), $(RM) $(c);)
	$(RM) $(OBJS)

install: $(BINS)
	$(MKDIR) $(PREFIX)/bin
	$(foreach c, $(BINS), $(CP) $(c) $(PREFIX)/bin/$(c);)

uninstall:
	$(foreach c, $(BINS), $(RM) $(PREFIX)/bin/$(c);)

test:
	@./test.sh

.PHONY: test all clean install uninstall
