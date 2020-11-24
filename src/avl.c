#include "tupperware/avl.h"

#include <limits.h>

#define AVL_NO_CHANGE (INT_MIN)

void avl_init(struct avl *tree, avl_cmp_f cmp, void *cookie) {
    if (!tree)
        return;
    tree->root = NULL;
    tree->cmp = cmp;
    tree->cookie = cookie;
}

static void avl_clear_helper(struct avl_node *n,
        void (*dtor)(struct avl_node *n, void *cookie), void *cookie) {
    if (!n)
        return;

    avl_clear_helper(n->left, dtor, cookie);
    avl_clear_helper(n->right, dtor, cookie);

    n->left = NULL;
    n->right = NULL;

    dtor(n, cookie);
}

void avl_clear(struct avl *tree,
        void (*dtor)(struct avl_node *n, void *cookie), void *cookie) {
    if (!tree)
        return;
    avl_clear_helper(tree->root, dtor, cookie);
}

static void left_rotate(struct avl_node **r) {
    struct avl_node *rhs = (*r)->right;
    (*r)->right = rhs->left;
    rhs->left = *r;
    *r = rhs;

    rhs->left->balance = -1 - rhs->balance;
    rhs->balance = -rhs->left->balance;
}

static void right_rotate(struct avl_node **r) {
    struct avl_node *lhs = (*r)->left;
    (*r)->left = lhs->right;
    lhs->right = *r;
    *r = lhs;

    lhs->right->balance = 1 - lhs->balance;
    lhs->balance = -lhs->right->balance;
}

static void left_right_rotate(struct avl_node **r) {
    struct avl_node *gc = (*r)->left->right;
    (*r)->left->right = gc->left;
    gc->left = (*r)->left;
    (*r)->left = gc->right;
    gc->right = *r;
    *r = gc;

    gc->left->balance = (gc->balance * (gc->balance - 1)) >> 1;
    gc->right->balance = - (gc->balance * (gc->balance + 1)) >> 1;
    gc->balance = 0;
}

static void right_left_rotate(struct avl_node **r) {
    struct avl_node *gc = (*r)->right->left;
    (*r)->right->left = gc->right;
    gc->right = (*r)->right;
    (*r)->right = gc->left;
    gc->left = *r;
    *r = gc;

    gc->left->balance = (gc->balance * (gc->balance - 1)) >> 1;
    gc->right->balance = - (gc->balance * (gc->balance + 1)) >> 1;
    gc->balance = 0;
}

static int rebalance(struct avl_node **r) {
    switch ((*r)->balance) {
    case -2:
        if ((*r)->right->balance == 1)
            right_left_rotate(r);
        else
            left_rotate(r);
        break;
    case 2:
        if ((*r)->left->balance == -1)
            left_right_rotate(r);
        else
            right_rotate(r);
        break;
    }
    return (*r)->balance;
}

static void avl_detach_node(struct avl_node *n) {
    n->left = NULL;
    n->right = NULL;
    n->balance = 0;
}

static void avl_steal_node(struct avl_node *n1, struct avl_node *n2) {
    n1->left = n2->left;
    n1->right = n2->right;
    n1->balance = n2->balance;
    avl_detach_node(n2);
}

struct insert_parameters {
    avl_cmp_f cmp;
    void *cookie;
    enum { ALLOW_MULTI, KEEP_OLD, REPLACE_OLD, } policy;
};

static int avl_insert_help(struct avl_node **r, struct avl_node *v,
        struct avl_node **inserted, const struct insert_parameters *params) {
    if (*r == NULL) {
        *r = v;
        v->balance = 0;
        v->left = NULL;
        v->right = NULL;
        *inserted = v;
        return 1;
    }

    int c = params->cmp(v, *r, params->cookie);
    if (c == 0 && params->policy != ALLOW_MULTI) {
        *inserted = *r;
        if (params->policy == REPLACE_OLD) {
            avl_steal_node(v, *r);
            *r = v;
        }
        return AVL_NO_CHANGE;
    }

    struct avl_node **target = (c < 0 ? &((*r)->left) : &((*r)->right));
    int dir = (c < 0 ? 1 : -1);

    int tmp = avl_insert_help(target, v, inserted, params);
    if (tmp == 0 || tmp == AVL_NO_CHANGE)
        return tmp;

    (*r)->balance += dir;

    return rebalance(r);
}

