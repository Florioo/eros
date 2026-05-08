#ifndef EROS_PORT_H
#define EROS_PORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque types — each port supplies its own definition. */
typedef struct eros_port_queue eros_port_queue_t;
typedef struct eros_port_task  eros_port_task_t;

/* Timeouts everywhere are in milliseconds. EROS_WAIT_FOREVER = block until done. */
#define EROS_WAIT_FOREVER ((uint32_t) UINT32_MAX)

eros_port_queue_t *eros_port_queue_create(size_t depth, size_t item_size);
void               eros_port_queue_destroy(eros_port_queue_t *queue);
bool               eros_port_queue_send(eros_port_queue_t *queue, const void *item, uint32_t timeout_ms);
bool               eros_port_queue_recv(eros_port_queue_t *queue, void *item, uint32_t timeout_ms);
size_t             eros_port_queue_count(eros_port_queue_t *queue);

eros_port_task_t *eros_port_task_create(const char *name,
                                        void (*entry)(void *),
                                        void *arg,
                                        size_t stack_bytes);
void              eros_port_task_destroy(eros_port_task_t *task);
void              eros_port_sleep_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* EROS_PORT_H */
