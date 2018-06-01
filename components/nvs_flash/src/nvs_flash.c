#include "nvs_flash.h"


/**
 * @brief Initialize the default NVS partition.
 */
esp_err_t nvs_flash_init(void)
{
    return 0;
}

/**
 * @brief Initialize NVS flash storage for the specified partition.
 */
esp_err_t nvs_flash_init_partition(const char *partition_label)
{
    return 0;
}

/**
 * @brief Deinitialize NVS storage for the default NVS partition
 */
esp_err_t nvs_flash_deinit(void)
{
    return 0;
}

/**
 * @brief Deinitialize NVS storage for the given NVS partition
 */
esp_err_t nvs_flash_deinit_partition(const char* partition_label)
{
    return 0;
}

/**
 * @brief Erase the default NVS partition
 */
esp_err_t nvs_flash_erase(void)
{
    return 0;
}

/**
 * @brief Erase specified NVS partition
 */
esp_err_t nvs_flash_erase_partition(const char *part_name)
{
    return 0;
}
