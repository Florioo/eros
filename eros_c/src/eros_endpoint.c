

#include "eros.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void eros_print_packag_debug(const char *tag, eros_package_t *package)
{
  #if 0
  printf("%s: src(realm: %d, id: %d) -> dest(realm: %d, id: %d) "
         "seq: %d size: %d\n",
         tag, package->source.realm_id, package->source.id,
         package->target.destination.realm_id, package->target.destination.id,
         package->sequence_number, package->size);
  #endif
}

eros_endpoint_t *eros_buffered_endpoint_new(int id, eros_router_t *router,
                                            int queue_size)
{
  assert(router);

  QueueHandle_t queue = xQueueCreate(queue_size, sizeof(eros_package_t *));

  if (queue == NULL)
  {
    return NULL;
  }

  eros_endpoint_t endpoint = {
      .id =
          {
              .id = id,
              .realm_id = router->realm_id,
          },
      .type = EROS_ENDPOINT_BUFFERED,
      .endpoint.buffered_endpoint =
          {
              .queue = queue,
          },
      .router = router,
  };

  eros_endpoint_t *endpoint_ptr = malloc(sizeof(eros_endpoint_t));

  if (endpoint_ptr == NULL)
  {
    vQueueDelete(queue);
    return NULL;
  }

  memcpy(endpoint_ptr, &endpoint, sizeof(eros_endpoint_t));
  return endpoint_ptr;
}

eros_endpoint_t *
eros_unbuffered_endpoint_new(int id, eros_router_t *router,
                             eros_package_callback_t callback)
{
  assert(router);

  eros_endpoint_t endpoint = {
      .id =
          {
              .id = id,
              .realm_id = router->realm_id,
          },
      .type = EROS_ENDPOINT_UNBUFFERED,
      .endpoint.unbuffered_endpoint =
          {
              .callback = callback,
          },
      .router = router,
  };

  eros_endpoint_t *endpoint_ptr = malloc(sizeof(eros_endpoint_t));

  if (endpoint_ptr == NULL)
  {
    return NULL;
  }

  memcpy(endpoint_ptr, &endpoint, sizeof(eros_endpoint_t));
  return endpoint_ptr;
}

eros_endpoint_t *
eros_buffered_gateway_endpoint_new(int id, eros_router_t *router,
                                   int queue_size,
                                   eros_realm_id_t remote_realm_id)
{
  assert(router);

  QueueHandle_t queue = xQueueCreate(queue_size, sizeof(eros_package_t *));

  if (queue == NULL)
  {
    return NULL;
  }
  // TODO: remove double remote realm id

  eros_endpoint_t endpoint = {
      .id =
          {
              .id = id,
              .realm_id = remote_realm_id,
          },
      .type = EROS_GATEWAY_BUFFERED,
      .endpoint.buffered_gateway_endpoint =
          {
              .queue = queue,
              .remote_realm_id = remote_realm_id,
              .gateway_mode = EROS_GATEWAY_MODE_PROMISCUOUS,
          },
      .router = router,
  };

  eros_endpoint_t *endpoint_ptr = malloc(sizeof(eros_endpoint_t));

  if (endpoint_ptr == NULL)
  {
    vQueueDelete(queue);
    return NULL;
  }

  memcpy(endpoint_ptr, &endpoint, sizeof(eros_endpoint_t));
  return endpoint_ptr;
}

eros_endpoint_t *
eros_unbuffered_gateway_endpoint_new(int id, eros_router_t *router,
                                     eros_realm_id_t remote_realm_id,
                                     eros_data_callback_t callback)
{
  assert(router);

  eros_endpoint_t endpoint = {
      .id =
          {
              .id = id,
              .realm_id = remote_realm_id,
          },
      .type = EROS_UNBUFFERED_GATEWAY,
      .endpoint.unbuffered_gateway_endpoint =
          {
              .callback = callback,
              .remote_realm_id = remote_realm_id,
              .gateway_mode = EROS_GATEWAY_MODE_PROMISCUOUS,
          },
      .router = router,
  };

  eros_endpoint_t *endpoint_ptr = malloc(sizeof(eros_endpoint_t));

  if (endpoint_ptr == NULL)
  {
    return NULL;
  }

  memcpy(endpoint_ptr, &endpoint, sizeof(eros_endpoint_t));
  return endpoint_ptr;
}

