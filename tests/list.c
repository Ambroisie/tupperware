#include <criterion/criterion.h>

#include "tupperware/list.h"

TestSuite(list, .timeout = 15);

struct int_list {
    int val;
    struct list_node list;
};

Test(list, init_null) {
    list_init(NULL);
}

Test(list, init) {
    struct list l = { (void *)0x42 };
    list_init(&l);

    cr_assert_null(l.head);
}

static void int_list_dtor(struct list_node *list, void *cookie) {
    size_t *count = cookie;

    struct int_list *l = CONTAINER_OF(struct int_list, list, list);
    cr_assert_eq(l->val, ++*count);
}

Test(list, clear_null) {
    size_t count = 0;

    list_clear(NULL, int_list_dtor, &count);
    cr_assert_eq(count, 0);

    struct list l = { NULL };
    list_clear(&l, int_list_dtor, &count);


    cr_assert_eq(count, 0);
}

Test(list, clear_one) {
    struct int_list list = { 1, { &list.list, &list.list } };
    struct list l = { &list.list };

    size_t count = 0;
    list_clear(&l, int_list_dtor, &count);

    cr_assert_eq(count, 1);
    cr_assert_null(l.head);
}

static void init_list_arr(struct int_list *list_arr, size_t n, int offset) {
    for (size_t i = 0; i < n; ++i) {
        list_arr[i].val = i + 1 + offset;
        list_arr[i].list.next = &list_arr[(i + 1) % n].list;
        list_arr[(i + 1) % n].list.prev = &list_arr[i].list;
    }
}

static void assert_list(struct int_list *list_arr, size_t n, int offset) {
    for (size_t i = 0; i < n; ++i) {
        cr_assert_eq(list_arr[i]. val, i + 1 + offset);
        cr_assert_eq(list_arr[i].list.next, &list_arr[(i + 1) % n].list);
        cr_assert_eq(list_arr[(i + 1) % n].list.prev, &list_arr[i % n].list);
    }
}

#define ARR_SIZE(Arr) (sizeof(Arr) / sizeof(*Arr))

Test(list, clear_multiples) {
    struct int_list arr[10];
    init_list_arr(arr, ARR_SIZE(arr), 0);
    struct list l = { &arr[0].list };

    size_t count = 0;
    list_clear(&l, int_list_dtor, &count);

    cr_assert_eq(count, ARR_SIZE(arr));
    cr_assert_null(l.head);
}

Test(list, node_insert_prev_null) {
    list_node_insert_prev(NULL, NULL);

    struct list_node l = { NULL, NULL };
    list_node_insert_prev(&l, NULL);
    list_node_insert_prev(NULL, &l);
}

Test(list, node_insert_prev_one) {
    struct int_list arr[] = {
        { 1, { NULL, NULL, }, },
        { 2, { NULL, NULL, }, },
    };
    init_list_arr(arr + 1, ARR_SIZE(arr) - 1, 1);

    list_node_insert_prev(&arr[1].list, &arr[0].list);

    assert_list(arr, ARR_SIZE(arr), 0);
}

Test(list, node_insert_prev) {
    struct int_list arr[] = {
        { 1, { NULL, NULL, }, },
        { 2, { NULL, NULL, }, },
        { 3, { NULL, NULL, }, },
    };
    init_list_arr(arr + 1, ARR_SIZE(arr) - 1, 1);

    list_node_insert_prev(&arr[1].list, &arr[0].list);

    assert_list(arr, ARR_SIZE(arr), 0);
}

Test(list, node_insert_next_null) {
    list_node_insert_next(NULL, NULL);

    struct list_node l;
    list_node_insert_next(&l, NULL);
    list_node_insert_next(NULL, &l);
}

Test(list, node_insert_next_one) {
    struct int_list arr[] = {
        { 1, { NULL, NULL, }, },
        { 2, { NULL, NULL, }, },
    };
    init_list_arr(arr, ARR_SIZE(arr) - 1, 0);

    list_node_insert_next(
            &arr[ARR_SIZE(arr) - 2].list, &arr[ARR_SIZE(arr) - 1].list);

    assert_list(arr, ARR_SIZE(arr), 0);
}

Test(list, node_insert_next) {
    struct int_list arr[] = {
        { 1, { NULL, NULL, }, },
        { 2, { NULL, NULL, }, },
        { 3, { NULL, NULL, }, },
    };
    init_list_arr(arr, ARR_SIZE(arr) - 1, 0);

    list_node_insert_next(
            &arr[ARR_SIZE(arr) - 2].list, &arr[ARR_SIZE(arr) - 1].list);

    assert_list(arr, ARR_SIZE(arr), 0);
}

