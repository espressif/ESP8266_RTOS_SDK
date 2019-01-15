#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "unity.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"

#include "esp_log.h"
#include "esp_system.h"

// Pointers to the head and tail of linked list of test description structs:
static struct test_desc_t* s_unity_tests_first = NULL;
static struct test_desc_t* s_unity_tests_last = NULL;

// Inverse of the filter
static bool s_invert = false;

/* Each unit test is allowed to "leak" this many bytes.

   TODO: Make this value editable by the test.

   Will always need to be some value here, as fragmentation can reduce free space even when no leak is occuring.
*/
const size_t WARN_LEAK_THRESHOLD = 256;
const size_t CRITICAL_LEAK_THRESHOLD = 4096;

extern int uart_rx_one_char(char *c);

/* setUp runs before every test */
void setUp(void)
{
    printf("%s", ""); /* sneakily lazy-allocate the reent structure for this test task */
}

/* tearDown runs after every test */
void tearDown(void)
{
    /* some FreeRTOS stuff is cleaned up by idle task */
    vTaskDelay(5);

    /* We want the teardown to have this file in the printout if TEST_ASSERT fails */
    const char *real_testfile = Unity.TestFile;
    Unity.TestFile = __FILE__;

    Unity.TestFile = real_testfile; // go back to the real filename
}

void unity_putc(int c)
{
    if (c == '\n') 
    {
        putchar('\r');
        putchar('\n');
    }
    else if (c == '\r') 
    {
    }
    else 
    {
        putchar(c);
    }
}

void unity_flush()
{
//    uart_tx_wait_idle(0);   // assume that output goes to UART0
}

static int UART_RxString(char *s, size_t len)
{
    size_t i = 1;
    char *s_local = s;

    while (i < len) {
        while (uart_rx_one_char(s_local) != 0);

        if ((*s_local == '\n') || (*s_local == '\r')) {
            break;
        }

        s_local++;
        i++;
    }

    s_local++;
    *s_local = '\0';

    return 0;
}

void unity_testcase_register(struct test_desc_t* desc)
{
    if (!s_unity_tests_first)
    {
        s_unity_tests_first = desc;
        s_unity_tests_last = desc;
    }
    else
    {
        struct test_desc_t* temp = s_unity_tests_first;
        s_unity_tests_first = desc;
        s_unity_tests_first->next = temp;
    }
}

/* print the multiple function case name and its sub-menu
 * e.g:
 * (1) spi master/slave case
 *       (1)master case
 *       (2)slave case
 * */
static void print_multiple_function_test_menu(const struct test_desc_t* test_ms)
 {
    printf("%s\n", test_ms->name);
    for (int i = 0; i < test_ms->test_fn_count; i++)
    {
        printf("\t(%d)\t\"%s\"\n", i+1, test_ms->test_fn_name[i]);
    }
 }

void multiple_function_option(const struct test_desc_t* test_ms)
{
    int selection;
    char cmdline[256] = {0};

    print_multiple_function_test_menu(test_ms);
    while(strlen(cmdline) == 0)
    {
        /* Flush anything already in the RX buffer */
        while(uart_rx_one_char(cmdline) == 0);
        UART_RxString(cmdline, sizeof(cmdline) - 1);
        if(strlen(cmdline) == 0) {
            /* if input was newline, print a new menu */
            print_multiple_function_test_menu(test_ms);
        }
    }
    selection = atoi((const char *) cmdline) - 1;
    if(selection >= 0 && selection < test_ms->test_fn_count) {
        UnityDefaultTestRun(test_ms->fn[selection], test_ms->name, test_ms->line);
    } else {
        printf("Invalid selection, your should input number 1-%d!", test_ms->test_fn_count);
    }
}

static void unity_run_single_test(const struct test_desc_t* test)
{
    printf("Running %s...\n", test->name);
    // Unit test runner expects to see test name before the test starts
    fflush(stdout);

    Unity.TestFile = test->file;
    Unity.CurrentDetail1 = test->desc;
    if(test->test_fn_count == 1) {
        UnityDefaultTestRun(test->fn[0], test->name, test->line);
    } else {
        multiple_function_option(test);
    }
}

static void unity_run_single_test_by_index(int index)
{
    const struct test_desc_t* test;
    for (test = s_unity_tests_first; test != NULL && index != 0; test = test->next, --index)
    {

    }
    if (test != NULL)
    {
        unity_run_single_test(test);
    }
}

