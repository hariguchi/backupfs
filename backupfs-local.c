/* $Id: backupfs-local.c,v 1.15 2015/03/26 22:37:45 cvsremote Exp $

   backupfs-local.c: local host (server) side functions


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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "string-rbt.h"
#include "backupfs.h"
#include "error.h"



/* Make a SSH secret key (ID file) name and store it to info->sshid.
   Also check whether the file exists or not.
 */
void
makeSshKey (bkupInfo* info)
{
    int    len;
    mode_t mode;


    /* SSH secret key file: <backup root dir>/.id_rsa
     */
    len = strlen(info->dest) + strlen(ID_FILE) + 2;
    info->sshid = malloc(len);
    if (!info->sshid) {
        errSysExit(("malloc(sshid:%d)", len));
    }
    snprintf(info->sshid, len, "%s/%s", info->dest, ID_FILE);
    if (!getPathMode(info->sshid, &mode)) {
        errExit(("getPathMode(%s)", info->sshid));
    }
    if (mode & (S_IRGRP|S_IROTH)) {
        errRet(("%s: mode must be 400. Changing mode...", info->sshid));
        if (chmod(info->sshid, S_IRUSR)) {
            errSysExit(("chmod(%s)", info->sshid));
        }
    } else if (!(mode & S_IRUSR)) {
        errRet(("%s: can't read. Changing mode...", info->sshid));
        if (chmod(info->sshid, S_IRUSR)) {
            errSysExit(("chmod(%s)", info->sshid));
        }
    }
}


/* Check whether the backup source directory on remote host exists or not
 */
int
chkRemoteSrc (bkupInfo* info)
{
    char*      cmd;
    int        len;
    int        status;
    pipeExitSt st;


    assert(info);
    assert(info->bdir);

    len = strlen(RMT_PASS1_1) + strlen(info->sshid) +
           strlen(info->host) + strlen(info->src) + 1;
    cmd = malloc(len);
    if (!cmd) errSysExit(("malloc(%d)", len));
    snprintf(cmd, len,
                RMT_PASS1_1, info->sshid, info->user, info->host, info->src);
    st = execCommands(cmd, RMT_PASS1_2);
    status = chkPipeExitSt (st, cmd, RMT_PASS1_2);
    free(cmd);
    return status;
}


/* Check if:
     1. destination directory info->dest/yyyy/mm/dd (day is today) exists
     2. if exists, check if it is accessible
     3. otherwise create the destination directory
 */
