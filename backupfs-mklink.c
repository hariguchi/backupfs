/* $Id: backupfs-mklink.c,v 1.5 2005/04/21 04:46:01 cvsremote Exp $

   backupfs-mklink.c: main for bkupfs-mklink (run on local host)
   Usage: backup-mklink <backup-root-dir> <backup-dir>

   This command makes hard links to the last backed-up files.
   The last backup directory is `backup-rootdir/yyyy/mm/dd' and
   the first line of stdin contains the last backup date in hex.
   Backup-dir must have date string (yyyy/mm/dd) at the end.


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

static int IsDebug = 0;

#define _GNU_SOURCE

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "backupfs.h"
#include "platform.h"
#include "error.h"

typedef struct {
    char*  dir;                 /* directory path name */
    char*  date;                /* begining of date in `dir' */
    char*  dend;                /* end of date in `dir' */
    size_t len;                 /* strlen(dir) */
    size_t mlen;                /* malloc'ed length */
} bdir;


static void
usage (void)
{
    fprintf(stderr, "%s\n" "Compiled: %s\n"
            "Usage: %s <backup-root-dir> <backup-dir>\n",
            VERSION, CompilationDate, PROGNAME_MKLINK);
    exit(1);
}


/* 1. allocate memory to p->dir for `<path>/yyyy/mm/dd/<file-name>'
   2. make p->date point to `yyyy/mm/dd/<file-name>'
   3. make p->dend point to `<file-name>'
 */
void
setupDir (bdir* p, char* path)
{
    assert(p);
    assert(path);

    p->mlen = strlen(path);
    if (p->mlen + strlen(BKUP_DIR) + 1 > MAXCHARS) {
        p->mlen *= 2;            /* pick up long enough value */
        p->dir = malloc(p->mlen);
    } else {
        p->mlen = MAXCHARS;
        p->dir = malloc(p->mlen);
    }
    if (!p->dir) {
        errSysExit(("p->dir: malloc(%d)", p->mlen));
    }
    strcpy(p->dir, path);
    p->len = strlen(p->dir);
    if (p->dir[p->len-1] != '/') { /* add '/' to the end */
        p->dir[p->len++] = '/';
        p->dir[p->len]   = '\0';
    }
    p->date = p->dir + p->len;
    strcat(p->dir, BKUP_DIR);
    p->len = strlen(p->dir);
    p->dend = p->dir + p->len;
}


int
main (int argc, char* argv[])
{
    ssize_t     len;
    size_t      l;
    time_t      t;
    struct tm*  ptm;
    bdir        dst, src;
    int         est;            /* exit status */
    char*       file;
    char*       p;


    if (!IsDebug && getuid() != ROOT_UID) {
        fprintf(stderr, "must be root\n");
        exit(1);
    }

    if (argc < 3) {
        usage();
    }

    if (*argv[1]  != '/') goto errorExit;  /*  must be full path */
    if (*argv[2]  != '/') goto errorExit;  /*  must be full path */

    setupDir(&src, argv[1]);
    dst.len  = strlen(argv[2]);
    dst.mlen = (dst.len + 2 > MAXCHARS) ? 2*dst.len : MAXCHARS;
    dst.dir  = malloc(dst.mlen);
    if (!dst.dir) {
        free(src.dir);
        errSysExit(("malloc(%d)", dst.mlen));
    }
    strcpy(dst.dir, argv[2]);
    if (dst.dir[dst.len-1] == '/') {
        dst.dir[dst.len] = '\0';
    }
    dst.dend = dst.dir + dst.len;
    dst.date = dst.dend - strlen(BKUP_DIR) - 1;

    file = malloc(MAXCHARS);
    if (!file) {
        free(dst.dir);
        free(src.dir);
        errSysExit(("malloc(%d)", MAXCHARS));
    }

    /* Read last backup time
     */
    len = getline(&file, &l, stdin);
    if (len < 0) {
        /* No Last backup, which means first time backup.
           Just do exit(0).
         */
        free(dst.dir);
        free(src.dir);
        free(file);
        exit(0);
    }
    t = strtol(file, NULL, 16);
    ptm = localtime(&t);
    if (!ptm) {
        free(dst.dir);
        free(src.dir);
        free(file);
        errExit(("localtime(0x%08x) failed", t));
    }

    /* src.dir: last backup directory
     */
    sprintf(src.date, "%04d/%02d/%02d",
                         ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday);

    for (;;) {
        len = getdelim(&file, &l, '\0', stdin); /* get file name */
        if (len < 0) {
            est = feof(stdin) ? 0 : 1;
            free(dst.dir);
            free(src.dir);
            free(file);
            exit(est);
        }
        if (src.len + len > src.mlen) { /* not enough memory for src.dir */
            l = src.len + len + 1;
            p = realloc(src.dir, l);
            if (p) {
                src.dir  = p;
                src.mlen = l;
            } else {
                errSysRet(("realloc(%d)", src.mlen));
                continue;
            }
        }
        if (dst.len + len > dst.mlen) { /* not enough memory for src.dir */
            l = dst.len + len + 1;
            p = realloc(dst.dir, l);
            if (p) {
                dst.dir  = p;
                dst.mlen = l;
            } else {
                errSysRet(("realloc(%d)", src.mlen));
                continue;
            }
        }
        strcat(src.dir, file);
        strcat(dst.dir, file);

        if (IsDebug) {
            printf("%s %s\n", src.dir, dst.dir);
        } else {
            if (link(src.dir, dst.dir)) {
                errSysRet(("link(%s, %s)", src.dir, dst.dir));
            }
        }
        *dst.dend = '\0';
        *src.dend = '\0';
    }
    exit(0);

errorExit:
    fprintf(stderr,
            "Source and Destination directories must be full path\n");
    exit(1);
}
