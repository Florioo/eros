#include "eros_port.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include <stdlib.h>

struct eros_port_queue {
    QueueHandle_t handle;
};

struct eros_port_task {
    TaskHandle_t handle;
};

static TickType_t ms_to_ticks(uint32_t timeout_ms)
{
    if (timeout_ms == EROS_WAIT_FOREVER) {
        return portMAX_DELAY;
    }
    return pdMS_TO_TICKS(timeout_ms);
}

eros_port_queue_t *eros_port_queue_create(size_t depth, size_t item_size)
{
    QueueHandle_t handle = xQueueCreate(depth, item_size);
    if (handle == NULL) {
        return NULL;
    }
    eros_port_queue_t *q = pvPortMalloc(sizeof(*q));
    if (q == NULL) {
        vQueueDelete(handle);
        return NULL;
    }
    q->handle = handle;
    return q;
}

void eros_port_queue_destroy(eros_port_queue_t *q)
{
    if (q == NULL) return;
    vQueueDelete(q->handle);
    vPortFree(q);
}

bool eros_port_queue_send(eros_port_queue_t *q, const void *item, uint32_t timeout_ms)
{
    return xQueueSend(q->handle, item, ms_to_ticks(timeout_ms)) == pdPASS;
}

bool eros_port_queue_recv(eros_port_queue_t *q, void *item, uint32_t timeout_ms)
{
    return xQueueReceive(q->handle, item, ms_to_ticks(timeout_ms)) == pdPASS;
}

size_t eros_port_queue_count(eros_port_queue_t *q)
{
    return uxQueueMessagesWaiting(q->handle);
}

eros_port_task_t *eros_port_task_create(const char *name,
                                        void (*entry)(void *),
                                        void *arg,
                                        size_t stack_bytes)
{
    eros_port_task_t *t = pvPortMalloc(sizeof(*t));
    if (t == NULL) return NULL;

    /* xTaskCreate's stack depth is in StackType_t units. */
    const uint32_t stack_depth = (uint32_t) (stack_bytes / sizeof(StackType_t));
    if (xTaskCreate(entry, name, stack_depth, arg, tskIDLE_PRIORITY + 1, &t->handle) != pdPASS) {
        vPortFree(t);
        return NULL;
    }
    return t;
}

void eros_port_task_destroy(eros_port_task_t *t)
{
    if (t == NULL) return;
    vTaskDelete(t->handle);
    vPortFree(t);
}

void eros_port_sleep_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}
