/* $Id: backupfs.c,v 1.12 2015/03/04 20:44:32 cvsremote Exp $

   backupfs.c: making backup filesystem


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
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "string-rbt.h"
#include "backupfs.h"
#include "error.h"


/* Check source directoy to be backed up.
   Recurring backup if there is a journal file.
   First time backup otherwise.
 */
bkupType
chkSource (bkupInfo* info)
{
    struct stat stbuf;
    FILE*       fp;
    char*       journal;
    char*       oldJournal;     /* old journal */
    bkupType    rv = bkupError;


    assert(info);
    assert(info->src);

    if (stat(info->src, &stbuf)) {
        errSysExit(("stat(%s)", info->src));
    }
    if ((stbuf.st_mode & S_IFMT) != S_IFDIR) {
        errExit(("%s is not a directory", info->src));
    }
    journal = malloc(strlen(info->src) + strlen(JNL_FILE) + 2);
    if (!journal) {
        errExit(("can't get buffer for %s/%s", info->src, JNL_FILE));
    }
    oldJournal = malloc(strlen(OLD_JNL_FILE) + 1);
    if (!oldJournal) {
        errExit(("can't get buffer for %s", OLD_JNL_FILE));
    }
    strcpy(journal, info->src);       /* journal file path */
    strcat(journal, "/");
    strcat(journal, JNL_FILE);
    strcpy(oldJournal, OLD_JNL_FILE);
    info->jpath = journal;
    info->oldJpath = oldJournal;

    if (stat(journal, &stbuf)) {
        if (errno == ENOENT) {  /* no journal file in src dir. First time */
            rv = bkupFirstTime;
        } else {
            errSysExit(("stat(%s)", journal));
        }
    } else {
        /* Journal file is found. Recurrent backup.
           Make journal tree whose entries have
           path (search key), ctime, and mtime.
         */
        if (!(stbuf.st_mode & (S_IRUSR|S_IWUSR))) {
            errExit(("%s: no read and write permission", journal));
        }
        switch (stbuf.st_mode & S_IFMT) {
        case S_IFLNK:
        case S_IFREG:
            rv = bkupRecurrent;
            fp = makeTemp(oldJournal, "r");
            if (fp) {
                fclose(fp);
            } else {
                errExit(("%s: can't create temp file", oldJournal));
            }
            if (!moveFile(journal, oldJournal)) {
                errExit(("can't move %s to %s", journal, oldJournal));
            }
            if (!makeJournalTree(info)) {
                errRet(("can't create journal tree"));
                backupfsExit(info, 1);
            }
            if (!getLastBkupDir(info)) {
                rv = bkupFirstTime;
            }
            break;
        default:
            errExit(("%s: wrong file type (0x%x)",
                     journal, stbuf.st_mode & S_IFMT));
        }
    }
    return rv;
}


/* Local:  set the directory name of the latest backup to `info->lbdir'
   Remote: write last backup time to info->links.
   Return 1 if found.  Return 0 otherwise.
 */
int
getLastBkupDir (bkupInfo* info)
{
    journalEntry* pEnt;


    assert(info);
    assert(info->jt);
    assert(info->jpath);

    /* Try to find the journal file in the journal tree.
       Consider first time backup if there is no entry.
     */
    pEnt = stringRBTfind(info->jt, info->jpath);
    if (!pEnt) {
        fprintf(stderr, "%s: No journal file found\n", __FUNCTION__);
        return 0;
    }
    return lastBkupDirFromTime(pEnt->mtime, info);
}


/* Add all the file under info->src to info->tar for backup.
   Also create journal file.
 */
void
firstTimeBackup (char* dir, char* file, bkupInfo* info)
{
    char* buf;
    char* path;
    int   buflen;


    assert(dir);
    assert(file);
    assert(info);
    
    buflen = strlen(dir) + strlen(file) + 80; /* 80 is just margin */
    buf = malloc(buflen);
    if (!buf) {
        errSysExit(("malloc(%d)", buflen));
    }
    snprintf(buf, buflen, "%08lx %08lx ", info->ctime, info->mtime);
    path = buf + strlen(buf);
    strcpy(path, dir);
    strcat(path, "/");
    strcat(path, file);

    fwriteExit(path, info->tar, info->tpath, info);
    fwriteExit("\n", info->tar, info->tpath, info);

    fwriteExit(buf, info->jnl, info->jpath, info);
    fwriteExit("\n", info->jnl, info->jpath, info);
    free(buf);
}


/* Check each file under info->src and:
     1. add it to info->tar for backup if it is new or changed
     2. make hard link from the last backup if it is not changed.
 */
void
recurrentBackup (char* dir, char* file, bkupInfo* info)
{
    journalEntry* pEnt;
    char* buf;
    char* path;                 /* backup source path */
    char* bpath;                /* backup path */
    char* lspath;               /* link src path */
    int   buflen;
    int   bplen;                /* backup path length */


    assert(dir);
    assert(file);
    assert(info);
    assert(info->bdir);
    assert(info->jt);

    buflen = info->lblen + strlen(dir) + strlen(file) + 40;
    buf  = malloc(buflen);  /* 40 for ctime and mtime */
    if (!buf) {
        errSysExit(("malloc(%s, %s/%s)",
                           info->lbdir ? info->lbdir : "REMOTE", dir, file));
    }
    /* Precise bplen = info->blen + strlen(dir) + strlen(file)
       So use `info->blen + buflen - info->lblen' and save
       local variables and recalc string length.
     */
    bplen = info->blen + buflen - info->lblen;
    bpath = malloc(bplen);
    if (!bpath) {
        errSysExit(("malloc(%s/%s/%s)", info->bdir, dir, file));
    }
    snprintf(buf, buflen, "%08lx %08lx ", info->ctime, info->mtime);
    lspath = buf + strlen(buf) + 1; /* don't concat lspath and time */
    path   = lspath + info->lblen;
    strcpy(path, dir);
    strcat(path, "/");
    strcat(path, file);
    strcpy(bpath, info->bdir);
    strcat(bpath, path);        /* "/" no need since dir is absolute */
    pEnt = stringRBTfind(info->jt, path);
    if (pEnt &&
           ((info->ctime == pEnt->ctime) && (info->mtime == pEnt->mtime))) {
        memcpy(lspath, info->lbdir, info->lblen);
        if (makeLink(lspath, bpath, info)) {
            printf("unchanged: %s\n", path);
            goto writeJournal;
        } else {
            errRet(("makeLink %s %s\n", lspath, bpath));
            goto writeTar;
        }
    }

writeTar:
    /* New file or file was modified.
     */
    if (pEnt) {
        printf("changed:   %s\n", path);
    } else {
        printf("new file:  %s\n", path);
    }
    fwriteExit(path, info->tar, info->tpath, info);
    fwriteExit("\n", info->tar, info->tpath, info);

writeJournal:
    fwriteExit(buf, info->jnl, info->jpath, info);
    fwriteExit(path, info->jnl, info->jpath, info);
    fwriteExit("\n", info->jnl, info->jpath, info);
    free(buf);
    free(bpath);
}
