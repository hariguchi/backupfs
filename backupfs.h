/* $Id: backupfs.h,v 1.29 2005/04/21 23:49:59 cvsremote Exp $

   backupfs.h: Local definitions and function prototypes


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

#ifndef __backupfs_h__
#define __backupfs_h__


#define PROGNAME "backupfs"
#define PROGNAME_REMOTE  "backupfs-remote"
#define PROGNAME_CHKSRC  "backupfs-chksrc"
#define PROGNAME_MKDIR   "backupfs-mkdir"
#define PROGNAME_MKLINK  "backupfs-mklink"
#define PROGNAME_NEWFILE "newfiles"
#define DEBUG            "DEBUG"   /* env. var. for debugging */
#define WAITGDB          "WAITGDB" /* env. var. to debug children */

#define JNL_FILE     ".backupfs-journal"
#define OLD_JNL_FILE "/tmp/.backupfs-old-journal-XXXXXX"
#define TAR_FILE     "/tmp/backupfs-tar-XXXXXX"
#define LINK_FILE    "/tmp/backupfs-link-XXXXXX"
#define ID_FILE      ".id_rsa"
#define BKUP_DIR     "2003/01/02" /* backup directory template */
#define TAR_SRC      "tar -c -T %s -f -"
#define TAR_DST      "tar xpf -"
#define RMT_DIR_FILE "dirs-%s-%s"
#define RMT_LNK_FILE "links-%s-%s"
#define RMT_TAR_FILE "tar-%s-%s"
#define DEFAULT_USER "backupfs"
#define SSH          "ssh -i %s %s@%s "
#define RMT_PASS1_1  SSH "backupfs-chksrc %s"
#define RMT_PASS1_2  "backupfs-mkdir"
#define RMT_PASS2    SSH "backupfs-remote %s %s %s %s"
#define RMT_PASS3_1  SSH "cat %s"
#define RMT_PASS3_2  "backupfs-mkdir"
#define RMT_PASS4_1  SSH "cat %s"
#define RMT_PASS4_2  "backupfs-mklink %s %s"
#define RMT_PASS5_1  SSH "backupfs-exctar %s %s"
#define RMT_PASS5_2  TAR_DST
#define RMT_PASS6    SSH "rm -f %s %s %s"
#define VERSION      "backupfs Version 1.0 Beta 5 ($Revision: 1.29 $)"


typedef enum {
    bkupError = 0,
    bkupFirstTime,              /* first time backup */
    bkupRecurrent,              /* recurrent backup */
} bkupType;

enum {
    defUmask    = 0022,
    destDirMode = 0755,
    MAXARGS     = 32,           /* max command arguments */
    MAXCHARS    = 1024,         /* max characters per line */
};


typedef struct _bkupInfo* pbkupInfo;
typedef void (*pMakeCmd)(char* dir, char* file, pbkupInfo pInfo);

typedef struct _bkupInfo {
    char*    dest;              /* backup destination root */
    char*    src;               /* source directory */
    char*    host;              /* remote host (backup source) */
    char*    user;              /* user name on remote host */
    FILE*    jnl;               /* new journal file */
    char*    jpath;             /* new journal file path name */
    char*    oldJpath;          /* old journal file path name */
    void*    jt;                /* journal tree */
    FILE*    tar;               /* tar input file */
    char*    tpath;             /* tar input file path name */
    char*    bdir;              /* backup directory */
    int      blen;              /* length of bdir */
    char*    lbdir;             /* last backup directory */
    int      lblen;             /* length of lbdir */
    pMakeCmd func;              /* func ptr to make backup command */
    time_t   ctime;             /* current file ctime */
    time_t   mtime;             /* current file mtime */
    char*    sshid;             /* ssh secret key (id) file path name */
    char*    ndpath;            /* new directories info file in remote host */
    FILE*    newdirs;           /* new directories in remote host */
    char*    linkpath;          /* hard link info file in remote host */
    FILE*    links;
    struct stat* stbuf;         /* for newfiles and changedfiles */
} bkupInfo;


typedef struct {
    time_t ctime;
    time_t mtime;
    char*  path;
} journalEntry;


typedef struct {
    struct sigaction ignore;
    struct sigaction svIntr;
    struct sigaction svQuit;
    int isSet;
} intQuitSigs;

/* isNormalExit
     bit 7:   1: cmd2 exited, 0: not yet
     bit 6:   1: cmd1 exited, 0: not yet
     bit 5-2: not used
     bit 1:   1: cmd2 exited normally, 0: abnormal exit
     bit 0:   1: cmd1 exited normally, 0: abnormal exit
 */
typedef struct {
    unsigned char exitInfo;
    char          status[2];    /* exit status, 0: cmd1, 1: cmd2 */
} pipeExitSt;


extern char CompilationDate[];


