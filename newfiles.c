/* $Id: newfiles.c,v 1.12 2015/03/04 20:44:32 cvsremote Exp $

   newfiles.c: main for newfiles and changedfiles


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
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "string-rbt.h"
#include "backupfs.h"
#include "error.h"



static void
usage (char* path, bkupInfo* info)
{
    char* cmd;


    assert(path);
    assert(info);

    cmd = rindex(path, '/');
    if (cmd) {
        ++cmd;
    } else {
        cmd = path;
    }

    fprintf(stderr, "%s\n" "Compiled: %s\n"
                                "Usage: %s ", VERSION, CompilationDate, cmd);
    fprintf(stderr, "[-l] <src-dir> <backup-root-dir> [yyyy mm dd]\n");
    exit(1);
}


int
newDirectory (char* bkupdir, struct stat* pst, bkupInfo* info)
{
    return 1;
}


/* print new or changed file
 */
void
showFile (char* dir, char* file, bkupInfo* info)
{
    struct stat   stbuf;
    journalEntry* pEnt;
    char* lbpath;               /* last backed-up path */
    char* path;                 /* source path */
    int   buflen;


    assert(dir);
    assert(file);
    assert(info);
    assert(info->lbdir);
    assert(info->stbuf);

    buflen = strlen(dir) + strlen(file) + 2; /* 1 is for `/' */
    lbpath = malloc(buflen);
    if (!lbpath) errSysExit(("malloc(%s/%s)", dir, file));

    strcpy(lbpath, info->lbdir);
    strcat(lbpath, dir + info->lblen); /* skip backup directory */
    strcat(lbpath, "/");
    strcat(lbpath, file);
    path = lbpath + info->lblen;
    pEnt = stringRBTfind(info->jt, path);
    if (info->host) {
        if (!pEnt) goto freeReturn; /* not changed file but new file */
        if (lstat(lbpath, &stbuf)) goto freeReturn;
        if (!S_ISREG(stbuf.st_mode)) goto freeReturn;
        if (stbuf.st_ino == info->stbuf->st_ino) goto freeReturn;
    } else {
        if (!pEnt) goto printReturn; /* new file */
        goto freeReturn;
    }


printReturn:
    if (info->jpath) {
        info->bdir[info->lblen] = '\0';
        if (info->host) {
            printf("%s %s%s\n", lbpath, info->bdir, path);
        } else {
            printf("%s%s\n", info->bdir, path);
        }
        info->bdir[info->lblen] = '/';
    } else {
        printf("%s\n", path);
    }

freeReturn:
    free(lbpath);
}


/* Set up info->bdir, info->lbdir (src dir truncated), info->oldJpath
 */
void
setBkupDir (time_t t, bkupInfo* info)
{
    struct tm   tm;
    struct stat stbuf;
    size_t dlen;                /* strlen(info->dst) */
    size_t len;
    time_t t0;                  /* time zero */
    char*  p;


    assert(info);
    assert(info->dest);
    assert(info->src);

    dlen = strlen(info->dest);
    p    = info->dest + dlen;
    len  = dlen + strlen(info->src) + strlen(BKUP_DIR) + 1;
    info->bdir = malloc(len);
    if (!info->bdir) errSysExit(("1: malloc(%d)", len));

    /* backup directory
     */
    strcpy(info->bdir, info->dest);
    p = info->bdir + dlen;
    if (!localtime_r(&t, &tm)) errExit(("localtime(%d)", t));
    sprintf(p, "/%04d/%02d/%02d",
                               tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    info->lblen = strlen(info->bdir); /* bkup dir len (src dir truncated) */
    strcat(info->bdir, info->src);
    info->blen = strlen(info->bdir);
    if (stat(info->bdir, &stbuf)) errSysExit(("%s: ", info->bdir));
    if (!S_ISDIR(stbuf.st_mode)) errExit(("%s: not a directory", info->bdir));

    /* last backup directory
     */
    info->lbdir = malloc(len);
    if (!info->lbdir) errSysExit(("2: malloc(%d)", len));
    strcpy(info->lbdir, info->dest);

    /* last backup journal file path
     */
    len += strlen(JNL_FILE) + 2; /* 1 is for `/' */
    info->oldJpath = malloc(len);
    if (!info->oldJpath) errSysExit(("3: malloc(%d)", len));
    strcpy(info->oldJpath, info->dest);
    p = info->oldJpath + dlen;

    /* find last backup directory
     */
    t0 = 0;                     /* epoch */
    do {
        t -= 3600 * 24;         /* one day before */
        if (!localtime_r(&t, &tm)) errExit(("localtime(%d)", t));
        sprintf(p, "/%04d/%02d/%02d",
                               tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
        if (stat(info->oldJpath, &stbuf)) continue;
        if (!S_ISDIR(stbuf.st_mode)) {
            errRet(("%s: not a directory", info->oldJpath));
            continue;
        }
        break;
    } while (t > t0);           /* continue to epoch ;-) */

    if (t <= t0) {
        fprintf(stderr, "%s: first backup directory\n", info->bdir);
        exit(1);
    }

    /* last backup journal file
     */
    strcat(info->oldJpath, info->src);
    strcat(info->oldJpath, "/");
    strcat(info->oldJpath, JNL_FILE);

    /* last backup directory (source directory portion truncated)
     */
    p = info->lbdir + dlen;
    sprintf(p, "/%04d/%02d/%02d",
                               tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
}


int
main (int argc, char* argv[])
{
    time_t    t;
    struct tm tm;

    bkupInfo info;
    int      i;
    int      len;


    if (argc <= 2) {
        usage(argv[0], &info);
    }

    memset(&info, 0, sizeof(info));

    len = strlen(argv[0]) - strlen(PROGNAME_NEWFILE);
    if (strcmp(argv[0] + len, PROGNAME_NEWFILE)) {
        /* Use info.host as a flag
           NULL: newfile, otherwise changedfile
         */
        info.host = argv[0] + len;
    }

    for (i = 1; argv[i][0] == '-'; ++i) {
        switch (argv[i][1]) {
        case 'l':
            info.jpath = argv[0] + len; /* use jpath as flag for long format */
            break;
        default:
            fprintf(stderr, "%s: unknown option\n", argv[i]);
            exit(1);
        }
    }

    --i;                        /* now `i' is # of options */
    argc -= i;
    if (argc <= 2) {
        usage(argv[0], &info);
    }
    if (argc == 3) {
        t = time(NULL);
        if (t < 0) errSysExit(("time(NULL)"));
    } else {
        memset(&tm, 0, sizeof(tm));
        tm.tm_year  = strtol(argv[i + 3], NULL, 10) - 1900;
        tm.tm_mon   = strtol(argv[i + 4], NULL, 10) - 1;
        tm.tm_mday  = strtol(argv[i + 5], NULL, 10);
        tm.tm_isdst = -1;
        t = mktime(&tm);
        if (t < 0) {
            errExit(("mktime: %04d/%02d/%02d",
                                         tm.tm_year, tm.tm_mon, tm.tm_mday));
        }
    }

    info.src  = argv[i + 1];
    info.dest = argv[i + 2];
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

    setBkupDir(t, &info);
    if (!makeJournalTree(&info)) errExit(("failed in makeJournalTree"));

    info.func = showFile;
    if (!dirwalk(info.bdir, &info)) {
        errExit(("failed in dirwalk(%s)", info.bdir));
    }
    exit(0);


errorExit:
    fprintf(stderr,
            "Source and Destination directories must be full path\n");
    exit(1);
    return 0;                   /* to make gcc happy */
}
