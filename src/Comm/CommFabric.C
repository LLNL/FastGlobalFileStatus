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

#include "CommFabric.h"

using namespace FastGlobalFileStatus;
using namespace FastGlobalFileStatus::CommLayer;

///////////////////////////////////////////////////////////////////
//
//  static data
//
//

///////////////////////////////////////////////////////////////////
//
//  PUBLIC INTERFACE:   namespace FastGlobalFileStatus
//
//

///////////////////////////////////////////////////////////////////
//
//  class CommFabric
//
//

CommFabric::CommFabric()
{

}


CommFabric::~CommFabric()
{

}


bool
CommFabric::initialize(void *net, void *channel)
{
    return true;
}


void *
CommFabric::getNet()
{
    return NULL;
}


void *
CommFabric::getChannel()
{
    return NULL;
}

