/* $Id: getline.c,v 1.2 2005/04/21 04:46:02 cvsremote Exp $

   getline.c: in case some system does not have GNU getline()


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
#include <sys/types.h>

#include "error.h"


enum {
    INIT_NUM_CHARS = 1024,
};


ssize_t
getdelim (char **lineptr, size_t *size, int delim, FILE *fp)
{
    char*   buf;
    int     c;
    int     newsize;
    ssize_t nChars;


    assert(lineptr);
    assert(*lineptr);
    assert(size);
    assert(fp);

    if (lineptr) {
        buf = *lineptr;
    } else {
        buf = malloc(INIT_NUM_CHARS);
        if (!buf) {
            errSysRet(("no memory"));
            return -1;
        }
        *size = INIT_NUM_CHARS;
    }

    nChars = 0;
    for (;;) {
        if (nChars >= *size - 1) {
            newsize = *size + INIT_NUM_CHARS;
            buf = realloc(buf, newsize);
            if (!buf) {
                errSysRet(("no more memory"));
                return -1;
            }
            *lineptr = buf;
            *size    = newsize;
        }
        c = getc(fp);
        if (c == EOF) {
            buf[nChars] = '\0';
            return nChars;
        }
        buf[nChars++] = c;
        if (c == delim) {
            buf[nChars] = '\0';
            return nChars;
        }
    }
}


ssize_t
getline (char **lineptr, size_t *n, FILE *stream)
{
    return getdelim(lineptr, n, '\n', stream);
}