Test(list, node_detach_null) {
    cr_assert_null(list_node_detach(NULL));
}

Test(list, node_detach_one) {
    struct int_list l = { 0, { &l.list, &l.list } };

    struct list_node *n = list_node_detach(&l.list);

    cr_assert_eq(n, &l.list);
    cr_assert_eq(n->next, n);
    cr_assert_eq(n->prev, n);
}

Test(list, node_detach) {
    struct int_list arr[3];
    init_list_arr(arr, ARR_SIZE(arr), 0);

    struct list_node *n = list_node_detach(&arr[0].list);

    cr_assert_eq(n, &arr[0].list);
    cr_assert_eq(n->next, n);
    cr_assert_eq(n->prev, n);

    assert_list(arr + 1, ARR_SIZE(arr) - 1, 1);
}

Test(list, node_detach_back) {
    struct int_list arr[3];
    init_list_arr(arr, ARR_SIZE(arr), 0);

    struct list_node *n = list_node_detach(&arr[2].list);

    cr_assert_eq(n, &arr[2].list);
    cr_assert_eq(n->next, n);
    cr_assert_eq(n->prev, n);

    assert_list(arr, ARR_SIZE(arr) - 1, 0);
}

Test(list, node_safe_detach_null) {
    cr_assert_null(list_node_safe_detach(NULL));
}

Test(list, node_safe_detach_null_pointed) {
    struct list_node *n = NULL;
    cr_assert_null(list_node_safe_detach(&n));
}

Test(list, node_safe_detach_one) {
    struct int_list l = { 0, { &l.list, &l.list } };
    struct list_node *n = &l.list;

    cr_assert_eq(list_node_safe_detach(&n), &l.list);
    cr_assert_null(n);
}

Test(list, node_safe_detach) {
    struct int_list arr[3];
    init_list_arr(arr, ARR_SIZE(arr), 0);
    struct list_node *n = &arr[0].list;

    cr_assert_eq(list_node_safe_detach(&n), &arr[0].list);
    cr_assert_eq(n, &arr[1].list);

    assert_list(arr + 1, ARR_SIZE(arr) - 1, 1);
}

Test(list, pop_front_null) {
    cr_assert_null(list_pop_front(NULL));

    struct list l = { NULL };
    cr_assert_null(list_pop_front(&l));
}

Test(list, pop_front_one) {
    struct int_list list = { 1, { &list.list, &list.list } };
    struct list l = { &list.list };

    cr_assert_eq(list_pop_front(&l), &list.list);
    cr_assert_null(l.head);
}

Test(list, pop_front) {
    struct int_list arr[5];
    init_list_arr(arr, ARR_SIZE(arr), 0);
    struct list l = { &arr[0].list };

    cr_assert_eq(list_pop_front(&l), &arr[0].list);
    assert_list(arr + 1, ARR_SIZE(arr) - 1, 1);
}

Test(list, pop_back_null) {
    cr_assert_null(list_pop_front(NULL));

    struct list l = { NULL };
    cr_assert_null(list_pop_front(&l));
}

Test(list, pop_back_one) {
    struct int_list list = { 1, { &list.list, &list.list } };
    struct list l = { &list.list };

    cr_assert_eq(list_pop_back(&l), &list.list);
    cr_assert_null(l.head);
}

Test(list, pop_back) {
    struct int_list arr[5];
    init_list_arr(arr, ARR_SIZE(arr), 0);
    struct list l = { &arr[0].list };

    cr_assert_eq(list_pop_back(&l), &arr[ARR_SIZE(arr) - 1].list);
    assert_list(arr, ARR_SIZE(arr) - 1, 0);
}

Test(list, length_null) {
    cr_assert_eq(list_length(NULL), 0);
}

Test(list, length_one) {
    struct int_list l = { 0, { &l.list, &l.list } };

    cr_assert_eq(list_length(&(struct list){ &l.list }), 1);
}

Test(list, length) {
    struct int_list arr[3];
    init_list_arr(arr, ARR_SIZE(arr), 0);

    cr_assert_eq(list_length(&(struct list) { &arr[0].list }), ARR_SIZE(arr));
}

Test(list, node_concat_null) {
    list_node_concat(NULL, NULL);

    struct int_list l = { 0, {&l.list, &l.list, } };
    list_node_concat(&l.list, NULL);
    list_node_concat(NULL, &l.list);
}