void
chkDest (bkupInfo* info)
{
    struct stat stbuf;
    struct tm*  curTm;
    time_t      ctime;
    mode_t      mode;
    int         len;
    char*       src;            /* copy of source directory */
    char*       s;
    char*       bdir;           /* backup directory */


    assert(info);
    assert(info->dest);
    assert(info->src);

    if (stat(info->dest, &stbuf)) {
        errSysExit(("stat(%s)", info->dest));
    }
    if ((stbuf.st_mode & S_IFMT) != S_IFDIR) {
        errExit(("%s is not directory", info->dest));
    }
    info->blen = strlen(info->dest) + strlen(BKUP_DIR) + 1; /* 1 for '/' */
    info->bdir = malloc(info->blen+1);
    if (!info->bdir) {
        errSysExit(("malloc(%s/)", info->dest, BKUP_DIR));
    }
    len = strlen(info->src);
    src = malloc(len+2);        /* 1 for tailing '/' */
    if (!src) {
        errSysExit(("malloc(%s)", info->src));
    }
    strcpy(src, info->src);
    src[len] = '/';          /* to make index() work in for{} later */
    src[len+1] = '\0';

    /* Check destination directory.
       Make the directory if it doesn't exist.
     */
    ctime = time(NULL);
    curTm = localtime(&ctime);
    if (!curTm) {
        errSysExit(("localtime"));
    }
    curTm->tm_year += 1900;
    ++curTm->tm_mon;
    sprintf(info->bdir, "%s/%04d", info->dest, curTm->tm_year);
    if (mkdir(info->bdir, destDirMode)) {
        if (errno != EEXIST) {
            errSysExit(("mkdir(%s)", info->bdir));
        }
    }
    if (chmod(info->bdir, destDirMode)) {
        errSysExit(("chmod(%s)", info->bdir));
    }
    sprintf(info->bdir, "%s/%04d/%02d",
            info->dest, curTm->tm_year, curTm->tm_mon);
    if (mkdir(info->bdir, destDirMode)) {
        if (errno != EEXIST) {
            errSysExit(("mkdir(%s)", info->bdir));
        }
    }
    if (chmod(info->bdir, destDirMode)) {
        errSysExit(("chmod(%s)", info->bdir));
    }
    sprintf(info->bdir, "%s/%04d/%02d/%02d",
            info->dest, curTm->tm_year, curTm->tm_mon, curTm->tm_mday);
    if (mkdir(info->bdir, destDirMode)) {
        if (errno != EEXIST) {
            errSysExit(("mkdir(%s)", info->bdir));
        }
    }
    if (chmod(info->bdir, destDirMode)) {
        errSysExit(("chmod(%s)", info->bdir));
    }
    if (chdir(info->bdir)) {
        errSysExit(("chdir(%s)", info->bdir));
    }

    if (info->host) {
        if (!chkRemoteSrc(info)) {
            errExit(("%s:%s: no such directory or can't create directories",
                                                     info->host, info->src));
        }
        bdir = malloc(strlen(info->bdir) + strlen(info->src) + 1);
        if (!bdir) {
            errSysRet(("malloc(%d)",
                       malloc(strlen(info->bdir) + strlen(info->src) + 1)));
        } else {
            strcpy(bdir, info->bdir);
            strcat(bdir, info->src);
            if (!stat(bdir, &stbuf)) {
                if (!isDirEmpty(bdir)) {
                    errExit(("%s: directory not empty", bdir));
                }
            }
        }
        free(bdir);
        return;
    }

    /* Src always ends with '/' so that
       for{} can also take care of the last directory
     */
    for (s = index(src+1, '/'); s; s = index(s+1, '/')) {
        *s = '\0';
        /* Source directory check is not necessary for remote backup
           because it is already done with chkRemoteSrc()
         */
        if (stat(src, &stbuf)) {
            errSysExit(("stat(%s)", src));
        }
#if 0
        mode = stbuf.st_mode & 
                     (S_IRWXU|S_IRWXG|S_IRWXO|S_ISUID|S_ISGID|S_ISVTX);
#endif
        mode = stbuf.st_mode;
        if (mkdir(src+1, mode)) {
            if (errno != EEXIST) {
                errSysExit(("mkdir(%s%s)", info->bdir, src));
            }
        }
        if (chmod(src+1, mode)) {
            errSysExit(("chmod(%s%s)", info->bdir, src));
        }
        if (chown(src+1, stbuf.st_uid, stbuf.st_gid)) {
            errSysRet(("chown(%s, 0x%08x, 0x%08x)",
                                         src+1, stbuf.st_uid, stbuf.st_gid));
        }
        *s = '/';
    }
    free(src);
}


/* Set the last backup directory to info->lbdir from `mtime'
   Return 1 if success, 0 otherwise.
 */
int
lastBkupDirFromTime (time_t mtime, bkupInfo* info)
{
    struct tm*    lbtm;          /* last backup time */
    struct stat   stbuf;


    assert(info->dest);

    /* Latest backup directory is supposed be:
       info->dest/yyyy/mm/dd where the date is `mtime'
     */
    lbtm = localtime(&mtime);
    if (!lbtm) {
        errSysRet(("localtime(%d)", mtime));
        return 0;
    }

    lbtm->tm_year += 1900;
    ++lbtm->tm_mon;
    info->lblen = strlen(info->dest) + strlen(BKUP_DIR) + 1; /* 1 for '/' */
    info->lbdir = malloc(info->lblen + 1);
    if (!info->lbdir) {
        errSysRet(("malloc(%s/%s)", info->dest, BKUP_DIR));
        return 0;
    }
    sprintf(info->lbdir, "%s/%04d/%02d/%02d",
            info->dest, lbtm->tm_year, lbtm->tm_mon, lbtm->tm_mday);
    if (!strcmp(info->lbdir, info->bdir)) {
        moveFile(info->oldJpath, info->jpath);
        errExit(("%s: last backup dir and backup dir are the same",
                 info->bdir));
    }
    if (stat(info->lbdir, &stbuf)) {
        errSysRet(("stat(%s)", info->lbdir));
        return 0;
    }
    return 1;
}