void eros_gateway_set_fixed_id_mode(eros_endpoint_t *endpoint,
                                    eros_id_t target)
{
  assert(endpoint);
  if (endpoint->type == EROS_UNBUFFERED_GATEWAY)
  {
    endpoint->endpoint.unbuffered_gateway_endpoint.gateway_mode =
        EROS_GATEWAY_MODE_FIXED_ID;
    endpoint->endpoint.unbuffered_gateway_endpoint.fixed_target = target;
  }
  else
  {
    endpoint->endpoint.buffered_gateway_endpoint.gateway_mode =
        EROS_GATEWAY_MODE_FIXED_ID;
    endpoint->endpoint.buffered_gateway_endpoint.fixed_target = target;
  }
}

void eros_gateway_set_fixed_target_group_mode(eros_endpoint_t *endpoint,
                                              eros_group_t group)
{
  assert(endpoint);
  if (endpoint->type == EROS_UNBUFFERED_GATEWAY)
  {
    endpoint->endpoint.unbuffered_gateway_endpoint.gateway_mode =
        EROS_GATEWAY_MODE_FIXED_GROUP;
    endpoint->endpoint.unbuffered_gateway_endpoint.fixed_group = group;
  }
  else
  {
    endpoint->endpoint.buffered_gateway_endpoint.gateway_mode =
        EROS_GATEWAY_MODE_FIXED_GROUP;
    endpoint->endpoint.buffered_gateway_endpoint.fixed_group = group;
  }
}

void eros_endpoint_delete(eros_endpoint_t *endpoint)
{
  if (endpoint == NULL)
  {
    return;
  }

  switch (endpoint->type)
  {
  case EROS_ENDPOINT_BUFFERED:
    if (endpoint->endpoint.buffered_endpoint.queue != NULL)
    {
      vQueueDelete(endpoint->endpoint.buffered_endpoint.queue);
    }
    break;
  case EROS_GATEWAY_BUFFERED:
    if (endpoint->endpoint.buffered_gateway_endpoint.queue != NULL)
    {
      vQueueDelete(endpoint->endpoint.buffered_gateway_endpoint.queue);
    }
    break;
  default:
    break;
  }

  free(endpoint);
}

eros_gateway_mode_enum_t
eros_endpoint_gateway_get_mode(eros_endpoint_t *endpoint)
{
  assert(endpoint);
  if (endpoint->type == EROS_UNBUFFERED_GATEWAY)
  {
    return endpoint->endpoint.unbuffered_gateway_endpoint.gateway_mode;
  }
  else
  {
    return endpoint->endpoint.buffered_gateway_endpoint.gateway_mode;
  }
}

void eros_endpoint_subscribe_group(eros_endpoint_t *endpoint,
                                   eros_group_t group)
{
  endpoint->subscribed_group_bitmap |= (1 << group);
}

int eros_endpoint_send_data(eros_endpoint_t *endpoint, eros_id_t destination,
                            uint8_t *data, size_t size, TickType_t timeout)
{
  eros_package_t *package = eros_package_new(data, size);
  if (package == NULL)
  {
    return -1;
  }

  package->source = endpoint->id;
  package->type = EROS_PACKAGE_TYPE_ID;
  package->target.destination = destination;

  eros_print_packag_debug("TX", package);

  // Route the package to the router
  eros_router_route(endpoint->router, package, timeout);

  // Free the package (of not sent it will be freed)
  eros_package_delete(package);
  return 0;
}
int eros_endpoint_send_data_with_seq(eros_endpoint_t *endpoint, eros_id_t destination, uint8_t sequence,
                                     uint8_t *data, size_t size, TickType_t timeout)
{
  eros_package_t *package = eros_package_new(data, size);
  if (package == NULL)
  {
    return -1;
  }

  package->source = endpoint->id;
  package->type = EROS_PACKAGE_TYPE_ID;
  package->target.destination = destination;
  package->sequence_number = sequence;

  eros_print_packag_debug("TX", package);

  // Route the package to the router
  eros_router_route(endpoint->router, package, timeout);

  // Free the package (of not sent it will be freed)
  eros_package_delete(package);
  return 0;
}

int eros_endpoint_publish_data(eros_endpoint_t *endpoint, eros_group_t group,
                               uint8_t *data, size_t size, TickType_t timeout)
{
  eros_package_t *package = eros_package_new(data, size);
  if (package == NULL)
  {
    return -1;
  }

  package->source = endpoint->id;
  package->type = EROS_PACKAGE_TYPE_GROUP;
  package->target.group = group;

  eros_router_route(endpoint->router, package, timeout);
  eros_package_delete(package);
  return 0;
}

int eros_endpoint_publish_headered_data(eros_endpoint_t *endpoint,
                                        uint8_t *data, size_t size,
                                        TickType_t timeout)
{

  eros_package_t *package = eros_headered_package_new(data, size);

  if (package == NULL)
  {
    return -1;
  }

  eros_print_packag_debug("RX", package);

  eros_router_route(endpoint->router, package, timeout);
  eros_package_delete(package);
  return 0;
}

