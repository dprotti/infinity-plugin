## Process this file with autoconf to produce a configure script.

AC_PREREQ(2.65)
AC_INIT([Infinity plugin],[0.9.0-dev],[https://github.com/dprotti/infinity-plugin/issues],[infinity-plugin],[https://dprotti.github.io/infinity-plugin])
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([1.11 -Wno-portability no-dist-gzip dist-xz tar-ustar])

# Support silent build rules. Disable by either passing --disable-silent-rules
# to configure or passing V=1 to make
AM_SILENT_RULES([yes])

AC_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])

AC_PREFIX_PROGRAM(audacious)

# Check for programs.
AC_PROG_CXX
AC_PROG_INSTALL

AC_DISABLE_STATIC
LT_INIT

AC_PATH_PROG(PKG_CONFIG, [pkg-config], [no])
if test x$PKG_CONFIG = xno ; then
  AC_MSG_ERROR([*** pkg-config not found. See http://www.freedesktop.org/software/pkgconfig/])
fi
if $PKG_CONFIG --atleast-pkgconfig-version 0.14 ; then
  :
else
  AC_MSG_ERROR([*** pkg-config too old; version 0.14 or better required.])
fi

# Check dependencies
PKG_CHECK_MODULES(INFINITY, glib-2.0 >= 2.28 sdl2 >= 2,,)
AC_SUBST(INFINITY_LIBS)
AC_SUBST(INFINITY_CFLAGS)

PKG_CHECK_MODULES(AUDACIOUS, audacious >= 3.5,,)
AC_SUBST(AUDACIOUS_LIBS)
AC_SUBST(AUDACIOUS_CFLAGS)

# Check for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Check for typedefs, structures, and compiler characteristics.
AC_C_VOLATILE
AC_C_CONST
AC_C_INLINE
AC_C_BIGENDIAN(AC_MSG_WARN([*** You have a big endian system, Infinity have not been tested on these systems]),,)

AC_CHECK_SIZEOF(long long)
if test x$ac_cv_sizeof_long_long = x8; then
  :
else
  AC_MSG_ERROR([
*** Infinity requires a 64 bit long long type. You might want to consider
*** using the GNU C compiler.
	])
fi

AC_CHECK_SIZEOF([size_t])
AC_MSG_CHECKING([whether compiler is 32 or 64 bit])
if test x$ac_cv_sizeof_size_t = x8; then
  AC_MSG_RESULT([64])
  compiler_width=64
else
  AC_MSG_RESULT([32])
  compiler_width=32
fi

# Check for library functions.
AC_CHECK_FUNCS([floor sqrt])

# Arguments to specify certain features.
AC_ARG_ENABLE([mmx],
            AS_HELP_STRING([--enable-mmx],[use MMX if available and if building with a 32 bit compiler @<:@default=enabled@:>@]),
	    [mmx=$enableval],
	    [mmx=yes])
AC_MSG_CHECKING([whether to enable MMX])
if test x$mmx = xyes; then
  if test x$compiler_width = x32; then
    AC_MSG_RESULT([yes])
    AC_DEFINE([MMX_DETECTION], [1], [Activate MMX Extensions support])
  else
    AC_MSG_RESULT([no, was requested but feature is not supported with a 64bit compiler])
  fi
else
  AC_MSG_RESULT([no])
fi

AC_ARG_ENABLE([debug],
            AS_HELP_STRING([--enable-debug],[turn on debug mode @<:@default=disabled@:>@]),
	    [debug=$enableval],
	    [debug=no])

AC_MSG_CHECKING([whether to activate debugging])
if test "$debug" = yes; then
  AC_MSG_RESULT([yes])
  AC_DEFINE([INFINITY_DEBUG], [1], [Activate Infinity debugging])
else
  AC_MSG_RESULT([no])
fi

INF_CFLAGS_EXTRA=''

if test x"$GCC" = xyes; then
  INF_CFLAGS_EXTRA="${INF_CFLAGS_EXTRA} -Wall -Wimplicit -Wunused -Wmissing-prototypes"
  CXXFLAGS="${CXXFLAGS} -std=c++11"
fi

AC_ARG_ENABLE([vectorization],
            AS_HELP_STRING([--enable-vectorization],[turn on vectorization optimizations for gcc >= 4 @<:@default=enabled@:>@]),
	    [vectorization=$enableval],
	    [vectorization=yes])

AC_MSG_CHECKING([whether to activate auto vectorization with gcc >= 4])
if test x"$vectorization" = xyes; then
  if test x"$ac_cv_prog_CC" = xgcc; then
    AC_MSG_RESULT([yes])
    case `$ac_cv_prog_CC --version | sed -e 's,\..*,.,' -e q` in
      *2. | *3.)
        AC_MSG_ERROR([You need gcc version 4 or newer to build with vectorization optimizations])
        ;;
      *)
        INF_CFLAGS_EXTRA="${INF_CFLAGS_EXTRA} -ftree-vectorize"
        ;;
    esac
  else
    AC_MSG_RESULT([no, was requested but compiler is not gcc])
  fi
else
  AC_MSG_RESULT([no])
fi

AC_SUBST(INF_CFLAGS_EXTRA)

# Define EXPORT
if test "x$HAVE_MSWINDOWS" = "xyes" ; then
    EXPORT="__declspec(dllexport)"
elif test "x$GCC" = "xyes" ; then
    CFLAGS="$CFLAGS -fvisibility=hidden"
    CXXFLAGS="$CXXFLAGS -fvisibility=hidden"
    EXPORT="__attribute__((visibility(\"default\")))"
else
    AC_MSG_ERROR([Unknown syntax for EXPORT keyword])
fi
AC_DEFINE_UNQUOTED([EXPORT], [$EXPORT], [Compiler syntax for export of public symbols])

# Hack for Ubuntu on AMD64
AC_CHECK_FILE("${prefix}/lib/x86_64-linux-gnu/libSDL2.so",
  libdir="${libdir}/x86_64-linux-gnu/audacious/Visualization",
  libdir="${libdir}/audacious/Visualization"
)

pkglibdir="${libdir}"
AC_SUBST(libdir)
AC_SUBST(pkglibdir)

AC_CONFIG_FILES([Makefile
  minidocs/Makefile
  src/Makefile])
AC_OUTPUT

echo "
==============================================================
Infinity Plugin for the Audacious Player -- version $VERSION

Compiler               : ${CC}
Install path           : ${libdir}
CFLAGS                 : ${INF_CFLAGS_EXTRA} ${CFLAGS}
CXXFLAGS               : ${CXXFLAGS}
Debug enabled          : ${debug}
"
