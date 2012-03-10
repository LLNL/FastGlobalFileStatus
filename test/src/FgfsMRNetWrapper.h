/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *        Jul 13 2011 DHA: File created.
 *
 */

#ifndef FGFS_MRNET_WRAPPER_H
#define FGFS_MRNET_WRAPPER_H 1

extern "C" {
  typedef struct _pd_t {
      char * host_name;
      char * executable_name;
      int pid;
  } MPIR_PROCDESC;

  extern MPIR_PROCDESC *MPIR_proctable;
  extern int MPIR_proctable_size;
  extern int MPIR_being_debugged;
  extern int MPIR_debug_state;
  extern int MPIR_partial_attach_ok;

# define MPIR_DEBUG_NULL      0
# define MPIR_DEBUG_SPAWNED   1
# define MPIR_DEBUG_ABORTING  2
}

#include "mrnet/MRNet.h"
#include "Comm/MRNetCommFabric.h"

const int MRNetHelloTag = FastGlobalFileStatus::CommLayer::MMT_place_holder;

extern int MRNet_Init(
       FastGlobalFileStatus::CommLayer::MRNetCompKind mrnetComponent,
       int *argcPtr, char ***argvPtr, char *daemon, char *topo,
       MRN::Network **netObjPtr, MRN::Stream **channelObjPtr);

extern int MRNet_Finalize(
       FastGlobalFileStatus::CommLayer::MRNetCompKind mrnetComponent,
       MRN::Network *netObjPtr, MRN::Stream *channelObjPtr);

#endif //FGFS_MRNET_WRAPPER_H

