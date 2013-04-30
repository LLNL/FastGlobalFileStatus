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

extern "C" {
# include <unistd.h>
# include <limits.h>
}
#include <cstdio>
#include <cstdlib>

#include "FgfsMRNetWrapper.h"
#include "MountPointAttr.h"

using namespace MRN;
using namespace FastGlobalFileStatus;
using namespace FastGlobalFileStatus::MountPointAttribute;
using namespace FastGlobalFileStatus::CommLayer;

//
// Home location for those extern variables
//
MPIR_PROCDESC *MPIR_proctable = NULL;
int MPIR_proctable_size = 0;
int MPIR_being_debugged = 0;
int MPIR_debug_state = MPIR_DEBUG_NULL;
int MPIR_partial_attach_ok = 1;

extern "C" void MPIR_Breakpoint()
{
   // magic breakpoint
}


static void
fillProctable(const char *daemonpath, unsigned int count,
              unsigned char *debugBuf, unsigned int debugBufLen)
{
    unsigned char *trav = debugBuf;
    int i=0;
    char hn[PATH_MAX];

    // buffer format
    // |4:pid|+:host_name string|+:executable_name string
    // |4:pid|+:host_name string|+:executable_name string
    // ...

    MPIR_proctable_size = count+1;
    MPIR_proctable = (MPIR_PROCDESC *)
          malloc(MPIR_proctable_size * sizeof(MPIR_PROCDESC));

    unsigned int curLen = trav-debugBuf;

    if (gethostname(hn, PATH_MAX) < 0) {
        MPA_sayMessage("TEST",
                       true,
                       "gethostname return a neg rc");
    }

    MPIR_proctable[i].pid = getpid();
    MPIR_proctable[i].host_name = strdup(hn);
    MPIR_proctable[i].executable_name = strdup(daemonpath);
    i++;

    while (curLen < debugBufLen) {
        MPIR_proctable[i].pid = (*(int*)(trav));
        trav += sizeof(int);
        MPIR_proctable[i].host_name = strdup((const char*) trav);
        trav += (strlen((const char*)trav) + 1);
        MPIR_proctable[i].executable_name = strdup((const char*) trav);
        trav += (strlen((const char*)trav) + 1);
        curLen = trav-debugBuf;

#if 0
        MPA_sayMessage("TEST",
                       false,
                       "MPIR_proctable: %d:%s:%s",
                        MPIR_proctable[i].pid,
                        MPIR_proctable[i].executable_name,
                        MPIR_proctable[i].host_name);
#endif
       i++;
    }

    return;
}


static int
setupDebugFE(const char *daemonpath, Stream *strm)
{
    ///////////////////////////////////////////////////////////////////////////////
    //                                                                           //
    //                          DEBUG SUPPORT                                    //
    //                                                                           //
    ///////////////////////////////////////////////////////////////////////////////

    int rc = -1;
    int tag;
    unsigned int count;
    PacketPtr recvP;
    unsigned char *debugBuf;
    unsigned int debugBufLen;

    if (strm->send(MMT_debug_mpir, "%d", MMT_debug_mpir) == -1) {
        MPA_sayMessage("TEST",
                       true,
                       "Stream send returns an error.");
        goto ret_loc;
    }
 
    if (strm->recv(&tag, recvP) == -1) {
        MPA_sayMessage("TEST",
                       true,
                       "Stream recv returns an error.");
        goto ret_loc;
    }

    if (recvP->unpack("%d %auc", &count, &debugBuf, &debugBufLen) == -1) {
        MPA_sayMessage("TEST",
                       true,
                       "PacketPtr unpacked returns an error.");
        goto ret_loc;
    }

    fillProctable(daemonpath, count, debugBuf, debugBufLen);
    free(debugBuf);
    MPA_sayMessage("TEST", false,
            "Concurrency: %d", MPIR_proctable_size);
    MPIR_debug_state = MPIR_DEBUG_SPAWNED;
    MPIR_Breakpoint();

    if (strm->send(MMT_debug_mpir, "%d", MMT_debug_mpir) == -1) {
        MPA_sayMessage("TEST",
                       true,
                       "Stream send returns an error.");
        goto ret_loc;
    }

    rc = 0;

ret_loc:
    return rc;
    ///////////////////////////////////////////////////////////////////////////////
    //                                                                           //
    //                          DEBUG SUPPORT                                    //
    //                                                                           //
    ///////////////////////////////////////////////////////////////////////////////
}