/* Execute remote backup commands
    1. Create directory, link, and tar input files
       in ~backupfs/ on remote host
    2. Create directories for backup on server
    3. Create hard links on server
    4. Copy new files for backup from remote host and store them
 */
int
doRemote (bkupInfo* info)
{
    int        len, len2, cmdlen;
    time_t     tm;
    char*      cmd[2];
    char       stime[16];       /* time() in hex string */
    pipeExitSt st;


    assert(info);
    assert(info->bdir);
    assert(info->dest);
    assert(info->host);

    /* cd backup-directory for "tar xpf -"
     */
    if (chdir(info->bdir)) errSysExit(("chdir(%s)", info->bdir));

    cmdlen = MAXCHARS/2;
    cmd[0] = malloc(2*cmdlen);  /* for 2 commdands */
    if (!cmd[0]) errSysExit(("malloc(cmd[0]:%d)", cmdlen));
    cmd[1] = cmd[0] + cmdlen;

    tm = time(NULL);
    snprintf(stime, sizeof(stime), "%08lx", tm);

    /* Remote new directories name: dirs-<host>-<time>
     */
    len  = strlen(info->host) + strlen (stime) + 1;
    len2 = strlen(RMT_DIR_FILE);
    info->ndpath = malloc(len + len2);
    if (!info->ndpath) errSysExit(("malloc(ndpath:%d)", len));
    snprintf(info->ndpath, len+len2, RMT_DIR_FILE, info->host, stime);

    /* Remote hard link info file name: links-<host>-<time>
     */
    len2 = strlen(RMT_LNK_FILE);
    info->linkpath = malloc(len + len2);
    if (!info->linkpath) errSysExit(("malloc(linkpath:%d)", len));
    snprintf(info->linkpath, len+len2, RMT_LNK_FILE, info->host, stime);

    /* Remote tar file name: tar-<host>-<time>
     */
    len2 = strlen(RMT_TAR_FILE);
    info->tpath = malloc(len + len2);
    if (!info->tpath) errSysExit(("malloc(tpath:%d)", len));
    snprintf(info->tpath, len + len2, RMT_TAR_FILE, info->host, stime);


    /* ssh -i <rsa_id> backupfs@<host> \
       backupfs-remote <src-dir> <bkup-dir> <host> <time-in-hex>
     */
    if (snprintf(cmd[0], cmdlen, RMT_PASS2, info->sshid, info->user,
           info->host, info->src, info->bdir, info->host, stime) >= cmdlen) {
        errExit(("cmdlen (%d:%s) too short" , cmdlen, cmd[0]));
    }
    st = execCommands(cmd[0], NULL);
    if (!chkCmdExitSt(st, cmd[0])) goto removeFiles;

    /* ssh -i <rsa_id> backupfs@<host> cat <new-dir-path> | backupfs-mkdir
     */
    if (snprintf(cmd[0], cmdlen, RMT_PASS3_1, info->sshid, info->user,
                                       info->host, info->ndpath) >= cmdlen) {
        errRet(("cmdlen (%d) too short" , cmdlen));
        goto removeFiles;
    }
    if (strlen(RMT_PASS3_2) >= cmdlen) {
        errRet(("cmdlen (%d:RMT_PASS3_2) too short" , cmdlen));
        goto removeFiles;
    }
    strcpy(cmd[1], RMT_PASS3_2);
    st = execCommands(cmd[0], cmd[1]);
    if (!chkPipeExitSt(st, cmd[0], cmd[1])) goto removeFiles;

    /* ssh -i <rsa_id> backupfs@<host> cat <new-dir-path> | \
       backupfs-mklink <dest-dir> <backup-dir>
     */
    if (snprintf(cmd[0], cmdlen, RMT_PASS4_1, info->sshid, info->user,
                                     info->host, info->linkpath) >= cmdlen) {
        errRet(("cmdlen (%d:%s) too short" , cmdlen, cmd[0]));
        goto removeFiles;
    }
    if (snprintf(cmd[1], cmdlen, RMT_PASS4_2, info->dest, info->bdir)
                                                                 >= cmdlen) {
        errRet(("cmdlen (%d:%s) too short" , cmdlen, cmd[1]));
        goto removeFiles;
    }
    st = execCommands(cmd[0], cmd[1]);
    if (!chkPipeExitSt(st, cmd[0], cmd[1])) goto removeFiles;

    /* ssh -i <rsa_id> backupfs@<host> \
       backupfs-exectar <host> <time-in-hex> | tar xpf -
     */
    if (snprintf(cmd[0], cmdlen, RMT_PASS5_1, info->sshid, info->user,
                                  info->host, info->host, stime) >= cmdlen) {
        errRet(("cmdlen (%d) too short" , cmdlen));
        goto removeFiles;
    }
    if (strlen(RMT_PASS5_2) >= cmdlen) {
        errRet(("cmdlen (RMT_PASS5_2:%d) too short" , cmdlen));
        goto removeFiles;
    }
    strcpy(cmd[1], RMT_PASS5_2);
    st = execCommands(cmd[0], cmd[1]);
    chkPipeExitSt(st, cmd[0], cmd[1]);

removeFiles:
    if (snprintf(cmd[0], cmdlen, RMT_PASS6, info->sshid, info->user,
          info->host, info->ndpath, info->linkpath, info->tpath) >= cmdlen) {
        errExit(("cmdlen (%d) too short" , cmdlen));
    }
    st = execCommands(cmd[0], NULL);
    chkCmdExitSt(st, cmd[0]);

    return 0;                   /* success */
}


