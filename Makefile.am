SUBDIRS = include lib bin check

docdir = $(pkgdatadir)
doc_DATA = AUTHORS COPYING README NEWS ChangeLog
imagedir = $(pkgdatadir)
image_DATA = defaultmap.tiff

EXTRA_DIST = $(doc_DATA) $(image_DATA)

tags:
	find . -name '*.c' -o -name '*.h'|xargs ctags

