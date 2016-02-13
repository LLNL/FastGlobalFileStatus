/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *
 *        Apr 30 2013 DHA: Fix a memory leak in mapReduce 
 *        Jan 19 2011 DHA: File created.
 *
 */

#include <mpi.h>
#include <cstdlib>
#include "MPIReduction.h"
#include "MPICommFabric.h"
#include "MountPointAttr.h"

using namespace FastGlobalFileStatus;
using namespace FastGlobalFileStatus::CommLayer;
using namespace FastGlobalFileStatus::MountPointAttribute;


///////////////////////////////////////////////////////////////////
//
//  static data
//
//
double accumTime = 0.0f;


///////////////////////////////////////////////////////////////////
//
//  PUBLIC INTERFACE:   namespace FastGlobalFileStatus::CommLayer
//
//

MPICommFabric::MPICommFabric()
{

}


MPICommFabric::~MPICommFabric()
{

}


bool
MPICommFabric::initialize(int *argc, char ***argv,
                          void *net, void *channel)
{
    int rc = false;
    int flag;

    if (CommFabric::initialize(net, channel)) {

        rc = MPI_Initialized (&flag);
        if (!flag) {
            rc = MPI_Init(argc, argv);
        }
    }

    return (rc == MPI_SUCCESS) ? true : false;
}


bool
MPICommFabric::allReduce(bool global,
                         FgfsParDesc &pd,
                         void *s, void *r,
                         FgfsCount_t len,
                         ReduceDataType t,
                         ReduceOperator op) const
{
    int rc;

    MPI_Datatype myType = getMPIDataType(t);

    if (myType == MPI_DOUBLE_COMPLEX) {
        //
        // This is an error condition
        //
        return false;
    }

    MPI_Op myOp = getMPIOp(op);
    if (myOp == MPI_MAXLOC) {
        //
        // This is an error condition
        //
        return false;
    }

    if (!global && IS_YES(pd.isGroupingDone())
         && IS_NO(pd.isSingleGroup()) )  {

        //
        // Mult-group case
        //
        MPI_Comm newComm;

        int key = IS_YES(pd.isRep())? 0 : 1;

        double d1, d2;
        d1 = MPI_Wtime();

        rc = MPI_Comm_split(MPI_COMM_WORLD,
                            pd.getGroupId(),
                            key,
                            &newComm);
        d2 = MPI_Wtime();
        accumTime += (d2 - d1);

        if (rc == MPI_SUCCESS) {
            rc = MPI_Allreduce((void *) s,
                               (void *) r,
                               len,
                               myType,
                               myOp,
                               newComm);
            MPI_Comm_free(&newComm);
        }
    }
    else {
        rc = MPI_Allreduce((void *) s,
                           (void *) r,
                           len,
                           myType,
                           myOp,
                           MPI_COMM_WORLD);
    }

    return (rc == MPI_SUCCESS) ? true : false;
}


bool
MPICommFabric::broadcast(bool global, FgfsParDesc &pd,
                         unsigned char *b, FgfsCount_t count) const
{
    int rc;

    if (!global && IS_YES(pd.isGroupingDone())
         && IS_NO(pd.isSingleGroup())) {

        //
        // Multi-group case
        //
        MPI_Comm newComm;

        int key = IS_YES(pd.isRep())? 0 : 1;

        double d1, d2;
        d1 = MPI_Wtime();

        rc = MPI_Comm_split(MPI_COMM_WORLD,
                            pd.getGroupId(),
                            key,
                            &newComm);
        d2 = MPI_Wtime();
        accumTime += (d2 - d1);

        if (rc == MPI_SUCCESS) {
            rc = MPI_Bcast((void *) b,
                           count,
                           MPI_UNSIGNED_CHAR,
                           0,
                           newComm);
            MPI_Comm_free(&newComm);
        }
    }
    else {
        rc = MPI_Bcast((void *) b,
                       count,
                       MPI_UNSIGNED_CHAR,
                       0,
                       MPI_COMM_WORLD);
    }

    return (rc == MPI_SUCCESS) ? true : false;
}


bool
MPICommFabric::grouping(bool global, 
                        FgfsParDesc &pd, 
                        std::string &item,
                        bool elimAlias) const
{
    if (!global) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("MPICommFabric",
                true,
                "Only global grouping is supported");
        }
        return false;
    }

    if (IS_NO(pd.mapEmpty())) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("MPICommFabric",
                true,
                "pd.groupingMap isn't empty");
        }
        return false;
    }

    pd.setUriString(item);
    std::vector<std::string> itemList;
    itemList.push_back(item);

    //
    // mapReduce may reset uriString of the pd object.
    //
    mapReduce(global, pd, itemList, elimAlias);

    pd.setGroupInfo();

    return true;
}


