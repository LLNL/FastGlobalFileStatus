/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *        Jul 01 2011 DHA: File created.
 *
 */

#ifndef FGFS_TEST_GET_DSO_LIST_H
#define FGFS_TEST_GET_DSO_LIST_H 1

#include <vector>
#include <string>

extern int getDependentDSOs (const std::string &execPath,
                             std::vector<std::string> &dlibs);

extern uint32_t stampstart();

extern uint32_t stampstop(uint32_t start);

#endif

