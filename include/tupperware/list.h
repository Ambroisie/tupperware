#ifndef TUPPERWARE_LIST_H
#define TUPPERWARE_LIST_H

#include <stdbool.h>
#include <stddef.h>

struct list_node {
    struct list_node *next;
    struct list_node *prev;
};

struct list {
    struct list_node *head;
};

#define CONTAINER_OF(Type, Field, Ptr) \
      ((Type*)((char*)(Ptr) - offsetof(Type, Field)))

#define LIST_FOREACH(List, Cur) \
    for (struct list_node *Cur = (List).head; \
        (Cur); \
        (Cur) = ((Cur)->next == (List).head ? NULL : (Cur)->next))

#define LIST_FOREACH_CONST(List, Cur) \
    for (const struct list_node *Cur = (List).head; \
        (Cur); \
        (Cur) = ((Cur)->next == (List).head ? NULL : (Cur)->next))

#define LIST_FOREACH_ENTRY(Type, Field, List, Cur) \
    for (Type *Cur = (List).head == NULL \
            ? NULL \
            : CONTAINER_OF(Type, Field, (List).head); \
        (Cur); \
        (Cur) = ((Cur)->Field.next == (List).head \
                ? NULL \
                : CONTAINER_OF(Type, Field, (Cur)->Field.next)))

#define LIST_NODE_INIT_VAL \
    ((struct list_node){ \
        .next = NULL, \
        .prev = NULL, \
    })

typedef int (*list_cmp_f)(const struct list_node *lhs,
        const struct list_node *rhs, void *cookie);

typedef bool (*list_filter_f)(struct list_node *n, void *cookie);

typedef void (*list_map_f)(struct list_node *n, void *cookie);

void list_init(struct list *list);
void list_clear(struct list *list,
        void (*dtor)(struct list_node *n, void *cookie), void *cookie);

void list_node_insert_prev(struct list_node *at, struct list_node *n);
void list_node_insert_next(struct list_node *at, struct list_node *n);

void list_push_back(struct list *list, struct list_node *n);
void list_push_front(struct list *list, struct list_node *n);

struct list_node *list_node_detach(struct list_node *at);
struct list_node *list_node_safe_detach(struct list_node **at);

struct list_node *list_pop_front(struct list *list);
struct list_node *list_pop_back(struct list *list);

bool list_empty(const struct list *list);
size_t list_length(const struct list *list);

void list_node_concat(struct list_node *begin, struct list_node *end);
void list_concat(struct list *begin, struct list *end);

void list_insert_sorted(struct list *list,
        struct list_node *n, list_cmp_f cmp, void *cookie);

void list_sort(struct list *list, list_cmp_f cmp, void *cookie);
void list_merge_sorted(struct list *lhs,
        struct list *rhs, list_cmp_f cmp, void *cookie);

void list_map(struct list *list, list_map_f map, void *cookie);
void list_filter(struct list *res,
        struct list *list, list_filter_f filter, void *cookie);

#endif /* !TUPPERWARE_LIST_H */
