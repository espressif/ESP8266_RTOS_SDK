#include "esp_common.h"
#include "testrunner.h"
#include <stdlib.h>
#include "spiffs_test_params.h"

enum {
	CMD_SPIFFS,
    CMD_END,
};

#define SSC_CMD_N   (CMD_END + 1)

LOCAL void spiffs_test_init(void);

LOCAL ssc_cmd_t sscCmdSet[SSC_CMD_N] =		{
    {"fs", CMD_T_SYNC,  CMD_SPIFFS, spiffs_test_init, NULL},
    {"",   CMD_T_ASYNC, CMD_END,    NULL,               NULL}
};

void spiffs_test_init(void)
{
    char *argv[10], pLine[128];
    int argc;

    strcpy(pLine, ssc_param_str());
    argc = ssc_parse_param(pLine, argv);

    run_tests(argc, argv);
}

void spiffs_test_help(void)
{
    printf("\nhelp:\n");
    printf("$ fs \n");
}

void spiffs_fs1_init(void)
{
    struct esp_spiffs_config config;

    config.phys_size = FS1_FLASH_SIZE;
    config.phys_addr = FS1_FLASH_ADDR;
    config.phys_erase_block = SECTOR_SIZE;
    config.log_block_size = LOG_BLOCK;
    config.log_page_size = LOG_PAGE;
    config.fd_buf_size = FD_BUF_SIZE * 2;
    config.cache_buf_size = CACHE_BUF_SIZE;

    esp_spiffs_init(&config);
}

void user_init(void)
{
    spiffs_fs1_init();

	ssc_attach(SSC_BR_74880);
    ssc_register(sscCmdSet, SSC_CMD_N, spiffs_test_help);
}

