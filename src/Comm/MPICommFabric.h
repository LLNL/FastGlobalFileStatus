/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *
 *        Jan 19 2011 DHA: File created.
 *
 */

#ifndef MPI_COMM_FABRIC_H
#define MPI_COMM_FABRIC_H 1

#include <map>
#include <mpi.h>
#include "CommFabric.h"

extern double accumTime;

namespace FastGlobalFileStatus {

  namespace CommLayer {

    /**
     *   FGFS_CUSTOM_REDUCTION_TAG
     *   Defines a msg tag used for custom reduction
     */
    const int FGFS_CUSTOM_REDUCTION_TAG = 49391;

    /**
     *
     * Defines the MPI-based communication fabric class.
     */
    class MPICommFabric: public CommFabric {
    public:
        /**
         *   MPICommFabric Ctor
         *
         */
        MPICommFabric();

        /**
         *   MPICommFabric Dtor
         *
         */
        virtual ~MPICommFabric();

        /**
         *   PtoP send
         *   @param[in] receiver receiver
         *   @param[in] pd an FgfsStatDesc objec
         */
        void send(int receiver, FgfsParDesc &pd) const;

        /**
         *   PtoP receive
         *   @param[in] sender receiver
         *   @param[in,out] pd an FgfsStatDesc objec
         */
        void receive(int sender, FgfsParDesc &pd);

        /**
         *   MPI-based Class Static Initializer
         *
         *   @param[in,out] argc int* arguments count
         *   @param[in,out] argv char*** argugments vector 
         *   @param[in] net opaque network object
         *   @param[in] channel opaque channel object
         *
         *   @return a bool value
         */
        static bool initialize(int *argc,
                               char ***argv,
                               void *net=NULL,
                               void *channel=NULL);

        /**
         *   MPI-based global allReduce
         *
         *   @param[in] global bool indicating global vs. group
         *   @param[in] pd an FgfsStatDesc object
         *   @param[in] s source buffer
         *   @param[out] r receiver buffer
         *   @param[in] len length of the buffer
         *   @param[in] t ReduceDataType
         *   @param[in] op ReduceOperator
         *
         *   @return a bool value
         */
        virtual bool allReduce(bool global,
                               FgfsParDesc &pd,
                               void *s,
                               void *r,
                               FgfsCount_t len,
                               ReduceDataType t,
                               ReduceOperator op) const;

        /**
         *   MPI-based global broadcast
         *
         *   @param[in] global bool indicating global vs. group
         *   @param[in] pd an FgfsStatDesc object
         *   @param[in] s source buffer
         *   @param[in] len length of the buffer
         *
         *   @return a bool value
         */
        virtual bool broadcast(bool global,
                               FgfsParDesc &pd,
                               unsigned char *s,
                               FgfsCount_t len) const;

        /**
         *   MPI-based global grouping
         *
         *   @param[in] global bool indicating global vs. group
         *   @param[out] pd an FgfsStatDesc object
         *   @param[in] item data item to group
         *   @param[in] elimAlias flag that forces elimination of uri aliases 
         *
         *   @return a bool value
         */
        virtual bool grouping(bool global,
                              FgfsParDesc &pd,
                              std::string &item,
                              bool elimAlias) const;


        /**
         *   MPI-based mapReduce
         *
         *   @param[in] global bool indicating global vs. group
         *   @param[out] pd an FgfsStatDesc object
         *   @param[in] itemList a item list containing unique item (vector type)
         *   @param[in] elimAlias flag that forces elimination of uri aliases 
         *
         *   @return a bool value
         */
        virtual bool mapReduce(bool global,
                               FgfsParDesc &pd,
                               std::vector<std::string> &itemList,
                               bool elimAlias) const;


        /**
         *   MPI-based global grouping
         *
         *   @param[out] rank pointer to an int
         *   @param[out] size pointer to an int
         *   @param[out] glMaster is this rank the global master
         *
         *   @return a bool value
         */
        virtual bool getRankSize(int *rank, int *size, bool *glMaster) const;


    private:

        MPI_Datatype getMPIDataType(ReduceDataType t) const;

        MPI_Op getMPIOp(ReduceOperator op) const;

        MPICommFabric(const CommFabric &c);

    };
  }
}

#endif // MPI_COMM_FABRIC_H

