/* $Id: main-local.c,v 1.15 2005/04/21 04:46:02 cvsremote Exp $

   main-local.c: main for bkupfs


   Copyright (c) 2005, Yoichi Hariguchi
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

       o Redistributions of source code must retain the above copyright
         notice, this list of conditions and the following disclaimer.
       o Redistributions in binary form must reproduce the above
         copyright notice, this list of conditions and the following
         disclaimer in the documentation and/or other materials provided
         with the distribution.
       o Neither the name of the Yoichi Hariguchi nor the names of its
         contributors may be used to endorse or promote products derived
         from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "backupfs.h"
#include "platform.h"
#include "error.h"


static char DefaultUser[] = DEFAULT_USER;


static void
usage (void)
{
    fprintf(stderr, "%s\n" "Compiled: %s\n"
            "Usage: %s [[user@]host:]<src-dir> <dst-dir>\n",
            VERSION, CompilationDate, PROGNAME);
    exit(1);
}


int
main (int argc, char* argv[])
{
    bkupInfo info;
    bkupType type;
    int      len;
    int      rst;               /* return status */


    if (getuid() != ROOT_UID) {
        fprintf(stderr, "must be root\n");
        exit(1);
    }
    if (argc <= 2) {
        usage();
    }
    umask(defUmask);
    memset(&info, 0, sizeof(info));

    info.dest = argv[2];
    info.src  = index(argv[1], ':');
    if (info.src) {
        info.host = index(argv[1], '@');
        if (!info.host || info.host > info.src) {
            /* no user specified, or `@' appeared after `:'
             */
            info.host = argv[1];
            info.user = DefaultUser;
        } else {
            *info.host = '\0';
            ++info.host;
            info.user = argv[1];
        }
        *info.src = '\0';
        ++info.src;
    } else {
        info.src  = argv[1];
    }
    if (*info.src  != '/') goto errorExit;  /* src  must be full path */
    if (*info.dest != '/') goto errorExit;  /* dest must be full path */
    len = strlen(info.src) - 1;
    if (info.src[len] == '/') {  /* strip tail '/' */
        info.src[len] = '\0';
    }
    len = strlen(info.dest) - 1;
    if (info.dest[len] == '/') {
        info.dest[len] = '\0';
    }

    if (info.host) {
        makeSshKey(&info);
    }
    chkDest(&info);
    if (info.host) {
        exit(doRemote(&info));
    }

    type = chkSource(&info);
    openFilesLocal(&info);

    switch (type) {
    case bkupFirstTime:
        info.func = firstTimeBackup;
        break;
    case bkupRecurrent:
        info.func = recurrentBackup;
        break;
    default:
        errExit(("wrong bkup type (%d)", type));
    }
    rst = dirwalk(info.src, &info);
    closeFiles(&info);
    if ((type == bkupRecurrent) && unlink(info.oldJpath)) {
        errSysRet(("unlink(%s)", info.oldJpath));
    }
    rst = runCommands(&info);
    removeFiles(&info);
    if (type == bkupFirstTime && !rst) {
        if (info.jpath && unlink(info.jpath)) {
            errSysRet(("unlink(%s)", info.jpath));
        }
    }
    exit(!rst);


errorExit:
    fprintf(stderr,
            "Source and Destination directories must be full path\n");
    exit(1);
    return 0;                   /* to make gcc happy */
}


void
backupfsExit (bkupInfo* info, int exitStatus)
{
    assert(info);

    closeFiles(info);
    removeFiles(info);
    moveFile(info->oldJpath, info->jpath);
    exit(exitStatus);
}
