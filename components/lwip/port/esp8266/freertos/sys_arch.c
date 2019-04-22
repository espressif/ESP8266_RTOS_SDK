/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/* lwIP includes. */

#include <stdlib.h>

#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "arch/sys_arch.h"

#define LWIP_THREAD_TLS 0

/* Message queue constants. */
#define archMESG_QUEUE_LENGTH	(100)//( 6 )
#define archPOST_BLOCK_TIME_MS	( ( unsigned portLONG ) 10000 )


struct timeoutlist {
    //struct sys_timeouts timeouts;
    xTaskHandle pid;
};

/* This is the number of threads that can be started with sys_thread_new() */
#define SYS_THREAD_MAX 4

//static struct timeoutlist timeoutlist[SYS_THREAD_MAX];
//static u16_t nextthread = 0;
int intlevel = 0;

static xTaskHandle s_tcpip_task_handle;

/*-----------------------------------------------------------------------------------*/
// Initialize sys arch
void
sys_init(void)
{
}

/*-----------------------------------------------------------------------------------*/
//  Creates and returns a new semaphore. The "count" argument specifies
//  the initial state of the semaphore. TBD finish and test
err_t
sys_sem_new(sys_sem_t *sem, u8_t count)
{
    err_t xReturn = ERR_MEM;
    vSemaphoreCreateBinary(*sem);

    if ((*sem) != NULL) {
        if (count == 0) {	// Means it can't be taken
            xSemaphoreTake(*sem, 1);
        }

        xReturn = ERR_OK;
    } else {
        ;	// TBD need assert
    }

    return xReturn;
}


/*-----------------------------------------------------------------------------------*/
// Deallocates a semaphore
void
sys_sem_free(sys_sem_t *sem)
{
    //vQueueDelete( sem );
    vSemaphoreDelete(*sem);
}


/*-----------------------------------------------------------------------------------*/
// Signals a semaphore
void
sys_sem_signal(sys_sem_t *sem)
{
    xSemaphoreGive(*sem);
}

/*-----------------------------------------------------------------------------------*/
// Signals a semaphore (from ISR)
int sys_sem_signal_isr(sys_sem_t *sem)
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(*sem, &woken);
    return woken == pdTRUE;
}

/*-----------------------------------------------------------------------------------*/
/*
  Blocks the thread while waiting for the semaphore to be
  signaled. If the "timeout" argument is non-zero, the thread should
  only be blocked for the specified time (measured in
  milliseconds).

  If the timeout argument is non-zero, the return value is the number of
  milliseconds spent waiting for the semaphore to be signaled. If the
  semaphore wasn't signaled within the specified time, the return value is
  SYS_ARCH_TIMEOUT. If the thread didn't have to wait for the semaphore
  (i.e., it was already signaled), the function may return zero.

  Notice that lwIP implements a function with a similar name,
  sys_sem_wait(), that uses the sys_arch_sem_wait() function.
*/
u32_t
sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
    portTickType StartTime, EndTime, Elapsed;
    unsigned long ulReturn;

    StartTime = xTaskGetTickCount();

    if (timeout != 0) {
        if (xSemaphoreTake(*sem, timeout / portTICK_RATE_MS) == pdTRUE) {
            EndTime = xTaskGetTickCount();
            Elapsed = (EndTime - StartTime) * portTICK_RATE_MS;

            if (Elapsed == 0) {
                Elapsed = 1;
            }

            ulReturn = Elapsed;
        } else {
            ulReturn = SYS_ARCH_TIMEOUT;
        }
    } else { // must block without a timeout
        while (xSemaphoreTake(*sem, portMAX_DELAY) != pdTRUE);

        EndTime = xTaskGetTickCount();
        Elapsed = (EndTime - StartTime) * portTICK_RATE_MS;

        if (Elapsed == 0) {
            Elapsed = 1;
        }

        ulReturn = Elapsed;
    }

    return ulReturn ; // return time blocked
}