static int
setupDebugBE(Stream *strm, const char *daemonpath)
{
    ///////////////////////////////////////////////////////////////////////////////
    //                                                                           //
    //                          DEBUG SUPPORT                                    //
    //                                                                           //
    ///////////////////////////////////////////////////////////////////////////////
    int tag = MMT_place_holder;
    int rc = -1;
    unsigned int daemonLen = 0;
    unsigned int hnLen = 0; 
    unsigned char *buf = (unsigned char *) malloc(PATH_MAX*sizeof(unsigned char*));
    char hn[PATH_MAX];
    unsigned char *t = NULL;
    int count;
    unsigned int totalLen;
    pid_t myPid = -1;
    PacketPtr recvP;
 
    if (strm->recv(&tag, recvP) == -1) {
        MPA_sayMessage("TEST",
                       true,
                       "Stream return error");

        goto ret_loc;
    }

    if (tag != MMT_debug_mpir) {
        MPA_sayMessage("TEST",
                       true,
                       "tag isn't MMT_debug_mpir");

        goto ret_loc;
    }

    myPid = getpid();

    if (gethostname(hn, PATH_MAX) < 0) {
        MPA_sayMessage("TEST",
                       true,
                       "gethostname return a neg rc");

        goto ret_loc;
    }

    t = buf;
    memcpy((void*)t, (void*)&myPid, sizeof(myPid));
    t += sizeof(myPid);

    hnLen = (strnlen(hn, 128) + 1);
    memcpy((void *)t, (void *)hn, hnLen);
    t += hnLen;

    daemonLen = (strnlen(daemonpath, PATH_MAX) + 1);
    memcpy((void *)t, (void *)daemonpath, daemonLen);
    count = 1;
    totalLen = (sizeof(myPid) + hnLen + daemonLen);

    if (strm->send(MMT_debug_mpir,
                   "%d %auc",
                   count,
                   buf, totalLen) == -1) {

        MPA_sayMessage("TEST",
                       true,
                       "Stream send return error");
        goto ret_loc;
    }

    if (strm->recv(&tag, recvP) == -1) {

        MPA_sayMessage("TEST",
                       true,
                       "Stream send return error");
        goto ret_loc;
    }

    if (tag != MMT_debug_mpir) {

        MPA_sayMessage("TEST",
                       true,
                       "tag isn't MMT_debug_mpir");
        goto ret_loc;
    }

    free (buf);
    rc = 0;

ret_loc:
    return rc;
    ///////////////////////////////////////////////////////////////////////////////
    //                                                                           //
    //                          DEBUG SUPPORT                                    //
    //                                                                           //
    ///////////////////////////////////////////////////////////////////////////////
}


