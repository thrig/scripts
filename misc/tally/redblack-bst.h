#ifndef _RBBST_TREE_H
#define _RBBST_TREE_H 1

#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#define RBBST_RED true
#define RBBST_BLACK false

typedef char *rbbst_key;           /* a unique input line */
typedef unsigned long rbbst_value; /* tally of times line has been seen */

struct rbbst_node_t {
    rbbst_key line;
    rbbst_value count;
    struct rbbst_node_t *left;
    struct rbbst_node_t *right;
    bool color;
};
typedef struct rbbst_node_t *rbbst_node;

struct tally_t {
    rbbst_value count;
    rbbst_key line;
};
typedef struct tally_t *tally;

/***********************************************************************
 *
 * Public Functions */

void rbbst_add(rbbst_key line, ssize_t linelen);
void rbbst_tally(int showtotal);

#endif
