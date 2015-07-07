/* $Id: string-rbt.h,v 1.2 2015/03/04 21:10:41 cvsremote Exp $

   string-rbt.h: C header file for a red-black tree
                 whose key is an ASCII string


   Copyright (c) 2015, Yoichi Hariguchi
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

#ifndef __STRING_RBT_H__
#define __STRING_RBT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>

typedef void (*stringRBTcb)(const char *key, void *val, void *arg);

void*  stringRBTcreate (void);
int    stringRBTinsert (void *rbt, const char *key, void *value);
void*  stringRBTremove (void *rbt, const char *key);
void*  stringRBTfind (void *rbt, const char *key);
void   stringRBTwalk (void *rbt, stringRBTcb f, void* arg);
size_t stringRBTsize (void *rbt);

#ifdef __cplusplus
}
#endif


#endif /* __STRING_RBT_H__ */
