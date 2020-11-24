#include <criterion/criterion.h>

#include "tupperware/avl.h"

TestSuite(avl, .timeout = 15);

struct int_tree {
    int val;
    struct avl_node avl;
};

static int int_tree_cmp(const struct avl_node *lhs,
        const struct avl_node *rhs, void *cookie) {
    struct int_tree *l = CONTAINER_OF(struct int_tree, avl, lhs);
    struct int_tree *r = CONTAINER_OF(struct int_tree, avl, rhs);

    size_t *count = cookie;
    ++*count;

    if (l->val < r->val)
        return -1;
    return (l->val > r->val);
}

Test(avl, init_null) {
    avl_init(NULL, NULL, NULL);
}

Test(avl, init) {
    struct avl tree = { (avl_cmp_f)0x42, (void *)0x42, (void *)0x42, };
    size_t count = 0;
    avl_init(&tree, int_tree_cmp, &count);

    cr_assert_eq(tree.cmp, int_tree_cmp);
    cr_assert_eq(tree.cookie, &count);
    cr_assert_null(tree.root);
}

static void int_tree_dtor(struct avl_node *node, void *cookie) {
    size_t *count = cookie;

    struct int_tree *t = CONTAINER_OF(struct int_tree, avl, node);
    *count |= 1 << t->val;
}

static struct avl init_avl(struct avl_node *r, size_t *count) {
    return (struct avl){
        .cmp = int_tree_cmp,
        .cookie = count,
        .root = r,
    };
}

Test(avl, clear_null) {
    size_t count = 0;
    avl_clear(NULL, int_tree_dtor, &count);
    cr_assert_eq(count, 0);

    struct avl tree = init_avl(NULL, NULL);
    avl_clear(&tree, int_tree_dtor, &count);

    cr_assert_eq(count, 0);
}

Test(avl, clear_one) {
    size_t count = 0;
    struct int_tree t = { 12, AVL_NODE_INIT_VAL };
    struct avl tree = init_avl(&t.avl, NULL);

    avl_clear(&tree, int_tree_dtor, &count);
    cr_assert_eq(count, 1 << 12);
}

static void init_int_tree_arr(struct int_tree *arr, size_t n, int offset) {
    for (size_t i = 0; i < n; ++i) {
        arr[i].avl = AVL_NODE_INIT_VAL;
        arr[i].val = i + offset;
    }
}

static struct avl init_int_tree_avl(size_t *cookie,
        struct int_tree *arr, size_t n) {
    struct avl tree = init_avl(NULL, cookie);
    init_int_tree_arr(arr, n, 0);

    for (size_t i = 0; i < n; ++i) {
        struct avl_node *inserted = NULL;
        cr_assert(avl_insert(&tree, &arr[i].avl, &inserted));
        cr_assert_eq(&arr[i].avl, inserted);
    }

    return tree;
}

#define ARR_SIZE(Arr) (sizeof(Arr) / sizeof(*Arr))

Test(avl, clear) {
    struct int_tree arr[5];
    size_t count_inserts = 0;
    struct avl tree = init_int_tree_avl(&count_inserts, arr, ARR_SIZE(arr));

    size_t count = 0;
    avl_clear(&tree, int_tree_dtor, &count);
    cr_assert_eq(count, ((1 << 5) - 1)); // First  5 bits set
}

Test(avl, insert_null) {
    avl_insert(NULL, NULL, NULL);

    size_t count = 0;
    struct avl tree = init_avl(NULL, &count);
    avl_insert(&tree, NULL, NULL);
    cr_assert_eq(count, 0);

    tree.root = (void *)0x42;
    avl_insert(&tree, NULL, NULL);
    cr_assert_eq(count, 0);
}

Test(avl, insert_one) {
    size_t count = 0;
    struct avl tree = init_avl(NULL, &count);

    struct avl_node *target = NULL;
    struct int_tree n = { 42, { NULL, NULL, 0 } };
    avl_insert(&tree, &n.avl, &target);

    cr_assert_eq(count, 0);
    cr_assert_eq(tree.root, &n.avl);
    cr_assert_eq(target, &n.avl);
}

Test(avl, insert_preexisting) {
    size_t count = 0;
    struct int_tree n = { 42, { NULL, NULL, 0 } };
    struct avl tree = init_avl(&n.avl, &count);

    struct avl_node *target = NULL;
    struct int_tree n2 = { 42, { NULL, NULL, 0 } };
    avl_insert(&tree, &n2.avl, &target);

    cr_assert_eq(count, 1);
    cr_assert_eq(tree.root, &n.avl);
    cr_assert_eq(target, &n.avl);
}


