AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}
DISTCHECK_CONFIGURE_FLAGS = --enable-warnings=fatal --disable-scrollkeeper --disable-update-mime-database
AM_CPPFLAGS = -Ideps -Isrc
AM_CPPFLAGS += $(CURL_CFLAGS)

bin_PROGRAMS = cpplib cpplib-search cpplib-install

LIBS += \
    -lpthread \
    -lstdc++ \
    -lrt\
    -lz

deps__a_SOURCES =
include $(top_srcdir)/deps.mk

cpplib_SOURCES = $(deps__a_SOURCES) src/cpplib.c
cpplib_LDADD = $(CURL_LIBS)

cpplib_search_SOURCES = $(deps__a_SOURCES) src/cpplib-search.c
cpplib_search_LDADD = $(CURL_LIBS)

cpplib_install_SOURCES = $(deps__a_SOURCES) src/cpplib-install.c
cpplib_install_LDADD = $(CURL_LIBS)
