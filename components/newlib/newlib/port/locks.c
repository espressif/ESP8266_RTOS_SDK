// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <sys/lock.h>
#include <stdlib.h>
#include <reent.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#define portYIELD_FROM_ISR portYIELD

/* Initialize the given lock by allocating a new mutex semaphore
   as the _lock_t value.

   Called by _lock_init*, also called by _lock_acquire* to lazily initialize locks that might have
   been initialised (to zero only) before the RTOS scheduler started.
*/

static void lock_init_generic(_lock_t *lock, uint8_t mutex_type) {
    portENTER_CRITICAL();
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        /* nothing to do until the scheduler is running */
        portEXIT_CRITICAL();
        return;
    }

    if (*lock) {
         /* Lock already initialised (either we didn't check earlier,
          or it got initialised while we were waiting for the
          spinlock.) */
    }
    else
    {
        /* Create a new semaphore

           this is a bit of an API violation, as we're calling the
           private function xQueueCreateMutex(x) directly instead of
           the xSemaphoreCreateMutex / xSemaphoreCreateRecursiveMutex
           wrapper functions...

           The better alternative would be to pass pointers to one of
           the two xSemaphoreCreate___Mutex functions, but as FreeRTOS
           implements these as macros instead of inline functions
           (*party like it's 1998!*) it's not possible to do this
           without writing wrappers. Doing it this way seems much less
           spaghetti-like.
        */
        xSemaphoreHandle new_sem = xQueueCreateMutex(mutex_type);
        if (!new_sem) {
            abort(); /* No more semaphores available or OOM */
        }
        *lock = (_lock_t)new_sem;
    }
    portEXIT_CRITICAL();
}

void _lock_init(_lock_t *lock) {
    *lock = 0; // In case lock's memory is uninitialized
    lock_init_generic(lock, queueQUEUE_TYPE_MUTEX);
}

void _lock_init_recursive(_lock_t *lock) {
    *lock = 0; // In case lock's memory is uninitialized
    lock_init_generic(lock, queueQUEUE_TYPE_RECURSIVE_MUTEX);
}

/* Free the mutex semaphore pointed to by *lock, and zero it out.

   Note that FreeRTOS doesn't account for deleting mutexes while they
   are held, and neither do we... so take care not to delete newlib
   locks while they may be held by other tasks!

   Also, deleting a lock in this way will cause it to be lazily
   re-initialised if it is used again. Caller has to avoid doing
   this!
*/
static void lock_close_generic(_lock_t *lock,  uint8_t mutex_type) {
    portENTER_CRITICAL();
    if (*lock) {
        xSemaphoreHandle h = (xSemaphoreHandle)(*lock);
#if (INCLUDE_xSemaphoreGetMutexHolder == 1)
        configASSERT(xSemaphoreGetMutexHolder(h) == NULL); /* mutex should not be held */
#endif
        vSemaphoreDelete(h);
        *lock = 0;
    }
    portEXIT_CRITICAL();
}

void _lock_close(_lock_t *lock) {
    lock_close_generic(lock, queueQUEUE_TYPE_MUTEX);
}

void _lock_close_recursive(_lock_t *lock) {
    lock_close_generic(lock, queueQUEUE_TYPE_RECURSIVE_MUTEX);
}

/* Acquire the mutex semaphore for lock. wait up to delay ticks.
   mutex_type is queueQUEUE_TYPE_RECURSIVE_MUTEX or queueQUEUE_TYPE_MUTEX
*/
static int lock_acquire_generic(_lock_t *lock, uint32_t delay, uint8_t mutex_type) {
    /* If application function has disabled interrupt, then it must not acquire the mutex */
    if (interrupt_is_disable() == true && !xPortInIsrContext())
        return 0;

    xSemaphoreHandle h = (xSemaphoreHandle)(*lock);
    if (!h) {
        if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
            return 0; /* locking is a no-op before scheduler is up, so this "succeeds" */
        }
        /* lazy initialise lock - might have had a static initializer in newlib (that we don't use),
           or _lock_init might have been called before the scheduler was running... */
        lock_init_generic(lock, mutex_type);
        h = (xSemaphoreHandle)(*lock);
        configASSERT(h != NULL);
    }

    BaseType_t success;
    if (xPortInIsrContext()) {
        /* In ISR Context */
        if (mutex_type == queueQUEUE_TYPE_RECURSIVE_MUTEX) {
            abort(); /* recursive mutexes make no sense in ISR context */
        }
        BaseType_t higher_task_woken = false;
        success = xSemaphoreTakeFromISR(h, &higher_task_woken);
        if (!success && delay > 0) {
            abort(); /* Tried to block on mutex from ISR, couldn't... rewrite your program to avoid libc interactions in ISRs! */
        }
        if (higher_task_woken) {
            portYIELD_FROM_ISR();
        }
    }
    else {
        /* In task context */
        if (mutex_type == queueQUEUE_TYPE_RECURSIVE_MUTEX) {
            success = xSemaphoreTakeRecursive(h, delay);
        } else {
            success = xSemaphoreTake(h, delay);
        }
    }

    return (success == pdTRUE) ? 0 : -1;
}

void _lock_acquire(_lock_t *lock) {
    lock_acquire_generic(lock, portMAX_DELAY, queueQUEUE_TYPE_MUTEX);
}

void _lock_acquire_recursive(_lock_t *lock) {
    lock_acquire_generic(lock, portMAX_DELAY, queueQUEUE_TYPE_RECURSIVE_MUTEX);
}

int _lock_try_acquire(_lock_t *lock) {
    return lock_acquire_generic(lock, 0, queueQUEUE_TYPE_MUTEX);
}

int _lock_try_acquire_recursive(_lock_t *lock) {
    return lock_acquire_generic(lock, 0, queueQUEUE_TYPE_RECURSIVE_MUTEX);
}

/* Release the mutex semaphore for lock.
   mutex_type is queueQUEUE_TYPE_RECURSIVE_MUTEX or queueQUEUE_TYPE_MUTEX
*/
static void lock_release_generic(_lock_t *lock, uint8_t mutex_type) {
    /* If application function has disabled interrupt, then it must not release the mutex */
    if (interrupt_is_disable() == true && !xPortInIsrContext())
        return ;

    xSemaphoreHandle h = (xSemaphoreHandle)(*lock);
    if (h == NULL) {
        /* This is probably because the scheduler isn't running yet,
           or the scheduler just started running and some code was
           "holding" a not-yet-initialised lock... */
        return;
    }

    if (xPortInIsrContext()) {
        if (mutex_type == queueQUEUE_TYPE_RECURSIVE_MUTEX) {
            abort(); /* indicates logic bug, it shouldn't be possible to lock recursively in ISR */
        }
        BaseType_t higher_task_woken = false;
        xSemaphoreGiveFromISR(h, &higher_task_woken);
        if (higher_task_woken) {
            portYIELD_FROM_ISR();
        }
    } else {
        if (mutex_type == queueQUEUE_TYPE_RECURSIVE_MUTEX) {
            xSemaphoreGiveRecursive(h);
        } else {
            xSemaphoreGive(h);
        }
    }
}

void _lock_release(_lock_t *lock) {
    lock_release_generic(lock, queueQUEUE_TYPE_MUTEX);
}

void _lock_release_recursive(_lock_t *lock) {
    lock_release_generic(lock, queueQUEUE_TYPE_RECURSIVE_MUTEX);
}