bool avl_insert(struct avl *tree,
        struct avl_node *v, struct avl_node **inserted) {
    if (!tree || !v)
        return false;

    struct insert_parameters params = {
        .cmp = tree->cmp,
        .cookie = tree->cookie,
        .policy = KEEP_OLD,
    };
    struct avl_node *inserted_int = NULL;
    int ret = avl_insert_help(&tree->root, v, &inserted_int, &params);

    if (inserted)
        *inserted = inserted_int;

    return ret != AVL_NO_CHANGE;
}

struct avl_node *avl_insert_or_update(struct avl *tree, struct avl_node *v) {
    if (!tree || !v)
        return NULL;

    struct avl_node *previous_node = NULL;
    struct insert_parameters params = {
        .cmp = tree->cmp,
        .cookie = tree->cookie,
        .policy = REPLACE_OLD,
    };

    avl_insert_help(&tree->root, v, &previous_node, &params);

    return (previous_node == v ? NULL : previous_node);
}

void avl_insert_multi(struct avl *tree, struct avl_node *v) {
    if (!tree || !v)
        return;

    struct insert_parameters params = {
        .cmp = tree->cmp,
        .cookie = tree->cookie,
        .policy = ALLOW_MULTI,
    };
    struct avl_node *dummy = NULL;
    avl_insert_help(&tree->root, v, &dummy, &params);
}

static int avl_remove_min(struct avl_node **r, struct avl_node **target) {
    if ((*r)->left == NULL) {
        *target = *r;
        *r = (*target)->right;
        return 1;
    }
    if (avl_remove_min(&((*r)->left), target) == 0)
        return 0;
    (*r)->balance -= 1;
    int bal = rebalance(r);
    return 1 - bal * bal;
}

static int avl_remove_max(struct avl_node **r, struct avl_node **target) {
    if ((*r)->right == NULL) {
        *target = *r;
        *r = (*target)->right;
        return 1;
    }
    if (avl_remove_max(&((*r)->right), target) == 0)
        return 0;
    (*r)->balance -= 1;
    int bal = rebalance(r);
    return 1 - bal * bal;
}

static int avl_remove_helper_found(struct avl_node **r,
        struct avl_node **target) {
    *target = *r;

    if ((*r)->left && (*r)->right) {
        if ((*r)->balance < 0) {
            int b = avl_remove_min(&((*r)->right), r);
            avl_steal_node(*r, *target);
            if (b == 0)
                return 0;
            (*r)->balance = 0;
            return 1;
        } else {
            int b = avl_remove_max(&((*r)->left), r);
            avl_steal_node(*r, *target);
            if (b == 0)
                return 0;
            int bal = ((*r)->balance -= 1);
            return 1 - (bal * bal);
        }
    }

    if ((*r)->left != NULL)
        *r = (*r)->left;
    else
        *r = (*r)->right;

    avl_detach_node(*target);
    return 1;
}

struct remove_parameters {
    avl_cmp_f cmp;
    void *cookie;
    enum { REMOVE_ANY, REMOVE_EXACT, } policy;
};

static int avl_remove_helper(struct avl_node **r, struct avl_node *v,
        struct avl_node **target, const struct remove_parameters *params) {
    if (*r == NULL) {
        *target = NULL;
        return 0;
    }

    if (*r == v) // Implies equality according to `cmp`
        return avl_remove_helper_found(r, target);
    int c = params->cmp(v, *r, params->cookie);
    if (c == 0 && params->policy == REMOVE_ANY)
        return avl_remove_helper_found(r, target);

    if (c < 0) {
        if (avl_remove_helper(&((*r)->left), v, target, params) == 0)
            return 0;
        (*r)->balance -= 1;
    } else {
        if (avl_remove_helper(&((*r)->right), v, target, params) == 0)
            return 0;
        (*r)->balance += 1;
    }
    int bal = rebalance(r);
    return 1 - bal * bal;
}

