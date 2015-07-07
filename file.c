/* $Id: file.c,v 1.17 2015/03/04 20:44:32 cvsremote Exp $

   file.c: file manipulation functions


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

#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <unistd.h>

#include "string-rbt.h"
#include "backupfs.h"
#include "error.h"


FILE*
makeTemp (char* path, char* mode)
{
    int fd;
    FILE* fp;


    if (!path) {
        errSysRet(("path is NULL"));
        return NULL;
    }
    if (!mode) {
        errSysRet(("mode is NULL"));
        return NULL;
    }

    fd = mkstemp(path);
    if (fd < 0) {
        errSysRet(("mkstemp(%s)", path));
        return NULL;
    }
    fp = fdopen(fd, mode);
    if (!fp) {
        errSysRet(("fdopen(%s, %s)", path, mode));
    }
    return fp;
}


/* Set path (file, directory, etc.) mode to *mode.
   Return 1 if success, 0 otherwise
 */
int
getPathMode (char* path, mode_t* mode)
{
    struct stat stbuf;


    if (!mode) return 0;
    if (!path) return 0;

    if (stat(path, &stbuf)) {
        errSysRet(("stat(%s)", path));
        return 0;
    }
    *mode = stbuf.st_mode;
    return 1;
}


void
closeFiles (bkupInfo* info)
{
    assert(info);

    if (info->jnl) {
        if (fclose(info->jnl)) errSysRet(("fclose(jnl)"));
        info->jnl = NULL;
    }
    if (info->tar) {
        if (fclose(info->tar)) errSysRet(("fclose(tar)"));
        info->tar = NULL;
    }
    if (info->newdirs && fclose(info->newdirs)) {
        errSysRet(("fclose(newdirs)"));
    }
    info->newdirs = NULL;
    if (info->links && fclose(info->links)) {
        errSysRet(("fclose(links)"));
    }
    info->links = NULL;
}


void
removeFiles (bkupInfo* info)
{
    assert(info);

    /* Hard link and tar files may not be created yet.
     */
    if (info->tpath && unlink(info->tpath)) {
        errSysRet(("unlink(tpath:%s)", info->tpath));
    }
    if (info->ndpath && unlink(info->ndpath)) {
        errSysRet(("unlink(ndpath:%s)", info->ndpath));
    }
    if (info->linkpath && unlink(info->linkpath)) {
        errSysRet(("unlink(linkpath:%s)", info->linkpath));
    }
}


/* `from' must be a regular file.
 */
int
copyFile (char* from, char* to)
{
    struct stat stbuf;
    mode_t mode;
    int    rnum, wnum;
    int    rv;                  /* return value */
    FILE*  ffp;                 /* from fp */
    FILE*  tfp;                 /* to fp */
    static char buf[MAXCHARS];

   
    assert(from);
    assert(to);

    if (stat(from, &stbuf)) {
        errSysRet(("stat(%s)", from));
        return 0;
    } else {
        mode = stbuf.st_mode & S_IFMT;
        switch (mode) {
        case S_IFLNK:
        case S_IFREG:
            break;
        default:
            errRet(("%s: wrong file type (%x)", from, mode));
            return 0;
        }
    }
    ffp = fopen(from, "r");
    if (!ffp) {
        errSysRet(("fopen(%s)", from));
        return 0;
    }
    tfp = fopen(to, "w");
    if (!tfp) {
        errSysRet(("fopen(%s)", to));
        fclose(ffp);
        return 0;
    }
    rv = 1;
    while(!feof(ffp)) {
        rnum = fread(buf, 1, sizeof(buf), ffp);
        if ((rnum <= 0) && (!feof(ffp))) {
            errSysRet(("fread(%s)", from));
            rv = 0;
            break;
        }
        wnum = fwrite(buf, 1, rnum, tfp);
        if (wnum < rnum) {
            errSysRet(("fwrite(%s)", to));
            rv = 0;
            break;
        }
    }
    fclose(ffp);
    fclose(tfp);
    return rv;
}


