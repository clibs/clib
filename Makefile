CC     ?= cc
PREFIX ?= /usr/local

ifdef EXE
	BINS = clib.exe clib-install.exe clib-search.exe
	TEST_BINS = $(TEST_SRC:=.exe)
else
	BINS = clib clib-install clib-search
	TEST_BINS = $(basename $(TEST_SRC))
endif

CP      = cp -f
RM      = rm -f
MKDIR   = mkdir -p

DEPS = $(wildcard deps/*/*.c)

SRC = $(wildcard src/*.c) $(DEPS)
TEST_SRC = $(wildcard test/*.c)

OBJS = $(SRC:.c=.o)

ifdef STATIC
	CFLAGS  = -DCURL_STATICLIB -std=c99 -Ideps -Isrc -Wall -Wno-unused-function -U__STRICT_ANSI__ $(shell deps/curl/bin/curl-config --cflags)
	LDFLAGS =  -static $(shell deps/curl/bin/curl-config --static-libs)
else
	CFLAGS  = -std=c99 -Ideps -Isrc -Wall -Wno-unused-function -U__STRICT_ANSI__ $(shell curl-config --cflags)
	LDFLAGS = $(shell curl-config --libs)
endif

all: $(BINS)

$(BINS): $(SRC) $(OBJS)
	$(CC) $(CFLAGS) -o $@ src/bin/$(@:.exe=).c $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $< -c -o $@ $(CFLAGS)

test: $(TEST_BIN)
	$(foreach t, $^, $(TEST_RUNNER) ./$(t) || exit 1;)

test/%: test/%.o $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	$(foreach c, $(BINS), $(RM) $(c);)
	$(foreach c, $(TEST_BINS), $(RM) $(c);)
	$(RM) $(OBJS)

install: $(BINS)
	$(MKDIR) $(PREFIX)/bin
	$(foreach c, $(BINS), $(CP) $(c) $(PREFIX)/bin/$(c);)

uninstall:
	$(foreach c, $(BINS), $(RM) $(PREFIX)/bin/$(c);)

test: $(TEST_BINS)
	@./test.sh

.PHONY: test all clean install uninstall
