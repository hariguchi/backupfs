/* $Id: string-rbt.cc,v 1.4 2015/04/14 01:53:51 cvsremote Exp $

   string-rbt.cc: A red-black tree whose key is an ASCII string
                  that can be called by C functions.


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

#include "string-rbt.hpp"
#include "string-rbt.h"


/**
 * @name  stringRBTcreate
 *
 * @brief API Function.
 *        It creates a red-black tree object.
 *
 * @retval void* Pointer to a new red-black tree object.
 * @retval NULL  Failed to create a red-black tree object.
 */
void*
stringRBTcreate (void)
{
    std::unique_ptr<stringRBT::rbt> tree(new stringRBT::rbt);
    if (tree.get()) {
        return tree.release();
    } else {
        return NULL;
    }
}

/**
 * @name  stringRBTinsert
 *
 * @brief API Function.
 *        It inserts a (key, value) pair to the given red-black tree.
 *
 * @param[in] rbt   Pointer to a red-black tree
 * @param[in] key   Pointer to the search key to be inserted to 'rbt'
 * @param[in] value Pointer to the value associated with 'key'
 *                  to be inserted to 'rbt'
 *
 * @retval 0          (key, value) paire is successfully inserted to 'rbt'
 * @retval -EINVAL    'rbt' and/or key is NULL
 * @retval -ENOMEM    Failed to allocate memory
 * @retval -EOVERFLOW 'rbt' already has the same 'key'.
 */
int
stringRBTinsert (void* rbt, const char* key, void* value)
{
    if (!rbt) {
        return -EINVAL;
    }
    if (!key) {
        return -EINVAL;
    }
    std::pair<stringRBT::iterator, bool> rc;
    stringRBT::rbt* tree = reinterpret_cast<stringRBT::rbt*>(rbt);
    std::unique_ptr<stringRBT::node> node(new stringRBT::node);
    if (!node.get()) {
        return -ENOMEM;
    }
    node->setKey(key);
    node->setVal(value);
    rc = tree->insert_unique(*node);
    if (!rc.second) {
        return -EOVERFLOW;
    }
    node.release();
    return 0;
}

/**
 * @name  stringRBTfind
 *
 * @brief API function.
 *        It searches the given red-black tree for the entry
 *        that matchees given key.
 *
 * @param[in] rbt Pointer to a red-black tree
 * @param[in] key Pointer to the search key
 *
 * @retval void* Pointer to the matching entry
 * @retval NULL  No matching entry found
 */
void*
stringRBTfind (void* rbt, const char* key)
{
    stringRBT::rbt* tree = reinterpret_cast<stringRBT::rbt*>(rbt);
    stringRBT::const_iterator it = stringRBTfindNode(rbt, key);
    if (it == tree->end()) {
        return NULL;
    }
    return it->getVal();
}

/**
 * @name  stringRBTremove
 *
 * @brief API function.
 *        It removes the entry that matches the given key
 *        from the given red-black tree.
 *
 * @param[in] rbt Pointer to a red-black tree
 * @param[in] key Pointer to the search key to be removed
 *
 * @retval void* Pointer to the value whose associated key was
 *               found in `rbt'. The matching red-black tree entry
 *               is removed.
 * @retval NULL  No matching entry found.
 */
void*
stringRBTremove (void* rbt, const char* key)
{
    stringRBT::rbt* tree = reinterpret_cast<stringRBT::rbt*>(rbt);
    stringRBT::const_iterator it = stringRBTfindNode(rbt, key);
    if (it == tree->end()) {
        return NULL;
    }
    void* value = it->getVal();
    stringRBT::node* node = it->getSelf();
    tree->erase(it);
    delete(node);
    return value;
}

/**
 * @name  stringRBTsize
 *
 * @brief API function.
 *        It returns the number of entries in the given red-black tree.
 *
 * @param[in] rbt Pointer to a red-black tree
 *
 * @retval size_t The number of entries in `rbt'
 */
size_t
stringRBTsize (void* rbt)
{
    stringRBT::rbt* tree = reinterpret_cast<stringRBT::rbt*>(rbt);
    return tree->size();
}

/**
 * @name  stringRBTwalk
 *
 * @brief API function.
 *        It visits all the entries in the given red-black tree (`rbt')
 *        and calls the given function (`f') with the given parameter (`arg'.)
 *
 * @param[in] rbt Pointer to a red-black tree
 * @param[in] f   Pointer to a function to be called each time
 *                `stringRBTwalk' visits an entry in `rbt'. Function `f'
 *                is called with three parameters. The first parameter
 *                `key' is a pointer to the search key. The second
 *                parameter 'value' is a pointer to the value associated
 *                with `key'. The third parameter `arg' is a pointer whose
 *                value is the same as the third parameter to `stringRBTwalk'.
 * @param[in] arg Pointer to be used as the third parameter for function `f'.
 */
void
stringRBTwalk (void* rbt, stringRBTcb f, void* arg)
{
    stringRBT::rbt* tree = reinterpret_cast<stringRBT::rbt*>(rbt);
    stringRBT::const_iterator it;

    for (it = tree->begin(); it != tree->end(); ++it) {
        (*f)(it->getKey().c_str(), it->getVal(), arg);
    }
}

/**
 * @name  stringRBTfindNode
 *
 * @brief Internal function.
 *        It searches the given red-black tree for the entry
 *        that matchees given key.
 *
 * @param[in] rbt Pointer to a red-black tree
 * @param[in] key Pointer to the search key
 *
 * @retval stringRBT::const_iterator        Matching entry's const_iterator.
 * @retval stringRBT::const_iterator::end() No matching entry.
 */
stringRBT::const_iterator
stringRBTfindNode (void* rbt, const char* key)
{
    std::pair<stringRBT::iterator, bool> rc;
    stringRBT::rbt* tree = reinterpret_cast<stringRBT::rbt*>(rbt);
    stringRBT::node node;
    if (!rbt) {
        return tree->end();
    }
    if (!key) {
        return tree->end();
    }
    node.setKey(key);
    return tree->find(node);
}
