/* mbedTLS AES performance test
*/
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/errno.h>
#include "unity.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

void __set_errno(int i)
{
    errno = i;
}

void __get_errno(int *i)
{
    *i = errno;
}

static void errno_check_task(void *p)
{
    SemaphoreHandle_t wait_sem = p;

    for (volatile int i = 0; i < 10 * 1000 * 1000; i++) {
        int j;

        __set_errno(i);
        __get_errno(&j);
        assert(i == j);
    }

    xSemaphoreGive(wait_sem);

    vTaskDelete(NULL);
}

TEST_CASE("thread local errno", "[reent]")
{
    SemaphoreHandle_t wait1_sem, wait2_sem;

    wait1_sem = xSemaphoreCreateBinary();
    wait2_sem = xSemaphoreCreateBinary();

    xTaskCreatePinnedToCore(&errno_check_task, "errno1", 2048, wait1_sem, 3, NULL, cpuid_0);
    xTaskCreatePinnedToCore(&errno_check_task, "errno2", 2048, wait2_sem, 3, NULL, cpuid_1);

    xSemaphoreTake(wait1_sem, portMAX_DELAY);
    xSemaphoreTake(wait2_sem, portMAX_DELAY);

    vQueueDelete(wait1_sem);
    vQueueDelete(wait2_sem);
}