int eros_endpoint_gateway_ingest(eros_endpoint_t *endpoint, uint8_t *data,
                                 size_t size, TickType_t timeout)
{

  eros_gateway_mode_enum_t mode = eros_endpoint_gateway_get_mode(endpoint);

  if (mode == EROS_GATEWAY_MODE_PROMISCUOUS)
  {
    return eros_endpoint_publish_headered_data(endpoint, data, size, timeout);
  }
  else if (mode == EROS_GATEWAY_MODE_FIXED_GROUP)
  {
    return eros_endpoint_publish_data(
        endpoint, endpoint->endpoint.unbuffered_gateway_endpoint.fixed_group,
        data, size, timeout);
  }
  else
  {
    return eros_endpoint_send_data(
        endpoint, endpoint->endpoint.unbuffered_gateway_endpoint.fixed_target,
        data, size, timeout);
  }
}
eros_package_t *eros_buffered_endpoint_receive(eros_endpoint_t *endpoint,
                                               TickType_t timeout)
{
  assert(endpoint);
  assert(endpoint->type == EROS_ENDPOINT_BUFFERED);
  eros_package_t *package = NULL;
  xQueueReceive(endpoint->endpoint.buffered_endpoint.queue, &package, timeout);
  return package;
}

eros_package_t *
eros_buffered_gateway_endpoint_receive(eros_endpoint_t *endpoint,
                                       TickType_t timeout)
{
  assert(endpoint);
  assert(endpoint->type == EROS_GATEWAY_BUFFERED);

  eros_package_t *package = NULL;
  xQueueReceive(endpoint->endpoint.buffered_gateway_endpoint.queue, &package,
                timeout);

  // Add header
  eros_package_encode_header(package);

  return package;
}

int eros_endpoint_send(eros_endpoint_t *endpoint, eros_package_t *package,
                       TickType_t timeout)
{
  assert(endpoint);
  assert(package);
  int ret = 0;

  // increment the reference count
  switch (endpoint->type)
  {
  case EROS_ENDPOINT_BUFFERED:
    eros_package_increase_reference(package);

    ret = xQueueSend(endpoint->endpoint.buffered_endpoint.queue, &package,
                     timeout);
    if (ret != pdPASS)
    {
      eros_package_decrease_reference(package);
      return -1;
    }
    return 0;

  case EROS_GATEWAY_BUFFERED:
    eros_package_increase_reference(package);

    ret = xQueueSend(endpoint->endpoint.buffered_gateway_endpoint.queue,
                     &package, timeout);
    if (ret != pdPASS)
    {
      eros_package_decrease_reference(package);
      return -1;
    }

    return 0;

  case EROS_ENDPOINT_UNBUFFERED:
    if (endpoint->endpoint.unbuffered_endpoint.callback)
    {
      endpoint->endpoint.unbuffered_endpoint.callback(endpoint, package);
      return 0;
    }
    return -1;

  case EROS_UNBUFFERED_GATEWAY:
    if (endpoint->endpoint.unbuffered_gateway_endpoint.callback)
    {

      if (endpoint->endpoint.unbuffered_gateway_endpoint.gateway_mode ==
          EROS_GATEWAY_MODE_PROMISCUOUS)
      {
        // Add header data
        eros_package_encode_header(package);
        endpoint->endpoint.unbuffered_gateway_endpoint.callback(
            endpoint, package->data - sizeof(eros_header_t),
            package->size + sizeof(eros_header_t));
      }
      else
      {

        // Do not add header data
        endpoint->endpoint.unbuffered_gateway_endpoint.callback(
            endpoint, package->data, package->size);
      }

      return 0;
    }
    return -1;

  case EROS_ENDPOINT_WORKER:
    if (endpoint->endpoint.worker_endpoint.worker_callback)
    {
      endpoint->endpoint.worker_endpoint.worker_callback(endpoint, package);
      return 0;
    }
    return -1;

  default:
    return -1;
  }
}

void eros_endpoint_set_callback(eros_endpoint_t *endpoint,
                                eros_package_callback_t callback)
{
  assert(endpoint);
  switch (endpoint->type)
  {
  case EROS_ENDPOINT_UNBUFFERED:
    endpoint->endpoint.unbuffered_endpoint.callback = callback;
    break;
  case EROS_UNBUFFERED_GATEWAY:
    endpoint->endpoint.unbuffered_gateway_endpoint.callback =
        (eros_data_callback_t)(void *)callback;
    break;
  case EROS_ENDPOINT_WORKER:
    endpoint->endpoint.worker_endpoint.callback = callback;
    break;
  default:
    break;
  }
}
