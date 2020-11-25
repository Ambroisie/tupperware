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

    swap_at_end(v, 0, v->nmemb - 1);
    v->nmemb -= 1;
    sift_down(v, 0, cmp, cookie);

    return true;
}
