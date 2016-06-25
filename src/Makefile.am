## Process this file with automake to produce Makefile.in

lib_LTLIBRARIES = libinfinite.la
data_DATA = infinite_states

datadir = $(prefix)/share/audacious

# translations go into infinity's own directory not in audacious directory
infinity_datadir = @datadir@
localedir = $(infinity_datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

COMMON_FLAGS = @AUDACIOUS_CFLAGS@ @AUDCLIENT_CFLAGS@ @SDL_CFLAGS@ @GTK_CFLAGS@\
	@GLIB_CFLAGS@ $(INF_CFLAGS_EXTRA) -DDATADIR='"$(datadir)"'

AM_CFLAGS = @CFLAGS@ $(COMMON_FLAGS)
AM_CXXFLAGS = @CXXFLAGS@ $(COMMON_FLAGS)

libinfinite_la_LDFLAGS = ${CXX}
libinfinite_la_LIBADD = @AUDACIOUS_LIBS@ @AUDCLIENT_LIBS@ @SDL_LIBS@ @GTK_LIBS@ @GLIB_LIBS@
libinfinite_la_SOURCES = \
	main.cc \
	renderer.cc renderer.h\
	compute.c compute.h \
	display.c display.h \
	prefs.c prefs.h\
	effects.c effects.h\
	cputest.c cputest.h\
	mmx.h types.h

INCLUDES = -I$(top_srcdir)/src -I$(top_builddir)

EXTRA_DIST = infinite_states
