CC     ?= cc
PREFIX ?= /usr/local

BINS = clib clib-install clib-search clib-init clib-configure clib-build clib-update clib-upgrade

ifdef EXE
	BINS := $(addsuffix .exe,$(BINS))
endif

CP      = cp -f
RM      = rm -f
MKDIR   = mkdir -p

SRC  = $(wildcard src/*.c)
SDEPS = $(wildcard deps/*/*.c)
ODEPS = $(SDEPS:.c=.o)
DEPS = $(filter-out $(ODEPS), $(SDEPS))
OBJS = $(DEPS:.c=.o)
MAKEFILES = $(wildcard deps/*/Makefile)

export CC

ifdef STATIC
	CFLAGS  += -DCURL_STATICLIB -std=c99 -Ideps -Wall -Wno-unused-function -U__STRICT_ANSI__ $(shell deps/curl/bin/curl-config --cflags)
	LDFLAGS += -static $(shell deps/curl/bin/curl-config --static-libs)
else
	CFLAGS  += -std=c99 -Ideps -Wall -Wno-unused-function -U__STRICT_ANSI__ $(shell curl-config --cflags)
	LDFLAGS += $(shell curl-config --libs)
endif

ifneq (0,$(PTHREADS))
ifndef NO_PTHREADS
	CFLAGS += $(shell ./scripts/feature-test-pthreads && echo "-DHAVE_PTHREADS=1 -pthread" || echo "-DHAVE_PTHREADS=0")
endif
endif

ifdef DEBUG
	CFLAGS += -g -D CLIB_DEBUG=1 -D DEBUG="$(DEBUG)"
endif

default: all

all: $(BINS)

build: $(BINS)

$(BINS): $(SRC) $(MAKEFILES) $(OBJS)
	$(CC) $(CFLAGS) -o $@ src/$(@:.exe=).c $(OBJS) $(LDFLAGS)

$(MAKEFILES):
	$(MAKE) -C $@

%.o: %.c
	$(CC) $< -c -o $@ $(CFLAGS) -MMD

clean:
	$(foreach c, $(BINS), $(RM) $(c);)
	$(RM) $(OBJS)
	$(RM) $(AUTODEPS)

install: $(BINS)
	$(MKDIR) $(PREFIX)/bin
	$(foreach c, $(BINS), $(CP) $(c) $(PREFIX)/bin/$(c);)

uninstall:
	$(foreach c, $(BINS), $(RM) $(PREFIX)/bin/$(c);)

test:
	@./test.sh

# create a list of auto dependencies
AUTODEPS:= $(patsubst %.c,%.d, $(DEPS)) $(patsubst %.c,%.d, $(SRC))

# include by auto dependencies
-include $(AUTODEPS)


.PHONY: test all clean install uninstall
