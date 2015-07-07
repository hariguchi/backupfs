/* $Id: backupfs-hist.c,v 1.8 2005/04/21 04:46:01 cvsremote Exp $

   backupfs-hist.c: main for backupfs-hist


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
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "backupfs.h"
#include "error.h"


#define DATE "/yyyy/mm/dd"

typedef enum {
    INODE_NOPATH = 0,
    INODE_CHANGE = 1,
    INODE_SAME   = 2,
    INODE_NEW    = 3,
} pathInfo;

enum {
    NYEARS = 2,
};

static int Nyears = NYEARS;



static void
usage (void)
{
    fprintf(stderr, "%s\n" "Compiled: %s\n"
            "Usage: %s [-n nyears] <backup-root-dir> <file>\n",
            VERSION, CompilationDate, PROGNAME);
    exit(1);
}


/* Returns 1 if `path' exists, otherwise returns 0.
   However, it is regarded as `not exist' if `inode' is non-NULL
   and `path' has the same inode number as `*inode'.
 */
pathInfo
doesPathExist (char* path, ino_t* inode)
{
    struct stat stbuf;


    assert(path);

    if (!stat(path, &stbuf)) {
        if (!inode) return INODE_NEW; /* inode is NULL, don't check */
        if (*inode == 0) {      /* no inode info. Initialize it */
            *inode = stbuf.st_ino;
            return INODE_NEW;
        }
        if (*inode == stbuf.st_ino) {
            return INODE_SAME;     /* files are the same */
        } else {
            *inode = stbuf.st_ino; /* update inode info */
            return INODE_CHANGE;
        }
    }
    if (errno == EACCES) {
        printf("%s: permission denied\n", path);
        exit(1);
    }
    return INODE_NOPATH;
}


/* Decrement date info in `*ptm', update the date string in `dst',
   and check if the .../yyyy/mm directory (which is a part of `dst')
   exists. If it does not exist, decrementDate() goes beyound and
   tries to find an old .../yyyy/mm directory until Nyears back.
   Returns 1 if it exists, otherwise returns 0.
   This function is recursive.
 */
int
decrementDate (char* dst, char* date, struct tm* ptm)
{
    char* dend;                 /* end of date string */
    int   mchange = 0;          /* 1 if month changed */
    int   ychange = 0;          /* 1 if year changed */
    pathInfo rs;              /* return status from doesPathExist() */


    assert(dst);
    assert(date);
    assert(ptm);

    if (ptm->tm_mday == 1) {
        mchange = 1;            /* month changed */
        ptm->tm_mday = 31;      /* reset mday. Ok to ignore short months  */
        if (ptm->tm_mon <= 0) { /* year changed */
            ychange = 1;
            --ptm->tm_year;
            ptm->tm_mon = 11;   /* reset month */
        } else {
            --ptm->tm_mon;
        }
    } else {
        --ptm->tm_mday;
    }

    if (ychange) {
        dend = date + 4;        /* end ptr of .../yyyy */
        assert(*dend == '/');
        for (; ptm->tm_yday - ptm->tm_year <= Nyears; --ptm->tm_year) {
            sprintf(date, "%04d", ptm->tm_year + 1900);
            rs = doesPathExist(dst, NULL);
            *dend = '/';        /* sprintf() terminates string */
            if (rs) return 1;
        }
        return 0;               /* no old .../yyyy directories */
    }

    if (mchange) {
        dend = date + 7;        /* end ptr of .../yyyy/mm */
        assert(*dend == '/');
        for (; ptm->tm_mon >= 0; --ptm->tm_mon) {
            sprintf(date, "%04d/%02d", ptm->tm_year + 1900, ptm->tm_mon + 1);
            rs = doesPathExist(dst, NULL);
            *dend = '/';        /* sprintf() terminates string */
            if (rs) return 1;
        }
        ptm->tm_mday = 1;       /* reset date to 01/01/yyyy */
        ptm->tm_mon  = 0;
        return decrementDate(dst, date, ptm);
    } else {
        return 1;               /* no year/month change */
    }

    errExit(("should not happen"));
    return 0;
}


/* Print backup history of `dst'
 */
void
prHistory (char* dst, char* date, struct tm* ptm)
{
    ino_t inode;
    pathInfo rs;                /* return status */
    int   len = 0;
    char* prev;
    char* pd;                   /* date ptr for `prev' */


    assert(dst);
    assert(date);
    assert(ptm);

    prev = malloc(strlen(dst));
    if (!prev) {
        errSysExit(("malloc(strlen(dst))"));
    }
    pd = prev + (date - dst);

    strcpy(prev, dst);
    inode = 0;
    for (;;) {
        sprintf(date, "%04d/%02d/%02d",
                         ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday);
        if (len == 0) {
            len   = strlen(date);
        }
        *(date + len) = '/'; /* sprintf() tereminates string */
        rs = doesPathExist(dst, &inode);
        if (rs == INODE_CHANGE) {
            printf("%s\n", prev);
        }
        if (rs != INODE_NOPATH) {
            sprintf(pd, "%04d/%02d/%02d",
                         ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday);
            *(pd + len) = '/'; /* sprintf() tereminates string */
        }
        if (!decrementDate(dst, date, ptm)) break;
    }
    if (inode) {
        printf("%s\n", prev);
    }
}


int
main (int argc, char* argv[])
{
    char* dst;                  /* destination path */
    char* src;                  /* source path */
    char* date;                 /* ptr to date string */
    int   len;
    time_t    t;
    struct tm tm;


    if (argc <= 2) {
        usage();
    }

    if (argv[1][0] == '-' && argv[1][1] == 'n') {
        if (argc <= 4) {
            usage();
        }
        Nyears = strtol(argv[2], NULL, 10);
        argv += 2;
    }

    dst = argv[1];
    src = argv[2];
    if (*src != '/') goto errorExit;       /* src  must be full path */
    if (*dst != '/') goto errorExit;       /* dest must be full path */

    len = strlen(src) - 1;
    if (src[len] == '/') {  /* strip tail '/' */
        src[len] = '\0';
    }
    len = strlen(dst) - 1;
    if (dst[len] == '/') {
        dst[len] = '\0';
    }

    len = strlen(dst) + strlen(DATE) + strlen(src);
    dst = malloc(len + 1);
    if (!dst) {
        errSysExit(("malloc(%d)", len));
    }
    strcpy(dst, argv[1]);
    strcat(dst, DATE);
    strcat(dst, src);
    date = dst + strlen(argv[1]) + 1;
    t    = time(NULL);
    if (!localtime_r(&t, &tm)) {
        errSysExit(("localtime_r()"));
    }
    tm.tm_yday = tm.tm_year;    /* save this year info for prHistory */
    prHistory(dst, date, &tm);
    free(dst);
    exit(0);


errorExit:
    fprintf(stderr,
            "File, Source and Destination directories must be full path\n");
    exit(1);
    return 0;                   /* to make gcc happy */
}
