#include "tupperware/vector.h"

#include <stdlib.h>
#include <string.h>

#define VEC_AT(Vec, Ind) ((void *)((char *)(Vec)->arr + ((Vec)->size * (Ind))))

bool vector_init(struct vector *v, size_t size) {
    if (!v || !size)
        return false;

    v->arr = NULL;
    v->size = size;
    v->nmemb = 0;
    v->cap = 0;

    return true;
}

bool vector_with_cap(struct vector *v, size_t size, size_t cap) {
    if (!v || !size)
        return false;

    v->arr = calloc(cap, size);
    if (!v->arr)
        return false;

    v->size = size;
    v->nmemb = 0;
    v->cap = cap;

    return true;
}

void vector_clear(struct vector *v,
        void (*dtor)(void *v, void *cookie), void *cookie) {
    if (!v)
        return;

    if (dtor)
        for (size_t i = 0; i < v->nmemb; ++i)
            dtor(VEC_AT(v, i), cookie);

    v->cap = 0;
    v->size = 0;
    v->nmemb = 0;
    free(v->arr);
    v->arr = NULL;
}

bool vector_reserve(struct vector *v, size_t cap) {
    if (!v)
        return false;
    if (v->cap >= cap)
        return true;

    void *tmp = reallocarray(v->arr, cap, v->size);
    if (!tmp)
        return false;

    v->arr = tmp;
    v->cap = cap;

    return true;
}

size_t vector_length(const struct vector *v) {
    if (!v)
        return 0;
    return v->nmemb;
}

size_t vector_capacity(const struct vector *v) {
    if (!v)
        return 0;
    return v->cap;
}

size_t vector_elem_size(const struct vector *v) {
    if (!v)
        return 0;
    return v->size;
}

bool vector_empty(const struct vector *v) {
    return vector_length(v) == 0;
}

void *vector_at(const struct vector *v, size_t i) {
    if (!v)
        return NULL;
    if (v->nmemb <= i)
        return NULL;

    return VEC_AT(v, i);
}

bool vector_push_back(struct vector *v, void *elem) {
    if (!v)
        return false;

    return vector_insert_at(v, elem, v->nmemb);
}

bool vector_insert_at(struct vector *v, void *elem, size_t i) {
    if (!v || !elem)
        return false;

    if (v->nmemb == v->cap)
        if (!vector_reserve(v, (v->cap ? v->cap * 2 : 2)))
            return false;

    if (v->nmemb < i)
        i = v->nmemb;
    if (i < v->nmemb)
        memmove(VEC_AT(v, i + 1),
                VEC_AT(v, i),
                (v->nmemb - i) * v->size);

    memmove(VEC_AT(v, i), elem, v->size);
    v->nmemb += 1;

    return true;
}

bool vector_pop_back(struct vector *v, void *output) {
    if (!v || !v->nmemb)
        return false;
    return vector_pop_at(v, output, v->nmemb - 1);
}

bool vector_pop_at(struct vector *v, void *output, size_t i) {
    if (!v || !v->nmemb)
        return false;

    v->nmemb -= 1;
    if (i >= v->nmemb)
        i = v->nmemb;

    if (output)
        memmove(output, VEC_AT(v, i), v->size);

    if (i < v->nmemb) {
        memmove(VEC_AT(v, i), VEC_AT(v, i + 1), v->size * (v->nmemb - i));
    }

    return true;
}

bool vector_is_max_heap_helper(struct vector *v,
        size_t n, vector_cmp_f cmp, void *cookie) {
    size_t l = n * 2 + 1;
    size_t r = n * 2 + 2;

    if (l < v->nmemb) {
        if (cmp(VEC_AT(v, n), VEC_AT(v, l), cookie) < 0)
            return false;
        if (!vector_is_max_heap_helper(v, l, cmp, cookie))
            return false;
    }
    if (r < v->nmemb) {
        if (cmp(VEC_AT(v, n), VEC_AT(v, r), cookie) < 0)
            return false;
        if (!vector_is_max_heap_helper(v, r, cmp, cookie))
            return false;
    }

    return true;
}

