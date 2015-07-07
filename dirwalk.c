/* $Id: dirwalk.c,v 1.9 2005/04/21 04:46:01 cvsremote Exp $

   dirwalk.c: walking through directory tree


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
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include "backupfs.h"
#include "error.h"


/* This is a recursive function.
 */
int
dirwalk (char* dir, bkupInfo* info)
{
    DIR*           pDir;
    struct dirent* pEnt;
    struct stat    stbuf;
    char*          newdir;      /* new (next) source directory */
    char*          bkupdir;     /* backup directory */


    assert(dir);
    assert(info);
    assert(info->bdir);
    assert(info->func);

    pDir = opendir(dir);
    if (!pDir) {
        errSysRet(("opendir(%s)", dir));
        return 0;
    }
    if (chdir(dir)) {
        errSysRet(("chdir(%s)", dir));
        return 0;
    }
    for (pEnt = readdir(pDir); pEnt; pEnt = readdir(pDir)) {
        if (lstat(pEnt->d_name, &stbuf)) {
            errSysRet(("stat(%s)", pEnt->d_name));
            continue;
        }
        switch (stbuf.st_mode & S_IFMT) {
        case S_IFSOCK:
            printf("%s/%s: socket ignored\n", dir, pEnt->d_name);
            break;
        case S_IFBLK:
            printf("%s/%s: block device ignored\n", dir, pEnt->d_name);
            break;
        case S_IFCHR:
            printf("%s/%s: character device ignored\n", dir, pEnt->d_name);
            break;
        case S_IFIFO:
            printf("%s/%s: fifo ignored\n", dir, pEnt->d_name);
            break;
        case S_IFDIR:
            if (strcmp(".", pEnt->d_name) == 0) {
                break;
            }
            if (strcmp("..", pEnt->d_name) == 0) {
                break;
            }
            bkupdir = malloc(info->blen +
                             strlen(dir) + strlen(pEnt->d_name) + 3);
            if (!bkupdir) {
                errSysRet(("%s/%s/%s: can't alloc memory",
                           info->bdir, dir, pEnt->d_name));
                continue;
            }
            strcpy(bkupdir, info->bdir);
            if (*dir != '/') {
                strcat(bkupdir, "/");
            }
            strcat(bkupdir, dir);
            strcat(bkupdir, "/");
            strcat(bkupdir, pEnt->d_name);
            newdir = bkupdir + info->blen;
            if (!newDirectory(bkupdir, &stbuf, info)) {
                errRet(("newDirectory(%s, 0x%08x)", bkupdir, stbuf.st_mode));
                continue;
            }
            dirwalk(newdir, info); /* recursion */
            free(bkupdir);
            if (chdir(dir)) {
                errSysRet(("chdir(%s)", dir));
                return 0;
            }
            break;
        case S_IFLNK:
        case S_IFREG:
            info->ctime = stbuf.st_ctime;
            info->mtime = stbuf.st_mtime;
            info->stbuf = &stbuf;
            (*info->func)(dir, pEnt->d_name, info);
            break;
        default:
            errRet(("%s/%s: unknown type (0x%x) ignored\n",
                  dir, pEnt->d_name, stbuf.st_mode & S_IFMT));
            break;
        }
    }
    if (closedir(pDir)) {
        errSysRet(("closedir(%d)", dir));
    }
    return 1;
}