static void unity_run_single_test_by_index_parse(const char* filter, int index_max)
{
    if (s_invert)
    {
        printf("Inverse is not supported for that kind of filter\n");
        return;
    }
    int test_index = strtol(filter, NULL, 10);
    if (test_index >= 1 && test_index <= index_max)
    {
        extern uint32_t system_get_cpu_freq(void);

        uint32_t start;
        asm volatile ("rsr %0, CCOUNT" : "=r" (start));
        unity_run_single_test_by_index(test_index - 1);
        uint32_t end;
        asm volatile ("rsr %0, CCOUNT" : "=r" (end));
        uint32_t ms = (end - start) / (system_get_cpu_freq() * 1000000 / 1000);
        printf("Test ran in %dms\n", ms);
    }
}

static void unity_run_single_test_by_name(const char* filter)
{
    if (s_invert)
    {
        printf("Inverse is not supported for that kind of filter\n");
        return;
    }
    char tmp[256];
    strncpy(tmp, filter + 1, sizeof(tmp) - 1);
    tmp[strlen(filter) - 2] = 0;
    for (const struct test_desc_t* test = s_unity_tests_first; test != NULL; test = test->next)
    {
        if (strcmp(test->name, tmp) == 0)
        {
            unity_run_single_test(test);
        }
    }
}

void unity_run_all_tests()
{
    if (s_invert)
    {
        printf("Inverse is not supported for that kind of filter\n");
        return;
    }
    for (const struct test_desc_t* test = s_unity_tests_first; test != NULL; test = test->next)
    {
        unity_run_single_test(test);
    }
}

void unity_run_tests_with_filter(const char* filter)
{
    if (s_invert)
    {
        ++filter;
    }
    printf("Running tests %smatching '%s'...\n", s_invert ? "NOT " : "", filter);

    for (const struct test_desc_t* test = s_unity_tests_first; test != NULL; test = test->next)
    {
        if ((strstr(test->desc, filter) != NULL) == !s_invert)
        {
            unity_run_single_test(test);
        }
    }
}

static void trim_trailing_space(char* str)
{
    char* end = str + strlen(str) - 1;
    while (end >= str && isspace((int) *end))
    {
        *end = 0;
        --end;
    }
}

static int print_test_menu(void)
{
    int test_counter = 0;
    printf("\n\nHere's the test menu, pick your combo:\n");
    for (const struct test_desc_t* test = s_unity_tests_first;
         test != NULL;
         test = test->next, ++test_counter)
    {
        printf("(%d)\t\"%s\" %s\n", test_counter + 1, test->name, test->desc);
        if(test->test_fn_count > 1)
        {
            for (int i = 0; i < test->test_fn_count; i++)
            {
                printf("\t(%d)\t\"%s\"\n", i+1, test->test_fn_name[i]);
            }
         }
     }
     return test_counter;
}

static int get_test_count(void)
{
    int test_counter = 0;
    for (const struct test_desc_t* test = s_unity_tests_first;
         test != NULL;
         test = test->next)
    {
        ++test_counter;
    }
    return test_counter;
}

void unity_run_menu()
{
    printf("\n\nPress ENTER to see the list of tests.\n");
    int test_count = get_test_count();
    while (true)
    {
        char cmdline[256] = { 0 };
        while(strlen(cmdline) == 0)
        {
            /* Flush anything already in the RX buffer */
            while(uart_rx_one_char(cmdline) == 0);
            /* Read input */
            UART_RxString(cmdline, sizeof(cmdline) - 1);
            trim_trailing_space(cmdline);
            if(strlen(cmdline) == 0) {
                /* if input was newline, print a new menu */
                print_test_menu();
            }
        }
        /*use '-' to show test history. Need to do it before UNITY_BEGIN cleanup history */
        if (cmdline[0] == '-')
        {
            UNITY_END();
            continue;
        }

        UNITY_BEGIN();

        size_t idx = 0;
        if (cmdline[idx] == '!')
        {
            s_invert = true;
            ++idx;
        }
        else
        {
            s_invert = false;
        }

        if (cmdline[idx] == '*')
        {
            unity_run_all_tests();
        }
        else if (cmdline[idx] =='[')
        {
            unity_run_tests_with_filter(cmdline + idx);
        }
        else if (cmdline[idx] =='"')
        {
            unity_run_single_test_by_name(cmdline + idx);
        }
        else if (isdigit((unsigned char)cmdline[idx]))
        {
            unity_run_single_test_by_index_parse(cmdline + idx, test_count);
        }

        UNITY_END();

        printf("Enter next test, or 'enter' to see menu\n");
    }
}