int
MRNet_Init(MRNetCompKind mrnetComponent, int *argcPtr,
           char ***argvPtr, char *daemonpath, char *topo,
           Network **netObjPtr, Stream **channelObjPtr)
{
    int rc = -1;

    if (mrnetComponent == mck_frontEnd) {
        if (access(topo, R_OK) < 0) {
            MPA_sayMessage("TEST",
                           true,
                           "Can't access %s", topo);
            goto ret_loc;
        }

        (*netObjPtr) = Network::CreateNetworkFE(topo,
                                    daemonpath,
                                    (const char **)(*argvPtr));
        Communicator *commBC;
        commBC = (*netObjPtr)->get_BroadcastCommunicator();

        int upFltrId;
        upFltrId = (*netObjPtr)->load_FilterFunc(FGFS_UP_FILTER_SO_NAME,
                                                 FGFS_UP_FILTER_FN_NAME);
        if (upFltrId == -1) {
            MPA_sayMessage("TEST",
                           true,
                           "Failed to load an up-filter");
            goto ret_loc;
        }

        //
        // Note: In case a down-filter is needed, comment this in
        //       and pass downFilterId into the new_Stream method.
        //
        int downFilterId = (*netObjPtr)->load_FilterFunc(FGFS_DOWN_FILTER_SO_NAME,
                                                         FGFS_DOWN_FILTER_FN_NAME);
        if (downFilterId == -1) {
            MPA_sayMessage("TEST",
                           true,
                           "Failed to load an down-filter");

            return EXIT_FAILURE;
        }

        (*channelObjPtr) = (*netObjPtr)->new_Stream(commBC,
                                                    upFltrId,
                                                    SFILTER_WAITFORALL,
                                                    downFilterId);

        //
        // 4/30/2013 DHA: memScape reports a leak of commBC but it is
        // not clear if we are supposed to free this object at this
        // client level.
        //
        if ((*channelObjPtr)->send(MRNetHelloTag, "%d", MRNetHelloTag)) {
            MPA_sayMessage("TEST",
                           true,
                           "Stream send return an error");
            goto ret_loc;
        }

        (*channelObjPtr)->flush();
        setupDebugFE(daemonpath, *channelObjPtr);
        rc = 0;
    }
    else {
        int tag;
        int pbuf;
        PacketPtr recvP;

        (*netObjPtr) = Network::CreateNetworkBE(*argcPtr, *argvPtr);
        (*netObjPtr)->recv(&tag, recvP, channelObjPtr);
        if (recvP->unpack("%d", &pbuf) == -1) {
            MPA_sayMessage("TEST",
                           true,
                           "PacketPtr unpack returns an error?");
            goto ret_loc;
        }

        if ((tag != MRNetHelloTag) || (pbuf != MRNetHelloTag)) {
            MPA_sayMessage("TEST",
                           true,
                           "tag mismtach: Out of packet delivery?");

            goto ret_loc;
        }

        setupDebugBE(*channelObjPtr, daemonpath);
        rc = 0;
    }

ret_loc:
    return rc;
}


int MRNet_Finalize(MRNetCompKind mrnetComponent,
                   Network *netObjPtr, Stream *channelObjPtr)
{
    int rc = -1;
    int tag;
    PacketPtr recvP;

    if (mrnetComponent == mck_frontEnd) {

        if (channelObjPtr->send(MMT_debug_mpir, "%d", MMT_debug_mpir) == -1) {
            MPA_sayMessage("TEST",
                           true,
                           "Stream send returns an error.");
            goto ret_loc;
        }
        channelObjPtr->flush();

        if (channelObjPtr->recv(&tag, recvP) == -1) {
            MPA_sayMessage("TEST",
                           true,
                           "Stream recv returns an error.");
            goto ret_loc;
        }
        int tmp;
        unsigned int tmpsize;
        recvP->unpack("%auc", (unsigned char *) &tmp, &tmpsize);

        if (netObjPtr) {
            //delete channelObjPtr;
            delete netObjPtr;
        } 
        rc = 0;
    }
    else {
        if (channelObjPtr->recv(&tag, recvP) == -1) {
            MPA_sayMessage("TEST",
                           true,
                           "Stream send return error");
            goto ret_loc;
        }

        if (tag != MMT_debug_mpir) {
            MPA_sayMessage("TEST",
                           true,
                           "tag isn't MMT_debug_mpir");
            goto ret_loc;
        }

        int tmp = MMT_op_allreduce_int_max;
        if (channelObjPtr->send(MMT_op_allreduce_int_max, "%auc", 
                                (unsigned char *) &tmp, sizeof(tmp)) == -1) {
            MPA_sayMessage("TEST",
                           true,
                           "Stream send returns an error.");
            goto ret_loc;
        }
        channelObjPtr->flush();

        netObjPtr->waitfor_ShutDown();
        if (netObjPtr) {
            delete netObjPtr;
        }
        rc = 0;
    }

ret_loc:
   return rc;
}

