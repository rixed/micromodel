#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT()
AM_INIT_AUTOMAKE(libmicromodel, 1.2)

BACK_COMPAT=2
VERSION_INFO=`echo $VERSION | sed -e 's/\./:/'`:$BACK_COMPAT
AC_SUBST(VERSION_INFO)

AC_CONFIG_SRCDIR([config.h.in])
AM_CONFIG_HEADER(config.h)

AM_CONDITIONAL(DEBUG, test "$enable_debug" = yes)

# Checks for programs.
AC_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.

AC_ARG_WITH(
	cnt,
	AC_HELP_STRING([--with-cnt=path], [Set libcnt path]),
	[case "$withval" in
		(yes|no|"") ;;
		(*)
			LDFLAGS="-L$withval/lib $LDFLAGS"
			CPPFLAGS="-I$withval/include $CPPFLAGS"
			;;
	esac],
)
AC_CHECK_LIB(cnt, log_init,, AC_MSG_ERROR([libMicroModel requires libcnt (can probably be found in the same place)]))
AC_ARG_WITH(
	formsGL,
	AC_HELP_STRING([--with-formsGL=path], [Set libformsGL path]),
	[case "$withval" in
		(yes|no|"") ;;
		(*)
			LDFLAGS="-L$withval/lib $LDFLAGS"
			CPPFLAGS="-I$withval/include $CPPFLAGS"
			;;
	esac],
)
AC_ARG_WITH(
	forms,
	AC_HELP_STRING([--with-forms=path], [Set libforms path]),
	[case "$withval" in
		(yes|no|"") ;;
		(*)
			LDFLAGS="-L$withval/lib $LDFLAGS"
			CPPFLAGS="-I$withval/include $CPPFLAGS"
			;;
	esac],
)
AC_CHECK_LIB(formsGL, fl_activate_glcanvas,, AC_MSG_WARN([Cannot find libformsGL : please install it if you plan to use the MicroModeler]))
AC_CHECK_LIB(forms, fl_add_box,, AC_MSG_WARN([Cannot find libforms : please install it if you plan to use the MicroModeler]))
AC_CHECK_LIB(tiff, TIFFOpen,, AC_MSG_WARN([Cannot find libtiff : please install it if you plan to use the MicroModeler]))
AC_CHECK_LIB(GL, glBegin,, AC_MSG_WARN([Cannot find libGL : please install it if you plan to use the MicroModeler]))
AC_ARG_WITH(
	efence,
	AC_HELP_STRING([--with-efence=path], [Link with electric-fence]),
	[case "$withval" in
		(yes|"") AC_CHECK_LIB(efence, malloc)
			;;
		(no) ;;
		(*) ;;
	esac],
)

AM_CONDITIONAL(MMODELER, [echo $LIBS | grep -i 'lformsGL ' >/dev/null && echo $LIBS | grep 'ltiff ' >/dev/null])
# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h libcnt/cnt.h X11/forms.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_STDBOOL
AC_C_INLINE
AC_C_RESTRICT

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 include/Makefile
                 include/libmicromodel/Makefile
                 bin/Makefile
                 lib/Makefile
                 check/Makefile])
AC_OUTPUT
