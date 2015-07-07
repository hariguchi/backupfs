/* $Id: string-rbt.hpp,v 1.2 2015/04/14 01:53:51 cvsremote Exp $

   string-rbt.hpp: C++ header file for a red-black tree whose key
                   is an ASCII string that can be called by C functions.


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

#ifndef __STRING_RBT_HPP__
#define __STRING_RBT_HPP__

#include <iostream>
#include <memory>
#include <boost/intrusive/rbtree.hpp>
#include <boost/format.hpp>

namespace stringRBT
{

class node : public boost::intrusive::
                    set_base_hook<boost::intrusive::optimize_size<true> >
{
private:
    std::string key;
    void *value;
    node *self;
public:
    node () { self = this; };

    friend bool operator<(const node &a, const node &b)
        { return a.key < b.key; };
    friend bool operator>(const node &a, const node &b)
        { return a.key > b.key; };
    friend bool operator==(const node &a, const node &b)
        { return a.key == b.key; };
    void  setKey (const std::string &s) { key = s; };
    void  setKey (const char *s) { key = s; };
    void* getVal (void) const { return value; };
    void  setVal (void *val) { value = val; };
    node* getSelf() const { return self; };
  const std::string getKey() const { return std::move(key); };
};

typedef boost::intrusive::rbtree<node> rbt;
typedef rbt::iterator iterator;
typedef rbt::const_iterator const_iterator;

} // namespace stringRBT

/*
 * Function prototypes
 */
stringRBT::const_iterator stringRBTfindNode (void* rbt, const char* key);


#endif // __STRING_RBT_HPP__
