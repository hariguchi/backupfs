/* $Id: backupfs-shell.c,v 1.6 2015/03/26 20:57:04 cvsremote Exp $

   backupfs-shell.c: main for bkupfs-shell (run on local host)


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
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "backupfs.h"
#include "error.h"


#if 0
static void
usage (void)
{
    fprintf(stderr, "%s\n" "Compiled: %s\n"
            "Usage: %s\n", VERSION, CompilationDate, PROGNAME_CHKSRC);
    exit(1);
}
#endif


/* backupfs-shell: shell for backupfs remote hosts.
   This allows only several commands to be executed.
   This does not allow interactive shell session.
 */
int
main (int argc, char* argv[])
{
    static char* cmd[32];
    char* p;
    int   cnt;                  /* # of args including command name */


    /* No interactive session allowed.
     */
    if (argc <= 2) exit(1);

    /* Command is passed as: backupfs-shell -c <command>
       Command is stored in argv[2] (not split). So we have to split.
     */
    cnt = 0;
    p   = argv[2];
    for(;;) {
        if (cnt >= sizeof(cmd)) {
            fprintf(stderr, "%d: command line too long\n", cnt);
            exit(1);
        }
        cmd[cnt++] = p;
        p    = index(p, ' ');
        if (!p) break;
        *p++ = '\0';
    }
    if (strcmp(cmd[0], "backupfs-chksrc") &&
        strcmp(cmd[0], "backupfs-remote") &&
        strcmp(cmd[0], "backupfs-exctar") &&
        strcmp(cmd[0], "cat") && strcmp(cmd[0], "rm")) {
        fprintf(stderr, "%s: command not allowed\n", cmd[0]);
        exit(1);
    }

    if (!strcmp(cmd[0], "cat")) {
        if (!cmd[1]) {
            exit(2);
        }
        if (strncmp(cmd[1], "dirs-", 5) &&
            strncmp(cmd[1], "links-", 6) &&
            strncmp(cmd[1], "tar-", 4)) {
            fprintf(stderr, "%s %s: file not allowed\n", cmd[0], cmd[1]);
            exit(3);
        }
    } else if (!strcmp(cmd[0], "rm")) {
        if (!cmd[1]) {
            exit(4);
        }
        if (!cmd[2]) {
            exit(5);
        }
        if (strncmp(cmd[2], "dirs-", 5)) {
            fprintf(stderr, "%s %s: file not allowed\n", cmd[0], cmd[2]);
            exit(7);
        }
    }
        
    exit(execvp(cmd[0], cmd));
}