bool vector_is_max_heap(struct vector *v, vector_cmp_f cmp, void *cookie) {
    if (!v || !v->nmemb)
        return true;

    return vector_is_max_heap_helper(v, 0, cmp, cookie);
}

static void swap_using(struct vector *v, size_t lhs, size_t rhs, void *buffer) {
    memmove(buffer, VEC_AT(v, lhs), v->size);
    memmove(VEC_AT(v, lhs), VEC_AT(v, rhs), v->size);
    memmove(VEC_AT(v, rhs), buffer, v->size);
}

static void sift_down(struct vector *v,
    size_t pos, vector_cmp_f cmp, void *cookie, void *buffer) {
    size_t l = 2 * pos + 1;
    size_t r = 2 * pos + 2;

    size_t max = pos;

    if (l < v->nmemb && cmp(VEC_AT(v, max), VEC_AT(v, l), cookie) < 0)
        max = l;
    if (r < v->nmemb && cmp(VEC_AT(v, max), VEC_AT(v, r), cookie) < 0)
        max = r;
    if (max != pos) {
        swap_using(v, max, pos, buffer);
        sift_down(v, max, cmp, cookie, buffer);
    }
}

static void sift_up(struct vector *v,
        size_t pos, vector_cmp_f cmp, void *cookie, void *buffer) {
    if (!pos)
        return;

    size_t parent = (pos - 1) / 2;

    if (cmp(VEC_AT(v, parent), VEC_AT(v, pos), cookie) < 0) {
        swap_using(v, pos, parent, buffer);
        sift_up(v, parent, cmp, cookie, buffer);
    }
}

bool vector_make_heap(struct vector *v, vector_cmp_f cmp, void *cookie) {
    if (!v || !v->nmemb)
        return false;

    void *buffer = malloc(v->size);
    if (!buffer)
        return false;

    size_t i = v->nmemb / 2;
    do
    {
        sift_down(v, i, cmp, cookie, buffer);
    } while (i-- != 0);

    free(buffer);

    return true;
}

bool vector_push_heap(struct vector *v,
        void *elem, vector_cmp_f cmp, void *cookie) {
    if (!v || !elem)
        return false;

    void *buffer = malloc(v->size);
    if (!buffer)
        return false;

    if (!vector_push_back(v, elem))
        return false;

    sift_up(v, v->nmemb - 1, cmp, cookie, buffer);

    free(buffer);

    return true;
}

bool vector_pop_heap(struct vector *v,
        void *output, vector_cmp_f cmp, void *cookie) {
    if (!v || !v->nmemb)
        return false;

    // The swap would take care of putting it at the end of the array
    if (output && output != VEC_AT(v, v->nmemb - 1))
        memmove(output, VEC_AT(v, 0), v->size);

    if (v->nmemb == 1) {
        v->nmemb -= 1;
        return true;
    }

    void *buffer = malloc(v->size);
    if (!buffer)
        return false;

    swap_using(v, 0, v->nmemb - 1, buffer);
    v->nmemb -= 1;
    sift_down(v, 0, cmp, cookie, buffer);

    free(buffer);

    return true;
}

bool vector_is_sorted(const struct vector *v, vector_cmp_f cmp, void *cookie) {
    if (!v)
        return true;

    for (size_t i = 1; i < v->nmemb; ++i) {
        if (cmp(VEC_AT(v, 0), VEC_AT(v, 1), cookie) > 0)
            return false;
    }

    return true;
}

struct sort_params {
    size_t begin;
    size_t end;
    void *buffer;
};

static void insert_sort_helper(struct vector *v,
        vector_cmp_f cmp, void *cookie, const struct sort_params *params) {
    size_t b = params->begin;
    size_t e = params->end;

    for (size_t i = 1; i < (e - b); ++i) {
        memmove(params->buffer, VEC_AT(v, b + i), v->size);
        size_t j = i;
        for (; j && cmp(VEC_AT(v, b + j - 1), params->buffer, cookie) > 0; --j)
            memmove(VEC_AT(v, b + j), VEC_AT(v, b + j - 1), v->size);
        memmove(VEC_AT(v, b + j), params->buffer, v->size);
    }
}

