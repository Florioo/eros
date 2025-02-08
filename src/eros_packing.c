
#include "eros_packing.h"
#include "eros_endpoint.h"
#include "eros_package.h"
#include "eros_router.h"

int eros_ingest_packed_data(eros_endpoint_t *endpoint, uint8_t *data,
                            size_t size, TickType_t timeout) {

  eros_package_t *package = eros_headered_package_new(data, size);

  if (package == NULL) {
    return -1;
  }

  package->source = endpoint->id;

  eros_router_route(endpoint->router, package, timeout);
  eros_package_delete(package);
  return 0;
}