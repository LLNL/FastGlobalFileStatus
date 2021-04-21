dnl $Header: $
dnl
dnl configure.ac
dnl
dnl --------------------------------------------------------------------------------
dnl Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
dnl the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
dnl LLNL-CODE-xxxxxx. All rights reserved. 
dnl
dnl   Update Log:
dnl         June 29 2011 DHA: File created.
dnl                          

AC_PREREQ(2.59)
AC_INIT(FastGlobalFileStatus, 1.1.1, ahn1@llnl.gov)
dnl Version numbers, remember to change them just *before* a release. 
dnl (Interfaces removed: CURRENT++, AGE=0, REVISION=0)
dnl (Interfaces added: CURRENT++, AGE++, REVISION=0)
dnl (No interfaces changed: REVISION++) 
FGFS_CURRENT=2
FGFS_REVISION=0
FGFS_AGE=1
AC_SUBST(FGFS_CURRENT)
AC_SUBST(FGFS_REVISION)
AC_SUBST(FGFS_AGE)
AC_CONFIG_AUX_DIR([config]) 
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([config])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE([enable])
AC_CANONICAL_HOST


dnl -----------------------------------------------
dnl enable debug support
dnl -----------------------------------------------
X_AC_ENABLE_DEBUG


dnl -----------------------------------------------
dnl Mount Point Attributes support
dnl -----------------------------------------------
X_AC_MPA


dnl -----------------------------------------------
dnl Maximum Distribution Degree support
dnl -----------------------------------------------
X_AC_DIST_DEGREE


dnl -----------------------------------------------
dnl Checks for programs.
dnl -----------------------------------------------
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_CC_C_O


dnl -----------------------------------------------
dnl Checks for MPI
dnl -----------------------------------------------
AC_LANG_PUSH([C++])
X_FIND_MPI
AC_LANG_POP([C++])


dnl -----------------------------------------------
dnl Checks for MRNet
dnl -----------------------------------------------
X_AC_MRNET


dnl -----------------------------------------------
dnl Checks for libraries.
dnl -----------------------------------------------
AC_MSG_CHECKING(for libelf)
AC_CHECK_LIB(elf,elf_begin,libelf_found=yes,libelf_found=no)
if test "$libelf_found" = yes; then
AC_SUBST(LIBELF,-lelf)
else
AC_MSG_ERROR([libelf is needed to build this package])
fi


dnl -----------------------------------------------
dnl Checks for header files.
dnl -----------------------------------------------
AC_LANG_PUSH([C++])
AC_CHECK_HEADERS([map vector iostream string sstream stdexcept])
AC_LANG_POP([C++])
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h limits.h stdint.h stdlib.h string.h unistd.h stdarg.h math.h string.h sys/mman.h sys/vfs.h sys/stat.h openssl/md5.h libelf.h libelf/libelf.h arpa/inet.h sys/types.h sys/sockets.h netdb.h])

if test "$ac_cv_header_libelf_h" = "yes" ; then
     AC_DEFINE(LOCATION_OF_LIBELFHEADER, [<libelf.h>],
         [Define to header that first defines elf.])
elif test "$ac_cv_header_libelf_libelf_h" = "yes"; then
     AC_DEFINE(HAVE_LIBELF_H, 1, [Define 1 if we have libelf/libelf.h])
     AC_DEFINE(LOCATION_OF_LIBELFHEADER, [<libelf/libelf.h>],
         [Define to header that first defines elf.])
fi


dnl -----------------------------------------------
dnl Checks for typedefs, structures, and compiler characteristics.
dnl -----------------------------------------------
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T


dnl -----------------------------------------------
dnl Checks for library functions.
dnl -----------------------------------------------
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([gettimeofday memset munmap])


dnl -----------------------------------------------
dnl configure's OUTPUTS 
dnl -----------------------------------------------
AC_CONFIG_FILES([Makefile
                 src/Makefile
                 test/Makefile
                 test/src/Makefile])

AC_OUTPUT
