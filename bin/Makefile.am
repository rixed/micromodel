if DEBUG
AM_CFLAGS = -I $(top_srcdir)/include -fstrict-aliasing -D_GNU_SOURCE -std=c99 -Wall -W -Wno-unused-parameter -Wno-uninitialized -pedantic
CFLAGS = -g -O0
else
AM_CFLAGS = -I $(top_srcdir)/include -fstrict-aliasing -D_GNU_SOURCE -std=c99 -DNDEBUG
CFLAGS = -O2
endif

if MMODELER
bin_PROGRAMS = mmodeler
mmodeler_SOURCES = \
	mmodeler.c \
	mmodel_ui.c \
	mmodel_cb.c \
	grid2gcl.c \
	mmodeler.h \
	mmodel_ui.h \
	grid2gcl.h \
	srcbuf.c \
	srcbuf.h \
	uv2triangles.h \
	uv2triangles.c

mmodeler_LDADD = ../lib/libmicromodel.la -lm
endif