Test(list, node_concat_one) {
    const size_t n = 1;
    struct int_list arr[n * 2];
    init_list_arr(arr, n * 2 - n, 0);
    init_list_arr(arr + n, n * 2 - n, n);

    list_node_concat(&arr[0].list, &arr[n].list);

    assert_list(arr, n * 2, 0);
}

Test(list, node_concat) {
    const size_t n = 3;
    struct int_list arr[n * 2];
    init_list_arr(arr, n * 2 - n, 0);
    init_list_arr(arr + n, n * 2 - n, n);

    list_node_concat(&arr[0].list, &arr[n].list);

    assert_list(arr, n * 2, 0);
}

static int int_list_cmp(const struct list_node *lhs,
        const struct list_node *rhs, void *cookie) {
    struct int_list *l = CONTAINER_OF(struct int_list, list, lhs);
    struct int_list *r = CONTAINER_OF(struct int_list, list, rhs);

    size_t *count = cookie;
    ++*count;

    if (l->val < r->val)
        return -1;
    return (l->val > r->val);
}

Test(list, insert_sorted_null) {
    list_insert_sorted(NULL, NULL, int_list_cmp, NULL);

    struct list l = { (void *)0x42 };
    list_insert_sorted(&l, NULL, int_list_cmp, NULL);
    list_insert_sorted(NULL, l.head, int_list_cmp, NULL);
}

Test(list, insert_sorted_empty) {
    struct int_list list = { 0, {NULL, NULL } };
    struct list l = { NULL };

    list_insert_sorted(&l, &list.list, int_list_cmp, NULL);

    cr_assert_eq(l.head, &list.list);
    cr_assert_eq(list.list.next, &list.list);
    cr_assert_eq(list.list.prev, &list.list);
}

Test(list, insert_sorted_one_before) {
    struct int_list arr[5] = {
        { 1, { NULL, NULL } },
    };
    init_list_arr(arr + 1, ARR_SIZE(arr) - 1, 1);
    struct list l = { &arr[1].list };

    size_t count = 0;
    list_insert_sorted(&l, &arr[0].list, int_list_cmp, &count);

    assert_list(arr, ARR_SIZE(arr), 0);
    cr_assert_eq(count, 1);
    cr_assert_eq(l.head, &arr[0].list);
}

Test(list, insert_sorted_one_after) {
    struct int_list arr[5];
    init_list_arr(arr, ARR_SIZE(arr) - 1, 0);
    arr[ARR_SIZE(arr) - 1].val = ARR_SIZE(arr);

    struct list l = { &arr[0].list };

    size_t count = 0;
    list_insert_sorted(&l, &arr[ARR_SIZE(arr) - 1].list, int_list_cmp, &count);

    assert_list(arr, ARR_SIZE(arr), 0);
    cr_assert_eq(count, ARR_SIZE(arr) - 1);
}

Test(list, sort_null) {
    size_t count = 0;
    list_sort(NULL, int_list_cmp, &count);

    cr_assert_eq(count, 0);
}


Test(list, sort_one) {
    struct int_list list = { 0, { &list.list, &list.list } };
    struct list l = { &list.list };

    size_t count = 0;
    list_sort(&l, int_list_cmp, &count);

    cr_assert_eq(count, 0);
}

Test(list, sort_sorted) {
    struct int_list arr[5];
    init_list_arr(arr, ARR_SIZE(arr), 0);
    struct list l = { &arr[0].list };

    size_t count = 0;
    list_sort(&l, int_list_cmp, &count);

    cr_assert_eq(l.head, &arr[0].list);
    assert_list(arr, ARR_SIZE(arr), 0);
    cr_assert_eq(count, ARR_SIZE(arr) * (ARR_SIZE(arr) - 1) / 2);
}

Test(list, sort_inverted) {
    struct int_list arr[5] = {
        { 1, { &arr[4].list, &arr[1].list } },
        { 2, { &arr[0].list, &arr[2].list } },
        { 3, { &arr[1].list, &arr[3].list } },
        { 4, { &arr[2].list, &arr[4].list } },
        { 5, { &arr[3].list, &arr[0].list } },
    };
    struct list l = { &arr[ARR_SIZE(arr) - 1].list };

    size_t count = 0;
    list_sort(&l, int_list_cmp, &count);

    cr_assert_eq(l.head, &arr[0].list);
    assert_list(arr, ARR_SIZE(arr), 0);
}