static int assert_balance_helper(struct avl_node *r) {
    if (r == NULL)
        return -1;

    cr_assert_geq(r->balance, -1);
    cr_assert_leq(r->balance, 1);

    int h1 = assert_balance_helper(r->left);
    int h2 = assert_balance_helper(r->right);

    cr_assert_eq(r->balance, h1 - h2);

    return 1 + (h1 > h2 ? h1 : h2);
}

static void assert_balance(struct avl *tree) {
    if (!tree)
        return;

    assert_balance_helper(tree->root);
}

Test(avl, insert) {
    size_t count = 0;
    struct avl tree = init_avl(NULL, &count);
    struct int_tree arr[5];
    init_int_tree_arr(arr, ARR_SIZE(arr), 0);

    for (size_t i = 0; i < ARR_SIZE(arr); ++i) {
        struct avl_node *target = NULL;
        cr_assert(avl_insert(&tree, &arr[i].avl, &target));
        cr_assert_eq(target, &arr[i].avl);
        assert_balance(&tree);
    }
}

Test(avl, insert_twice) {
    size_t count = 0;
    struct avl tree = init_avl(NULL, &count);
    struct int_tree arr[5];
    init_int_tree_arr(arr, ARR_SIZE(arr), 0);

    for (size_t i = 0; i < ARR_SIZE(arr); ++i) {
        struct avl_node *target = NULL;
        cr_assert(avl_insert(&tree, &arr[i].avl, &target));
        cr_assert_eq(target, &arr[i].avl);
    }

    for (size_t i = 0; i < ARR_SIZE(arr); ++i) {
        struct avl_node *target = NULL;
        cr_assert(!avl_insert(&tree, &arr[i].avl, &target));
        cr_assert_eq(target, &arr[i].avl);
        assert_balance(&tree);
    }
}

Test(avl, insert_or_update) {
    size_t count = 0;
    struct int_tree n = { 42, { NULL, NULL, 0 } };
    struct avl tree = init_avl(NULL, &count);
    cr_assert_null(avl_insert_or_update(&tree, &n.avl));
    cr_assert_eq(count, 0);

    struct int_tree n2 = { 42, { NULL, NULL, 0 } };
    struct avl_node *old = avl_insert_or_update(&tree, &n2.avl);

    cr_assert_eq(count, 1);
    cr_assert_eq(old, &n.avl);
    cr_assert_eq(tree.root, &n2.avl);
}

Test(avl, insert_multi) {
    size_t count = 0;
    struct int_tree n = { 42, { NULL, NULL, 0 } };
    struct avl tree = init_avl(NULL, &count);
    avl_insert_multi(&tree, &n.avl);
    cr_assert_eq(count, 0);

    struct int_tree n2 = { 42, { NULL, NULL, 0 } };
    avl_insert_multi(&tree, &n2.avl);

    cr_assert_eq(count, 1);
    cr_assert_eq(tree.root, &n.avl);
    cr_assert_null(tree.root->left);
    cr_assert_eq(tree.root->right, &n2.avl);
}

Test(avl, remove_null) {
    avl_remove(NULL, NULL);

    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    struct avl tree = init_avl(&t.avl, NULL);

    avl_remove(&tree, NULL);
    avl_remove(NULL, &t.avl);
}

Test(avl, remove) {
    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    size_t count = 0;
    struct avl tree = init_avl(&t.avl, &count);

    cr_assert_eq(avl_remove(&tree, &t.avl), &t.avl);
    cr_assert_eq(count, 0);
    cr_assert_null(tree.root);
}

Test(avl, remove_cmp) {
    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    struct int_tree t2 = { 42, AVL_NODE_INIT_VAL };
    size_t count = 0;
    struct avl tree = init_avl(&t.avl, &count);

    cr_assert_eq(avl_remove(&tree, &t2.avl), &t.avl);
    cr_assert_eq(count, 1);
    cr_assert_null(tree.root);
}

Test(avl, remove_twice) {
    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    size_t count = 0;
    struct avl tree = init_avl(&t.avl, &count);

    cr_assert_eq(avl_remove(&tree, &t.avl), &t.avl);
    cr_assert_null(tree.root);
    cr_assert_null(avl_remove(&tree, &t.avl));
}

Test(avl, remove_at_null) {
    avl_remove(NULL, NULL);

    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    struct avl tree = init_avl(&t.avl, NULL);

    avl_remove_at(&tree, NULL);
    avl_remove_at(NULL, &t.avl);
}

