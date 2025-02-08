
#include "eros.h"
#include <eros_packing.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

eros_package_t *eros_package_new(uint8_t *data, size_t size) {
  
  eros_package_t *package = malloc(sizeof(eros_package_t));
  if (package == NULL) {
    return NULL;
  }

  // Allocate buffer, but leave space for the header at the start
  uint8_t *data_buffer = malloc(size + sizeof(eros_header_t));
  if (data_buffer == NULL) {
    free(data_buffer);
    return NULL;
  }
  data_buffer += sizeof(eros_header_t);


  memcpy(data_buffer, data, size);
  package->data = data_buffer;
  package->size = size;
  package->reference_count = 1;
  return package;
}

eros_package_t *eros_headered_package_new(uint8_t *data_with_header, size_t size) {

  if (size < sizeof(eros_header_t)) {
    return NULL;
  }

  eros_package_t *package = eros_package_new(data_with_header + sizeof(eros_header_t),
                                             size - sizeof(eros_header_t));
  if (package == NULL) {
    return NULL;
  }

  // Copy header
  eros_header_t *header = (eros_header_t *)data_with_header;
  package->type = EROS_PACKAGE_TYPE_ID;
  package->target.destination.id = header->id;
  package->target.destination.realm_id = header->realm_id;

  return package;
}

void eros_package_write_header(eros_package_t *package) {
    eros_header_t *header = (eros_header_t *)package->data - sizeof(eros_header_t);
    header->id = package->target.destination.id;
    header->realm_id = package->target.destination.realm_id;
}

void eros_package_increase_reference(eros_package_t *package) {
  package->reference_count++;
}

void eros_package_decrease_reference(eros_package_t *package) {
  if (package->reference_count > 0) {
    package->reference_count--;
  }
}

void eros_package_delete(eros_package_t *package) {
  eros_package_decrease_reference(package);

  if (package->reference_count == 0) {
    free(package->data - sizeof(eros_header_t));
    free(package);
  }
}
