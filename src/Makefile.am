## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libinfinity.la

lib_LTLIBRARIES = libinfinite.la

data_DATA = infinite_states

datadir = $(prefix)/share/infinity-plugin

COMMON_FLAGS = @INFINITY_CFLAGS@ \
	$(INF_CFLAGS_EXTRA) -DDATADIR='"$(datadir)"'

AM_CFLAGS = @CFLAGS@ $(COMMON_FLAGS)
AM_CXXFLAGS = @CXXFLAGS@ $(COMMON_FLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_builddir)

libinfinity_la_SOURCES = \
	infinity.c infinity.h \
	compute.c compute.h \
	display.c display.h \
	effects.c effects.h \
	music-player.h types.h

libinfinite_la_SOURCES = audacious.cc
libinfinite_la_LDFLAGS = ${CXX}
libinfinite_la_LIBADD = libinfinity.la @AUDACIOUS_LIBS@ @INFINITY_LIBS@
libinfinite_la_CFLAGS = @AUDACIOUS_CFLAGS@ $(COMMON_FLAGS)

EXTRA_DIST = infinite_states
