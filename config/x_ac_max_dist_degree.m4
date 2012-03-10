# $Header: $
#
# x_ac_dist_degree.m4
#
# --------------------------------------------------------------------------------
# Copyright (c) 2012, Lawrence Livermore National Security, LLC. Produced at
# the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
# LLNL-CODE-xxxxxx. All rights reserved.
# --------------------------------------------------------------------------------
# 
#   Update Log:
#         Feb 27 2012 DHA: File created. 
#
AC_DEFUN([X_AC_DIST_DEGREE], [  

  AC_MSG_CHECKING([whether to specify the max num of unique servers serving a file path across a cluster])

  AC_ARG_WITH([max-dist-degree],  
    AS_HELP_STRING(--with-max-dist-degree=intVal,max num of unique servers serving a file path across a cluster),
    [with_dist_degree=$withval],  
    [with_dist_degree="check"]
  )

  res_msg="not given, bloom filter size is probably overestimated, leading to sub-optimal performance"

  if test "x$with_dist_degree" != "xcheck"; then 
    res_msg="$with_dist_degree"
    AC_DEFINE_UNQUOTED(MAX_DEGREE_DISTRIBUTION,$with_dist_degree,[number of unique file servers serving a logical file path that a cluster can see])
  fi # with_mpa

  AC_MSG_RESULT($res_msg)
])

