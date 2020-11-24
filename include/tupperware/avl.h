#ifndef TUPPERWARE_AVL_H
#define TUPPERWARE_AVL_H

#include <stdbool.h>
#include <stddef.h>

struct avl_node {
    struct avl_node *left;
    struct avl_node *right;
    int balance;
};

typedef int (*avl_cmp_f)(const struct avl_node *lhs,
        const struct avl_node *rhs, void *cookie);
typedef void (*avl_map_f)(struct avl_node *n, void *cookie);
typedef bool (*avl_filter_f)(struct avl_node *n, void *cookie);

struct avl {
    avl_cmp_f cmp;
    void *cookie;
    struct avl_node *root;
};

#define CONTAINER_OF(Type, Field, Ptr) \
      ((Type*)((char*)(Ptr) - offsetof(Type, Field)))

#define AVL_NODE_INIT_VAL \
    ((struct avl_node){ \
        .left = NULL, \
        .right = NULL, \
        .balance = 0, \
    })

void avl_init(struct avl *tree, avl_cmp_f cmp, void *cookie);
void avl_clear(struct avl *tree,
        void (*dtor)(struct avl_node *n, void *cookie), void *cookie);

bool avl_insert(struct avl *tree,
        struct avl_node *v, struct avl_node **inserted);
struct avl_node *avl_insert_or_update(struct avl *tree, struct avl_node *v);
void avl_insert_multi(struct avl *tree, struct avl_node *v);

struct avl_node *avl_remove(struct avl *tree, struct avl_node *v);
bool avl_remove_at(struct avl *tree, struct avl_node *v);

struct avl_node *avl_find(const struct avl *tree, const struct avl_node *v);
struct avl_node *avl_lower_bound(const struct avl *tree,
        const struct avl_node *v);
struct avl_node *avl_upper_bound(const struct avl *tree,
        const struct avl_node *v);

bool avl_empty(const struct avl *tree);
size_t avl_size(const struct avl *tree);
size_t avl_height(const struct avl *tree);

void avl_merge(struct avl *tree, struct avl *more);
void avl_update(struct avl *tree, struct avl *more);
void avl_merge_all(struct avl *tree, struct avl *more);

#endif /* !TUPPERWARE_AVL_H */
