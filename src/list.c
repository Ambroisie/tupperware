#include "tupperware/list.h"

void list_init(struct list *list) {
    if (!list)
        return;
    list->head = NULL;
}

void list_clear(struct list *list,
        void (*dtor)(struct list_node *n, void *cookie), void *cookie) {
    if (!list)
        return;

    while (list->head) {
        struct list_node *tmp = list_node_safe_detach(&list->head);
        dtor(tmp, cookie);
    }

    list->head = NULL;
}

void list_node_insert_prev(struct list_node *at, struct list_node *n) {
    if (!at || !n)
        return;

    n->next = at;
    n->prev = at->prev;

    n->next->prev = n;
    n->prev->next = n;
}

void list_node_insert_next(struct list_node *at, struct list_node *n) {
    if (!at || !n)
        return;

    n->next = at->next;
    n->prev = at;

    n->prev->next = n;
    n->next->prev = n;
}

void list_push_back(struct list *list, struct list_node *n) {
    if (!list)
        return;

    if (!list->head) {
        list->head = n;
        n->prev = n;
        n->next = n;
        return;
    }

    list_node_insert_prev(list->head, n);
}

void list_push_front(struct list *list, struct list_node *n) {
    if (!list)
        return;

    if (!list->head) {
        list->head = n;
        n->prev = n;
        n->next = n;
        return;
    }

    list_node_insert_prev(list->head, n);
    list->head = n;
}

struct list_node *list_node_detach(struct list_node *at) {
    if (!at)
        return NULL;

    if (at->next != at) {
        at->next->prev = at->prev;
        at->prev->next = at->next;
    }

    at->next = at;
    at->prev = at;

    return at;
}

struct list_node *list_node_safe_detach(struct list_node **at) {
    if (!at || !*at)
        return NULL;

    struct list_node *tmp = *at;
    *at = (tmp->next == tmp ? NULL : tmp->next);
    return list_node_detach(tmp);
}

struct list_node *list_pop_front(struct list *list) {
    if (!list || !list->head)
        return NULL;

    return list_node_safe_detach(&list->head);
}
struct list_node *list_pop_back(struct list *list) {
    if (!list || !list->head)
        return NULL;

    list->head = list->head->prev;
    return list_pop_front(list);
}

size_t list_length(const struct list *list) {
    if (!list || !list->head)
        return 0;

    size_t len = 0;
    LIST_FOREACH_CONST (*list, it)
        ++len;
    return len;
}

void list_node_concat(struct list_node *first, struct list_node *second) {
    if (!first || !second)
        return;

    struct list_node *tmp = first->prev;

    first->prev->next = second;
    first->prev = second->prev;

    second->prev->next = first;
    second->prev = tmp;
}

void list_concat(struct list *begin, struct list *end) {
    if (!begin || !end || !end->head)
        return;

    if (!begin->head) {
        begin->head = end->head;
        end->head = NULL;
        return;
    }

    list_node_concat(begin->head, end->head);
    end->head = NULL;
}

void list_insert_sorted(struct list *list,
        struct list_node *n, list_cmp_f cmp, void *cookie) {
    if (!list || !n)
        return;

    LIST_FOREACH (*list, it) {
        if (cmp(it, n, cookie) > 0) {
            list_node_insert_prev(it, n);
            if (list->head == it)
                list->head = n; // New smallest value
            return;
        }
    }
    // Insert at the end if larger than all elements
    list_push_back(list, n);
}

void list_sort(struct list *list, list_cmp_f cmp, void *cookie) {
    if (!list || !list->head)
        return;

    struct list_node *sorted = list_node_safe_detach(&list->head);
    struct list_node *tmp = list->head;
    list->head = sorted;

    while (tmp) {
        struct list_node *it = list_node_safe_detach(&tmp);
        list_insert_sorted(list, it, cmp, cookie);
    }
}

void list_merge_sorted(struct list *lhs,
        struct list *rhs, list_cmp_f cmp, void *cookie) {
    if (!lhs || !rhs || !rhs->head)
        return;

    if (!lhs->head) {
        lhs->head = rhs->head;
        rhs->head = NULL;
        return;
    }

    LIST_FOREACH (*lhs, it) {
        // Steal from `rhs` as long as it comes before `it`
        while (rhs->head && cmp(it, rhs->head, cookie) > 0) {
            struct list_node *tmp = list_node_safe_detach(&rhs->head);
            list_node_insert_prev(it, tmp);
            if (lhs->head == it)
                lhs->head = tmp; // New smallest value
        }
    }
    // Append all remaining elements at the end
    list_concat(lhs, rhs);
}

void list_map(struct list *list, list_map_f map, void *cookie) {
    if (!list)
        return;

    LIST_FOREACH (*list, it) {
        map(it, cookie);
    }
}

void list_filter(struct list *res,
        struct list *list, list_filter_f filter, void *cookie) {
    if (!list || !res)
        return;

    while (list->head && filter(list->head, cookie)) {
        struct list_node *tmp = list_node_safe_detach(&list->head);
        list_push_back(res, tmp);
    }

    if (!list->head)
        return;

    struct list_node *cur = list->head->next;
    while (cur != list->head) {
        if (filter(cur, cookie)) {
            struct list_node *tmp = list_node_safe_detach(&cur);
            list_push_back(res, tmp);
            continue;
        }
        cur = cur->next;
    }
}