int
moveFile (char* from, char* to)
{
    struct stat stbuf;


    assert(from);
    assert(to);

    if (stat(to, &stbuf)) {
        if (errno != ENOENT) {
            errSysRet(("stat(%s)", to));
            return 0;
        }
    } else {
        if (S_ISDIR(stbuf.st_mode)) {
            errRet(("%s is a directory", to));
            return 0;
        }
        if (unlink(to)) {
            errSysRet(("unlink(%s)", to));
            return 0;
        }
    }
    if (link(from, to)) {
        if (errno != EXDEV) {
            errSysRet(("link(%s, %s)", from, to));
            return 0;
        }
        if (!copyFile(from, to)) return 0;
    }
    if (unlink(from)) {
        errSysRet(("unlink(%s)", from));
    }

    return 1;
}


#if 0
/* tavl: function to return comparison key pointer
         of data node required by tavl_init.
 */
static void*
getCmpKey (void *node)
{
    assert(node);

    return ((journalEntry*)node)->path;
}

/* tavl: function to create a new tavl entry (node)
         and return its pointer required by tavl_init.
 */
static void *
makeEntry (const void *p)
{
    journalEntry* ent = (journalEntry*)p;
    journalEntry* new;
    int len;


    assert(p);

    len = sizeof(*new) + strlen(ent->path) + 1;
    new = malloc(len);
    if (!new) {
        errSysRet(("malloc(%d)", sizeof(*new)));
        return NULL;
    }
    new->path = (char*)(new + 1);
    new->ctime = ent->ctime;
    new->mtime = ent->mtime;
    memcpy(new->path, ent->path, len - sizeof(*new));
    return new;
}


/* tavl: function to copy a tavl entry (node) required by tavl_init.
 */
void*
copyEntry (void* dest, const void* src)
{
    journalEntry* ent = (journalEntry*)src;
    int len;


    assert(dest);
    assert(ent);
    assert(ent->path);

    len = sizeof(journalEntry) + strlen(ent->path) + 1;
    memcpy(dest, ent, len);
    return dest;
}
#endif/*0*/


int
makeJournalTree (bkupInfo* info)
{
    FILE* fp;                   /* journal file */
    char* buf;
    char* s;
    journalEntry* ent;
    size_t  len;
    ssize_t rdlen;


    assert(info);
    assert(info->oldJpath);

    buf = malloc(MAXCHARS);
    if (!buf) {
        errSysRet(("malloc(%d)", MAXCHARS));
        return 0;
    }
    fp = fopen(info->oldJpath, "r");
    if (!fp) {
        errSysRet(("fopen(%s)", info->oldJpath));
        return 0;
    }
    info->jt = stringRBTcreate();
    if (!info->jt) {
        errRet(("stringRBTcreate() failed"));
        return 0;
    }

    while ((rdlen = getline(&buf, &len, fp)) > 0) {
        buf[rdlen-1] = '\0';
        ent = malloc(sizeof(*ent));
        if (!ent) {
            errSysRet(("malloc: %s", buf));
        } else {
            ent->ctime = strtol(buf, &s, 16);
            ent->mtime = strtol(s, &s, 16);
            ent->path = ++s;
            if (stringRBTinsert(info->jt, ent->path, ent)) {
                errRet(("stringRBTinsert(%s)", ent->path));
                goto errReturn;
            }
        }
    }
    if (!feof(fp)) {
        errSysRet(("getline"));
        goto errReturn;
    }

    free(buf);
    return 1;

errReturn:
    free(buf);
    return 0;
}


