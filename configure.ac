#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([Kayrebt::Callgraphs], [0.1], [laurent.georget@supelec.fr])
AM_INIT_AUTOMAKE([1.11 foreign -Wall -Werror])
AC_CONFIG_SRCDIR([gcc-plugin/callgraphs.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])
AC_LANG([C++])

# Checks for programs.
AC_PROG_CXX([g++])
AX_COMPILER_VENDOR
AM_CONDITIONAL([GXX_AVAIL], [test x${ax_cv_cxx_compiler_vendor} = xgnu])
AM_COND_IF([GXX_AVAIL], [:],
	   [AC_MSG_ERROR([g++ is required to build gcc plugins])]
	  )
AX_COMPILER_VERSION
gcc_ver=`echo ${ax_cv_cxx_compiler_version} | awk -F'[[^[0-9]]*]' '{print (1000 * $1 + $2)}'`
AM_CONDITIONAL([GXX_4_8_AVAIL], [test [${gcc_ver}] -eq 4008])
AM_COND_IF([GXX_4_8_AVAIL], [:],
	   [AC_MSG_ERROR([g++ version 4.8 exactly is required])]
	  )
AC_CHECK_PROGS([DOT], [dot])
if test -z "$DOT";
then AC_MSG_WARN([[Graphs will not be available in Doxygen documentation]])
fi
AC_CHECK_PROGS([DOXYGEN], [doxygen])
if test -z "$DOXYGEN";
   then AC_MSG_WARN([[Doxygen not found - continuing without Doxygen support]])
fi
AM_CONDITIONAL([HAVE_DOXYGEN],[test -n "$DOXYGEN"])
AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([Doxyfile custom.css])])
AM_CONDITIONAL([HAVE_DOT],[test -n "$DOT"])
AM_COND_IF([HAVE_DOT], [AC_SUBST(DOT_IS_AVAILABLE,["YES"])], [AC_SUBST(DOT_IS_AVAILABLE,["NO"])])

# To build nice relocatable objects (GCC plugins are akin to libraries)
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT

# Checks for libraries.
PKG_CHECK_MODULES([SQLITE3],[sqlite3])

# Checks for header files.
AX_GCC_PLUGIN

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.

AC_CONFIG_FILES([Makefile gcc-plugin/Makefile])
AC_SUBST([SQLITE3_CFLAGS])
AC_SUBST([SQLITE3_LIBS])
AC_OUTPUT
