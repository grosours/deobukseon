#define DBKS_PERSISTENT_VECTOR_NODE_STRUCT(type)                     \
    struct dbks_persistent_vector_node_ ## type ## _s;          \
    typedef struct dbks_persistent_vector_node_ ## type ## _s   \
    dbks_persistent_vector_node_ ## type;                       \
    struct dbks_persistent_vector_node_ ## type ## _s           \
    {                                                           \
    long retain_count;                                          \
    union {                                                     \
        dbks_persistent_vector_node_ ## type  *nodes[32];       \
        type values[32];                                        \
    } _;                                                        \
    };                                                          \
    dbks_persistent_vector_node_ ## type *                               \
    dbks_persistent_vector_node_ ## type ## _new();                          \
    dbks_persistent_vector_node_ ## type  *                             \
    dbks_persistent_vector_node_ ## type ## _copy(dbks_persistent_vector_node_ ## type *node);
#define __children(node) ((node)->_.nodes)
#define __values(node) ((node)->_.values)

#define DBKS_PERSISTENT_VECTOR_NODE_IMPL(type)                          \
    dbks_persistent_vector_node_ ## type  *                             \
    dbks_persistent_vector_node_ ## _new()                              \
    {                                                                   \
        dbks_persistent_vector_node_ ## type * node = malloc(sizeof(*node)); \
        *node = (dbks_persistent_vector_node_ ## type){};           \
        return node;                                                \
    }                                                                   \
                                                                        \
    dbks_persistent_vector_node_ ## type *                                       \
    dbks_persistent_vector_node_ ## type ## _copy(dbks_persistent_vector_node_ ## type  *node) \
    {                                                                   \
        dbks_persistent_vector_node_ ## type *copy = malloc(sizeof(*copy));      \
        if (node != NULL) {                                             \
            copy = memcpy(copy, node, sizeof(*copy));                   \
            copy->retain_count = 0;                                     \
        }                                                               \
        else {                                                          \
            bzero(copy, sizeof(*copy));                                 \
        }                                                               \
        return copy;                                                    \
    }
