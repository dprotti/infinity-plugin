## Process this file with automake to produce Makefile.in

lib_LTLIBRARIES = libinfinite.la
data_DATA = infinite_states

datadir = $(prefix)/share/audacious

# translations go into infinity's own directory not in audacious directory
infinity_datadir = @datadir@
localedir = $(infinity_datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

AM_CFLAGS = @CFLAGS@ @AUDACIOUS_CFLAGS@ @AUDCLIENT_CFLAGS@ @SDL_CFLAGS@ @GTK_CFLAGS@\
	@GLIB_CFLAGS@ @DBUSGLIB1_CFLAGS@ $(INF_CFLAGS_EXTRA) -DDATADIR='"$(datadir)"'

libinfinite_la_LDFLAGS = -export-dynamic -avoid-version
libinfinite_la_LIBADD = @AUDACIOUS_LIBS@ @AUDCLIENT_LIBS@ @SDL_LIBS@ @GTK_LIBS@ @GLIB_LIBS@ @DBUSGLIB1_LIBS@
libinfinite_la_SOURCES = \
	main.c \
	renderer.c renderer.h\
	compute.c compute.h \
	display.c display.h \
	infconfig.c infconfig.h\
	config-dialog.c config-dialog.h\
	effects.c effects.h\
	cputest.c cputest.h\
	mmx.h gettext.h types.h

INCLUDES = -I$(top_srcdir)/src -I$(top_builddir)

EXTRA_DIST = infinite_states maintest.c spectrum.c

