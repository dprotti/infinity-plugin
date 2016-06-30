## Process this file with automake to produce Makefile.in

lib_LTLIBRARIES = libinfinite.la
data_DATA = infinite_states

datadir = $(prefix)/share/audacious

COMMON_FLAGS = @AUDACIOUS_CFLAGS@ @SDL_CFLAGS@ @GLIB_CFLAGS@ \
	$(INF_CFLAGS_EXTRA) -DDATADIR='"$(datadir)"'

AM_CFLAGS = @CFLAGS@ $(COMMON_FLAGS)
AM_CXXFLAGS = @CXXFLAGS@ $(COMMON_FLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_builddir)

libinfinite_la_LDFLAGS = ${CXX}
libinfinite_la_LIBADD = @AUDACIOUS_LIBS@ @SDL_LIBS@ @GLIB_LIBS@
libinfinite_la_SOURCES = \
	audacious.cc \
	infinity.c infinity.h \
	compute.c compute.h \
	display.c display.h \
	effects.c effects.h \
	cputest.c cputest.h \
	mmx.h music-player.h types.h

EXTRA_DIST = infinite_states
