/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *
 *        Jun 27 2011 DHA: File created
 *
 */

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "DistDesc.h"
#include "MountPointAttr.h"

using namespace FastGlobalFileStatus;
using namespace FastGlobalFileStatus::CommLayer;
using namespace FastGlobalFileStatus::MountPointAttribute;

///////////////////////////////////////////////////////////////////
//
//  static data
//
//


///////////////////////////////////////////////////////////////////
//
//  PUBLIC INTERFACE:   namespace FastGlobalFileStatus::CommLayer
//
//

///////////////////////////////////////////////////////////////////
//
//  class ReduceDesc
//
//
ReduceDesc::ReduceDesc()
{
    rd[0] = 0;
    rd[1] = 0;
}


ReduceDesc::~ReduceDesc()
{

}


ReduceDesc::ReduceDesc(const ReduceDesc &rhs)
{
    rd[0] = rhs.rd[0];
    rd[0] = rhs.rd[1];
}


void
ReduceDesc::setFirstRank(FgfsId_t rnk)
{
    rd[0] = rnk;
}


void
ReduceDesc::countIncr()
{
    rd[1]++;
}


void
ReduceDesc::incrCountBy(FgfsCount_t incr)
{
    rd[1] += incr;
}


FgfsId_t
ReduceDesc::getFirstRank() const
{
    return (rd[0]);
}


FgfsCount_t
ReduceDesc::getCount() const
{
    return (FgfsCount_t (rd[1]));
}


///////////////////////////////////////////////////////////////////
//
//  class FgfsParDesc
//
//

FgfsParDesc::FgfsParDesc()
    : mGlobalRank(FGFS_NOT_FILLED),
      mGlobalMaster(false),
      mSize(FGFS_NOT_FILLED),
      numOfGroups(FGFS_NOT_FILLED),
      mGroupId(FGFS_NOT_FILLED),
      mRankInGroup(FGFS_NOT_FILLED),
      mGroupSize(FGFS_NOT_FILLED),
      mRepInGroup(FGFS_NOT_FILLED)
{

}


FgfsParDesc::FgfsParDesc(const FgfsParDesc &o)
{
    mGlobalRank = o.mGlobalRank;
    mGlobalMaster = o.mGlobalMaster;
    mSize = o.mSize;
    numOfGroups = o.numOfGroups;

    mGroupId = o.mGroupId;
    mRankInGroup = o.mRankInGroup;
    mGroupSize = o.mGroupSize;
    mRepInGroup = o.mRepInGroup;

    mUriString = o.mUriString;
    groupingMap = o.groupingMap;
}


FgfsParDesc::~FgfsParDesc()
{
    if (!groupingMap.empty()) {
        groupingMap.clear();
    }
}


FgfsParDesc &
FgfsParDesc::operator=(const FgfsParDesc &rhs)
{
    mGlobalRank = rhs.mGlobalRank;
    mGlobalMaster = rhs.mGlobalMaster;
    mSize = rhs.mSize;
    numOfGroups = rhs.numOfGroups;

    mGroupId = rhs.mGroupId;
    mRankInGroup = rhs.mRankInGroup;
    mGroupSize = rhs.mGroupSize;
    mRepInGroup = rhs.mRepInGroup;

    mUriString = rhs.mUriString;
    groupingMap = rhs.groupingMap;
    return *this;
}



FGFSInfoAnswer
FgfsParDesc::isRep() const
{
    FGFSInfoAnswer answer = ans_error;

    if(IS_YES(isGroupingDone())) {
        if (mRankInGroup == mRepInGroup) {
            answer = ans_yes;
        }
        else {
            answer = ans_no;
        }
    }

    return answer;
}


FGFSInfoAnswer
FgfsParDesc::isGroupingDone() const
{
    FGFSInfoAnswer answer = ans_no;

    if (mGroupId != FGFS_NOT_FILLED) {
        answer = ans_yes;
    }

    return answer;
}


FGFSInfoAnswer
FgfsParDesc::isSingleGroup() const
{
    FGFSInfoAnswer answer = ans_no;

    if (numOfGroups == 1) {
        answer = ans_yes;
    }

    return answer;
}


FGFSInfoAnswer
FgfsParDesc::isGlobalMaster() const
{
    FGFSInfoAnswer answer = ans_no;

    if (mGlobalMaster) {
        answer = ans_yes;
    }

    return answer;
}


