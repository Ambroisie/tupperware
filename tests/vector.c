#include <criterion/criterion.h>

#include "tupperware/vector.h"

struct vector v;
const size_t init_n = 10;

static void setup(void) {

    v = (struct vector){
        .arr = calloc(init_n, sizeof(int)),
        .nmemb = 0,
        .size = sizeof(int),
        .cap = init_n,
    };
}

static void teardown(void) {
    free(v.arr);
}

TestSuite(vector, .timeout = 15, .init = setup, .fini = teardown);

Test(vector, init_null) {
    cr_assert_not(vector_init(NULL, 1));
}

Test(vector, init_zero) {
    struct vector vec;
    cr_assert_not(vector_init(&vec, 0));
}

Test(vector, init) {
    struct vector vec;

    cr_assert(vector_init(&vec, 1));

    cr_assert_eq(vec.size, 1);
    cr_assert_eq(vec.cap, 0);
    cr_assert_eq(vec.nmemb, 0);
    cr_assert_null(vec.arr);
}

Test(vector, with_cap_null) {
    cr_assert_not(vector_with_cap(NULL, 1, 1));
}

Test(vector, with_cap_zero_size) {
    struct vector vec;
    cr_assert_not(vector_with_cap(&vec, 0, 1));
}

Test(vector, with_cap) {
    struct vector vec;

    cr_assert(vector_with_cap(&vec, 1, 1));

    cr_assert_eq(vec.size, 1);
    cr_assert_eq(vec.cap, 1);
    cr_assert_eq(vec.nmemb, 0);
    cr_assert_not_null(vec.arr);

    free(vec.arr);
}

Test(vector, with_cap_overflow) {
    struct vector vec;

    cr_assert_not(vector_with_cap(&vec, -1, -1));
}

static void int_dtor(void *val, void *cookie) {
    int *n = val;
    int *count = cookie;

    cr_assert_eq(*n, (*count)++);
}

Test(vector, clear_null) {
    int count = 0;

    vector_clear(NULL, int_dtor, &count);

    cr_assert_eq(count, 0);
}

static void fill_v(void) {
    int *arr = v.arr;
    for (size_t i = 0; i < v.cap; ++i) {
        arr[v.nmemb++] = i;
    }
}

Test(vector, clear_no_dtor) {
    fill_v();

    vector_clear(&v, NULL, NULL);

    cr_assert_null(v.arr);
    cr_assert_eq(v.size, 0);
    cr_assert_eq(v.cap, 0);
    cr_assert_eq(v.nmemb, 0);
}

Test(vector, clear) {
    fill_v();
    int count = 0;

    vector_clear(&v, int_dtor, &count);

    cr_assert_eq(count, init_n);
}

Test(vector, reserve_null) {
    cr_assert_not(vector_reserve(NULL, 0));
}

Test(vector, reserve_zero) {
    void *prev_arr = v.arr;
    size_t prev_cap = v.cap;

    cr_assert(vector_reserve(&v, 0));

    cr_assert_eq(v.arr, prev_arr);
    cr_assert_eq(v.cap, prev_cap);
}

Test(vector, reserve_less_than_cap) {
    void *prev_arr = v.arr;
    size_t prev_cap = v.cap;

    cr_assert(vector_reserve(&v, init_n - 1));

    cr_assert_eq(v.arr, prev_arr);
    cr_assert_eq(v.cap, prev_cap);
}

Test(vector, reserve_cap) {
    void *prev_arr = v.arr;
    size_t prev_cap = v.cap;

    cr_assert(vector_reserve(&v, init_n));

    cr_assert_eq(v.arr, prev_arr);
    cr_assert_eq(v.cap, prev_cap);
}

Test(vector, reserve_overflow) {
    void *prev_arr = v.arr;
    size_t prev_cap = v.cap;

    cr_assert_not(vector_reserve(&v, -1));

    cr_assert_eq(v.arr, prev_arr);
    cr_assert_eq(v.cap, prev_cap);
}

Test(vector, reserve) {
    cr_assert(vector_reserve(&v, 2 * init_n));

    cr_assert_eq(v.cap, 2 * init_n);
}

Test(vector, length_null) {
    cr_assert_eq(vector_length(NULL), 0);
}

Test(vector, length_empty) {
    cr_assert_eq(vector_length(&v), 0);
}

Test(vector, length) {
    fill_v();
    cr_assert_eq(vector_length(&v), init_n);
}

Test(vector, capacity_null) {
    cr_assert_eq(vector_capacity(NULL), 0);
}

Test(vector, capacity_empty) {
    cr_assert_eq(vector_capacity(&v), init_n);
}

Test(vector, capacity) {
    fill_v();
    cr_assert_eq(vector_capacity(&v), init_n);
}

Test(vector, elem_size_null) {
    cr_assert_eq(vector_elem_size(NULL), 0);
}

Test(vector, elem_size) {
    fill_v();
    cr_assert_eq(vector_elem_size(&v), sizeof(int));
}

Test(vector, empty_null) {
    cr_assert(vector_empty(NULL));
}

Test(vector, empty_empty) {
    cr_assert(vector_empty(&v));
}

Test(vector, empty) {
    fill_v();
    cr_assert_not(vector_empty(&v));
}

Test(vector, at_null) {
    cr_assert_null(vector_at(NULL, 0));
}

Test(vector, at_empty) {
    cr_assert_null(vector_at(&v, 0));
}

Test(vector, at_out_of_range) {
    fill_v();

    cr_assert_null(vector_at(&v, -1));
}

Test(vector, at) {
    fill_v();

    int *arr = v.arr;
    for (size_t i = 0; i < init_n; ++i)
        cr_assert_eq(vector_at(&v, i), &arr[i]);
}

