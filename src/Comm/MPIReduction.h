/*
 * reduction.h
 *
 * Template classes to perform different tree-based reduction algorithms.
 * The reductions are used primary when counting the number of times edges
 * are present in SMMs globally. This is done before compressing SSMs.
 *
 *  Created on: Jan 6, 2011
 *      Author: Ignacio Laguna
 *     Contact: ilaguna@purdue.edu
 *
 *  Update: June 27 2011 DHA Changed interface to pass
 *                           the reference to a FgfsStatDesc object
 *
 */

#ifndef MPI_REDUCTION_H
#define MPI_REDUCTION_H

extern "C" {
#include <math.h>
}

#include <iostream>
#include "DistDesc.h"


namespace FastGlobalFileStatus {

  namespace CommLayer {

    /**
     * Abstraction to calculate 'senders' and 'receivers'
     * in a tree-based reduction.
     *
     * The class T needs to implement two member functions:
     * send(int destination)
     * receive(int source)
     *
     * The reduction operations are performed inside these functions.
     */
    template <class T>
    class Reducer {
    protected:
        Reducer(){}

    public:
        virtual ~Reducer(){}

        /**
         * Virtual reduction operation (implemented in derived classes)
         */
        virtual void reduce(int root, FgfsParDesc &pd, T *reducedObject) = 0;
    };


    /**
     * ----------------------------------
     * Binomial Tree Reduction from MPICH
     * ----------------------------------
     *
     * This algorithm is adapted from MPICH. It performs a binomial-tree reduction
     * assuming that the operations are commutative. The root process can be
     * specified in the reduce() function.
     */
    template <class T>
    class BinomialReducer : public Reducer<T> {
    public:
         void reduce(int root, FgfsParDesc &pd, T *reducedObject)
         {
             int mask = 0x1, relrank, source, destination;
             relrank = (pd.getRank() - root + pd.getSize()) % pd.getSize();
             while (mask < pd.getSize()) {
                 // Receive
                 if ((mask & relrank) == 0) {
                     source = (relrank | mask);
                     if (source < pd.getSize()) {
                         source = (source + root) % pd.getSize();
                         //cout << "Proc " << processRank << " recv from " << source << endl;
                         reducedObject->receive(source, pd);
                     }
                 } else {
                     // I've received all that I'm going to.  Send my result to my parent
                     destination = ((relrank & (~ mask)) + root) % pd.getSize();
                     //cout << "Proc " << processRank << " send to " << destination << endl;
                     reducedObject->send(destination, pd);
                     break;
                 }
                 mask <<= 1;
             }
        }
    };

  } // CommLayer namespace
} // FastGlobalFileStatus namespace

#endif // MPI_REDUCTION_H