bool
MPICommFabric::mapReduce(bool global,
                         FgfsParDesc &pd,
                         std::vector<std::string> &itemList,
                         bool elimAlias) const
{
    std::vector<std::string>::iterator i;

    for (i=itemList.begin(); i != itemList.end(); ++i) {
        ReduceDesc redDescObj;
        redDescObj.setFirstRank(pd.getRank());
        redDescObj.countIncr();
        pd.insert(*i, redDescObj);
    }

    Reducer<MPICommFabric> *reducer = new BinomialReducer<MPICommFabric>;
    if (!reducer) {
      return false;
    }
    reducer->reduce(0, pd, (MPICommFabric *) this);

    //
    // When this is the global master and alias elimination is
    // wanted, 
    //
    if (IS_YES(pd.isGlobalMaster()) && elimAlias) {
        if (pd.eliminateUriAlias()) {
	    if (ChkVerbose(1)) {
                MPA_sayMessage("MPICommFabric",
                    false,
                    "Uri Alias eliminated");
	    }
	}
    }

    //
    // When your URI isn't there as a result of the elimniation,
    // you should adjust your URI as well. If not adjusted, 
    // grouping information will become bogus.
    //
    if (elimAlias) {
        if (pd.adjustUri()) {
            if (ChkVerbose(1)) {
                MPA_sayMessage("MRNetCommFabric",
                    false,
                    "URI adjusted");
                }
        }
    }


    int bufSize;
    if (pd.getRank() == 0) {
        bufSize = (int) pd.packedSize();
    }

    MPI_Bcast(&bufSize, 1, MPI_INT, 0, MPI_COMM_WORLD);

    char *bbuf = (char *) malloc(bufSize);
    if (!bbuf) {
      return false;
    }
    pd.pack(bbuf, bufSize);

    MPI_Bcast(bbuf, bufSize, MPI_CHAR, 0, MPI_COMM_WORLD);
    if (pd.getRank() != 0) {
        pd.clearMap();
        pd.unpack(bbuf, bufSize);
    }

    //
    // 2013/04/30: DHA memcheck detected a leak 
    //
    delete reducer; 
    free(bbuf);

    return true;
}


bool
MPICommFabric::getRankSize(int *rank, int *size, bool *glMaster) const
{
    int rc;

    MPI_Comm_rank(MPI_COMM_WORLD, (int *) rank);
    rc = MPI_Comm_size(MPI_COMM_WORLD, (int *) size);
    if (!(*rank)) {
        (*glMaster) = true;
    }
    else {
        (*glMaster) = false;
    }

    return (rc != MPI_SUCCESS)? false : true;
}


void
MPICommFabric::send(int receiver, FgfsParDesc &pd) const
{
    int bufSize = (int) pd.packedSize();
    MPI_Send((void *)&(bufSize), 1, MPI_INT,
             receiver, FGFS_CUSTOM_REDUCTION_TAG,
             MPI_COMM_WORLD);

    char *sendBuf = (char *) malloc(bufSize);
    pd.pack(sendBuf, bufSize);
    MPI_Send((void *)sendBuf, bufSize, MPI_CHAR,
             receiver, FGFS_CUSTOM_REDUCTION_TAG+1,
             MPI_COMM_WORLD);

    free(sendBuf);
    return;
}


void
MPICommFabric::receive(int sender, FgfsParDesc &pd)
{
    MPI_Status status;

    int bufSize;
    MPI_Recv((void *)&bufSize, 1, MPI_INT,
             sender, FGFS_CUSTOM_REDUCTION_TAG,
             MPI_COMM_WORLD, &status);

    char *recvBuf = (char *) malloc(bufSize);
    MPI_Recv((void *) recvBuf, bufSize, MPI_CHAR,
             sender, FGFS_CUSTOM_REDUCTION_TAG+1,
             MPI_COMM_WORLD, &status);

    pd.unpack(recvBuf, bufSize);
    free(recvBuf);
    return;
}


///////////////////////////////////////////////////////////////////
//
//  PRIVATE INTERFACE:   namespace FastGlobalFileStatus::CommLayer
//
//

MPICommFabric::MPICommFabric(const CommFabric &c)
{
    //
    // Making the copy constructor private preventing 
    // accidental copy
    //
}


MPI_Datatype
MPICommFabric::getMPIDataType(ReduceDataType t) const
{
    // MPI_DOUBLE_COMPLEX is the error type in this function
    MPI_Datatype rt = MPI_DOUBLE_COMPLEX;

    switch(t) {
    case REDUCE_INT:
        rt = MPI_INT;
        break;

    case REDUCE_LONG_LONG_INT:
        rt = MPI_LONG_LONG_INT;
        break;

    case REDUCE_CHAR_ARRAY:
        rt = MPI_CHAR;
        break;

    case REDUCE_UNKNOWN_TYPE:
    default:
        break;

    }

    return rt;
}


MPI_Op
MPICommFabric::getMPIOp(ReduceOperator op) const
{
    // MPI_MAXLOC is the error op in this function
    MPI_Op myOp = MPI_MAXLOC;

    switch(op) {
    case REDUCE_MAX:
        myOp = MPI_MAX;
        break;

    case REDUCE_MIN:
        myOp = MPI_MIN;
        break;

    case REDUCE_SUM:
        myOp = MPI_SUM;
        break;

    case REDUCE_BOR:
        myOp = MPI_BOR;
        break;

    case REDUCE_UNKNOWN_OP:
    default:
        break;

    }

    return myOp;
}
