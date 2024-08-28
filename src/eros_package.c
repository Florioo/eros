
#include "eros.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

eros_package_t *eros_package_new(uint8_t *data, size_t size)
{
    eros_package_t *package = malloc(sizeof(eros_package_t));
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
    package->data = data_copy;
    package->size = size;
    package->reference_count = 1;
    return package;
}
void eros_package_increase_reference(eros_package_t *package)
{
    package->reference_count++;
}

void eros_package_decrease_reference(eros_package_t *package)
{
    if (package->reference_count > 0)
    {
        package->reference_count--;
    }
}

void eros_package_delete(eros_package_t *package)
{
    eros_package_decrease_reference(package);
    
    if (package->reference_count == 0)
    {
        free(package->data);
        free(package);
    }
}