/*-----------------------------------------------------------------------------------*/
//  Creates an empty mailbox.
err_t
sys_mbox_new(sys_mbox_t *mbox, int size)
{
    err_t xReturn = ERR_MEM;

    *mbox = xQueueCreate(size, sizeof(void *));

    if (*mbox != NULL) {
        xReturn = ERR_OK;
    }

    return xReturn;
}

/*-----------------------------------------------------------------------------------*/
/*
  Deallocates a mailbox. If there are messages still present in the
  mailbox when the mailbox is deallocated, it is an indication of a
  programming error in lwIP and the developer should be notified.
*/
void
sys_mbox_free(sys_mbox_t *mbox)
{
    vQueueDelete(*mbox);
}

/*-----------------------------------------------------------------------------------*/
//   Posts the "msg" to the mailbox.
void
sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
    while (xQueueSendToBack(*mbox, &msg, portMAX_DELAY) != pdTRUE);
}

err_t
sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
    err_t xReturn;

    if (xQueueSend(*mbox, &msg, (portTickType)0) == pdPASS) {
        xReturn = ERR_OK;
    } else {
        xReturn = ERR_MEM;
    }

    return xReturn;
}

/*-----------------------------------------------------------------------------------*/
/*
  Blocks the thread until a message arrives in the mailbox, but does
  not block the thread longer than "timeout" milliseconds (similar to
  the sys_arch_sem_wait() function). The "msg" argument is a result
  parameter that is set by the function (i.e., by doing "*msg =
  ptr"). The "msg" parameter maybe NULL to indicate that the message
  should be dropped.

  The return values are the same as for the sys_arch_sem_wait() function:
  Number of milliseconds spent waiting or SYS_ARCH_TIMEOUT if there was a
  timeout.

  Note that a function with a similar name, sys_mbox_fetch(), is
  implemented by lwIP.
*/
u32_t
sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
    void *dummyptr;
    portTickType StartTime, EndTime, Elapsed;
    unsigned long ulReturn;

    StartTime = xTaskGetTickCount();

    if (msg == NULL) {
        msg = &dummyptr;
    }

    if (timeout != 0) {
        if (pdTRUE == xQueueReceive(*mbox, &(*msg), timeout / portTICK_RATE_MS)) {
            EndTime = xTaskGetTickCount();
            Elapsed = (EndTime - StartTime) * portTICK_RATE_MS;

            if (Elapsed == 0) {
                Elapsed = 1;
            }

            ulReturn = Elapsed;
        } else { // timed out blocking for message
            *msg = NULL;
            ulReturn = SYS_ARCH_TIMEOUT;
        }
    } else { // block forever for a message.
        while (pdTRUE != xQueueReceive(*mbox, &(*msg), portMAX_DELAY));

        EndTime = xTaskGetTickCount();
        Elapsed = (EndTime - StartTime) * portTICK_RATE_MS;

        if (Elapsed == 0) {
            Elapsed = 1;
        }

        ulReturn = Elapsed;
    }

    return ulReturn ; // return time blocked TBD test
}

u32_t
sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
    void *pvDummy;
    unsigned long ulReturn;

    if (msg == NULL) {
        msg = &pvDummy;
    }

    if (pdTRUE == xQueueReceive(*mbox, &(*msg), 0)) {
        ulReturn = ERR_OK;
    } else {
        ulReturn = SYS_MBOX_EMPTY;
    }

    return ulReturn;
}

/** Create a new mutex
 * @param mutex pointer to the mutex to create
 * @return a new mutex */
err_t
sys_mutex_new(sys_mutex_t *pxMutex)
{
    err_t xReturn = ERR_MEM;

    *pxMutex = xSemaphoreCreateMutex();

    if (*pxMutex != NULL) {
        xReturn = ERR_OK;

    } else {
        ;
    }

    return xReturn;
}

/** Lock a mutex
 * @param mutex the mutex to lock */
void
sys_mutex_lock(sys_mutex_t *pxMutex)
{
    while (xSemaphoreTake(*pxMutex, portMAX_DELAY) != pdPASS);
}

err_t
sys_mutex_trylock(sys_mutex_t *pxMutex)
{
	if (xSemaphoreTake(*pxMutex, 0) == pdPASS) return 0;
	else return -1;
}

