/* $Id: backupfs-exctar.c,v 1.5 2005/04/21 04:46:01 cvsremote Exp $

   backupfs-exctar.c: main for bkupfs-exctar (run on remote host)


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


static char File[MAXCHARS];


static void
usage (void)
{
    fprintf(stderr, "%s\n" "Compiled: %s\n"
            "Usage: %s <host> <time-in-hex>\n",
            VERSION, CompilationDate, PROGNAME_CHKSRC);
    exit(1);
}


int
main (int argc, char* argv[])
{
    int   len;
    char* host;
    char* tm;                   /* time in hex */


    if (argc <= 2) {
        usage();
    }

    host = argv[1];
    tm   = argv[2];
    len  = sizeof(File);
    if (snprintf(File, len, RMT_TAR_FILE, host, tm) >= len) {
        errExit((RMT_TAR_FILE ": file name too long", host, tm));
    }
    exit(execlp("tar", "tar", "-c", "-T", File, "-f", "-", NULL));
}
