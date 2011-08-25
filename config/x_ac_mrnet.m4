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
  AC_COMPILE_IFELSE([#include "mrnet/MRNet.h"
    using namespace MRN;
    int main()
    {
      Network *net;
      net->register_EventCallback(Event::TOPOLOGY_EVENT, TopologyEvent::TOPOL_ADD_BE, NULL, NULL);
    }],
    [AC_DEFINE([MRNET3], [], [MRNet 3.X])
      AC_DEFINE([MRNET22], [], [MRNet 2.2]) 
      AC_DEFINE([MRNET2], [], [MRNet 2.X])
      mrnet_vers=3.X
    ]
  )
  if test $mrnet_vers = -1; then
    AC_COMPILE_IFELSE([#include "mrnet/MRNet.h"
      using namespace MRN;
      Network *net=Network::CreateNetworkFE(NULL, NULL, NULL);],
      [AC_DEFINE([MRNET22], [], [MRNet 2.2]) 
        AC_DEFINE([MRNET2], [], [MRNet 2.X])
        mrnet_vers=2.2
      ]
    )
  fi
  if test $mrnet_vers = -1; then
    AC_COMPILE_IFELSE([#include "mrnet/MRNet.h"
      using namespace MRN;
      NetworkTopology *topo;],
      [AC_DEFINE([MRNET2], [], [MRNet 2.X]) mrnet_vers=2.X]
    )
  fi
  AC_MSG_RESULT([$mrnet_vers])
  AC_MSG_CHECKING(for libmrnet)
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
