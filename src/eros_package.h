#include <stdint.h>
#include <stdlib.h>

#ifndef EROS_PACKAGE_H
#define EROS_PACKAGE_H

typedef uint8_t eros_realm_id_t;
typedef uint8_t eros_id_type_t;
typedef uint8_t eros_group_t;

typedef struct
{
    eros_id_type_t id;
    eros_realm_id_t realm_id;
} eros_id_t;

typedef enum
{
    EROS_PACKAGE_TYPE_GROUP,
    EROS_PACKAGE_TYPE_ID
} eros_package_type_t;

typedef struct
{
    eros_package_type_t type;

    eros_id_t source;

    union
    {
        eros_id_t destination;
        eros_group_t group;
    } target;

    uint8_t *data;
    size_t size;
    uint8_t reference_count;
} eros_package_t;

eros_package_t *eros_package_new(uint8_t *data, size_t size);
void eros_package_delete(eros_package_t *package);
void eros_package_decrease_reference(eros_package_t *package);
void eros_package_increase_reference(eros_package_t *package);

#endif // EROS_PACKAGE_H