Test(vector, push_back_null) {
    cr_assert_not(vector_push_back(NULL, NULL));

    cr_assert_not(vector_push_back(&v, NULL));

    int n = 42;
    cr_assert_not(vector_push_back(NULL, &n));
}

Test(vector, push_back_empty) {
    int n = 42;
    cr_assert(vector_push_back(&v, &n));

    int *varr = v.arr;
    cr_assert_eq(varr[0], 42);
    cr_assert_eq(v.nmemb, 1);
}

Test(vector, push_back) {
    int n = 42;
    fill_v();

    cr_assert(vector_push_back(&v, &n));

    int *varr = v.arr;
    cr_assert_eq(varr[init_n], 42);
    cr_assert_eq(v.nmemb, init_n + 1);
}

Test(vector, insert_at_null) {
    cr_assert_not(vector_insert_at(NULL, NULL, 0));

    cr_assert_not(vector_insert_at(&v, NULL, 0));

    int n = 42;
    cr_assert_not(vector_insert_at(NULL, &n, 0));
}

Test(vector, insert_at_empty_out_of_bounds) {
    int n = 42;
    cr_assert(vector_insert_at(&v, &n, -1));

    int *varr = v.arr;
    cr_assert_eq(varr[0], 42);
    cr_assert_eq(v.nmemb, 1);
}

Test(vector, insert_at_empty) {
    int n = 42;
    cr_assert(vector_insert_at(&v, &n, 0));

    int *varr = v.arr;
    cr_assert_eq(varr[0], 42);
    cr_assert_eq(v.nmemb, 1);
}

Test(vector, insert_at_out_of_bounds) {
    fill_v();
    int n = 42;
    cr_assert(vector_insert_at(&v, &n, -1));

    int *varr = v.arr;
    cr_assert_eq(varr[init_n], 42);
    cr_assert_eq(v.nmemb, init_n + 1);

    for (size_t i = 0; i < init_n; ++i)
        cr_assert_eq(varr[i], i);
}

Test(vector, insert_at_begin) {
    fill_v();
    int n = 42;
    cr_assert(vector_insert_at(&v, &n, 0));

    int *varr = v.arr;
    cr_assert_eq(varr[0], 42);
    cr_assert_eq(v.nmemb, init_n + 1);

    for (size_t i = 0; i < init_n; ++i)
        cr_assert_eq(varr[i + 1], i);
}

Test(vector, insert_at) {
    fill_v();
    int n = 42;
    cr_assert(vector_insert_at(&v, &n, 2));

    int *varr = v.arr;
    cr_assert_eq(varr[2], 42);
    cr_assert_eq(v.nmemb, init_n + 1);

    for (size_t i = 0; i < init_n; ++i)
        cr_assert_eq(varr[i + (i >= 2)], i);
}

Test(vector, pop_back_null) {
    cr_assert_not(vector_pop_back(NULL, NULL));
}

Test(vector, pop_back_empty) {
    cr_assert_not(vector_pop_back(&v, NULL));
}

Test(vector, pop_back_null_output) {
    fill_v();
    cr_assert(vector_pop_back(&v, NULL));

    cr_assert_eq(v.nmemb, init_n - 1);
}

Test(vector, pop_back_once) {
    fill_v();
    int res = 42;
    cr_assert(vector_pop_back(&v, &res));

    cr_assert_eq(res, init_n - 1);
    cr_assert_eq(v.nmemb, init_n - 1);
}

Test(vector, pop_back) {
    fill_v();
    int res = 42;
    for (size_t i = 0; i < init_n; ++i) {
        cr_assert(vector_pop_back(&v, &res));
        cr_assert_eq(res, init_n - i - 1);
    }

    cr_assert_not(vector_pop_back(&v, &res));
    cr_assert_eq(v.nmemb, 0);
}

Test(vector, pop_at_null) {
    cr_assert_not(vector_pop_at(NULL, NULL, 0));
}

Test(vector, pop_at_empty) {
    cr_assert_not(vector_pop_at(&v, NULL, 0));
}

Test(vector, pop_at_null_output_begin) {
    fill_v();
    cr_assert(vector_pop_at(&v, NULL, 0));

    cr_assert_eq(v.nmemb, init_n - 1);
    int *varr = v.arr;
    for (size_t i = 0; i < (init_n - 1); ++i) {
        cr_assert_eq(varr[i], i + 1);
    }
}

Test(vector, pop_at_once_begin) {
    fill_v();
    int res = 42;
    cr_assert(vector_pop_at(&v, &res, 0));

    cr_assert_eq(res, 0);
    cr_assert_eq(v.nmemb, init_n - 1);

    int *varr = v.arr;
    for (size_t i = 0; i < (init_n - 1); ++i) {
        cr_assert_eq(varr[i], i + 1);
    }
}

Test(vector, pop_at_once_middle) {
    fill_v();
    int res = 42;
    cr_assert(vector_pop_at(&v, &res, 2));

    cr_assert_eq(res, 2);
    cr_assert_eq(v.nmemb, init_n - 1);

    int *varr = v.arr;
    for (size_t i = 0; i < (init_n - 1); ++i) {
        cr_assert_eq(varr[i], i + (i >= 2));
    }
}

Test(vector, pop_at) {
    fill_v();
    int res = 42;
    for (size_t i = 0; i < init_n; ++i) {
        cr_assert(vector_pop_at(&v, &res, 0));
        cr_assert_eq(res, i);
    }

    cr_assert_not(vector_pop_at(&v, &res, 0));
    cr_assert_eq(v.nmemb, 0);
}
