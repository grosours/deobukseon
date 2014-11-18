// #include "array.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define SWAP(v,op ) (do {                                       \
            dbks_persistent_vector __swap_vector = (op);        \
            dbks_persistent_vector_release(v);                  \
            (v) = __swap_vector;                                \
} while(0))

static char * const dbks_persistent_vector_not_found = "not found";

#define __children(node) ((node)->_.nodes)
#define __values(node) ((node)->_.values)

struct dbks_persistent_vector_node_s;
typedef struct dbks_persistent_vector_node_s dbks_persistent_vector_node;
struct dbks_persistent_vector_node_s
{
    long retain_count;
    union {
        dbks_persistent_vector_node *nodes[32];
        int values[32];
    } _;
};

dbks_persistent_vector_node *
dbks_persistent_vector_node_new();
dbks_persistent_vector_node *
dbks_persistent_vector_node_copy(dbks_persistent_vector_node *node);

dbks_persistent_vector_node *
dbks_persistent_vector_node_new()
{
    dbks_persistent_vector_node * node = malloc(sizeof(*node));
    *node = (dbks_persistent_vector_node){};
    return node;
}

dbks_persistent_vector_node *
dbks_persistent_vector_node_copy(dbks_persistent_vector_node *node)
{
    dbks_persistent_vector_node *copy = malloc(sizeof(*copy));
    if (node != NULL) {
      copy = memcpy(copy, node, sizeof(*copy));
      copy->retain_count = 0;
    }
    else {
      bzero(copy, sizeof(*copy));
    }
    return copy;
}

typedef struct dbks_persistent_vector_s {
    int cnt;
    int shift;
    dbks_persistent_vector_node * root;
    dbks_persistent_vector_node * tail;
} dbks_persistent_vector;

#define dbks_persistent_vector_new(...)                                 \
    dbks_persistent_vector_retain(((dbks_persistent_vector)             \
        {.cnt = 0, .shift = 5, .root = NULL, .tail = NULL, __VA_ARGS__ }))

dbks_persistent_vector
dbks_persistent_vector_empty();

int
dbks_persistent_vector_tail_index(dbks_persistent_vector vector);

dbks_persistent_vector_node *
dbks_persistent_vector_array_for_index(
    dbks_persistent_vector vector,
    int const index);

dbks_persistent_vector_node*
dbks_persistent_vector_create_path(
    int level,
    dbks_persistent_vector_node * node);

dbks_persistent_vector_node*
dbks_persistent_vector_push_tail(
    dbks_persistent_vector vector,
    int level,
    dbks_persistent_vector_node *parent,
    dbks_persistent_vector_node *tail);

dbks_persistent_vector
dbks_persistent_vector_cons(dbks_persistent_vector vector, int value);

dbks_persistent_vector
dbks_persistent_vector_assoc_n(
    dbks_persistent_vector vector,
    int value, int index);

dbks_persistent_vector_node *
dbks_persistent_vector_node_retain(dbks_persistent_vector_node *node, int level);

dbks_persistent_vector_node *
dbks_persistent_vector_node_release(dbks_persistent_vector_node *node, int level);

dbks_persistent_vector
dbks_persistent_vector_retain(dbks_persistent_vector vector);

dbks_persistent_vector
dbks_persistent_vector_release(dbks_persistent_vector vector);


dbks_persistent_vector
dbks_persistent_vector_empty()
{
    return dbks_persistent_vector_new();
}

int
dbks_persistent_vector_tail_index(dbks_persistent_vector vector)
{
    if (vector.cnt < 32) {
        return 0;
    }
    else {
        return ((vector.cnt -1) >> 5) << 5;
    }
}

dbks_persistent_vector_node *
dbks_persistent_vector_array_for_index(dbks_persistent_vector vector, int const index)
{
    if (index >= 0 && index < vector.cnt) {
        if (index >= dbks_persistent_vector_tail_index(vector)) {
            return vector.tail;
        }
        else {
            dbks_persistent_vector_node * node = vector.root;
            for (int level = vector.shift ; level > 0 ; level -= 5) {
                node = (dbks_persistent_vector_node*) __children(node)[(index >> level) & 0x01f];
            }
            return node;
        }
    }
    else {
        return (dbks_persistent_vector_node*)dbks_persistent_vector_not_found;
    }
}

dbks_persistent_vector_node *
dbks_persistent_vector_do_assoc(int level, dbks_persistent_vector_node *node, int index, int value)
{
    dbks_persistent_vector_node *new_node = dbks_persistent_vector_node_copy(node);
    if (level == 0) {
        __values(new_node)[index & 0x1f] = value;
    }
    else {
        int subindex = (((unsigned) index) >> level) & 0x01f;
        __children(new_node)[subindex] = dbks_persistent_vector_do_assoc(
            level -5, __children(node)[subindex], index, value);
    }
    return new_node;
}

dbks_persistent_vector_node*
dbks_persistent_vector_create_path(int level, dbks_persistent_vector_node * node)
{
    if (level == 0) {
        return node;
    }
    else {
        dbks_persistent_vector_node *new_node = dbks_persistent_vector_node_new();
        __children(new_node)[0] = dbks_persistent_vector_create_path(level-5, node);
        return new_node;
    }
}

