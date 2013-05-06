/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *        Jul 05 2011 DHA: Added the reduceMap interface
 *        Jun 27 2011 DHA: Changed the interface to support "stateless"
 *                         communication fabric. The most state is hold
 *                         in FgfsStatDec object that gets passed into
 *                         the interface.
 *        Jun 24 2011 DHA: File created. (Copied from old CommFabric.h)
 *
 */

#ifndef COMM_FABRIC_H
#define COMM_FABRIC_H 1

extern "C" {
#include <stdint.h>
}

#include <string>
#include <vector>
#include "FgfsCommon.h"
#include "DistDesc.h"

namespace FastGlobalFileStatus {

  namespace CommLayer {


    enum ReduceDataType {
        REDUCE_INT = 0,
        REDUCE_LONG_LONG_INT,
        REDUCE_CHAR_ARRAY,
        REDUCE_UNKNOWN_TYPE
    };


    enum ReduceOperator {
        REDUCE_MAX = 0,
        REDUCE_MIN,
        REDUCE_SUM,
        REDUCE_BOR,
        REDUCE_UNKNOWN_OP
    };


    /**
     * Defines the base communication fabric class. This class must be
     * dervided with the target communication fabric. The derived class
     * must implement the virtual methods providing scalable communication
     * mechanisms for the abstraction of the methods.
     */
    class CommFabric {
    public:

        /**
         *   CommFabric Ctor
         *
         */
        CommFabric();

        /**
         *   CommFabric Dtor
         *
         */
        virtual ~CommFabric();

        /**
         *   Virtual Interface: Class Static Initializer
         *
         *   @param[in] net opaque network object
         *   @param[in] channel opaque channel object
         *
         *   @return a bool value
         */
        static bool initialize(void *net=NULL, void *channel=NULL);

        /**
         *   Virtual Interface: allReduce
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
                               ReduceOperator op) const = 0;

        /**
         *   Virtual Interface: broadcast
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
                               FgfsCount_t len) const = 0;

        /**
         *   Virtual Interface: grouping
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
                              bool elimAlias) const = 0;


        /**
         *   Virtual Interface: grouping
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
                               bool elimAlias) const = 0;


        /**
         *   Virtual Interface: getRankSize
         *
         *   @param[out] rank pointer to an int
         *   @param[out] size pointer to an int
         *   @param[out] glMaster pointer to a bool (true if a global master)
         *
         *   @return a bool value
         */
        virtual bool getRankSize(int *rank, int *size, bool *glMaster) const = 0;

        /**
         *   Virtual Interface: return the net object
         *
         *   @return an opaque object pointer
         */
        virtual void *getNet();

        /**
         *   Virtual Interface: return the channel object

         *   @return an opaque object pointer
         */
        virtual void *getChannel();


    private:

        CommFabric(const CommFabric &c);

    };

  } // CommLayer namespace

} // FastGlobalFileStatus namespace

#endif // COMM_FABRIC_H

