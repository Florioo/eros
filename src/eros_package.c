
#include "eros.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

eros_package_reference_counted_t *eros_alloc_package(uint8_t *data, size_t size)
{
    printf("Allocating package\n");
    eros_package_reference_counted_t *package = malloc(sizeof(eros_package_reference_counted_t));
    if (package == NULL)
    {
        return NULL;
    }
    uint8_t *data_copy = malloc(size);
    if (data_copy == NULL)
    {
        free(data_copy);
        return NULL;
    }

    memcpy(data_copy, data, size);
    package->package.data = data_copy;
    package->package.size = size;
    package->reference_count = 1;
    return package;
}

void eros_free_package(eros_package_reference_counted_t *package)
{
    if (package->reference_count > 0)
    {
        package->reference_count--;
    }

    if (package->reference_count == 0)
    {
        free(package->package.data);
        free(package);
        printf("Freeing package\n");
    }
}