bool vector_insert_sort(struct vector *v, vector_cmp_f cmp, void *cookie) {
    if (!v || !v->nmemb)
        return true;
    struct sort_params params = {
        .begin = 0,
        .end = v->nmemb,
        .buffer = malloc(v->size),
    };
    if (!params.buffer)
        return false;

    insert_sort_helper(v, cmp, cookie, &params);

    free(params.buffer);

    return true;
}

static void sift_down_between(struct vector *v, size_t pos,
        vector_cmp_f cmp, void *cookie, const struct sort_params *params) {
    size_t b = params->begin;
    size_t e = params->end;

    size_t l = 2 * pos + 1;
    size_t r = 2 * pos + 2;

    size_t max = pos;

    if (l < (e - b) && cmp(VEC_AT(v, b + max), VEC_AT(v, b + l), cookie) < 0)
        max = l;
    if (r < (e - b) && cmp(VEC_AT(v, b + max), VEC_AT(v, b + r), cookie) < 0)
        max = r;
    if (max != pos) {
        swap_using(v, b + max, b + pos, params->buffer);
        sift_down_between(v, max, cmp, cookie, params);
    }
}

static void make_heap_between(struct vector *v,
        vector_cmp_f cmp, void *cookie, const struct sort_params *params) {
    size_t b = params->begin;
    size_t e = params->end;
    if ((e - b) <= 1)
        return;
    size_t i = (e - b) / 2;
    do
    {
        sift_down_between(v, i, cmp, cookie, params);
    } while (i-- != 0);
}

static void pop_heap_between(struct vector *v,
        vector_cmp_f cmp, void *cookie, struct sort_params *params) {
    params->end -= 1;

    if (params->end - params->begin == 0)
        return;

    swap_using(v, params->begin, params->end, params->buffer);
    sift_down_between(v, 0, cmp, cookie, params);
}

static void heap_sort_helper(struct vector *v,
        vector_cmp_f cmp, void *cookie, struct sort_params *params) {
    make_heap_between(v, cmp, cookie, params);

    while (params->end - params->begin) {
        pop_heap_between(v, cmp, cookie, params); // Modifies the end
    }
}

bool vector_heap_sort(struct vector *v, vector_cmp_f cmp, void *cookie) {
    if (!v || !v->nmemb)
        return true;
    struct sort_params params = {
        .begin = 0,
        .end = v->nmemb,
        .buffer = malloc(v->size),
    };
    if (!params.buffer)
        return false;

    heap_sort_helper(v, cmp, cookie, &params);

    free(params.buffer);

    return true;
}

static void merge_to_buf(struct vector *v,
        vector_cmp_f cmp, void *cookie, struct sort_params *params) {
    size_t b = params->begin;
    size_t e = params->end;
    size_t mid = b + (e - b) / 2;

    size_t i = b;
    size_t j = mid;
    for (size_t k = b; k < e; k++) {
        if (i < mid
            && (j >= e || cmp(VEC_AT(v, i), VEC_AT(v, j), cookie) <= 0)) {
            memmove((char *)params->buffer + v->size * k, VEC_AT(v, i), v->size);
            i = i + 1;
        } else {
            memmove((char *)params->buffer + v->size * k, VEC_AT(v, j), v->size);
            j = j + 1;
        }
    }
}

static void merge_sort_helper(struct vector *v,
        vector_cmp_f cmp, void *cookie, struct sort_params *params) {
    if (params->end - params->begin <= 1)
        return;
    size_t b = params->begin;
    size_t e = params->end;
    size_t mid = b + (e - b) / 2;
    void *buf = params->buffer;
    void *arr = v->arr;

    v->arr = buf;
    params->buffer = arr;

    params->begin = mid;
    params->end = e;
    merge_sort_helper(v, cmp, cookie, params);
    params->begin = b;
    params->end = mid;
    merge_sort_helper(v, cmp, cookie, params);

    params->begin = b;
    params->end = e;
    merge_to_buf(v, cmp, cookie, params);

    v->arr = arr;
    params->buffer = buf;
}