void
FgfsParDesc::setRank(FgfsId_t rnk)
{
    mGlobalRank = rnk;
}


FgfsId_t 
FgfsParDesc::getRank() const
{
    return mGlobalRank;
}


void
FgfsParDesc::setGlobalMaster()
{
    mGlobalMaster = true;
}


void
FgfsParDesc::unsetGlobalMaster()
{
    mGlobalMaster = false;
}


void
FgfsParDesc::setSize(FgfsCount_t sz)
{
    mSize = sz;
}


FgfsCount_t
FgfsParDesc::getSize() const
{
    return mSize;
}


void
FgfsParDesc::setNumOfGroups(FgfsCount_t sz)
{
    numOfGroups = sz;
}


FgfsCount_t
FgfsParDesc::getNumOfGroups() const
{
    return numOfGroups;

}


void
FgfsParDesc::setGroupId(FgfsId_t gid)
{
    mGroupId = gid;
}


FgfsId_t
FgfsParDesc::getGroupId() const
{
    return mGroupId;
}


void
FgfsParDesc::setRankInGroup(FgfsId_t grnk)
{
    mRankInGroup = grnk;
}


FgfsId_t
FgfsParDesc::getRankInGroup() const
{
    return mRankInGroup;
}


void
FgfsParDesc::setGroupSize(FgfsCount_t sz)
{
    mGroupSize = sz;
}


FgfsCount_t
FgfsParDesc::getGroupSize() const
{
    return mGroupSize;
}


void
FgfsParDesc::setRepInGroup(FgfsId_t rnk)
{
    mRepInGroup = rnk;
}


FgfsId_t
FgfsParDesc::getRepInGroup() const
{
    return mRepInGroup;
}


void 
FgfsParDesc::setUriString(const std::string &uri)
{
    mUriString = uri;
}


const std::string & 
FgfsParDesc::getUriString() const
{
    return mUriString;
}


size_t
FgfsParDesc::pack(char *buf, size_t s) const
{
    char *t = buf;

    std::map<std::string, ReduceDesc>::const_iterator i;

    for(i = groupingMap.begin(); i != groupingMap.end(); ++i) {
        memcpy((void *)t, i->second.rd, sizeof(i->second.rd));
        t += sizeof(i->second.rd);
        memcpy((void *)t, i->first.c_str(), i->first.length() + 1);
        t += i->first.length() + 1;
    }

    return (size_t) (t - buf);
}


size_t
FgfsParDesc::unpack(char *buf, size_t s)
{
    char *t = buf;
    ReduceDesc redDescObj;
    std::string uriKey;

    //
    //  buffer format: rank|count|uriString1|rank|count|uriString2 ...
    //
    while ((size_t)(t - buf) < s) {
        memcpy((void *) redDescObj.rd, (void *)t, sizeof(redDescObj.rd));
        t += sizeof(redDescObj.rd);
        uriKey = t;
        t += uriKey.length() + 1;

        std::map<std::string, ReduceDesc>::iterator iter;
        iter = groupingMap.find(uriKey);
        if (iter != groupingMap.end()) {
            redDescObj.setFirstRank(iter->second.getFirstRank());
            redDescObj.incrCountBy(iter->second.getCount());
        }

        groupingMap[uriKey] = redDescObj;
    }

    return (size_t) (t - buf);
}


size_t
FgfsParDesc::packedSize() const
{
    std::map<std::string, ReduceDesc>::const_iterator i;
    size_t s = 0;

    for(i = groupingMap.begin(); i != groupingMap.end(); ++i) {
        s += (size_t) (sizeof(i->second.rd) + i->first.length() + 1);
    }

    return s;
}


FGFSInfoAnswer
FgfsParDesc::insert(std::string &item, ReduceDesc &robj)
{
    FGFSInfoAnswer answer = ans_yes;

    groupingMap[item] = robj;

    return answer;
}


FGFSInfoAnswer
FgfsParDesc::mapEmpty()
{
    return (groupingMap.empty())? ans_yes : ans_no;
}


FGFSInfoAnswer
FgfsParDesc::clearMap()
{
    FGFSInfoAnswer answer = ans_yes;

    if (!groupingMap.empty()) {
        groupingMap.clear();
    }
    else {
        answer = ans_error;
    }

    return answer;
}