Test(list, merge_sorted_null) {
    list_merge_sorted(NULL, NULL, int_list_cmp, NULL);

    struct list l = { (void *)0x42 };
    list_merge_sorted(&l, NULL, int_list_cmp, NULL);
    list_merge_sorted(NULL, &l, int_list_cmp, NULL);
}

Test(list, merge_sorted) {
    struct int_list arr[5] = {
        { 1, { &arr[2].list, &arr[4].list } },
        { 2, { &arr[3].list, &arr[3].list } },
        { 3, { &arr[4].list, &arr[0].list } },
        { 4, { &arr[1].list, &arr[1].list } },
        { 5, { &arr[0].list, &arr[2].list } },
    };

    struct list odd = { &arr[0].list };
    struct list even = { &arr[1].list };

    size_t count = 0;
    list_merge_sorted(&odd, &even, int_list_cmp, &count);

    assert_list(arr, ARR_SIZE(arr), 0);
    cr_assert_eq(odd.head, &arr[0].list);
    cr_assert_null(even.head);
}

Test(list, merge_sorted_alternate) {
    struct int_list arr[5] = {
        { 1, { &arr[2].list, &arr[4].list } },
        { 2, { &arr[3].list, &arr[3].list } },
        { 3, { &arr[4].list, &arr[0].list } },
        { 4, { &arr[1].list, &arr[1].list } },
        { 5, { &arr[0].list, &arr[2].list } },
    };

    struct list odd = { &arr[0].list };
    struct list even = { &arr[1].list };

    size_t count = 0;
    list_merge_sorted(&even, &odd, int_list_cmp, &count);

    assert_list(arr, ARR_SIZE(arr), 0);
    cr_assert_eq(even.head, &arr[0].list);
    cr_assert_null(odd.head);
}

static void int_list_map(struct list_node *n, void *cookie) {
    size_t *count = cookie;

    struct int_list *node = CONTAINER_OF(struct int_list, list, n);
    node->val = ++*count;
}

Test(list, map_null) {
    size_t count = 0;
    list_map(NULL, int_list_map, &count);
    cr_assert_eq(count, 0);

    struct list l = { NULL };
    list_map(&l, int_list_map, &count);
    cr_assert_eq(count, 0);
}

Test(list, map) {
    struct int_list arr[5] = {
        { 0, { &arr[1].list, &arr[4].list } },
        { 0, { &arr[2].list, &arr[0].list } },
        { 0, { &arr[3].list, &arr[1].list } },
        { 0, { &arr[4].list, &arr[2].list } },
        { 0, { &arr[0].list, &arr[3].list } },
    };
    struct list l = { &arr[0].list };

    size_t count = 0;
    list_map(&l, int_list_map, &count);

    cr_assert_eq(count, ARR_SIZE(arr));
    assert_list(arr, ARR_SIZE(arr), 0);
}

static bool int_list_is_even(struct list_node *n, void *cookie) {
    size_t *count = cookie;
    ++*count;

    struct int_list *node = CONTAINER_OF(struct int_list, list, n);
    return node->val % 2 == 0;
}

Test(list, filter_null) {
    size_t count = 0;
    list_filter(NULL, NULL, int_list_is_even, &count);
    cr_assert_eq(count, 0);

    struct list l = { (void *)0x42 };
    list_filter(&l, NULL, int_list_is_even, &count);
    cr_assert_eq(count, 0);
    list_filter(NULL, &l, int_list_is_even, &count);
    cr_assert_eq(count, 0);
    l.head = NULL;
    list_filter(&l, &l, int_list_is_even, &count);
    cr_assert_eq(count, 0);
}

Test(list, filter) {
    struct int_list arr[5];
    init_list_arr(arr, ARR_SIZE(arr), 0);
    struct list l = { &arr[0].list };
    struct list res = { NULL };

    size_t count = 0;
    list_filter(&res, &l, int_list_is_even, &count);

    cr_assert_eq(count, ARR_SIZE(arr));

    size_t count_even = 0;
    LIST_FOREACH_CONST(res, it) {
        struct int_list *n = CONTAINER_OF(struct int_list, list, it);
        cr_assert_eq(n->val, ++count_even * 2);
    }
    size_t count_odd = 0;
    LIST_FOREACH_CONST(l, it) {
        struct int_list *n = CONTAINER_OF(struct int_list, list, it);
        cr_assert_eq(n->val, ++count_odd * 2 - 1);
    }
    cr_assert_eq(count_even + count_odd, ARR_SIZE(arr));
}
