/* $Id: backupfs-remote.c,v 1.12 2015/03/04 20:44:32 cvsremote Exp $

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


/* Write the backup destination directory to info->newdirs
   This doesn't check if info->src exists on remote host.
 */
void
writeDestDir (bkupInfo* info)
{
    struct stat stbuf;
    char*       s;
    char*       src;
    char*       dir;
    int         len;


    assert(info);
    assert(info->src);
    assert(info->bdir);

    len = strlen(info->bdir) + strlen(info->src);
    dir = malloc(len + 2);      /* 1 for trailing `/' */
    if (!dir) {
        errSysExit(("malloc(%d)", len + 2));
    }
    strcpy(dir, info->bdir);
    strcat(dir, info->src);
    dir[len] = '/';
    dir[len+1] = '\0';
    len = strlen(info->bdir);
    src = dir + len;
    for (s = index(dir + len + 1, '/'); s; s = index(s+1, '/')) {
        *s = '\0';
        if (stat(src, &stbuf)) {
            errSysExit(("stat(%s)", src));
        }
        newDirectory(dir, &stbuf, info);
        *s = '/';
    }
    free(dir);
}


/* Write last backup time to info->linkpath.
   This function must be called before writing link info.
 */
static void
writeLastBakupTime (time_t mtime, bkupInfo* info)
{
    static char buf[32];


    assert(info);
    assert(info->links);
    assert(info->linkpath);
    assert(info->oldJpath);
    assert(ftell(info->links) == 0);

    if (snprintf(buf, sizeof(buf), "0x%08lx\n", mtime) >= sizeof(buf)) {
        errExit(("0x%08lx\\n: string too long", mtime));
    }
    fwriteExit(buf, info->links, info->linkpath, info);
}


/* On remote host, simply call writeLastBakupTime().
   Return always 1 (success).
 */
int
lastBkupDirFromTime (time_t mtime, bkupInfo* info)
{
    writeLastBakupTime(mtime, info);
    return 1;
}


/* Write directory name and mode to info->ndpath
 */
int
newDirectory (char* bkupdir, struct stat* pst, bkupInfo* info)
{
    char buf[64];


    assert(bkupdir);
    assert(pst);
    assert(info);
    assert(info->newdirs);

    snprintf(buf, sizeof(buf), "0x%08x 0x%08x 0x%08x ",
                           (int)pst->st_uid, (int)pst->st_gid, pst->st_mode);
    fwriteExit(buf, info->newdirs, info->ndpath, info);
    fwriteExit(bkupdir, info->newdirs, info->ndpath, info);
    fwriteExit("\n", info->newdirs, info->ndpath, info);
    return 1;
}


/* Write path to be hard linked (on server) to info->linkpath
 */
int
makeLink(char* src, char* dest, bkupInfo* info)
{
    fwriteExit(src, info->links, info->linkpath, info);
    fwriteExit("\0", info->links, info->linkpath, info);
    return 1;
}


void
openFilesRemote (bkupInfo* info)
{
    int len, hlen, tlen;


    assert(info);
    assert(info->dest);
    assert(info->host);

    hlen = strlen(info->host);
    tlen = strlen(info->dest);  /* info->dest is used to hold time string */
    len  = strlen(RMT_DIR_FILE) + hlen + tlen;
    info->ndpath = malloc(len);
    if (!info->ndpath) {
        errSysRet(("malloc(ndpath:%d)", len));
        goto errorExit;
    }
    snprintf(info->ndpath, len, RMT_DIR_FILE, info->host, info->dest);

    len = strlen(RMT_LNK_FILE) + hlen + tlen;
    info->linkpath = malloc(len);
    if (!info->linkpath) {
        errSysRet(("malloc(linkpath:%d)", len));
        goto errorExit;
    }
    snprintf(info->linkpath, len, RMT_LNK_FILE, info->host, info->dest);

    len = strlen(RMT_TAR_FILE) + hlen + tlen;
    info->tpath = malloc(len);
    if (!info->tpath) {
        errSysRet(("malloc(tpath:%d)", len));
        goto errorExit;
    }
    snprintf(info->tpath, len, RMT_TAR_FILE, info->host, info->dest);

    info->newdirs = fopen(info->ndpath, "w");
    if (!info->newdirs) {
        errSysRet(("fopen(%s)", info->ndpath));
        goto errorExit;
    }
    info->links = fopen(info->linkpath, "w");
    if (!info->links) {
        errSysRet(("fopen(%s)", info->linkpath));
        goto errorExit;
    }
    info->tar = fopen(info->tpath, "w");
    if (!info->tar) {
        errSysRet(("fopen(%s)", info->tpath));
        goto errorExit;
    }

    return;                     /* success */


errorExit:
    closeFiles(info);
    exit(1);
}


void
openJournalFile (bkupInfo* info)
{
    assert(info);
    assert(info->jpath);

    info->jnl = fopen(info->jpath, "w");
    if (!info->jnl) {
        errSysRet(("fopen(%s)", info->jpath));
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
