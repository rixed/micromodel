AM_CFLAGS = -Wall -W -Wno-unused-parameter -I $(top_srcdir)/include -std=c99 -pedantic -D_GNU_SOURCE -fstrict-aliasing

check_PROGRAMS = pantin_check mml_check

pantin_check_SOURCES = pantin_check.c
pantin_check_LDADD = ../lib/libmicromodel.la -lm -lefence -lpthread

mml_check_SOURCES = mml_check.c
mml_check_LDADD = ../lib/libmicromodel.la -lm -lefence -lpthread

TESTS = pantin_check mml_check

