# $Header: $
#
# x_ac_enable_debug.m4
#
# --------------------------------------------------------------------------------
# Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
# the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
# LLNL-CODE-xxxxxx. All rights reserved.
# --------------------------------------------------------------------------------
# 
#   Update Log:
#         May 23 2011 DHA: File created. 
#

AC_DEFUN([X_AC_ENABLE_DEBUG], [  
  AC_MSG_CHECKING([whether to enable debug codes])
  AC_ARG_ENABLE([debug], 
    AS_HELP_STRING(--enable-debug,enable debug codes), [
    if test "x$enableval" = "xyes"; then
      AC_DEFINE(DEBUG,1,[Define DEBUG flag])
      case "$CFLAGS" in
        *-O2*)
          CFLAGS=`echo $CFLAGS| sed s/-O2//g`
          ;;
        *-O*)
          CFLAGS=`echo $CFLAGS| sed s/-O//g`
          ;;
        *)
          ;;
      esac
      CFLAGS="-g -g -O0 $CFLAGS"
      CXXFLAGS="-g -g -O0 $CXXFLAGS"
      AC_MSG_RESULT([yes])
    else
      AC_MSG_RESULT([no])
    fi
    ], [
    AC_MSG_RESULT([no])
    ])
])
