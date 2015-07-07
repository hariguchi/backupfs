/* -*- C -*- */

#include <stdio.h>
#include <stdlib.h>
#include "string-rbt.h"

void
walk_cb (const char *key, void* val, void* arg)
{
    int* value = val;
    int* param = arg;

    printf("key: %s, val: %d, param: %d\n", key, *value, *param);
}

int
main (int argc, char *argv[])
{
    const char *key[] = {
        "abc",
        "abcd",
        "abcde",
        "abcdef",
    };
    int val[] = {
        0,
        1,
        2,
        3,
    };
    void *rbt;
    int i;
    int rc;
    int *value;

    rbt = stringRBTcreate();
    if (!rbt) {
        fprintf(stderr, "rbt is NULL\n");
        exit(1);
    }
    for (i = 0; i < 4; ++i) {
        rc = stringRBTinsert(rbt, key[i], &val[i]);
        if (rc < 0) {
            fprintf(stderr, "%d: error\n", i);
        }
        printf("size: %ld\n", stringRBTsize(rbt));
    }
    for (i = 0; i < 4; ++i) {
        value = (int *)stringRBTfind(rbt, key[i]);
        if (value) {
            printf("found: key: %s, val: %d\n", key[i], *value);
        } else {
            fprintf(stderr, "Error: key: %s\n", key[i]);
        }
    }

    value = (int *)stringRBTfind(rbt, "foobar");
    if (value) {
        fprintf(stderr, "Error: shouldn't happen.\n");
    }

    i = 12345678;
    stringRBTwalk(rbt, walk_cb, &i);

    for (i = 0; i < 4; ++i) {
        value = (int *)stringRBTremove(rbt, key[i]);
        if (value) {
            printf("removed: key: %s, val:%d\n", key[i], *value);
        } else {
            fprintf(stderr, "FAILED to remove: key: %s\n", key[i]);
        }
    }
}
