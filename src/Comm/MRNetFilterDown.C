/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *
 *        Jul 09 2011 DHA: Dropped multicast support for now
 *        Jul 09 2011 DHA: Copied from the old file
 *
 */

#include <vector>
#include "MountPointAttr.h"
#include "MRNetCommFabric.h"

using namespace FastGlobalFileStatus;
using namespace FastGlobalFileStatus::CommLayer;
using namespace FastGlobalFileStatus::MountPointAttribute;
using namespace MRN;

extern "C" {

const char *FGFSFilterDown_format_string = "";

void FGFSFilterDown(std::vector<PacketPtr> &in,
                    std::vector<PacketPtr> &out,
                    std::vector<PacketPtr> &, void **,
                    PacketPtr &, TopologyLocalInfo &)
{
    out.push_back(in[0]);

#if 0
    //
    // TODO: In case special downstream filtering is needed, please
    // comment this in and fill out details
    //
    unsigned int i;
    MRNetMsgType msgType = (MRNetMsgType) (in[0]->get_Tag());
    switch(msgType) {
        case MMT_op_init: {
            break;
        }

        default: {
            out.push_back(in[0]);
            break;
        }
    }
#endif

    return;
}

} // extern "C"
