/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *
 *        Jul 09 2011 DHA: Copied from the old file
 *
 */

#include <cstdio>
#include <vector>
#include <string>
#include <climits>
#include <unistd.h>

#include "MountPointAttr.h"
#include "MRNetCommFabric.h"

using namespace FastGlobalFileStatus;
using namespace FastGlobalFileStatus::CommLayer;
using namespace FastGlobalFileStatus::MountPointAttribute;
using namespace MRN;

FILE *mFilePtr = NULL;

extern "C" {

const char *FGFSFilterUp_format_string = "";

void FGFSFilterUp(std::vector<PacketPtr> &in,
                  std::vector<PacketPtr> &out,
                  std::vector<PacketPtr> &, void **,
                  PacketPtr &, TopologyLocalInfo &)
{
    unsigned int i;
    MRNetMsgType msgType = (MRNetMsgType) (in[0]->get_Tag());

    switch(msgType) {
        case MMT_op_allreduce_int_max: {
            int redu = INT_MIN;
            for (i=0; i < in.size(); ++i) {
                int localTag;
                int value;
                unsigned char *charray;
                unsigned int arrLen;
                PacketPtr curPacket = in[i];
                localTag = curPacket->get_Tag();
                curPacket->unpack("%auc", &charray, &arrLen);
                value = (*(int *)charray);
                free(charray);
                if (localTag != msgType) {
                    MPA_sayMessage("FGFSFilterUp: MMT_op_allreduce_int_max",
                        true,
                        "Different msg types: current(%d) vs. arrived(%d)",
                        MMT_op_allreduce_int_max,
                        localTag);

                    continue;
                }

                if (value > redu) {
                    redu = value;
                }
            }

            unsigned char *outArray 
                = (unsigned char *) malloc(sizeof(redu));
            memcpy(outArray, &redu, sizeof(redu));
            PacketPtr newPacket(new Packet(in[0]->get_StreamId(),
                                in[0]->get_Tag(),
                                "%auc",
                                outArray,
                                sizeof(redu)));

            newPacket->set_DestroyData(true);
            out.push_back(newPacket);
            break;
        }

        case MMT_op_allreduce_int_min: {
            int redu = INT_MAX;
            for (i=0; i < in.size(); ++i) {
                int localTag;
                int value;
                unsigned char *charray;
                unsigned int arrLen;
                PacketPtr curPacket = in[i];
                localTag = curPacket->get_Tag();
                curPacket->unpack("%auc", &charray, &arrLen);
                value = (*(int *)charray);
                free(charray);
                if (localTag != msgType) {
                    MPA_sayMessage("FGFSFilterUp: MMT_op_allreduce_int_min",
                        true,
                        "Different msg types: current(%d) vs. arrived(%d)",
                        MMT_op_allreduce_int_min,
                        localTag);

                    continue;
                }

                if (value < redu) {
                    redu = value;
                }
            }

            unsigned char *outArray 
                = (unsigned char *) malloc(sizeof(redu));
            memcpy(outArray, &redu, sizeof(redu));
            PacketPtr newPacket(new Packet(in[0]->get_StreamId(),
                                in[0]->get_Tag(),
                                "%auc",
                                outArray,
                                sizeof(redu)));

            newPacket->set_DestroyData(true);
            out.push_back(newPacket);
            break;
        }

        case MMT_op_allreduce_int_sum: {
            int redu = 0;
            for (i=0; i < in.size(); ++i) {
                int localTag;
                int value;
                unsigned char *charray;
                unsigned int arrLen;
                PacketPtr curPacket = in[i];
                localTag = curPacket->get_Tag();
                curPacket->unpack("%auc", &charray, &arrLen);
                value = (*(int *)charray);
                free(charray);
                if (localTag != msgType) {
                    MPA_sayMessage("FGFSFilterUp: MMT_op_allreduce_int_sum",
                        true,
                        "Different msg types: current(%d) vs. arrived(%d)",
                        MMT_op_allreduce_int_sum,
                        localTag);

                    continue;
                }

                redu += value;
            }

            unsigned char *outArray
                = (unsigned char *) malloc(sizeof(redu));
            memcpy(outArray, &redu, sizeof(redu));
            PacketPtr newPacket(new Packet(in[0]->get_StreamId(),
                                in[0]->get_Tag(),
                                "%auc",
                                outArray,
                                sizeof(redu)));

            newPacket->set_DestroyData(true);
            out.push_back(newPacket);
            break;
        }

        case MMT_op_allreduce_long_long_max: {
            long long int redu = LLONG_MIN;
            for (i=0; i < in.size(); ++i) {
                int localTag;
                long long int value;
                unsigned char *charray;
                unsigned int arrLen;
                PacketPtr curPacket = in[i];
                localTag = curPacket->get_Tag();
                curPacket->unpack("%auc", &charray, &arrLen);
                value = (*(long long int *)charray);
                free(charray);
                if (localTag != msgType) {
                    MPA_sayMessage("FGFSFilterUp: MMT_op_allreduce_long_long_max",
                        true,
                        "Different msg types: current(%d) vs. arrived(%d)",
                        MMT_op_allreduce_long_long_max,
                        localTag);

                    continue;
                }

                if (value > redu) {
                    redu = value;
                }
            }

            unsigned char *outArray 
                = (unsigned char *) malloc(sizeof(redu));
            memcpy(outArray, &redu, sizeof(redu));
            PacketPtr newPacket(new Packet(in[0]->get_StreamId(),
                                in[0]->get_Tag(),
                                "%auc",
                                outArray,
                                sizeof(redu)));

            newPacket->set_DestroyData(true);
            out.push_back(newPacket);
            break;
        }

        case MMT_op_allreduce_long_long_min: {
            long long int redu = LLONG_MAX;

            for (i=0; i < in.size(); ++i) {
                int localTag;
                long long int value;
                unsigned char *charray;
                unsigned int arrLen;
                PacketPtr curPacket = in[i];
                localTag = curPacket->get_Tag();
                curPacket->unpack("%auc", &charray, &arrLen);
                value = (*(long long int *)charray);
                free(charray);
                if (localTag != msgType) {
                    MPA_sayMessage("FGFSFilterUp: MMT_op_allreduce_long_long_min",
                        true,
                        "Different msg types: current(%d) vs. arrived(%d)",
                        MMT_op_allreduce_long_long_min,
                        localTag);

                    continue;
                }

                if (value < redu) {
                    redu = value;
                }
            }

            unsigned char *outArray 
                = (unsigned char *) malloc(sizeof(redu));
            memcpy(outArray, &redu, sizeof(redu));
            PacketPtr newPacket(new Packet(in[0]->get_StreamId(),
                                in[0]->get_Tag(),
                                "%auc",
                                outArray,
                                sizeof(redu)));

            newPacket->set_DestroyData(true);
            out.push_back(newPacket);
            break;
        }

        case MMT_op_allreduce_long_long_sum: {
            long long int redu = 0;

            for (i=0; i < in.size(); ++i) {
                int localTag;
                long long int value;
                unsigned char *charray;
                unsigned int arrLen;
                PacketPtr curPacket = in[i];
                localTag = curPacket->get_Tag();
                curPacket->unpack("%auc", &charray, &arrLen);
                value = (*(long long int *)charray);
                free(charray);
                if (localTag != msgType) {
                    MPA_sayMessage("FGFSFilterUp: MMT_op_allreduce_int_sum",
                        true,
                        "Different msg types: current(%d) vs. arrived(%d)",
                        MMT_op_allreduce_int_sum,
                        localTag);

                    continue;
                }

                redu += value;
            }

            unsigned char *outArray
                = (unsigned char *) malloc(sizeof(redu));
            memcpy(outArray, &redu, sizeof(redu));
            PacketPtr newPacket(new Packet(in[0]->get_StreamId(),
                                in[0]->get_Tag(),
                                "%auc",
                                outArray,
                                sizeof(redu)));

            newPacket->set_DestroyData(true);
            out.push_back(newPacket);
            break;
        }

        case MMT_op_allreduce_char_bor: {
            unsigned char *bor = NULL;
            unsigned char *charBor;
            unsigned int borSize=0;
            unsigned int rsize;

            for (i=0; i < in.size(); ++i) {
                int localTag;
                PacketPtr curPacket = in[i];
                localTag = curPacket->get_Tag();
                curPacket->unpack("%auc", &charBor, &rsize);

                if (localTag != msgType) {
                    MPA_sayMessage("FGFSFilterUp",
                        true,
                        "Different msg type: current(%d) vs. arrived(%d)",
                        MMT_op_allreduce_char_bor,
                        localTag);

                    continue;
                }

                if (!borSize) {
                    borSize = rsize;
                }
                else if (borSize != rsize) {
                    MPA_sayMessage("FGFSFilterUp: MMT_op_allreduce_char_bor",
                        true,
                        "Bit set size different for (%d)",
                        MMT_op_allreduce_char_bor);

                    continue;
                }

                if (!bor) {
                    bor = (unsigned char *) malloc((size_t)rsize);
                    borSize = rsize;
                    memcpy(bor, charBor, rsize);
                }
                else {
                    size_t j;
                    //
                    // From the upper layer, we know the bit array
                    // is sizeof(BloomFilterAlign_t)-byte aligned.
                    //
                    BloomFilterAlign_t *btrav;
                    BloomFilterAlign_t *newbtrav;
                    btrav = (BloomFilterAlign_t *) bor;
                    newbtrav = (BloomFilterAlign_t *) charBor;
                    for (j=0; j < borSize/sizeof(BloomFilterAlign_t); ++j) {
                         (*btrav) |= (*newbtrav);
                         btrav++;
                         newbtrav++;
                    }
                    free(charBor);
                }
            }

            PacketPtr newPacket(new Packet(in[0]->get_StreamId(),
                                in[0]->get_Tag(),
                                "%auc",
                                bor,
                                borSize));

            newPacket->set_DestroyData(true);
            out.push_back(newPacket);

            break;
        }

        case MMT_op_allreduce_map: 
        case MMT_op_allreduce_map_elim_alias: {

            FgfsParDesc pd;

            for (i=0; i < in.size(); ++i) {
                int localTag;
                unsigned char *buf;
                unsigned int bufSize = 0;

                PacketPtr curPacket = in[i];
                localTag = curPacket->get_Tag();
                curPacket->unpack("%auc", &buf, &bufSize);

                if (localTag != msgType) {
                    MPA_sayMessage("FGFSFilterUp",
                        true,
                        "Different msg type: current(%d) vs. arrived(%d)",
                        MMT_op_allreduce_char_bor,
                        localTag);

                    continue;
                }

                pd.unpack((char*) buf, (size_t) bufSize);
                //
                // 4/30/2013 DHA: totalview memScape reports a leak 
                // of buf, but it isn't clear if it is OK to free
                // this at the client level. Leaving leaked for now. 
                //
            }

            size_t packSize = pd.packedSize();
            unsigned char *sendBuf = (unsigned char *) malloc(packSize);
            if (!sendBuf) {
                MPA_sayMessage("FGFSFilterUp: MMT_op_allreduce_map",
                    true,
                    "malloc returned NULL");
            }
            pd.pack((char*)sendBuf, packSize);

            PacketPtr newPacket(new Packet(in[0]->get_StreamId(),
                                in[0]->get_Tag(),
                                "%auc",
                                sendBuf,
                                (unsigned int) packSize));

            newPacket->set_DestroyData(true);
            out.push_back(newPacket);

            break;
        }

        case MMT_debug_mpir: {

#if 0
            if (!mFilePtr) {
                char outfile[PATH_MAX];
                char hn[128];
                gethostname(hn, 128);
                pid_t pid = getpid();

                sprintf(outfile, "/g/g0/dahn/COMMNODE.out.%s.%d", hn, pid);
                mFilePtr = fopen(outfile, "w");
                MPA_registerMsgFd(mFilePtr, 2);
            }
#endif

            std::vector<unsigned int> lenVector;
            std::vector<unsigned char *> ucharVector;
            unsigned char *sendBuf = NULL;
            unsigned char *traverse = NULL;
            int globalCount = 0;
            unsigned int globalLen = 0;

            for (i=0; i < in.size(); ++i) {
                int localTag;
                int count;  
                unsigned char *value;
                unsigned int valueLen;
                PacketPtr curPacket = in[i];
                localTag = curPacket->get_Tag();
                curPacket->unpack("%d %auc",
                                  &count,
                                  &value,
                                  &valueLen);

                if (localTag != msgType) {
                    MPA_sayMessage("FGFSFilterUp: MMT_debug_mpir",
                        true,
                        "Different msg type: current(%d) vs. arrived(%d)",
                        MMT_debug_mpir,
                        localTag);

                    continue;
                }

                globalCount += count;
                globalLen += valueLen; 
                ucharVector.push_back(value);
                ucharVector[i] = value;
                lenVector.push_back(valueLen);
                //
                // 4/30/2013 DHA: totalview memScape reports a leak 
                // of unpack allocations, but it isn't clear if it is OK to free
                // these at this client level. Leaving leaked for now. 
                //
            }

            sendBuf = (unsigned char *)
                malloc(globalLen*sizeof(unsigned char)); 
            traverse = sendBuf;

            for(i=0; i < lenVector.size(); i++) {
                memcpy((void*)traverse, (void *) ucharVector[i], lenVector[i]);
                //free(ucharVector[ix]);
                //ucharVector[ix] = NULL;
                traverse += lenVector[i];
            }

            PacketPtr newPacket(new Packet(in[0]->get_StreamId(),
                                in[0]->get_Tag(),
                                "%d %auc",
                                globalCount,
                                sendBuf,
                                globalLen));
            newPacket->set_DestroyData(true);
            out.push_back(newPacket);

            break;
        }

        default: {
            MPA_sayMessage("FGFSFilterUp: default",
                           true,
                           "Unknown message types: %d",
                           msgType);
            out.push_back(in[0]);

            break;
        }
    }
}

} // extern "C"