Test(avl, remove_at) {
    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    struct int_tree t2 = { 42, AVL_NODE_INIT_VAL };
    size_t count = 0;
    struct avl tree = init_avl(&t.avl, &count);

    cr_assert_not(avl_remove_at(&tree, &t2.avl));
    cr_assert_eq(count, 1);
    cr_assert_eq(tree.root, &t.avl);

    cr_assert(avl_remove_at(&tree, &t.avl));
    cr_assert_eq(count, 1);
    cr_assert_null(tree.root);
}

Test(avl, remove_at_multi) {
    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    struct int_tree t2 = { 42, AVL_NODE_INIT_VAL };
    t2.avl.right = &t.avl;
    t2.avl.balance = -1;
    size_t count = 0;
    struct avl tree = init_avl(&t2.avl, &count);

    cr_assert(avl_remove_at(&tree, &t.avl));
    cr_assert_eq(count, 1);
    cr_assert_eq(tree.root, &t2.avl);

    cr_assert(avl_remove_at(&tree, &t2.avl));
    cr_assert_eq(count, 1);
    cr_assert_null(tree.root);
}

Test(avl, remove_at_multi_root_first) {
    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    struct int_tree t2 = { 42, AVL_NODE_INIT_VAL };
    t2.avl.right = &t.avl;
    t2.avl.balance = -1;
    size_t count = 0;
    struct avl tree = init_avl(&t2.avl, &count);

    cr_assert(avl_remove_at(&tree, &t2.avl));
    cr_assert_eq(count, 0);
    cr_assert_eq(tree.root, &t.avl);

    cr_assert(avl_remove_at(&tree, &t.avl));
    cr_assert_eq(count, 0);
    cr_assert_null(tree.root);
}

Test(avl, find_null) {
    cr_assert_null(avl_find(NULL, NULL));

    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    struct avl tree = init_avl(&t.avl, NULL);

    cr_assert_null(avl_find(&tree, NULL));
    cr_assert_null(avl_find(NULL, &t.avl));
}

Test(avl, find_none) {
    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    size_t count = 0;
    struct avl tree = init_avl(NULL, &count);

    cr_assert_null(avl_find(&tree, &t.avl));
}

Test(avl, find_one) {
    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    size_t count = 0;
    struct avl tree = init_avl(&t.avl, &count);

    cr_assert_eq(avl_find(&tree, &t.avl), &t.avl);
}

Test(avl, find_same) {
    struct int_tree arr[5];
    size_t count = 0;
    struct avl tree = init_int_tree_avl(&count, arr, ARR_SIZE(arr));

    for (size_t i = 0; i < ARR_SIZE(arr); ++i) {
        cr_assert_eq(avl_find(&tree, &arr[i].avl), &arr[i].avl);
    }
}

Test(avl, find) {
    struct int_tree arr[5];
    size_t count = 0;
    struct avl tree = init_int_tree_avl(&count, arr, ARR_SIZE(arr));

    for (size_t i = 0; i < ARR_SIZE(arr); ++i) {
        struct int_tree t = { i, AVL_NODE_INIT_VAL };
        cr_assert_eq(avl_find(&tree, &t.avl), &arr[i].avl);
    }
}

Test(avl, lower_bound_null) {
    cr_assert_null(avl_lower_bound(NULL, NULL));

    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    struct avl tree = init_avl(&t.avl, NULL);

    cr_assert_null(avl_lower_bound(&tree, NULL));
    cr_assert_null(avl_lower_bound(NULL, &t.avl));
}

Test(avl, lower_bound_none) {
    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    size_t count = 0;
    struct avl tree = init_avl(NULL, &count);

    cr_assert_null(avl_lower_bound(&tree, &t.avl));
}

Test(avl, lower_bound_one_lower) {
    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    struct int_tree t2 = { 41, AVL_NODE_INIT_VAL };
    size_t count = 0;
    struct avl tree = init_avl(&t.avl, &count);

    cr_assert_eq(avl_lower_bound(&tree, &t2.avl), &t.avl);
}

Test(avl, lower_bound_one_same) {
    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    struct int_tree t2 = { 42, AVL_NODE_INIT_VAL };
    size_t count = 0;
    struct avl tree = init_avl(&t.avl, &count);

    cr_assert_eq(avl_lower_bound(&tree, &t2.avl), &t.avl);
}

Test(avl, lower_bound_one_higher) {
    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    struct int_tree t2 = { 43, AVL_NODE_INIT_VAL };
    size_t count = 0;
    struct avl tree = init_avl(&t.avl, &count);

    cr_assert_null(avl_lower_bound(&tree, &t2.avl));
}