int
isDirEmpty (char* path)
{
    DIR*           pDir;
    struct dirent* pEnt;


    if (!path) {
        errRet(("null path name. Regarded as empty"));
        return 1;
    }
    pDir = opendir(path);
    if (!pDir) {
        errSysRet(("opendir(%s). Regarded as empty", path));
        return 1;
    }
    for (pEnt = readdir(pDir); pEnt; pEnt = readdir(pDir)) {
        if (!strcmp(".", pEnt->d_name)) continue;
        if (!strcmp("..", pEnt->d_name)) continue;
        return 0;
    }
    return 1;
}


int
runCommands (bkupInfo* info)
{
    char*      cmd;
    int        len;
    int        status;
    pipeExitSt st;              /* return status in wait() format */


    assert(info);
    assert(info->tpath);
    assert(info->bdir);

    /* cd backup-directory for "tar xpf -"
     */
    if (chdir(info->bdir)) errSysExit(("chdir(%s)", info->bdir));
 
    len = strlen(TAR_SRC) + strlen(info->tpath) + 1;
    cmd = malloc(len);
    if (!cmd) errSysExit(("1: malloc(%d)", len));
    snprintf(cmd, len, TAR_SRC, info->tpath);
    st = execCommands(cmd, TAR_DST);
    status = chkPipeExitSt(st, cmd, TAR_DST);
    free(cmd);
    return status;
}


/* Split command and arguments and store them to argv
   1. cmdstr must not have leading blanks
   2. caller must call:
       free(argv[0]);
       free(argv);
     in the above order
 */
char**
mkArgs (char* cmdstr)
{
    char** argv;
    char*  cmd;
    char*  p;
    int    i, j;


    argv = malloc(sizeof(argv) * MAXARGS);
    if (!argv) errSysExit(("1: malloc(%d)", sizeof(argv) * MAXARGS));
    cmd = malloc(strlen(cmdstr) + 1);
    if (!cmd) errSysExit(("2: malloc(%d)", strlen(cmdstr) + 1));
    strcpy(cmd, cmdstr);

    argv[0] = cmd;
    j = 1;
    for (i = 1, p = index(cmd, ' ');  p; p = index(p, ' '), ++i) {
        *p++ = '\0';
        argv[i] = p;
        if (i >= j * MAXARGS) {
            ++j;
            argv = realloc(argv, sizeof(argv) * MAXARGS);
            if (!argv) errSysExit(("realloc(%d)", sizeof(argv) * MAXARGS));
        }
    }
    if (i >= j * MAXARGS) {
        argv = realloc(argv, sizeof(argv));
        if (!argv) errSysExit(("realloc(%d)", sizeof(argv)));
    }
    argv[i] = NULL;
    return argv;
}


void
waitGdb ()
{
    int i;

    if (!isChildDebugOn()) return; /* no child debugging */

    i = 1;
    while (i);        /* attach the process and set i to 0  to exit */
}


/* Ignore SIGINT and SIGQUIT
   Return 1 on success, 0 otherwise.
 */
int
ignoreSigIntQuit (intQuitSigs* p)
{
    assert(p);

    if (p->isSet) {
        errRet(("already set"));
        return 0;
    }
    p->ignore.sa_handler = SIG_IGN;
    sigemptyset(&p->ignore.sa_mask);
    p->ignore.sa_flags = 0;
    if (sigaction(SIGINT, &p->ignore, &p->svIntr) < 0) return 0;
    if (sigaction(SIGQUIT, &p->ignore, &p->svQuit) < 0) {
        if (sigaction(SIGINT, &p->svIntr, NULL) < 0) {
            errSysRet(("can't restore SIGINT handler"));
        }
        return 0;
    }

    p->isSet = 1;               /* signal handlers are set */
    return 1;
}


int
restoreSigIntQuit (intQuitSigs* p)
{
    assert(p);

    if (!p->isSet) {
        errRet(("not set"));
        return 0;
    }
    if (sigaction(SIGINT, &p->svIntr, NULL) < 0) {
        errSysRet(("sigaction(SIGINT)"));
        return 0;
    }
    if (sigaction(SIGQUIT, &p->svQuit, NULL) < 0) {
        errSysRet(("sigaction(SIGQUIT)"));
        if (sigaction(SIGINT, &p->ignore, &p->svIntr) < 0) {
            errSysRet(("can't set SIGINT handler to SIG_IGN"));
        }
        return 0;
    }

    p->isSet = 0;               /* signal handlers are restored */
    return 1;
}


