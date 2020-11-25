#ifndef TUPPERWARE_VECTOR_H
#define TUPPERWARE_VECTOR_H

#include <stdbool.h>
#include <stddef.h>

struct vector {
    void *arr;
    size_t size;
    size_t nmemb;
    size_t cap;
};

bool vector_init(struct vector *v, size_t size);
bool vector_with_cap(struct vector *v, size_t size, size_t cap);
void vector_clear(struct vector *v,
        void (*dtor)(void *v, void *cookie), void *cookie);

bool vector_reserve(struct vector *v, size_t cap);

size_t vector_length(const struct vector *v);
size_t vector_capacity(const struct vector *v);
size_t vector_elem_size(const struct vector *v);
bool vector_empty(const struct vector *v);

void *vector_at(const struct vector *v, size_t i);

bool vector_push_back(struct vector *v, void *elem);
bool vector_insert_at(struct vector *v, void *elem, size_t i);

bool vector_pop_back(struct vector *v, void *output);
bool vector_pop_at(struct vector *v, void *output, size_t i);

#endif /* !TUPPERWARE_VECTOR_H */
