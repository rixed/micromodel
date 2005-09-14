if DEBUG
AM_CFLAGS = -I $(top_srcdir)/include -fstrict-aliasing -D_GNU_SOURCE -std=c99 -Wall -W -Wno-unused-parameter -Wno-uninitialized -pedantic
CFLAGS = -O0 -g
else
AM_CFLAGS = -I $(top_srcdir)/include -fstrict-aliasing -D_GNU_SOURCE -std=c99 -DNDEBUG
CFLAGS = -O2
endif

lib_LTLIBRARIES = libmicromodel.la
libmicromodel_la_SOURCES = \
	vertex.c \
	edge.c \
	facet.c \
	grid.c \
	grid.h \
	mml2bin.c \
	gridsel.c \
	basis.c \
	color.c \
	bevel.c \
	extrude.c \
	rules.c \
	rules.h \
	mcommander.c \
	gridsel.h \
	smooth.c \
	connect.c \
	convert.c \
	platon.c \
	separate.c \
	mapping.c \
	mirror.c

libmicromodel_la_LDFLAGS = -version-info @VERSION_INFO@ -lm -Wl,--warn-common