/* Function prototypes
 */
void     backupfsExit(bkupInfo* info, int exitStatus);

int      dirwalk(char* dir, bkupInfo* pInfo);

bkupType chkSource(bkupInfo* info);
void     chkDest(bkupInfo* info);
void     firstTimeBackup(char* dir, char* file, bkupInfo* info);
void     recurrentBackup(char* dir, char* file, bkupInfo* info);

FILE*      makeTemp(char* path, char* mode);
int        getPathMode(char* path, mode_t* mode);
void       openFilesLocal(bkupInfo* info);
void       openFilesRemote(bkupInfo* info);
void       closeFilesLocal(bkupInfo* info);
void       closeFiles(bkupInfo* info);
void       removeFiles(bkupInfo* info);
int        moveFile(char* from, char* to);      
int        isDirEmpty(char* dir);
int        makeJournalTree(bkupInfo* info);
int        runCommands(bkupInfo* info);
pipeExitSt execCommands(char* cmd1, char* cmd2);
int        chkCmdExitSt(pipeExitSt st, char* cmd);
int        chkPipeExitSt(pipeExitSt st, char* cmd1, char* cmd2);

void       makeSshKey(bkupInfo* info);
int        chkRemoteSrc(bkupInfo* info);
int        lastBkupDirFromTime(time_t mtime, bkupInfo* info);
int        getLastBkupDir(bkupInfo* info);
int        doRemote(bkupInfo* info);
int        newDirectory(char* bkupdir, struct stat* pstat, bkupInfo* info);
int        makeLink(char* src, char* dest, bkupInfo* info);
void       writeDestDir(bkupInfo* info);
void       openJournalFile (bkupInfo* info);


/* Inline functions
 */
#include <sys/types.h>
#include <sys/wait.h>
#include "error.h"

static inline int
isChildDebugOn (void)
{
    return getenv(WAITGDB) ? 1 : 0;
}


static inline int
isDebugOn (void)
{
    return getenv(DEBUG) ? 1 : 0;
}


/* Write character string `s' to `fp'.
   `file' must be the filename of `fp'
 */
static inline void
fwriteExit (char* s, FILE* fp, char* file, bkupInfo* info)
{
    size_t len, wrlen;


    len = strlen(s);
    if (len == 0) len = 1;         /* write "\0" */
    wrlen = fwrite(s, 1, len, fp);
    if (wrlen < len) {
        errSysRet(("fwrite(%s): %s", file, s));
        backupfsExit(info, 1);
    }
}

static inline int
isIllegalCmdID (int i)
{
    if (i == 0 || i == 1) return 0; /* legal */
    errRet(("%d: cmd ID must 0 or 1", i));
    return 1;                   /* illegal */
}
static inline void
cmdExited (int i, pipeExitSt* st)
{
    if (isIllegalCmdID(i)) return;
    st->exitInfo |= (1<<(6+i));
}
static inline void
cmd1Exited (pipeExitSt* st)
{
    st->exitInfo |= (1<<6);
}
static inline void
cmd2Exited (pipeExitSt* st)
{
    st->exitInfo |= (1<<7);
}
static inline void
setCmdExitType (int i, pipeExitSt* st, int status)
{
    if (isIllegalCmdID(i)) return;
    if (WIFEXITED(status)) {
        st->exitInfo |= (1<<i);
    } else {
        st->exitInfo &= ~(1<<i);
    }
}
static inline void
setCmd1ExitType (pipeExitSt* st, int status)
{
    if (WIFEXITED(status)) {
        st->exitInfo |= 1;
    } else {
        st->exitInfo &= ~1;
    }
}
static inline void
setCmd2ExitType (pipeExitSt* st, int status)
{
    if (WIFEXITED(status)) {
        st->exitInfo |= (1<<1);
    } else {
        st->exitInfo &= ~(1<<1);
    }
}

static inline int
didCmdExit (int i, pipeExitSt st)
{
    if (isIllegalCmdID(i)) return 0;
    return (st.exitInfo & (1<<(6+i))) ? 1 : 0;
}
static inline int
didCmd1Exit (pipeExitSt st)
{
    return (st.exitInfo & (1<<6)) ? 1 : 0;
}
static inline int
didCmd2Exit (pipeExitSt st)
{
    return (st.exitInfo & (1<<7)) ? 1 : 0;
}
static inline int
isCmdNormalExit (int i, pipeExitSt st)
{
    if (isIllegalCmdID(i)) return 0;
    return (st.exitInfo & (1<<i)) ? 1 : 0;
}
static inline int
isCmd1NormalExit (pipeExitSt st)
{
    return (st.exitInfo & 1);
}
static inline int
isCmd2NormalExit (pipeExitSt st)
{
    return ((st.exitInfo>>1) & 1);
}

#endif /* __backupfs_h__ */