/** Unlock a mutex
 * @param mutex the mutex to unlock */
void
sys_mutex_unlock(sys_mutex_t *pxMutex)
{
    xSemaphoreGive(*pxMutex);
}


/** Delete a semaphore
 * @param mutex the mutex to delete */
void
sys_mutex_free(sys_mutex_t *pxMutex)
{
    vQueueDelete(*pxMutex);
}

u32_t
sys_now(void)
{
    return xTaskGetTickCount() * portTICK_RATE_MS;
}

/*-----------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------*/
// TBD
/*-----------------------------------------------------------------------------------*/
/*
  Starts a new thread with priority "prio" that will begin its execution in the
  function "thread()". The "arg" argument will be passed as an argument to the
  thread() function. The id of the new thread is returned. Both the id and
  the priority are system dependent.
*/
sys_thread_t
sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
    portBASE_TYPE result;

    result = xTaskCreate(thread, (const char *)name, stacksize, arg, prio, &s_tcpip_task_handle);

    if (result == pdPASS) {
        return s_tcpip_task_handle;
    } else {
        return NULL;
    }
}

int sys_current_task_is_tcpip(void)
{
    return xTaskGetCurrentTaskHandle() == s_tcpip_task_handle ? 1 : 0;
}

char *sys_current_task_name(void)
{
    return pcTaskGetTaskName(xTaskGetCurrentTaskHandle());
}

void
sys_arch_msleep(int ms)
{
	vTaskDelay(ms / portTICK_RATE_MS);
}

u8_t sys_thread_priority_get(sys_thread_t thread_handle)
{
    return (u8_t)uxTaskPriorityGet(thread_handle);
}

void sys_thread_priority_set(sys_thread_t thread_handle, u8_t priority)
{
    vTaskPrioritySet(thread_handle, (UBaseType_t)priority);
}

#if LWIP_NETCONN_SEM_PER_THREAD

static void sys_thread_sem_free(int index, void *data) // destructor for TLS semaphore
{
    sys_sem_t *sem = (sys_sem_t *)(data);

    if (sem && *sem){
        LWIP_DEBUGF(ESP_THREAD_SAFE_DEBUG, ("sem del, sem=%p\n", *sem));
        vSemaphoreDelete(*sem);
    }

    if (sem) {
        LWIP_DEBUGF(ESP_THREAD_SAFE_DEBUG, ("sem pointer del, sem_p=%p\n", sem));
        free(sem);
    }
}

/*
 * get per thread semphore
 */
sys_sem_t* sys_thread_sem_init(void)
{
  sys_sem_t *sem = (sys_sem_t*)mem_malloc(sizeof(sys_sem_t*));

    if (!sem){
        LWIP_DEBUGF(ESP_THREAD_SAFE_DEBUG, "thread_sem_init: out of memory\n");
        return 0;
    }

    *sem = xSemaphoreCreateBinary();
    if (!(*sem)){
        free(sem);
        LWIP_DEBUGF(ESP_THREAD_SAFE_DEBUG, "thread_sem_init: out of memory\n");
        return 0;
    }

    vTaskSetThreadLocalStoragePointerAndDelCallback(NULL, LWIP_THREAD_TLS, sem, sys_thread_sem_free);

    return sem;
}

/*
 * get per thread semphore
 */
sys_sem_t* sys_thread_sem_get(void)
{
    sys_sem_t *sem = pvTaskGetThreadLocalStoragePointer(NULL, LWIP_THREAD_TLS);

    if (!sem) {
        sem = sys_thread_sem_init();
    }
    LWIP_DEBUGF(ESP_THREAD_SAFE_DEBUG, ("sem_get s=%p\n", sem));

    return sem;
}

void sys_thread_sem_deinit(void)
{
    sys_sem_t *sem = pvTaskGetThreadLocalStoragePointer(NULL, LWIP_THREAD_TLS);
    if (sem != NULL) {
        sys_thread_sem_free(LWIP_THREAD_TLS, sem);
        vTaskSetThreadLocalStoragePointerAndDelCallback(NULL, LWIP_THREAD_TLS, NULL, NULL);
    }
}

#endif
