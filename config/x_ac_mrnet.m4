AC_DEFUN([X_AC_MRNET], [
  AC_LANG_PUSH(C++)
  AC_ARG_WITH(mrnet,
    [AS_HELP_STRING([--with-mrnet=prefix],
      [Add the compile and link search paths for mrnet]
    )],
    [MRNET_CXXFLAGS="-I${withval}/include"
      MRNETPREFIX="${withval}"
      MRNET_LDFLAGS="-L${withval}/lib -Wl,-rpath=${withval}/lib"
    ],
    [CXXFLAGS=$CXXFLAGS
      MRNETPREFIX=""
    ]
  )
  mrn_incs=`ls -d $MRNETPREFIX/lib/*/include` 
  for mrn_inc in $mrn_incs
  do
    MRNET_CXXFLAGS="-I$mrn_inc $MRNET_CXXFLAGS"
  done

  TMP_CXXFLAGS=$CXXFLAGS
  CXXFLAGS="$MRNET_CXXFLAGS $CXXFLAGS"
  TMP_LDFLAGS=$LDFLAGS
  LDFLAGS="$LDFLAGS $MRNET_LDFLAGS"

  AC_CHECK_HEADER(mrnet/MRNet.h,
    [],
    [AC_MSG_ERROR([mrnet/MRNet.h is required.  Specify mrnet prefix with --with-mrnet])],
    AC_INCLUDES_DEFAULT
  )
  AC_MSG_CHECKING([Checking MRNet Version])
  mrnet_vers=-1
  AC_COMPILE_IFELSE([AC_LANG_SOURCE([#include "mrnet/MRNet.h"
    using namespace MRN;
    using namespace std;
    int main()
    {
      uint64_t bufLength;
      DataType type;
      DataElement pkt;
      pkt.get_array(&type, &bufLength);
    }])],
    [AC_DEFINE([MRNET40], [], [MRNet 4.0])
      mrnet_vers=4.0
    ]
  )
  if test $mrnet_vers = -1; then
    AC_MSG_ERROR([FGFS requires MRNet 4.0 or greater. Specify MRNet prefix with --with-mrnet])
  fi
  AC_MSG_RESULT([$mrnet_vers])
  AC_MSG_CHECKING(for libmrnet)
  TMP_LDFLAGS=$LDFLAGS
  AC_ARG_WITH(cray-xt,
    [],
    [mrnet_libs="-lmrnet -lxplat -lpthread -ldl -lalps -lxmlrpc -lalpsutil"],
    [mrnet_libs="-lmrnet -lxplat -lpthread -ldl"]
  )
  MRNET_LDFLAGS="$MRNET_LDFLAGS $mrnet_libs"
  LDFLAGS="$LDFLAGS $mrnet_libs"
  AC_LINK_IFELSE([AC_LANG_PROGRAM(#include "mrnet/MRNet.h"
    using namespace MRN;
    Network *net;)],
    [libmrnet_found=yes],
    [libmrnet_found=no]
  )
  AC_MSG_RESULT($libmrnet_found)
  if test "$libmrnet_found" = yes; then
    FELIBS="$FELIBS $mrnet_libs"
    BELIBS="$BELIBS $mrnet_libs"
  else
    AC_MSG_ERROR([libmrnet is required.  Specify libmrnet prefix with --with-mrnet])
  fi
  AC_LANG_POP(C++)
  CXXFLAGS="${TMP_CXXFLAGS}"
  LDFLAGS="${TMP_LDFLAGS}"

  AC_PATH_PROG([MRNETCOMMNODEBIN], [mrnet_commnode], [no], [$MRNETPREFIX/bin$PATH_SEPARATOR$PATH])
  if test $MRNETCOMMNODEBIN = no; then
    AC_MSG_ERROR([the mrnet_commnode executable is required.  Specify mrnet prefix with --with-mrnet])
  fi

  AC_PATH_PROG([MRNETTOPGENBIN], [mrnet_topgen], [no], [$MRNETPREFIX/bin$PATH_SEPARATOR$PATH])
  if test $MRNETTOPGENBIN = no; then
    AC_MSG_ERROR([the mrnet_topgen executable is required.  Specify mrnet prefix with --with-mrnet])
  fi
])

AC_SUBST(MRNETCOMMNODEBIN)
AC_SUBST(MRNETTOPGENBIN)
AC_SUBST(MRNET_CXXFLAGS)
AC_SUBST(MRNET_LDFLAGS)
