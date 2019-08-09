CC     ?= cc
PREFIX ?= /usr/local

BINS = clib clib-install clib-search clib-init

ifdef EXE
	BINS := $(addsuffix .exe,$(BINS))
endif

CP      = cp -f
RM      = rm -f
MKDIR   = mkdir -p

SRC  = $(wildcard src/*.c)
DEPS = $(wildcard deps/*/*.c)
OBJS = $(DEPS:.c=.o)

export CC

ifdef STATIC
	CFLAGS  = -DCURL_STATICLIB -std=c99 -Ideps -Wall -Wno-unused-function -U__STRICT_ANSI__ $(shell deps/curl/bin/curl-config --cflags)
	LDFLAGS =  -static $(shell deps/curl/bin/curl-config --static-libs)
else
	CFLAGS  = -std=c99 -Ideps -Wall -Wno-unused-function -U__STRICT_ANSI__ $(shell curl-config --cflags)
	LDFLAGS = $(shell curl-config --libs)
endif

ifneq (0,$(PTHREADS))
ifndef NO_PTHREADS
	CFLAGS += $(shell ./scripts/feature-test-pthreads && echo "-DHAVE_PTHREADS=1 -pthread" || echo "-DHAVE_PTHREADS=0")
endif
endif

ifdef DEBUG
	CFLAGS += -g -D CLIB_DEBUG=1 -D DEBUG="$(DEBUG)"
endif

all: $(BINS)

$(BINS): $(SRC) $(OBJS)
	$(CC) $(CFLAGS) -o $@ src/$(@:.exe=).c $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $< -c -o $@ $(CFLAGS)

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
