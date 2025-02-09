
#include "eros.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Created a new eros package
 *
 * @param data Data to be copied into the package
 * @param size Size of the data
 * @return eros_package_t* Pointer to the new package, this package must be
 * freed with eros_package_delete
 */

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

  memcpy(data_buffer + sizeof(eros_header_t), data, size);
  package->data = data_buffer + sizeof(eros_header_t);
  package->size = size;
  package->reference_count = 1;

  return package;
}

eros_package_t *eros_headered_package_new(uint8_t *data_with_header,
                                          size_t size) {

  if (size < sizeof(eros_header_t)) {
    return NULL;
  }

  eros_package_t *package = eros_package_new(
      data_with_header + sizeof(eros_header_t), size - sizeof(eros_header_t));
  if (package == NULL) {
    return NULL;
  }
  package->type = EROS_PACKAGE_TYPE_ID;

  // Copy header
  eros_header_t *header = (eros_header_t *)data_with_header;
  package->target.destination.id = header->target_id;
  package->target.destination.realm_id = header->target_realm;
  package->source.id = header->source_id;

  return package;
}

void eros_package_write_header(eros_package_t *package) {
  eros_header_t *header =
      (eros_header_t *)(package->data - sizeof(eros_header_t));
  header->target_id = package->target.destination.id;
  header->target_realm = package->target.destination.realm_id;
  header->source_id = package->source.id;
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
