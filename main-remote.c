/* $Id: main-remote.c,v 1.9 2015/03/04 20:44:32 cvsremote Exp $

   main.c: main for bkupfs


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
#include "error.h"



static void
usage (void)
{
    fprintf(stderr, "%s\n" "Compiled: %s\n"
            "Usage: %s <src-dir> <backup-dir> <host> <time-in-hex>\n",
                        VERSION, CompilationDate, PROGNAME_REMOTE);
    exit(1);
}


int
main (int argc, char* argv[])
{
    bkupInfo info;
    bkupType type;
    int      i;
    int      len;
    int      rst;               /* return status */


    if (argc <= 4) {
        usage();
    }
    for ( i = 1; i <= 2; ++i ) {
        if (argv[i][0] != '/') goto errorExit; /* dirs must be full path */
        len = strlen(argv[i]) - 1;
        if (argv[i][len] == '/') {  /* strip tail '/' */
            argv[i][len] = '\0';
        }
    }

    umask(defUmask);
    memset(&info, 0, sizeof(info));
    info.src   = argv[1];
    info.bdir  = argv[2];
    info.blen  = strlen(info.bdir);
    info.host  = argv[3];
    info.dest  = argv[4];       /* use info.dest for time string */
    openFilesRemote(&info);
    type = chkSource(&info);
    openJournalFile(&info);

    switch (type) {
    case bkupFirstTime:
        info.func = firstTimeBackup;
        break;
    case bkupRecurrent:
        info.func = recurrentBackup;
        break;
    default:
        errRet(("wrong bkup type (%d)", type));
        backupfsExit(&info, 1);
    }
    writeDestDir(&info);
    rst = dirwalk(info.src, &info);
    if (!rst) {
        errRet(("dirwalk()"));
    }
    closeFiles(&info);
    if ((type == bkupRecurrent) && unlink(info.oldJpath)) {
        errSysRet(("unlink(%s)", info.oldJpath));
    }
    exit(0);


errorExit:
    fprintf(stderr, "Source directory must be full path\n");
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