struct avl_node *avl_remove(struct avl *tree, struct avl_node *v) {
    if (!tree || !v)
        return NULL;

    struct avl_node *target = NULL;
    const struct remove_parameters params = {
        .cmp = tree->cmp,
        .cookie = tree->cookie,
        .policy = REMOVE_ANY,
    };
    avl_remove_helper(&tree->root, v, &target, &params);
    return target;
}

bool avl_remove_at(struct avl *tree, struct avl_node *v) {
    if (!tree || !v)
        return NULL;

    struct avl_node *target = NULL;
    const struct remove_parameters params = {
        .cmp = tree->cmp,
        .cookie = tree->cookie,
        .policy = REMOVE_EXACT,
    };
    avl_remove_helper(&tree->root, v, &target, &params);
    return target == v;
}

static struct avl_node *avl_find_helper(struct avl_node *r,
        const struct avl_node *v, avl_cmp_f cmp, void *cookie) {
    int c = 0;

    while (r && (c = cmp(v, r, cookie))) {
        if (c < 0)
            r = r->left;
        else
            r = r->right;
    }

    return r;
}

struct avl_node *avl_find(const struct avl *tree, const struct avl_node *v) {
    if (!tree || !v)
        return NULL;

    return avl_find_helper(tree->root, v, tree->cmp, tree->cookie);
}

static struct avl_node *avl_lower_bound_helper(struct avl_node *r,
        const struct avl_node *v, avl_cmp_f cmp, void *cookie) {
    if (!r)
        return NULL;

    int c = cmp(v, r, cookie);

    if (c <= 0) {
        struct avl_node *lhs = avl_lower_bound_helper(r->left, v, cmp, cookie);
        return (lhs ? lhs : r);
    }

    return avl_lower_bound_helper(r->right, v, cmp, cookie);
}

struct avl_node *avl_lower_bound(const struct avl *tree,
        const struct avl_node *v) {
    if (!tree || !v)
        return NULL;

    return avl_lower_bound_helper(tree->root, v, tree->cmp, tree->cookie);
}

static struct avl_node *avl_upper_bound_helper(struct avl_node *r,
        const struct avl_node *v, avl_cmp_f cmp, void *cookie) {
    if (!r)
        return NULL;

    int c = cmp(v, r, cookie);

    if (c < 0) {
        struct avl_node *lhs = avl_upper_bound_helper(r->left, v, cmp, cookie);
        return (lhs ? lhs : r);
    }

    return avl_upper_bound_helper(r->right, v, cmp, cookie);
}

struct avl_node *avl_upper_bound(const struct avl *tree,
        const struct avl_node *v) {
    if (!tree || !v)
        return NULL;

    return avl_upper_bound_helper(tree->root, v, tree->cmp, tree->cookie);
}

bool avl_empty(const struct avl *tree) {
    if (!tree || !tree->root)
        return true;
    return false;
}

static size_t avl_size_helper(const struct avl_node *n) {
    if (!n)
        return 0;

    return 1 + avl_size_helper(n->left) + avl_size_helper(n->right);
}

size_t avl_size(const struct avl *tree) {
    if (!tree)
        return 0;

    return avl_size_helper(tree->root);
}

static size_t avl_height_helper(const struct avl_node *n) {
    if (!n)
        return 0;

    size_t lhs = avl_height_helper(n->left);
    size_t rhs = avl_height_helper(n->right);
    return 1 + (lhs > rhs ? lhs : rhs);
}

size_t avl_height(const struct avl *tree) {
    if (!tree)
        return 0;

    return avl_height_helper(tree->root);
}

void avl_merge(struct avl *tree, struct avl *more) {
    if (!tree || !more)
        return;

    struct avl tmp = {
        more->cmp,
        more->cookie,
        NULL,
    };

    while (more->root) {
        struct avl_node *v = more->root;
        avl_remove_at(more, more->root); // Won't fail
        if ((!avl_insert(tree, v, NULL))) // Keep old value
            avl_insert_multi(&tmp, v); // Be conservative when reinserting
    }

    more->root = tmp.root;
}

