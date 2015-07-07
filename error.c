/* $Id: error.c,v 1.3 2005/04/21 04:46:01 cvsremote Exp $

   error.c: Error handlers


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

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"

#define MAXERRCHARS 1024


static void
errorDoit (int errnoflag, const char* fmt, va_list ap)
{
    int errno_save;
    int len;
    static char buf[MAXERRCHARS];


    errno_save = errno;
    vsnprintf(buf, sizeof(buf), fmt, ap);
    if (errnoflag) {
        len = strlen(buf);
        snprintf(buf+len, sizeof(buf)-len, ": %s", strerror(errno_save));
    }
    len = strlen(buf)+1;
    strncat(buf, "\n", sizeof(buf)-len);
    fflush(stdout);             /* in case stdout and stderr are the same */
    fputs(buf, stderr);
    fflush(NULL);               /* flushes all stdio output streams */
}


void
errorSysReturn (const char* fmt, ...)
{
    va_list ap;


    va_start(ap, fmt);
    errorDoit(1, fmt, ap);
    va_end(ap);
}


void
errorSysExit (const char* fmt, ...)
{
    va_list ap;


    va_start(ap, fmt);
    errorDoit(1, fmt, ap);
    va_end(ap);
    exit(1);
}


void
errorSysDump (const char* fmt, ...)
{
    va_list ap;


    va_start(ap, fmt);
    errorDoit(1, fmt, ap);
    va_end(ap);
    abort();
    exit(1);
}


void
errorReturn (const char* fmt, ...)
{
    va_list ap;


    va_start(ap, fmt);
    errorDoit(0, fmt, ap);
    va_end(ap);
}


void
errorExit (const char* fmt, ...)
{
    va_list ap;


    va_start(ap, fmt);
    errorDoit(0, fmt, ap);
    va_end(ap);
    exit(1);
}