/* Execute command strings cmd1 and cmd2 wherein
   cmd1's stdout is connected to cmd's stdin
   Return value: cmd2's exit status in wait() format
 */
pipeExitSt
execPipe (char* cmd1, char* cmd2, intQuitSigs* sigs)
{
    pid_t      rpid;
    pid_t      pid[2];
    int        fd[2];
    int        i;
    char*      cmd;
    int        status;
    pipeExitSt st;
    char**     argv;


    assert(cmd1);
    assert(cmd2);
    assert(sigs);

    if (pipe(fd) < 0) {
        errSysExit(("pipe"));
    }

    pid[0] = fork();
    if (pid[0] < 0) errSysExit(("1: fork"));

    if (pid[0] == 0) {          /* child 1 for cmd2 */
        waitGdb();
        restoreSigIntQuit(sigs);
        argv = mkArgs(cmd2);
        if (!argv) {
            errRet(("mkArgs(%s)", cmd2));
            _exit(127);
        }
        close(fd[1]);           /* close write end */
        if (fd[0] != 0) {
            if (dup2(fd[0], 0) != 0) {
                errSysRet(("dup2: stdin"));
                _exit(127);
            }
            close(fd[0]);       /* fd[0] is duped to stdin */
        }
        execvp(argv[0], argv);
        free(argv[0]);          /* exec error */
        free(argv);
        _exit(127);
    } else {                    /* parent */
        if (isChildDebugOn()) dbgInfo(("pid[0] = %d", pid[0]));

        pid[1] = fork();
        if (pid[1] < 0) {
            dbgInfo(("2: fork"));
            if (kill(pid[0], SIGKILL) < 0) {
                errSysRet(("couldn't kill %d", pid[0]));
            }
            close(fd[0]);
            close(fd[1]);
            exit(1);
        }
        if (pid[1] == 0) {      /* child 2 for cmd1 */
            waitGdb();
            restoreSigIntQuit(sigs);
            argv = mkArgs(cmd1);
            if (!argv) {
                errRet(("mkArgs(%s)", cmd1));
                close(fd[0]);
                close(fd[1]);
                _exit(127);
            }
            close(fd[0]);       /* close read end */
            if (fd[1] != 1) {
                if (dup2(fd[1], 1) != 1) {
                    errSysRet(("dup2: stdout"));
                    _exit(127);
                }
                close(fd[1]);   /* fd[1] is duped to stdout */
            }
            execvp(argv[0], argv);
            free(argv[0]);      /* exec error */
            free(argv);
            _exit(127);
        } else {                /* parent */
            close(fd[0]);
            close(fd[1]);
            if (isChildDebugOn()) dbgInfo(("pid[1] = %d", pid[1]));

            memset(&st, 0, sizeof(st));
            for (; !(didCmd1Exit(st) && didCmd2Exit(st));) {
                while((rpid = wait(&status)) < 0) {
                    errSysRet(("wait(%d)", status));
                    if (errno != EINTR) {
                        status = -1; /* error other than EINTR from wait() */
                        break;
                    }
                }
                if (rpid == pid[0]) {        /* pid[0] is for cmd2 */
                    i = 1;
                    cmd = cmd2;
                } else if (rpid == pid[1]) { /* pid[1] is for cmd1 */
                    i = 0;
                    cmd = cmd1;
                } else {
                    i = 2;    /* illegal cmd ID */
                    cmd = NULL;
                    errRet(("%d: shouldn't happen", rpid));
                }
                cmdExited(i, &st);
                setCmdExitType(i, &st, status);
                if (isCmdNormalExit(i, st)) {
                    st.status[i] = WEXITSTATUS(status);
                }
                if (isDebugOn()) {
                    dbgInfo(("pid/exType/exSt: %d/%d/%d: %s", rpid,
                             isCmdNormalExit(i, st), st.status[i],
                                     cmd ? cmd : "<no-associated-command>"));
                }
            }
        }
    }
    return st;
}