void avl_update(struct avl *tree, struct avl *more) {
    if (!tree || !more)
        return;

    struct avl tmp = {
        more->cmp,
        more->cookie,
        NULL,
    };

    while (more->root) {
        struct avl_node *v = more->root;
        avl_remove_at(more, more->root); // Won't fail
        if ((v = avl_insert_or_update(tree, v))) // Keep new value
            avl_insert_multi(&tmp, v); // Be conservative when reinserting
    }

    more->root = tmp.root;
}

void avl_merge_all(struct avl *tree, struct avl *more) {
    if (!tree || !more)
        return;

    while (more->root) {
        struct avl_node *v = more->root;
        avl_remove_at(more, more->root); // Won't fail
        avl_insert_multi(tree, v); // Keep all values
    }
}

static void avl_prefix_map_helper(struct avl_node *n,
        avl_map_f map, void *cookie) {
    if (!n)
        return;

    map(n, cookie);
    avl_prefix_map_helper(n->left, map, cookie);
    avl_prefix_map_helper(n->right, map, cookie);
}

void avl_prefix_map(struct avl *tree, avl_map_f map, void *cookie) {
    if (!tree)
        return;
    avl_prefix_map_helper(tree->root, map, cookie);
}


static void avl_infix_map_helper(struct avl_node *n,
        avl_map_f map, void *cookie) {
    if (!n)
        return;

    avl_infix_map_helper(n->left, map, cookie);
    map(n, cookie);
    avl_infix_map_helper(n->right, map, cookie);
}

void avl_infix_map(struct avl *tree, avl_map_f map, void *cookie) {
    if (!tree)
        return;
    avl_infix_map_helper(tree->root, map, cookie);
}

static void avl_postfix_map_helper(struct avl_node *n,
        avl_map_f map, void *cookie) {
    if (!n)
        return;

    avl_postfix_map_helper(n->left, map, cookie);
    avl_postfix_map_helper(n->right, map, cookie);
    map(n, cookie);
}

void avl_postfix_map(struct avl *tree, avl_map_f map, void *cookie) {
    if (!tree)
        return;
    avl_postfix_map_helper(tree->root, map, cookie);
}

struct between_parameters {
    struct avl_node *end;
    avl_map_f map;
    void *cookie;
};

static bool map_between_helper(struct avl_node *cur,
    const struct avl_node *end, avl_map_f map, void *cookie) {
    if (!cur)
        return false;
    if (cur == end)
        return true;
    if (map_between_helper(cur->left, end, map, cookie))
        return true;
    map(cur, cookie);
    return map_between_helper(cur->right, end, map, cookie);
}

static bool map_between_right(struct avl_node *cur,
    const struct avl_node *end, avl_map_f map, void *cookie) {
    if (!cur)
        return false;
    if (cur == end)
        return true;
    map(cur, cookie);
    return map_between_helper(cur->right, end, map, cookie);
}

static struct avl_node *find_parent(const struct avl *tree,
        struct avl_node *cur, const struct avl_node *v) {
    struct avl_node *parent = NULL;

    int c = 0;
    while (cur && (c = tree->cmp(v, cur, tree->cookie))) {
        parent = cur;
        if (c < 0)
            cur = cur->left;
        else
            cur = cur->right;
    }

    if (!cur)
        return NULL;
    if (cur == v)
        return parent;
    if (cur->left == v || cur->right == v)
        return cur; // NOTE: needs better handling
    if ((parent = find_parent(tree, cur->right, v)))
        return parent;
    if ((parent = find_parent(tree, cur->left, v)))
        return parent;

    return NULL;
}

void avl_map_between(const struct avl *tree,
        struct avl_node *inter[2], avl_map_f map, void *cookie) {
    if (!tree || !tree->root)
        return;

    struct avl_node *begin = inter[0];
    const struct avl_node *end = inter[1];
    // FIXME: finding the parent is inefficient
    while (begin && !map_between_right(begin, end, map, cookie)) {
        begin = find_parent(tree, tree->root, begin);
    }
}
