#ifndef EROS_ENDPOINT_H
#define EROS_ENDPOINT_H

#include "eros_package.h"
#include "eros_port.h"

typedef enum
{
  // A regular endpoint preserves the data
  EROS_ENDPOINT_BUFFERED = 1,
  EROS_ENDPOINT_UNBUFFERED = 2,

  // Gateway must add and remove data from the package
  EROS_GATEWAY_BUFFERED = 4,
  EROS_UNBUFFERED_GATEWAY = 5,

  EROS_ENDPOINT_WORKER = 6,
} eros_endpoint_enum;

typedef struct
{
  eros_port_queue_t *queue;
} eros_buffered_endpoint_t;

typedef struct eros_router_t eros_router_t;
typedef struct eros_endpoint_t eros_endpoint_t;
typedef struct eros_worker_t eros_worker_t;

typedef void (*eros_package_callback_t)(eros_endpoint_t *endpoint,
                                        eros_package_t *package);
typedef void (*eros_data_callback_t)(eros_endpoint_t *endpoint, uint8_t *data,
                                     size_t size);
typedef enum
{
  EROS_GATEWAY_MODE_FIXED_GROUP = 1, // Publish data to fixed group
  EROS_GATEWAY_MODE_FIXED_ID = 0,    // Publish data to fixed id
  EROS_GATEWAY_MODE_PROMISCUOUS = 2, // Receive to all addresses
} eros_gateway_mode_enum_t;

typedef struct
{
  eros_package_callback_t callback;
} eros_unbuffered_endpoint_t;

typedef struct
{
  eros_realm_id_t remote_realm_id;
  eros_port_queue_t *queue;
  eros_gateway_mode_enum_t gateway_mode;
  eros_id_t fixed_target;
  eros_group_t fixed_group;
} eros_buffered_gateway_endpoint_t;

typedef struct
{
  eros_realm_id_t remote_realm_id;
  eros_data_callback_t callback;
  eros_gateway_mode_enum_t gateway_mode;
  eros_id_t fixed_target;
  eros_group_t fixed_group;
} eros_unbuffered_gateway_endpoint_t;

typedef struct
{
  eros_package_callback_t worker_callback;
  eros_package_callback_t callback;
  eros_worker_t *worker;
} eros_worker_endpoint_t;

struct eros_endpoint_t
{
  const eros_id_t id;
  eros_router_t *router;
  uint32_t subscribed_group_bitmap;
  eros_endpoint_enum type;
  union
  {
    eros_buffered_endpoint_t buffered_endpoint;
    eros_unbuffered_endpoint_t unbuffered_endpoint;
    eros_buffered_gateway_endpoint_t buffered_gateway_endpoint;
    eros_unbuffered_gateway_endpoint_t unbuffered_gateway_endpoint;
    eros_worker_endpoint_t worker_endpoint;
  } endpoint;
  void *user_data;
};

eros_endpoint_t *eros_buffered_endpoint_new(int id, eros_router_t *router,
                                            int queue_size);
eros_endpoint_t *eros_unbuffered_endpoint_new(int id, eros_router_t *router,
                                              eros_package_callback_t callback);
eros_endpoint_t *
eros_buffered_gateway_endpoint_new(int id, eros_router_t *router,
                                   int queue_size,
                                   eros_realm_id_t remote_realm_id);

eros_endpoint_t *
eros_unbuffered_gateway_endpoint_new(int id, eros_router_t *router,
                                     eros_realm_id_t remote_realm_id,
                                     eros_data_callback_t callback);

eros_gateway_mode_enum_t
eros_endpoint_gateway_get_mode(eros_endpoint_t *endpoint);
int eros_endpoint_gateway_ingest(eros_endpoint_t *endpoint, uint8_t *data,
                                 size_t size, uint32_t timeout_ms);

void eros_gateway_set_fixed_id_mode(eros_endpoint_t *endpoint,
                                    eros_id_t target);

void eros_gateway_set_fixed_target_group_mode(eros_endpoint_t *endpoint,
                                              eros_group_t group);
void eros_endpoint_delete(eros_endpoint_t *endpoint);
void eros_endpoint_set_callback(eros_endpoint_t *endpoint,
                                eros_package_callback_t callback);

void eros_endpoint_subscribe_group(eros_endpoint_t *endpoint,
                                   eros_group_t group);
int eros_endpoint_send_data(eros_endpoint_t *endpoint, eros_id_t destination,
                            uint8_t *data, size_t size, uint32_t timeout_ms);
int eros_endpoint_send_data_with_seq(eros_endpoint_t *endpoint, eros_id_t destination, uint8_t sequence,
                                     uint8_t *data, size_t size, uint32_t timeout_ms);
int eros_endpoint_publish_data(eros_endpoint_t *endpoint, eros_group_t group,
                               uint8_t *data, size_t size, uint32_t timeout_ms);
eros_package_t *eros_buffered_endpoint_receive(eros_endpoint_t *endpoint,
                                               uint32_t timeout_ms);
eros_package_t *
eros_buffered_gateway_endpoint_receive(eros_endpoint_t *endpoint,
                                       uint32_t timeout_ms);
int eros_endpoint_send(eros_endpoint_t *endpoint, eros_package_t *package,
                       uint32_t timeout_ms);

#endif // EROS_ENDPOINT_H
