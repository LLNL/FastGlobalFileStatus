# $Header: $
#
# x_ac_mpa.m4
#
# --------------------------------------------------------------------------------
# Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
# the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
# LLNL-CODE-xxxxxx. All rights reserved.
# --------------------------------------------------------------------------------
# 
#   Update Log:
#         Jun 29 2008 DHA: File created. 
#
AC_DEFUN([X_AC_MPA], [  

  AC_MSG_CHECKING([specify an alternative path to the required Mount Point Attribute package])

  AC_ARG_WITH([mpa],  
    AS_HELP_STRING(--with-mpa=MPA_PREFIX,the path to MountPointAttr @<:@default=PREFIX@:>@), 
    [with_mpa=$withval],  
    [with_mpa="check"]
  )

  mpa_configured="no"
  if test "x$with_mpa" != "xcheck"; then 
    if test -f $with_mpa/include/MountPointAttr.h; then
      AC_SUBST(MPALOC, [$with_mpa])
      AC_SUBST(LIBMPA, [-lmpattr])
      mpa_configured="yes"
    else
      AC_MSG_ERROR([$with_mpa/include/MountPointAttr.h not found])
    fi
  else
    if test -f $prefix/include/MountPointAttr.h; then
      AC_SUBST(MPALOC, [$prefix])
      AC_SUBST(LIBMPA, [-lmpattr])
      mpa_configured="yes"
    else
      AC_MSG_ERROR([path $prefix/include/MountPointAttr.h not found])
    fi
  fi # with_mpa

  AC_MSG_RESULT($mpa_configured)
])



