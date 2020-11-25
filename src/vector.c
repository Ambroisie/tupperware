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
