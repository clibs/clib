AC_INIT([cpplib], [1.8.0], [], [cpplib])
AC_CONFIG_AUX_DIR(build)
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([1.14 tar-pax subdir-objects foreign])
AM_SILENT_RULES([yes])
AC_PREREQ([2.69])
AC_CONFIG_HEADERS([src/config.h])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_RANLIB
AC_LANG_ASSERT([C])
AC_LANG([C++])

# Set target cpu.
arch=${target_cpu}
AC_SUBST(arch)
AC_MSG_NOTICE([Building for target cpu type $arch])
if test x"$arch" = x"aarch64"; then
  AC_DEFINE(HAVE_TARGET_AARCH64, 1, [Define if an aarch64 target is used])
fi
if test x"$arch" = x"x86_64"; then
  AC_DEFINE(HAVE_TARGET_X86_64, 1, [Define if an x86 64 target is used])
fi
AM_CONDITIONAL([HAVE_TARGET_AARCH64], [test x"$arch" = x"aarch64"])
AM_CONDITIONAL([HAVE_TARGET_X86_64], [test x"$arch" = x"x86_64"])

PKG_CHECK_MODULES(CURL, libcurl)

AC_MSG_NOTICE([Compiling With GCC 4.8 switches])
CFLAGS="-O2 -D_GCC48_ -std=c99 -g"
CPPFLAGS="-D_GCC48_ -Wall -fno-strict-aliasing -rdynamic -fexceptions \
   -Wno-unused \
   -Wno-narrowing \
   -Wno-sequence-point \
   -Wno-format "
CXXFLAGS="-O2 -D_GCC48_ -rdynamic -fexceptions -fnon-call-exceptions -fpermissive -std=c++0x \
   -Wno-unused \
   -Wno-narrowing \
   -Wno-sequence-point \
   -Wno-literal-suffix \
   -Wno-delete-non-virtual-dtor \
   -Wno-format \
   -Werror "

AC_ARG_ENABLE(debug, 
    AS_HELP_STRING([--enable-debug], [enable debugging, default: no]),
    [case "${enableval}" in
                 yes) debug=true ;;
                 no)  debug=false ;;
                 *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
    esac],
    [debug=false])
if test x"$debug" = x"true"; then
  CFLAGS+=" -g -D_DEBUG_"
  CXXFLAGS="-g -D_DEBUG_"
else
  CFLAGS+=" -DNDEBUG"
  CXXFLAGS+=" -DNDEBUG"
  CPPFLAGS+=" -DNDEBUG"
fi
AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")


AC_SUBST(VERSION)
AC_SUBST(INCREMENTAL_BUILD_TAG)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

