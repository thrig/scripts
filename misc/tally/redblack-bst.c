/* red-black binary search tree implmentation based on code in "Algorithms"
 * (4th Edition) by Robert Sedgewick and Kevin Wayne */

#include "redblack-bst.h"

int compare_tally(const void *x, const void *y);
void rbbst_flip_colors(rbbst_node n);
rbbst_node rbbst_insert(rbbst_node n, rbbst_key line, ssize_t linelen);
bool rbbst_is_red(rbbst_node n);
rbbst_node rbbst_newnode(rbbst_key line, ssize_t linelen);
rbbst_node rbbst_rotate_left(rbbst_node n);
rbbst_node rbbst_rotate_right(rbbst_node n);
void rbbst_walk(rbbst_node n, tally *t);

rbbst_node tree = NULL;
size_t line_count, tree_count;

/***********************************************************************
 *
 * Public Functions */

void rbbst_add(rbbst_key line, ssize_t linelen) {
    tree        = rbbst_insert(tree, line, linelen);
    tree->color = RBBST_BLACK;
    line_count++;
}

void rbbst_tally(int showtotal) {
    tally tly, tmp;
    if (!tree) exit(EXIT_SUCCESS);
    if ((tly = calloc(tree_count, sizeof(struct tally_t))) == NULL)
        err(EX_OSERR, "could not calloc tally list");
    tmp = tly;
    rbbst_walk(tree, &tmp);
    if (showtotal)
        fprintf(stderr, "tally: unique %lu of total %lu\n", tree_count,
                line_count);
    qsort(tly, tree_count, sizeof(struct tally_t), compare_tally);
    for (size_t i = 0; i < tree_count; i++) {
        printf("%lu %s", tly->count, tly->line);
        tly++;
    }
}

/***********************************************************************
 *
 * Private Functions */

int compare_tally(const void *x, const void *y) {
    tally a, b;
    a = (tally) x;
    b = (tally) y;
    if (a->count > b->count) {
        return -1;
    } else if (a->count < b->count) {
        return 1;
    }
    return 0;
}

inline void rbbst_flip_colors(rbbst_node n) {
    n->color        = !n->color;
    n->left->color  = !n->left->color;
    n->right->color = !n->right->color;
}

rbbst_node rbbst_insert(rbbst_node n, rbbst_key line, ssize_t linelen) {
    int cmp;
    if (n == NULL) return rbbst_newnode(line, linelen);
    cmp = strncmp(line, n->line, linelen);
    if (cmp < 0) {
        n->left = rbbst_insert(n->left, line, linelen);
    } else if (cmp > 0) {
        n->right = rbbst_insert(n->right, line, linelen);
    } else {
        n->count++;
        return n;
    }
    if (rbbst_is_red(n->right) && !rbbst_is_red(n->left))
        n = rbbst_rotate_left(n);
    if (rbbst_is_red(n->left) && rbbst_is_red(n->left->left))
        n = rbbst_rotate_right(n);
    if (rbbst_is_red(n->left) && rbbst_is_red(n->right)) rbbst_flip_colors(n);
    return n;
}

inline bool rbbst_is_red(rbbst_node n) {
    if (n == NULL) return false;
    return n->color == RBBST_RED;
}

rbbst_node rbbst_newnode(rbbst_key line, ssize_t linelen) {
    rbbst_node n;
    if ((n = calloc((size_t) 1, sizeof(struct rbbst_node_t))) == NULL)
        err(EX_OSERR, "could not calloc node");
    if ((n->line = strndup(line, linelen)) == NULL)
        err(EX_OSERR, "could not strndup");
    n->color = RBBST_RED;
    n->count = 1UL;
    tree_count++;
    return n;
}

inline rbbst_node rbbst_rotate_left(rbbst_node n) {
    rbbst_node x   = n->right;
    n->right       = x->left;
    x->left        = n;
    x->color       = x->left->color;
    x->left->color = RBBST_RED;
    return x;
}

inline rbbst_node rbbst_rotate_right(rbbst_node n) {
    rbbst_node x    = n->left;
    n->left         = x->right;
    x->right        = n;
    x->color        = x->right->color;
    x->right->color = RBBST_RED;
    return x;
}

void rbbst_walk(rbbst_node n, tally *t) {
    if (n->left) rbbst_walk(n->left, t);
    (*t)->count = n->count;
    (*t)->line  = n->line;
    (*t)++;
    if (n->right) rbbst_walk(n->right, t);
}