bool vector_merge_sort(struct vector *v, vector_cmp_f cmp, void *cookie) {
    if (!v || !v->nmemb)
        return true;
    struct sort_params params = {
        .begin = 0,
        .end = v->nmemb,
        .buffer = calloc(v->nmemb, v->size),
    };
    if (!params.buffer)
        return false;

    memmove(params.buffer, v->arr, v->nmemb * v->size);
    merge_sort_helper(v, cmp, cookie, &params);

    free(params.buffer);

    return true;
}

static size_t pivot_median3_between(struct vector *v,
        vector_cmp_f cmp, void *cookie, struct sort_params *params) {
    size_t b = params->begin;
    size_t e = params->end;
    size_t mid = b + (e - b) / 2;

    if (cmp(VEC_AT(v, b), VEC_AT(v, e - 1), cookie) == -1)
    {
        if (cmp(VEC_AT(v, e - 1), VEC_AT(v, mid), cookie) == -1)
            return e - 1;
        else if (cmp(VEC_AT(v, b), VEC_AT(v, mid), cookie) == -1)
            return mid;
        else
            return b;
    }
    else
    {
        if (cmp(VEC_AT(v, b), VEC_AT(v, mid), cookie) == -1)
            return b;
        else if (cmp(VEC_AT(v, mid), VEC_AT(v, e - 1), cookie) == -1)
            return e - 1;
        else
            return mid;
    }
}

static size_t partition_between(struct vector *v, size_t pivot,
        vector_cmp_f cmp, void *cookie, struct sort_params *params) {
    size_t i = params->begin;
    size_t j = params->end;
    void *p_val = VEC_AT(v, pivot);
    while (1) {
        while (cmp(VEC_AT(v, i), p_val, cookie) < 0) {
            ++i;
        }
        do
        {
            j--;
        } while (cmp(VEC_AT(v, j), p_val, cookie) > 0);

        if (i < j) {
            swap_using(v, i, j, params->buffer);
        }
        else
            return i + (params->begin == i);
    }
}

#define SMALL_THRESHOLD 10
static void intro_sort_helper(struct vector *v, size_t h_max,
        vector_cmp_f cmp, void *cookie, struct sort_params *params) {
    while (params->end - params->begin > SMALL_THRESHOLD) {
        size_t b = params->begin;
        size_t e = params->end;

        if (h_max-- == 0) {
            heap_sort_helper(v, cmp, cookie, params);
            return;
        }
        else {
            size_t p = pivot_median3_between(v, cmp, cookie, params);
            size_t m = partition_between(v, p, cmp, cookie, params);
            if (m <= (e - b) - m) {
                params->end = m;
                intro_sort_helper(v, h_max, cmp, cookie, params);
                params->begin = m;
                params->end = e;
            }
            else {
                params->begin = m;
                intro_sort_helper(v, h_max, cmp, cookie, params);
                params->begin = b;
                params->end = m;
            }
        }
    }

    insert_sort_helper(v, cmp, cookie, params);
}

static size_t log_2(size_t n) {
    size_t l = 0;
    while (n >>= 1)
        ++l;
    return l;
}

bool vector_sort(struct vector *v, vector_cmp_f cmp, void *cookie) {
    if (!v || !v->nmemb)
        return true;
    struct sort_params params = {
        .begin = 0,
        .end = v->nmemb,
        .buffer = calloc(v->nmemb, v->size),
    };
    if (!params.buffer)
        return false;

    size_t h_max = log_2(v->nmemb);
    intro_sort_helper(v, h_max, cmp, cookie, &params);

    free(params.buffer);

    return true;
}