bool 
FgfsParDesc::eliminateUriAlias()
{
    //
    // This currently only deals w/ a special case where only two
    // names are used for the hostname of a file source
    //
    
    if (!mGlobalMaster) {
        MPA_sayMessage("FgfsParDesc",
		       false,
		       "Slave invoked a master-only method...");
        return false;
    }


    if (groupingMap.size() != 2) {

        return false;
    }

    bool rc = false;
    unsigned long srcAddr1 = -1;
    unsigned long srcAddr2 = -1;
    bool isSymbolic1 = false;
    bool isSymbolic2 = false;
    unsigned delim; 

    std::map<std::string, ReduceDesc>::iterator iter;

    iter = groupingMap.begin();
    delim = iter->first.find_first_of("//");  
    if (delim != std::string::npos) {      
        std::string str1 
	  = iter->first.substr(delim+2);
	delim = str1.find_first_of("/");
	if (delim != std::string::npos) {
	    std::string srcName = str1.substr(0, delim);
	    if (srcName[0] >= 'A' && srcName[srcName.size()-1] >= 'A') {
	        isSymbolic1 = true;
	    }

            struct addrinfo *ai; // = (struct addrinfo *) malloc(sizeof(struct addrinfo));
            getaddrinfo(srcName.c_str(), NULL, NULL, &ai);
            if (ai->ai_addr->sa_family == AF_INET) {
                struct sockaddr_in *in = (struct sockaddr_in *) ai->ai_addr;
                srcAddr1 = in->sin_addr.s_addr; 
                //fprintf(stdout, "%s... %x\n", srcName.c_str(), srcAddr1);
                freeaddrinfo(ai); 
            }
	}
    }
  
    iter++;
    delim = iter->first.find_first_of("//");    
    if (delim != std::string::npos) {      
        std::string str1 
	  = iter->first.substr(delim+2);
	delim = str1.find_first_of("/");
	if (delim != std::string::npos) {
	    std::string srcName = str1.substr(0, delim);
	    if (srcName[0] >= 'A' && srcName[srcName.size()-1] >= 'A') {
	        isSymbolic2 = true;
	    }
            struct addrinfo *ai; // = (struct addrinfo *) malloc(sizeof(struct addrinfo));
            getaddrinfo(srcName.c_str(), NULL, NULL, &ai);
            if (ai->ai_addr->sa_family == AF_INET) {
                struct sockaddr_in *in = (struct sockaddr_in *) ai->ai_addr;
                srcAddr2 = in->sin_addr.s_addr; 
                //fprintf(stdout, "%s... %x\n", srcName.c_str(), srcAddr2);
                freeaddrinfo(ai); 
            }
	}
    }
    
    if ( (srcAddr1 != -1 && !srcAddr2 != -1) 
	 && (srcAddr1 == srcAddr2)) {
      //
      // Alias detected
      //
      iter = groupingMap.begin();
      iter++;
      if (isSymbolic1) {
	  iter->second.incrCountBy(
	      groupingMap.begin()->second.getCount());
	  groupingMap.erase(groupingMap.begin());
      }
      else {
	  groupingMap.begin()->second.incrCountBy(
              iter->second.getCount());
	  groupingMap.erase(iter);	
      }

      rc = true;
    }

    return rc;
}


bool 
FgfsParDesc::adjustUri()
{
    bool rc = false;
    if (groupingMap.find(mUriString) == groupingMap.end()) {
        if (groupingMap.size() == 1) {
            mUriString = groupingMap.begin()->first;
            rc = true;
        }
    }
    return rc;
}


std::map<std::string, ReduceDesc>&
FgfsParDesc::getGroupingMap()
{
    return groupingMap;
}



FGFSInfoAnswer
FgfsParDesc::setGroupInfo()
{
    FGFSInfoAnswer answer = ans_yes;

    std::map<std::string, ReduceDesc>::iterator iter;
    iter = groupingMap.find(mUriString);

    setNumOfGroups(groupingMap.size());

    if (iter != groupingMap.end()) {
        setRepInGroup(iter->second.getFirstRank());
        setGroupId(iter->second.getFirstRank());
#if 0
        if (getRepInGroup() == getRank()) {
            setRankInGroup(0);
        }
        else {
            setRankInGroup(getRank());
        }
#endif
        setRankInGroup(getRank());
        setGroupSize(iter->second.getCount());
    }
    else {
        answer = ans_error;
    }

    return answer;
}

