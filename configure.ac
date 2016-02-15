## Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([Infinity plugin],[0.8.0beta2],[dprotti@users.sourceforge.net],[infinity-plugin-4-audacious])
AM_INIT_AUTOMAKE([1.9.0 dist-bzip2])

AC_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])

AC_PREFIX_PROGRAM(audacious)

AC_CANONICAL_TARGET

# Check for programs.
AC_PROG_CC
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

# Check some dependencies
PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.8,,)
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.8,,)
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)

PKG_CHECK_MODULES(DBUSGLIB1, dbus-glib-1 >= 0.92,,)
AC_SUBST(DBUSGLIB1)
AC_SUBST(DBUSGLIB1)

PKG_CHECK_MODULES(AUDACIOUS, audacious >= 3.5,,)
AC_SUBST(AUDACIOUS_LIBS)
AC_SUBST(AUDACIOUS_CFLAGS)

PKG_CHECK_MODULES(AUDCLIENT, audclient >= 3.5,,)
AC_SUBST(AUDCLIENT_LIBS)
AC_SUBST(AUDCLIENT_CFLAGS)

AC_PATH_PROG(SDL_CONFIG, [sdl-config], [no])
if test x$SDL_CONFIG = xno ; then
  AC_MSG_ERROR([
*** sdl-config not found.
*** If you have installed from binaries, probably you have not 
*** installed SDL development package.
*** See http://www.libsdl.org
])
fi
AM_PATH_SDL(1.0.6,,AC_MSG_ERROR([*** SDL >= 1.0.6 not installed - please install first]))

# Check for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([libintl.h locale.h stdlib.h string.h unistd.h])

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
	    
# Check for library functions.
AC_CHECK_FUNCS([floor setlocale sqrt setlocale])

# Arguments to specify certain features.
AC_ARG_ENABLE([mmx],
            AS_HELP_STRING([--enable-mmx],[Turn on MMX detection support @<:@default=enabled@:>@]),
	    [mmx=$enableval],
	    [mmx=no])
AC_MSG_CHECKING([whether to enable MMX detection support])
if test x$mmx = xyes; then
  AC_MSG_RESULT([yes])
  AC_DEFINE([MMX_DETECTION], [1], [Activate MMX Extensions support])
else
  AC_MSG_RESULT([no])
fi

AC_ARG_ENABLE([debug],
            AS_HELP_STRING([--enable-debug],[Turn on debug mode @<:@default=disabled@:>@]),
	    [debug=$enableval],
	    [debug=no])

AC_MSG_CHECKING([whether to activate debugging])
if test "$debug" = yes; then
  AC_MSG_RESULT([yes])
  AC_DEFINE([INFINITY_DEBUG], [1], [Activate Infinity debugging])
else
  AC_MSG_RESULT([no])
fi

# Internationalization
ALL_LINGUAS="es es_AR"
GETTEXT_PACKAGE=infinity
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])
AM_GLIB_GNU_GETTEXT
#AC_PROG_INTLTOOL

INF_CFLAGS_EXTRA=''

if test x"$GCC" = xyes; then
  INF_CFLAGS_EXTRA="${INF_CFLAGS_EXTRA} -Wall -Wimplicit -Wunused -Wmissing-prototypes"
fi

AC_ARG_ENABLE([vectorization],
            AS_HELP_STRING([--enable-vectorization],[Turn on vectorization optimizations (gcc >= 4 only) @<:@default=enabled@:>@]),
	    [vectorization=$enableval],
	    [vectorization=yes])

AC_MSG_CHECKING([whether to activate vectorization under gcc >= 4])
if test x"$vectorization" = xyes; then
  AC_MSG_RESULT([yes])
  if test x"$GCC" = xno; then
    AC_MSG_ERROR([You are not using a gcc compiler])
  else
    # Some distros call gcc 4.x as gcc4
    if test -z "$ac_cv_prog_CC" || test x"$CC" = xgcc4; then
      our_gcc="$CC"
    else
      our_gcc="$ac_cv_prog_CC"
    fi
    case `$our_gcc --version | sed -e 's,\..*,.,' -e q` in
      *2. | *3.)
        AC_MSG_ERROR([You need gcc version 4 or newer to build with vectorization optimizations])
        ;;
      *)
        INF_CFLAGS_EXTRA="${INF_CFLAGS_EXTRA} -ftree-vectorize"
	case "$target_cpu" in
	  i*86|ia64*)
            # TODO check for sse2 availability
	    INF_CFLAGS_EXTRA="${INF_CFLAGS_EXTRA} -msse"
	    ;;
       	  powerpc|ppc)
	    INF_CFLAGS_EXTRA="${INF_CFLAGS_EXTRA} -maltivec"
	    ;;
	  *)
	    ;;
	esac
        ;;
    esac
  fi
else
  AC_MSG_RESULT([no])
fi

AC_SUBST(INF_CFLAGS_EXTRA)

AC_CHECK_FILE("${prefix}/lib/x86_64-linux-gnu/libSDL.so",
# Ubuntu on AMD64 puts libraries into x86_64-linux-gnu/
  libdir="${libdir}/x86_64-linux-gnu/audacious/Visualization",
  libdir="${libdir}/audacious/Visualization"
)

pkglibdir="${libdir}"
AC_SUBST(libdir)
AC_SUBST(pkglibdir)

AC_CONFIG_FILES([Makefile
		 po/Makefile.in
                 src/Makefile])
AC_OUTPUT

echo "
==============================================================
Infinity Plugin for the Audacious Player -- version $VERSION
==============================================================

Install path           : ${libdir}
Compiler               : ${CC}
CFLAGS		       : ${INF_CFLAGS_EXTRA} ${CFLAGS}
Debug enabled          : ${debug}
MMX Extensions support : ${mmx}
"

