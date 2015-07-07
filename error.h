/* $Id: error.h,v 1.4 2005/04/21 04:46:01 cvsremote Exp $

   error.h: Error handler macros and function prototypes


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


#ifndef __error_h__
#define __error_h__

#include <errno.h>


#define errSysRet(_arg_)  { fprintf(stderr, "Error(%d): %s(%s:%d): ", \
                            errno, __FUNCTION__, __FILE__, __LINE__); \
                            errorSysReturn _arg_; }
#define errSysExit(_arg_) { fprintf(stderr, "Error(%d): %s(%s:%d): ", \
                            errno, __FUNCTION__, __FILE__, __LINE__); \
                            errorSysExit _arg_; }
#define errSysDump(_arg_) { fprintf(stderr, "Error: %s(%s:%d): ", \
                            __FUNCTION__, __FILE__, __LINE__); \
                            errorSysDump _arg_; }
#define errRet(_arg_)     { fprintf(stderr, "Error: %s(%s:%d): ", \
                            __FUNCTION__, __FILE__, __LINE__); \
                            errorReturn _arg_; }
#define errExit(_arg_)    { fprintf(stderr, "Error: %s(%s:%d): ", \
                            __FUNCTION__, __FILE__, __LINE__); \
                            errorExit _arg_; }

#define dbgInfo(_arg_)    { fprintf(stderr, "%s(%s:%d): ", \
                            __FUNCTION__, __FILE__, __LINE__); \
                            errorReturn _arg_; }


void errorSysReturn(const char* fmt, ...);
void errorSysExit(const char* fmt, ...);
void errorSysDump(const char* fmt, ...);
void errorReturn(const char* fmt, ...);
void errorExit(const char* fmt, ...);

#endif /* __error_h__ */
