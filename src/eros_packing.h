#include "eros_endpoint.h"
#include <stdint.h>
#ifndef EROS_PACKING_H
#define EROS_PACKING_H

typedef struct {
  uint8_t realm_id : 4;
  uint8_t id : 4;
} eros_header_t;

int eros_ingest_packed_data(eros_endpoint_t *endpoint, uint8_t *data,
                            size_t size, TickType_t timeout);

#endif // EROS_PACKING_H