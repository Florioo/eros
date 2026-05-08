#include "eros_port.h"

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct eros_port_queue {
    pthread_mutex_t mutex;
    pthread_cond_t  not_empty;
    pthread_cond_t  not_full;
    size_t          item_size;
    size_t          capacity;
    size_t          count;
    size_t          head;
    size_t          tail;
    uint8_t        *buffer;
};

struct eros_port_task {
    pthread_t handle;
    void    (*entry)(void *);
    void     *arg;
};

static void timeout_to_abs(uint32_t timeout_ms, struct timespec *out)
{
    clock_gettime(CLOCK_REALTIME, out);
    out->tv_sec  += timeout_ms / 1000U;
    out->tv_nsec += (long)(timeout_ms % 1000U) * 1000000L;
    if (out->tv_nsec >= 1000000000L) {
        out->tv_sec  += 1;
        out->tv_nsec -= 1000000000L;
    }
}

eros_port_queue_t *eros_port_queue_create(size_t depth, size_t item_size)
{
    eros_port_queue_t *q = calloc(1, sizeof(*q));
    if (!q) return NULL;
    q->buffer = calloc(depth, item_size);
    if (!q->buffer) { free(q); return NULL; }
    q->capacity  = depth;
    q->item_size = item_size;
    pthread_mutex_init(&q->mutex,     NULL);
    pthread_cond_init (&q->not_empty, NULL);
    pthread_cond_init (&q->not_full,  NULL);
    return q;
}

void eros_port_queue_destroy(eros_port_queue_t *q)
{
    if (!q) return;
    pthread_cond_destroy (&q->not_full);
    pthread_cond_destroy (&q->not_empty);
    pthread_mutex_destroy(&q->mutex);
    free(q->buffer);
    free(q);
}

bool eros_port_queue_send(eros_port_queue_t *q, const void *item, uint32_t timeout_ms)
{
    pthread_mutex_lock(&q->mutex);

    if (q->count == q->capacity) {
        if (timeout_ms == 0) {
            pthread_mutex_unlock(&q->mutex);
            return false;
        }
        if (timeout_ms == EROS_WAIT_FOREVER) {
            while (q->count == q->capacity) {
                pthread_cond_wait(&q->not_full, &q->mutex);
            }
        } else {
            struct timespec deadline;
            timeout_to_abs(timeout_ms, &deadline);
            while (q->count == q->capacity) {
                if (pthread_cond_timedwait(&q->not_full, &q->mutex, &deadline) == ETIMEDOUT) {
                    pthread_mutex_unlock(&q->mutex);
                    return false;
                }
            }
        }
    }

    memcpy(q->buffer + q->tail * q->item_size, item, q->item_size);
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
    return true;
}

bool eros_port_queue_recv(eros_port_queue_t *q, void *item, uint32_t timeout_ms)
{
    pthread_mutex_lock(&q->mutex);

    if (q->count == 0) {
        if (timeout_ms == 0) {
            pthread_mutex_unlock(&q->mutex);
            return false;
        }
        if (timeout_ms == EROS_WAIT_FOREVER) {
            while (q->count == 0) {
                pthread_cond_wait(&q->not_empty, &q->mutex);
            }
        } else {
            struct timespec deadline;
            timeout_to_abs(timeout_ms, &deadline);
            while (q->count == 0) {
                if (pthread_cond_timedwait(&q->not_empty, &q->mutex, &deadline) == ETIMEDOUT) {
                    pthread_mutex_unlock(&q->mutex);
                    return false;
                }
            }
        }
    }

    memcpy(item, q->buffer + q->head * q->item_size, q->item_size);
    q->head = (q->head + 1) % q->capacity;
    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mutex);
    return true;
}

size_t eros_port_queue_count(eros_port_queue_t *q)
{
    pthread_mutex_lock(&q->mutex);
    size_t n = q->count;
    pthread_mutex_unlock(&q->mutex);
    return n;
}

static void *posix_task_entry(void *arg)
{
    eros_port_task_t *t = arg;
    t->entry(t->arg);
    return NULL;
}

eros_port_task_t *eros_port_task_create(const char *name,
                                        void (*entry)(void *),
                                        void *arg,
                                        size_t stack_bytes)
{
    (void) name;
    eros_port_task_t *t = calloc(1, sizeof(*t));
    if (!t) return NULL;
    t->entry = entry;
    t->arg   = arg;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (stack_bytes > 0) {
        pthread_attr_setstacksize(&attr, stack_bytes);
    }
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&t->handle, &attr, posix_task_entry, t) != 0) {
        pthread_attr_destroy(&attr);
        free(t);
        return NULL;
    }
    pthread_attr_destroy(&attr);
    return t;
}

void eros_port_task_destroy(eros_port_task_t *t)
{
    if (!t) return;
    pthread_cancel(t->handle);
    free(t);
}

void eros_port_sleep_ms(uint32_t ms)
{
    struct timespec req = { .tv_sec = ms / 1000U, .tv_nsec = (long)(ms % 1000U) * 1000000L };
    nanosleep(&req, NULL);
}
