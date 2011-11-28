dnl Process this file with autoconf to produce a configure script.

# get rid of that stupid cache mechanism
rm -f config.cache


m4_define([v_maj], [0])
m4_define([v_min], [1])
m4_define([v_mic], [0])
m4_define([v_ver], [v_maj.v_min.v_mic])

m4_define([lt_cur], m4_eval(v_maj + v_min))
m4_define([lt_rev], v_mic)
m4_define([lt_age], v_min)

AC_INIT([etwitt], [v_ver], puppetmaster@enlightenment.fr)
AC_PREREQ([2.52])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([1.6 dist-bzip2])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_LIBTOOL_WIN32_DLL
define([AC_LIBTOOL_LANG_CXX_CONFIG], [:])dnl
define([AC_LIBTOOL_LANG_F77_CONFIG], [:])dnl
AC_PROG_LIBTOOL

version_info="lt_cur:lt_rev:lt_age"
AC_SUBST([version_info])
VMAJ=v_maj
AC_SUBST([VMAJ])


### Needed information

AC_CANONICAL_BUILD
AC_CANONICAL_HOST


### Additional options to configure

EFL_WITH_BIN([edje], [edje-cc], [edje_cc])


### Checks for programs
AC_PROG_CC_C99

# pkg-config
PKG_PROG_PKG_CONFIG


### Checks for libraries

PKG_CHECK_MODULES([EBIRD], [eina eet ecore ecore-con oauth])
PKG_CHECK_MODULES([ETWITT], [eina evas ecore-file edje elementary oauth])
PKG_CHECK_MODULES([EBIRD_TEST], [eina ecore-file oauth])


### Checks for header files
AC_HEADER_STDC


### Checks for types


### Checks for structures


### Checks for compiler characteristics

AC_ISC_POSIX
AC_C_CONST
AM_PROG_CC_C_O
AC_C___ATTRIBUTE__

AC_CONFIG_FILES([
Makefile
data/Makefile
data/theme/Makefile
data/theme/phone/Makefile
src/Makefile
src/bin/Makefile
src/lib/Makefile
], [
])

AC_OUTPUT

### Info

echo
echo
echo
echo "------------------------------------------------------------------------"
echo "$PACKAGE_NAME $PACKAGE_VERSION"
echo "------------------------------------------------------------------------"
echo
echo "Configuration Options Summary:"
echo "  edje_cc..............: ${edje_cc}"
echo
echo "Compilation............: make (or gmake)"
echo "  CPPFLAGS.............: $CPPFLAGS"
echo "  CFLAGS...............: $CFLAGS"
echo "  LDFLAGS..............: $LDFLAGS"
echo
echo "Installation...........: make install (as root if needed, with 'su' or 'sudo')"
echo "  prefix...............: $prefix"
echo
