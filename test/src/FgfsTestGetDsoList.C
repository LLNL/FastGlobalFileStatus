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

extern "C" {
# include "config.h"
# include <unistd.h>
# include <stdlib.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# if HAVE_LIBELF_H
#  include LOCATION_OF_LIBELFHEADER
# else
#  error libelf.h is required
#endif
#include <sys/time.h>
#include <time.h>
}

#include <vector>
#include <string>
#include <algorithm>
#include "MountPointAttr.h"

#if __x86_64__
//
// 64 bit target
//
typedef Elf64_Shdr myElf_Shdr;
typedef Elf64_Dyn myElf_Dyn;
typedef Elf64_Addr myElf_Addr;
const char *DFLT_LIB_PATH = "/lib64:/usr/lib64";
# define myelf_getshdr elf64_getshdr
#else
typedef Elf32_Shdr myElf_Shdr;
typedef Elf32_Dyn myElf_Dyn;
typedef Elf32_Addr myElf_Addr;
const char *DFLT_LIB_PATH = "/lib:/usr/lib";
# define myelf_getshdr elf32_getshdr
#endif

using namespace FastGlobalFileStatus::MountPointAttribute;

int
getDependentDSOs (const std::string &execPath, std::vector<std::string> &dlibs)
{
    Elf_Kind kind;
    Elf *arf;
    Elf *elf_handler;
    int fd;

    if ( elf_version(EV_CURRENT) == EV_NONE ) {
        MPA_sayMessage("FgfsTestGetDsoList",
                       true,
                       "error in elf_version");
        return -1;
    }

    if ( ( fd = open (execPath.c_str(), O_RDONLY)) == -1 ) {
        MPA_sayMessage("FgfsTestGetDsoList",
                       true,
                       "error in openning %s",
                       execPath.c_str());
        return -1;
    }

    //
    // 2013/4/30 DHA: memcheck detects a memory leak as calling
    // elf_end doesn't seem to free it. 
    //
    if ( ( arf = elf_begin (fd, ELF_C_READ, NULL)) == NULL ) {
        MPA_sayMessage("FgfsTestGetDsoList",
                       true,
                       "error in elf_begin");
        return -1;
    }

    if ( ( kind = elf_kind (arf)) != ELF_K_ELF ) {
        MPA_sayMessage("FgfsTestGetDsoList",
                       true,
                       "error in elf_kind");
        return -1;
    }

    if ( ( elf_handler = elf_begin (fd, ELF_C_READ, arf)) == NULL ) {
        MPA_sayMessage("FgfsTestGetDsoList",
                       true,
                       "error in elf_begin");
        return -1;
    }

    Elf_Scn *sect = NULL;
    myElf_Shdr *shdr = NULL;
    char *dynstrtab = NULL;
    char *sh_strtab = NULL;
    std::vector<myElf_Addr> dynstrtab_offsets_libs;
    std::vector<myElf_Addr> dynstrtab_offsets_rpaths;

    while ( (sect = elf_nextscn(elf_handler, sect)) != NULL ) {
        if ( (shdr = myelf_getshdr (sect)) == NULL )
          continue;

        if ( shdr->sh_type == SHT_DYNAMIC ) {
            int ix;
            Elf_Data *sectdata;
            if (!(sectdata = elf_getdata ( sect, NULL)) ) {
                MPA_sayMessage("FgfsTestGetDsoList",
                               true,
                               "error in elf_begin");
              return -1;
            }

            for (ix=0; ix < (shdr->sh_size/shdr->sh_entsize); ++ix) {
                myElf_Dyn *dyndata = (myElf_Dyn *) sectdata->d_buf;
                dyndata += ix;

                switch (dyndata->d_tag) {
                case DT_NEEDED:
                    dynstrtab_offsets_libs.push_back(dyndata->d_un.d_ptr);
                    break;

                case DT_RPATH:
                    dynstrtab_offsets_rpaths.push_back(dyndata->d_un.d_ptr);
                    break;

                case DT_STRTAB:
                    dynstrtab = (char *) dyndata->d_un.d_ptr;
                    break;

                default:
                    break;
                }
            }
         }
         else if ( shdr->sh_type == SHT_STRTAB && shdr->sh_flags == 0) {
             Elf_Data *strsectd;
             if (!(strsectd = elf_getdata (sect, NULL))) {
                 MPA_sayMessage("FgfsTestGetDsoList",
                               true,
                               "elf_data returned null");
                 return -1;
             }

             std::string shname((char*)strsectd->d_buf+shdr->sh_name);
             if (std::string(".shstrtab") == shname) {
                 sh_strtab = (char*) strsectd->d_buf;
             }
         }
    }

    if (!sh_strtab) {
        MPA_sayMessage("FgfsTestGetDsoList",
                       true,
                       "section header string table is not found");

      //
      // This is just outright wrong returning -1
      //
      return -1;
    }


    //
    // searching for the interpreter
    //
    while ( (sect = elf_nextscn(elf_handler, sect)) != NULL ) {
        if ( (shdr = myelf_getshdr (sect)) == NULL )
            continue;

        if ( shdr->sh_type == SHT_PROGBITS) {
            if (std::string(".interp") == std::string(sh_strtab + shdr->sh_name)) {
                Elf_Data *elfdata;
                if (!(elfdata = elf_getdata(sect, NULL))) {
                    MPA_sayMessage("FgfsTestGetDsoList",
                                   true,
                                   "elf_data returned null");
                    return -1;
                }

                std::string interppath ((char *) elfdata->d_buf);
                if (access (interppath.c_str(), R_OK | X_OK) >= 0 ) {
                    dlibs.push_back(interppath);
                }
            }
        }
        else if (shdr->sh_type == SHT_STRTAB && shdr->sh_flags == SHF_ALLOC) {
            //
            // Looking for the dynamic symbol table that is loaded into
            // memory so sh_addr != 0 check
            //
            Elf_Data *strsectd;
            if (!(strsectd = elf_getdata (sect, NULL))) {
                MPA_sayMessage("FgfsTestGetDsoList",
                              true,
                              "elf_data returned null");
                return -1;
            }

            std::string shname((char*)sh_strtab+shdr->sh_name);
            if (std::string(".dynstr") == shname) {
                dynstrtab = (char*) strsectd->d_buf;
            }
         }
    }


    //
    // No interpreter? No dynamic string table? No DT_NEEDED entries?
    //
    if ( !dynstrtab || dynstrtab_offsets_libs.empty() ) {
        MPA_sayMessage("FgfsTestGetDsoList",
                       true,
                       "no dynamic library dependency?");

      //
      // Maybe no libraries need to be shipped returning 0
      //
      return 0;
    }


    //
    // Library search path order is RPATH, LD_LIBRARY_PATH and DFLT_LIB_PATH
    //
    std::vector<std::string> ld_lib_path;

    //
    // So RPATH
    //
    char *llp, *tllp, *t, *tok;
    std::vector<myElf_Addr>::iterator iter;
    for (iter = dynstrtab_offsets_rpaths.begin();
             iter != dynstrtab_offsets_rpaths.end(); ++iter) {
        tllp = strdup((char *)dynstrtab + (*iter));
        t = tllp;
        while ( (tok = strtok(t, ":")) != NULL ) {
            std::string apath((char *)tok);  
            if (std::find(ld_lib_path.begin(), ld_lib_path.end(), apath) 
		== ld_lib_path.end()) {
	      ld_lib_path.push_back(apath);
            }
            t = NULL;
        }
      free(tllp);
    }

    //
    // So LD_LIBRARY_PATH
    //
  if ( (llp = getenv("LD_LIBRARY_PATH")) != NULL) {
        tllp = strdup(llp);
        t = tllp;
        while ( (tok = strtok(t, ":")) != NULL ) {
            std::string apath(tok);
            if (std::find(ld_lib_path.begin(), ld_lib_path.end(), apath) 
		== ld_lib_path.end()) {
	      ld_lib_path.push_back(apath);
            }
            t = NULL;
        }
        free(tllp);
    }

    //
    // Next DFLT_LIB_PATH
    //
    tllp = strdup(DFLT_LIB_PATH);
    t = tllp;
    while ( (tok = strtok(t, ":")) != NULL) {
        std::string apath(tok);
        if (std::find(ld_lib_path.begin(), ld_lib_path.end(), apath) 
	    == ld_lib_path.end()) {
	  ld_lib_path.push_back(apath);
        }
        t = NULL;
    }
    free(tllp);

    //
    // Time to resolve libraries
    //
    std::vector<std::string>::iterator pathiter;
    for (iter = dynstrtab_offsets_libs.begin();
           iter != dynstrtab_offsets_libs.end(); ++iter) {
        bool resolved = false;
        std::string alib(dynstrtab + (*iter));
        if (alib.find('/') == alib.npos) {
            //
            // Base name only
            //
            for (pathiter = ld_lib_path.begin();
                 pathiter != ld_lib_path.end(); ++pathiter) {
                std::string pathtry = (*pathiter) + std::string("/") + alib;
                if (access(pathtry.c_str(), R_OK | X_OK) >= 0) {
                    dlibs.push_back(pathtry);
                    resolved = true; 
                    break;
                }
            }
        }
        else {
            //
            // With dirname
            //
            if (access(alib.c_str(), R_OK | X_OK) >= 0) {
                dlibs.push_back(alib);
                resolved = true;
            }
        }

        if (!resolved) {
            MPA_sayMessage("FgfsTestGetDsoList",
                           true,
                           "Couldn't resolve %s", alib.c_str());
        }
    }

    if (elf_end(elf_handler) < 0 ) {
        MPA_sayMessage("FgfsTestGetDsoList",
                       true,
                       "error in close");
        return -1;
    }

  close(fd);

  return 0;
}


