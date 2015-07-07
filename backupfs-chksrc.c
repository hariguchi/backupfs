/* $Id: backupfs-chksrc.c,v 1.9 2005/04/21 04:46:01 cvsremote Exp $

   backupfs-chksrc.c: main for bkupfs-chksrc (run on remote host)


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


static char Buf[MAXCHARS];


static void
usage (void)
{
    fprintf(stderr, "%s\n" "Compiled: %s\n"
            "Usage: %s <src-dir>\n",
            VERSION, CompilationDate, PROGNAME_CHKSRC);
    exit(1);
}


int
main (int argc, char* argv[])
{
    int len;
    char* p;
    char* s;
    char* src;
    struct stat stbuf;


    if (argc <= 1) {
        usage();
    }

    len = strlen(argv[1]);
    src = malloc(len + 2);      /* 1 for tailing '/' */
    if (!src) {
        errSysExit(("malloc(%d)", len));
    }
    strcpy(src, argv[1]);
    src[len] = '/';          /* to make index() work in for{} later */
    src[len+1] = '\0';

    /* Src always ends with '/' so that
       for{} can also take care of the last directory
     */
    p = Buf;
    for (s = index(src+1, '/'); s; s = index(s+1, '/')) {
        *s = '\0';
        if (stat(src, &stbuf)) {
            errSysExit(("stat(%s)", src));
        }
        snprintf(p, sizeof(Buf) - (p - Buf), "0x%08x 0x%08x 0x%08x ",
                        (int)stbuf.st_uid, (int)stbuf.st_gid, stbuf.st_mode);
        p  += strlen(p);
        len = strlen(src);
        if (p + len + 2 - Buf > sizeof(Buf)) { /* 2 is for `\n' and `\0' */
            errExit(("%s: argument too long", argv[1]));
        }
        strcpy(p, src+1);
        p   += len - 1;
        *p++ = '\n';
        *s = '/';
    }
    fputs(Buf, stdout);
    exit(0);
}
