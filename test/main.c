/* Standard includes. */
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

#include "eros.h"
#include <string.h>

static void sender_task(void *p);
static void receiver_task(void *p);

eros_router_t router;

typedef struct
{
    eros_router_t *router;
    int8_t id;
} receive_task_config_t;

int main(void)
{
    router = eros_router_init(1);
    printf("Router %p\n", &router);
    xTaskCreate(sender_task, "generator", 2048, NULL, 1, NULL);

    static receive_task_config_t task1_config = {
        .router = &router,
        .id = 1,
    };
    xTaskCreate(receiver_task, "receiver", 2048, &task1_config, 1, NULL);

    static receive_task_config_t task2_config = {
        .router = &router,
        .id = 2,
    };
    xTaskCreate(receiver_task, "receiver", 2048, &task2_config, 1, NULL);


    vTaskStartScheduler();

    while (1)
    {
        vTaskDelay(1000);
    }
}

static void sender_task(void *p)
{
    (void)p;

    eros_endpoint_t endpoint = eros_endpoint_init(1, &router);
    eros_router_register_endpoint(&router, &endpoint);
    
    printf("Endpoint %p\n", &endpoint);

    eros_topic_t topic_id = 1;

    while (1)
    {
        static int i = 0;
        char data[32];

        sprintf(data, "Hello %d", i++);
        printf("Sending: %s\n", data);

        eros_endpoint_sent_to_topic(&endpoint, topic_id, (uint8_t *)data, strlen(data));
        vTaskDelay(1000);
    }
}

static void receiver_task(void *p)
{
    receive_task_config_t *config = (receive_task_config_t *)p;

    eros_endpoint_t endpoint = eros_endpoint_init(config->id    , config->router);
    eros_router_register_endpoint(&router, &endpoint);

    eros_endpoint_subscribe_topic(&endpoint, 1);

    while (1)
    {
        eros_package_reference_counted_t *package = eros_endpoint_receive(&endpoint, 1000);

        if (package == NULL)
        {
            printf("Timeout\n");
            continue;
        }

        printf("Received: %s\n", package->package.data);
        eros_free_package(package);
    }
}

//     const TickType_t xTimerPeriod = mainTIMER_SEND_FREQUENCY_MS;

//     /* Create the queue. */
//     xQueue = xQueueCreate(mainQUEUE_LENGTH, sizeof(uint32_t));

//     if (xQueue != NULL)
//     {
//         /* Start the two tasks as described in the comments at the top of this
//         file. */
//         xTaskCreate(
//             prvQueueReceiveTask,             /* The function that implements the task. */
//             "Rx",                            /* The text name assigned to the task - for debug only as it
//                                                 is not used by the kernel. */
//             configMINIMAL_STACK_SIZE,        /* The size of the stack to allocate to
//                                                 the task. */
//             NULL,                            /* The parameter passed to the task - not used in this simple
//                                                 case. */
//             mainQUEUE_RECEIVE_TASK_PRIORITY, /* The priority assigned to the
//                                                 task. */
//             NULL);                           /* The task handle is not required, so NULL is passed. */

//         xTaskCreate(prvQueueSendTask, "TX", configMINIMAL_STACK_SIZE, NULL,
//                     mainQUEUE_SEND_TASK_PRIORITY, NULL);

//         /* Create the software timer, but don't start it yet. */
//         xTimer = xTimerCreate(
//             "Timer",                    /* The text name assigned to the software timer - for debug
//                                            only as it is not used by the kernel. */
//             xTimerPeriod,               /* The period of the software timer in ticks. */
//             1,                          /* xAutoReload is set to pdTRUE. */
//             NULL,                       /* The timer's ID is not used. */
//             prvQueueSendTimerCallback); /* The function executed when the timer
//                                            expires. */

//         if (xTimer != NULL)
//         {
//             xTimerStart(xTimer, 0);
//         }

//         /* Start the tasks and timer running. */
//         vTaskStartScheduler();
//     }

//     for (;;)
//         ;

//     return 0;
// }
// /*-----------------------------------------------------------*/

// static void prvQueueSendTask(void *pvParameters)
// {
//     TickType_t xNextWakeTime;
//     const TickType_t xBlockTime = mainTASK_SEND_FREQUENCY_MS;
//     const uint32_t ulValueToSend = mainVALUE_SENT_FROM_TASK;

//     /* Prevent the compiler warning about the unused parameter. */
//     (void)pvParameters;

//     /* Initialise xNextWakeTime - this only needs to be done once. */
//     xNextWakeTime = xTaskGetTickCount();

//     for (;;)
//     {
//         /* Place this task in the blocked state until it is time to run again.
//         The block time is specified in ticks, pdMS_TO_TICKS() was used to
//         convert a time specified in milliseconds into a time specified in ticks.
//         While in the Blocked state this task will not consume any CPU time. */
//         vTaskDelayUntil(&xNextWakeTime, xBlockTime);

//         /* Send to the queue - causing the queue receive task to unblock and
//         write to the console.  0 is used as the block time so the send operation
//         will not block - it shouldn't need to block as the queue should always
//         have at least one space at this point in the code. */
//         xQueueSend(xQueue, &ulValueToSend, 0U);
//     }
// }
// /*-----------------------------------------------------------*/

// static void prvQueueSendTimerCallback(TimerHandle_t xTimerHandle)
// {
//     const uint32_t ulValueToSend = mainVALUE_SENT_FROM_TIMER;

//     /* This is the software timer callback function.  The software timer has a
//     period of two seconds and is reset each time a key is pressed.  This
//     callback function will execute if the timer expires, which will only happen
//     if a key is not pressed for two seconds. */

//     /* Avoid compiler warnings resulting from the unused parameter. */
//     (void)xTimerHandle;

//     /* Send to the queue - causing the queue receive task to unblock and
//     write out a message.  This function is called from the timer/daemon task, so
//     must not block.  Hence the block time is set to 0. */
//     xQueueSend(xQueue, &ulValueToSend, 0U);
// }
// /*-----------------------------------------------------------*/

// static void prvQueueReceiveTask(void *pvParameters)
// {
//     uint32_t ulReceivedValue;

//     /* Prevent the compiler warning about the unused parameter. */
//     (void)pvParameters;

//     for (;;)
//     {
//         /* Wait until something arrives in the queue - this task will block
//         indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
//         FreeRTOSConfig.h.  It will not use any CPU time while it is in the
//         Blocked state. */
//         xQueueReceive(xQueue, &ulReceivedValue, portMAX_DELAY);

//         /* To get here something must have been received from the queue, but
//         is it an expected value?  Normally calling printf() from a task is not
//         a good idea.  Here there is lots of stack space and only one task is
//         using console IO so it is ok.  However, note the comments at the top of
//         this file about the risks of making Linux system calls (such as
//         console output) from a FreeRTOS task. */
//         if (ulReceivedValue == mainVALUE_SENT_FROM_TASK)
//         {
//             printf("Message received from task\n");
//         }
//         else if (ulReceivedValue == mainVALUE_SENT_FROM_TIMER)
//         {
//             printf("Message received from software timer\n");
//         }
//         else
//         {
//             printf("Unexpected message\n");
//         }
//     }
// }
// /*-----------------------------------------------------------*/