dbks_persistent_vector_node*
dbks_persistent_vector_push_tail(
    dbks_persistent_vector vector,
    int level,
    dbks_persistent_vector_node *parent,
    dbks_persistent_vector_node *tail)
{
    int subidx = (((unsigned) vector.cnt -1) >> level) & 0x01f;
    dbks_persistent_vector_node * new_node = dbks_persistent_vector_node_copy(parent);
    dbks_persistent_vector_node * node_to_insert = NULL;
    if (level == 5) {
        node_to_insert = tail;
    }
    else {
        dbks_persistent_vector_node *child = __children(parent)[subidx];
        node_to_insert = (child != NULL) ?
            dbks_persistent_vector_push_tail(vector, level -5, child, tail)
            :
            dbks_persistent_vector_create_path(level -5, tail);
    }
    __children(new_node)[subidx] = node_to_insert;
    return new_node;
}

dbks_persistent_vector
dbks_persistent_vector_cons(dbks_persistent_vector vector, int value)
{
    int index = vector.cnt;

    if (vector.cnt - dbks_persistent_vector_tail_index(vector) < 32) {
        dbks_persistent_vector_node *new_tail = dbks_persistent_vector_node_copy(vector.tail);
        __values(new_tail)[index & 0x01f] = value;
        return dbks_persistent_vector_new(
            .cnt = vector.cnt +1,
            .shift = vector.shift,
            .root = vector.root,
            .tail = new_tail);
    }

     dbks_persistent_vector_node *new_root = NULL;
    int new_shift = vector.shift;

    if ((((unsigned) vector.cnt) >> 5) > (1 << vector.shift)) {
        new_root = dbks_persistent_vector_node_new();
        __children(new_root)[0] = vector.root;
        __children(new_root)[1] = dbks_persistent_vector_create_path(vector.shift, vector.tail);
        new_shift += 5;
    }
    else {
        new_root = dbks_persistent_vector_push_tail(
            vector,
            vector.shift,
            vector.root,
            vector.tail);
    }
    dbks_persistent_vector_node *new_tail = dbks_persistent_vector_node_copy(NULL);
    __values(new_tail)[0] = value;
    return dbks_persistent_vector_new(
        .cnt = vector.cnt +1,
        .shift = new_shift,
        .root = new_root,
        .tail = new_tail);
}

dbks_persistent_vector
dbks_persistent_vector_assoc_n(dbks_persistent_vector vector, int value, int index)
{
    if (index >= 0 && index < vector.cnt) {
        if (index >= dbks_persistent_vector_tail_index(vector)) {
            dbks_persistent_vector_node * new_tail = dbks_persistent_vector_node_copy(vector.tail);
            __values(new_tail)[index & 0x01f] = value;

            return dbks_persistent_vector_new(.cnt = vector.cnt,
                                              .shift = vector.shift,
                                              .root = vector.root,
                                              .tail = new_tail);
        }
        else {
            return dbks_persistent_vector_new(
                .cnt = vector.cnt,
                .shift = vector.shift,
                .root = dbks_persistent_vector_do_assoc(
                    vector.shift,
                    vector.root,
                    index,
                    value),
                .tail = vector.tail);
        }
    }
    if (index == vector.cnt) {
        return dbks_persistent_vector_cons(vector, value);
    }

    return dbks_persistent_vector_empty();
}

dbks_persistent_vector_node *
dbks_persistent_vector_node_retain(dbks_persistent_vector_node *node, int level)
{
    if (node == NULL) {
        return NULL;
    }

    if (level == 5) {
        node->retain_count += 1;
    }
    else {
        for (int i = 0 ; i < 32 && __children(node)[i] != NULL ; ++i) {
            __children(node)[i] = dbks_persistent_vector_node_retain(
                __children(node)[i], level - 5);
        }
        node->retain_count += 1;
    }
    return node;
}

dbks_persistent_vector_node *
dbks_persistent_vector_node_release(dbks_persistent_vector_node *node, int level)
{
    if (node == NULL) {
        return NULL;
    }

    if (level == 5) {
        node->retain_count -= 1;
        if (node->retain_count < 1) {
            free(node);
            node = NULL;
        }
    }
    else {
        for (int i = 0 ; i < 32 && __children(node)[i] != NULL ; ++i) {
            __children(node)[i] = dbks_persistent_vector_node_release(
                __children(node)[i], level - 5);
        }
        node->retain_count -= 1;
        if (node->retain_count < 1) {
            free(node);
            node = NULL;
        }
    }
    return node;
}

dbks_persistent_vector
dbks_persistent_vector_retain(dbks_persistent_vector vector)
{
    dbks_persistent_vector_node_retain(vector.tail, 5);
    dbks_persistent_vector_node_retain(vector.root, vector.shift);
    return vector;
}

dbks_persistent_vector
dbks_persistent_vector_release(dbks_persistent_vector vector)
{
    dbks_persistent_vector_node_release(vector.tail, 5);
    dbks_persistent_vector_node_release(vector.root, vector.shift);
    return vector;
}

int
main()
{
    const unsigned count = 1058;
    dbks_persistent_vector vectors[count];

    vectors[0] = dbks_persistent_vector_empty();
    for (unsigned i = 1 ; i < count ; ++i) {
      vectors[i] = dbks_persistent_vector_cons(vectors[i-1], i);
      // dbks_persistent_vector_release(v);
      // v = v2;
    }

    for (unsigned i = 0 ; i  < count ; ++i) {
        dbks_persistent_vector_release(vectors[i]);
    }
    return 0;
}