uint32_t
stampstart()
{
    struct timeval  tv;
    struct timezone tz;
    struct tm      *tm;
    uint32_t         start;

    gettimeofday(&tv, &tz);
    tm = localtime(&tv.tv_sec);

    printf("TIMESTAMP-START\t  %d:%02d:%02d:%ld (~%ld ms)\n", tm->tm_hour,
               tm->tm_min, tm->tm_sec, (long) tv.tv_usec,
               tm->tm_hour * 3600 * 1000 + tm->tm_min * 60 * 1000 +
               tm->tm_sec * 1000 + tv.tv_usec / 1000);

    start = tm->tm_hour * 3600 * 1000 + tm->tm_min * 60 * 1000 +
                tm->tm_sec * 1000 + tv.tv_usec / 1000;

    return (start);
}


uint32_t
stampstop(uint32_t start)
{
    struct timeval  tv;
    struct timezone tz;
    struct tm      *tm;
    uint32_t         stop;

    gettimeofday(&tv, &tz);
    tm = localtime(&tv.tv_sec);

    stop = tm->tm_hour * 3600 * 1000 + tm->tm_min * 60 * 1000 +
                tm->tm_sec * 1000 + tv.tv_usec / 1000;

    printf("TIMESTAMP-END\t  %d:%02d:%02d:%ld (~%ld ms) \n", tm->tm_hour,
               tm->tm_min, tm->tm_sec, (long) tv.tv_usec,
               tm->tm_hour * 3600 * 1000 + tm->tm_min * 60 * 1000 +
               tm->tm_sec * 1000 + tv.tv_usec / 1000);

    printf("ELAPSED\t  %d ms\n", stop - start);

    return (stop);
}