/* Execute command string cmd1.
   If cmd2 is non NULL, cmd1's stdout is connected to cmd2's stdin
 */
pipeExitSt
execCommands (char* cmd1, char* cmd2)
{
    pid_t            pid;
    int              status;
    pipeExitSt       st;
    sigset_t         chldMask, svMask;
    char**           argv;
    intQuitSigs      sigs;


    memset(&st, 0, sizeof(st));
    if (!cmd1) {
        errRet(("NULL command string"));
        return st;              /* isCmd1NormalExit(st) == 0 */
    }

    /* ignore SIGINT, SIGQUIT
     */
    memset(&sigs, 0, sizeof(sigs));
    if (!ignoreSigIntQuit(&sigs)) {
        errRet(("sigIntQuitIgnore"));
        return st;              /* isCmd1NormalExit(st) == 0 */
    }

    /* block SIGCHLD
     */
    sigemptyset(&chldMask);
    sigaddset(&chldMask, SIGCHLD);
    if (sigprocmask(SIG_BLOCK, &chldMask, &svMask) < 0) {
        errSysRet(("sigprocmask"));
        goto restoreSigs;
    }

    if (cmd2) {
        st = execPipe(cmd1, cmd2, &sigs);
        goto restoreSigs;
    }
    pid = fork();
    if (pid < 0) errSysExit(("fork: %s", cmd1));
    if (pid == 0) {  /* child */
        waitGdb();
        /* restore previous signal actions and reset signal mask
         */
        restoreSigIntQuit(&sigs);
        argv = mkArgs(cmd1);
        if (!argv) _exit(127);
        execvp(argv[0], argv);
        free(argv[0]);          /* exec error */
        free(argv);
        _exit(127);
    } else {                    /* parent */
        while (waitpid(pid, &status, 0) < 0) {
            errSysRet(("wait(%d)", status));
            if (errno != EINTR) {
                status = -1; /* error other than EINTR from waitpid() */
                break;
            }
        }
        cmd1Exited(&st);
        setCmd1ExitType(&st, status);
        if (isCmd1NormalExit(st)) st.status[0] = WEXITSTATUS(status);
        if (isDebugOn()) {
            dbgInfo(("%d/%d/%d", pid, isCmd1NormalExit(st), st.status[0]));
        }
    }

restoreSigs:
    /* restore previous signal actions and reset signal mask
     */
    if (!restoreSigIntQuit(&sigs)) {
        errRet(("restoreSigIntQuit()"));
        st.status[0] = -1;
    }
    return st;
}


int
chkCommandExitSt (int i, pipeExitSt st, char* cmd)
{
    assert(cmd);

    if (!isCmdNormalExit(i, st)) {
        errRet(("cmd%d abnormal exit: %s", i+1, cmd));
        return 0;
    }
    if (st.status[i] != 0) {
        errRet(("cmd%d exit status(%d): %s", i+1, st.status[i], cmd));
        return 0;
    }
    return 1;
}

int
chkCmdExitSt (pipeExitSt st, char* cmd)
{
    return chkCommandExitSt(0, st, cmd);
}


int
chkPipeExitSt (pipeExitSt st, char* cmd1, char* cmd2)
{
    assert(cmd1);
    assert(cmd2);

    if (!chkCommandExitSt(0, st, cmd1)) goto errReturn;
    if (!chkCommandExitSt(1, st, cmd2)) goto errReturn;
    return 1;

errReturn:
    dbgInfo(("%s | %s", cmd1, cmd2));
    return 0;
}

