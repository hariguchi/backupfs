/* $Id: backupfs-mkdir.c,v 1.7 2005/04/21 04:46:01 cvsremote Exp $

   backupfs-mkdir.c: main for bkupfs-mkdir (run on local host)


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

#define _GNU_SOURCE

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "backupfs.h"
#include "platform.h"
#include "error.h"


static int Debug = 0;           /* debug flag */


static void
usage (void)
{
    fprintf(stderr, "%s\n" "Compiled: %s\n"
            "Usage: %s\n", VERSION, CompilationDate, PROGNAME_MKDIR);
    exit(1);
}


int
main (int argc, char* argv[])
{
    ssize_t     len;
    int         i;
    size_t      bufSize;
    mode_t      mode;
    uid_t       uid;
    gid_t       gid;
    struct stat stbuf;
    char*       line;
    char*       p;
    char*       q;


    if (!Debug && getuid() != ROOT_UID) {
        fprintf(stderr, "must be root\n");
        exit(1);
    }
    if (argc > 1) {
        usage();
    }


    p       = NULL;
    bufSize = MAXCHARS;
    line = malloc(bufSize);
    if (!line) {
        errSysExit(("malloc(%d)", bufSize));
    }
    for (;;) {
        len = getline(&line, &bufSize, stdin);
        if (len < 0) {
            if (feof(stdin)) exit(0);
            exit(1);
        }

        line[len-1] = '\0';
        p   = line;
        for (i = 0; i < 3; ++i) {
            q = index(p, ' ');
            switch (i) {
            case 0:
                uid = strtol(p, NULL, 16);
                break;
            case 1:
                gid = strtol(p, NULL, 16);
                break;
            case 2:
                mode = strtol(p, NULL, 16);
                break;
            default:
                errExit(("%d: shouldn't happen", i));
                break;
            }
            p = q + 1;
        }
        if (Debug) {
            printf("%5d %5d 0x%08x %s\n", (int)uid, (int)gid, mode, p);
        } else if (!stat(p, &stbuf)) {
            if (S_ISDIR(stbuf.st_mode)) {
                if (chown(p, uid, gid)) {
                    errSysRet(("chown(%s, 0x%08x, 0x%08x)", p, uid, gid));
                } else {
                    if (chmod(p, mode)){
                        errSysRet(("chmod(%s, 0x%08x)", p, mode));
                    }
                }
            } else {
                errSysRet(("%s: not a directory", p));
            }
        } else if (errno == ENOENT) {
            if (mkdir(p, mode)) {
                errSysRet(("mkdir(%s, 0x%08x)", p, mode));
            } else {
                if (chown(p, uid, gid)) {
                    errSysRet(("chown(%s, 0x%08x, 0x%08x)", p, uid, gid));
                }
            }
        } else {
            errSysRet(("stat(%s)", p));
        }
    }
    free(line);
    exit(0);
}