int
newDirectory (char* bkupdir, struct stat* pst, bkupInfo* info)
{
    mode_t mode;


    assert(bkupdir);

#if 0
    mode = pst->st_mode & (S_IRWXU|S_IRWXG|S_IRWXO|S_ISUID|S_ISGID|S_ISVTX);
#endif
    mode = pst->st_mode;
    if (mkdir(bkupdir, mode)) {
        errSysRet(("mkdir(%s)", bkupdir));
        return 0;
    }
    if (chmod(bkupdir, mode)) {
        errSysRet(("chmod(%s)", bkupdir));
        return 0;
    }
    if (chown(bkupdir, pst->st_uid, pst->st_gid)) {
        errSysRet(("chown(%s, 0x%08x, 0x%08x)",
                                         bkupdir, pst->st_uid, pst->st_gid));
        return 0;
    }
    return 1;
}


int
makeLink(char* src, char* dest, bkupInfo* info)
{
    assert(src);
    assert(dest);

    return !link(src, dest);
}


void
openFilesLocal (bkupInfo* info)
{
    assert(info);
    assert(info->jpath);

    info->tpath = malloc(strlen(TAR_FILE) + 1);
    if (!info->tpath) {
        errSysRet(("malloc(tpath:%d)", strlen(TAR_FILE) + 1));
        goto errorExit;
    }
    strcpy(info->tpath, TAR_FILE);
    
    info->jnl = fopen(info->jpath, "w");
    if (!info->jnl) {
        errSysRet(("fopen(%s)", info->jpath));
        goto errorExit;
    }
    info->tar = makeTemp(info->tpath, "w+");
    if (!info->tar) {
        errRet(("can't make temporary file (%s)", info->tpath));
        goto errorExit;
    }
    return;                     /* success */


errorExit:
    closeFiles(info);
    if (!moveFile(info->oldJpath, info->jpath)) {
        errRet(("can't move %s to %s", info->oldJpath, info->jpath));
    }
    exit(1);
}