Test(avl, lower_bound) {
    struct int_tree arr[5];
    size_t count = 0;
    struct avl tree = init_int_tree_avl(&count, arr, ARR_SIZE(arr));

    struct int_tree t = { -1, AVL_NODE_INIT_VAL };
    cr_assert_eq(avl_lower_bound(&tree, &t.avl), &arr[0].avl);
    for (size_t i = 0; i < ARR_SIZE(arr); ++i) {
        t.val += 1;
        cr_assert_eq(avl_lower_bound(&tree, &t.avl), &arr[i].avl);
    }
    t.val += 1;
    cr_assert_null(avl_lower_bound(&tree, &t.avl));
}

Test(avl, upper_bound_null) {
    cr_assert_null(avl_upper_bound(NULL, NULL));

    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    struct avl tree = init_avl(&t.avl, NULL);

    cr_assert_null(avl_upper_bound(&tree, NULL));
    cr_assert_null(avl_upper_bound(NULL, &t.avl));
}

Test(avl, upper_bound_none) {
    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    size_t count = 0;
    struct avl tree = init_avl(NULL, &count);

    cr_assert_null(avl_upper_bound(&tree, &t.avl));
}

Test(avl, upper_bound_one_upper) {
    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    struct int_tree t2 = { 41, AVL_NODE_INIT_VAL };
    size_t count = 0;
    struct avl tree = init_avl(&t.avl, &count);

    cr_assert_eq(avl_upper_bound(&tree, &t2.avl), &t.avl);
}

Test(avl, upper_bound_one_same) {
    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    struct int_tree t2 = { 42, AVL_NODE_INIT_VAL };
    size_t count = 0;
    struct avl tree = init_avl(&t.avl, &count);

    cr_assert_null(avl_upper_bound(&tree, &t2.avl));
}

Test(avl, upper_bound_one_higher) {
    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    struct int_tree t2 = { 43, AVL_NODE_INIT_VAL };
    size_t count = 0;
    struct avl tree = init_avl(&t.avl, &count);

    cr_assert_null(avl_upper_bound(&tree, &t2.avl));
}

Test(avl, upper_bound) {
    struct int_tree arr[5];
    size_t count = 0;
    struct avl tree = init_int_tree_avl(&count, arr, ARR_SIZE(arr));

    struct int_tree t = { -1, AVL_NODE_INIT_VAL };
    cr_assert_eq(avl_upper_bound(&tree, &t.avl), &arr[0].avl);
    for (size_t i = 1; i < ARR_SIZE(arr); ++i) {
        t.val += 1;
        cr_assert_eq(avl_upper_bound(&tree, &t.avl), &arr[i].avl);
    }
    t.val += 1;
    cr_assert_null(avl_upper_bound(&tree, &t.avl));
}

Test(avl, empty_null) {
    cr_assert(avl_empty(NULL));
}

Test(avl, empty_none) {
    struct avl tree = init_avl(NULL, NULL);
    cr_assert(avl_empty(&tree));
}

Test(avl, empty_one) {
    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    struct avl tree = init_avl(&t.avl, NULL);
    cr_assert_not(avl_empty(&tree));
}

Test(avl, size_null) {
    cr_assert_eq(avl_size(NULL), 0);
}

Test(avl, size_none) {
    struct avl tree = init_avl(NULL, NULL);

    cr_assert_eq(avl_size(&tree), 0);
}

Test(avl, size_one) {
    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    struct avl tree = init_avl(&t.avl, NULL);

    cr_assert_eq(avl_size(&tree), 1);
}

Test(avl, size) {
    struct int_tree arr[5];
    size_t count = 0;
    struct avl tree = init_int_tree_avl(&count, arr, ARR_SIZE(arr));

    cr_assert_eq(avl_size(&tree), ARR_SIZE(arr));
}

Test(avl, height_null) {
    cr_assert_eq(avl_height(NULL), 0);
}

Test(avl, height_none) {
    struct avl tree = init_avl(NULL, NULL);

    cr_assert_eq(avl_height(&tree), 0);
}

Test(avl, height_one) {
    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    struct avl tree = init_avl(&t.avl, NULL);

    cr_assert_eq(avl_height(&tree), 1);
}

Test(avl, height_left) {
    struct int_tree t2 = { 42, AVL_NODE_INIT_VAL };
    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    struct avl tree = init_avl(&t.avl, NULL);
    t.avl.left = &t2.avl;

    cr_assert_eq(avl_height(&tree), 2);
}

Test(avl, height_right) {
    struct int_tree t2 = { 42, AVL_NODE_INIT_VAL };
    struct int_tree t = { 42, AVL_NODE_INIT_VAL };
    struct avl tree = init_avl(&t.avl, NULL);
    t.avl.right = &t2.avl;

    cr_assert_eq(avl_height(&tree), 2);
}
