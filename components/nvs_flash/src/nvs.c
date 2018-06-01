#include "nvs.h"

esp_err_t nvs_open(const char* name, nvs_open_mode open_mode, nvs_handle *out_handle)
{
    return 0;
}

/**
 * @brief      Open non-volatile storage with a given namespace from specified partition
 */
esp_err_t nvs_open_from_partition(const char *part_name, const char* name, nvs_open_mode open_mode, nvs_handle *out_handle)
{
    return 0;
}

/**@{*/
/**
 * @brief      set value for given key
 */
esp_err_t nvs_set_i8  (nvs_handle handle, const char* key, int8_t value)
{
    return 0;
}
esp_err_t nvs_set_u8  (nvs_handle handle, const char* key, uint8_t value)
{
    return 0;
}
esp_err_t nvs_set_i16 (nvs_handle handle, const char* key, int16_t value)
{
    return 0;
}
esp_err_t nvs_set_u16 (nvs_handle handle, const char* key, uint16_t value)
{
    return 0;
}
esp_err_t nvs_set_i32 (nvs_handle handle, const char* key, int32_t value)
{
    return 0;
}
esp_err_t nvs_set_u32 (nvs_handle handle, const char* key, uint32_t value)
{
    return 0;
}
esp_err_t nvs_set_i64 (nvs_handle handle, const char* key, int64_t value)
{
    return 0;
}
esp_err_t nvs_set_u64 (nvs_handle handle, const char* key, uint64_t value)
{
    return 0;
}
esp_err_t nvs_set_str (nvs_handle handle, const char* key, const char* value)
{
    return 0;
}
/**@}*/ 

/**
 * @brief       set variable length binary value for given key
 */
esp_err_t nvs_set_blob(nvs_handle handle, const char* key, const void* value, size_t length)
{
    return 0;
}

/**@{*/
/**
 * @brief      get value for given key
 */
esp_err_t nvs_get_i8  (nvs_handle handle, const char* key, int8_t* out_value)
{
    return 0;
}
esp_err_t nvs_get_u8  (nvs_handle handle, const char* key, uint8_t* out_value)
{
    return 0;
}
esp_err_t nvs_get_i16 (nvs_handle handle, const char* key, int16_t* out_value)
{
    return 0;
}
esp_err_t nvs_get_u16 (nvs_handle handle, const char* key, uint16_t* out_value)
{
    return 0;
}
esp_err_t nvs_get_i32 (nvs_handle handle, const char* key, int32_t* out_value)
{
    return 0;
}
esp_err_t nvs_get_u32 (nvs_handle handle, const char* key, uint32_t* out_value)
{
    return 0;
}
esp_err_t nvs_get_i64 (nvs_handle handle, const char* key, int64_t* out_value)
{
    return 0;
}
esp_err_t nvs_get_u64 (nvs_handle handle, const char* key, uint64_t* out_value)
{
    return 0;
}
/**@}*/ 

/**
 * @brief      get value for given key
 */
/**@{*/
esp_err_t nvs_get_str (nvs_handle handle, const char* key, char* out_value, size_t* length)
{
    return 0;
}
esp_err_t nvs_get_blob(nvs_handle handle, const char* key, void* out_value, size_t* length)
{
    return 0;
}
/**@}*/

/**
 * @brief      Erase key-value pair with given key name.
 */
esp_err_t nvs_erase_key(nvs_handle handle, const char* key)
{
    return 0;
}

/**
 * @brief      Erase all key-value pairs in a namespace
 */
esp_err_t nvs_erase_all(nvs_handle handle)
{
    return 0;
}

/**
 * @brief      Write any pending changes to non-volatile storage
 */
esp_err_t nvs_commit(nvs_handle handle)
{
    return 0;
}

/**
 * @brief      Close the storage handle and free any allocated resources
 */
void nvs_close(nvs_handle handle)
{

}


/**
 * @brief      Fill structure nvs_stats_t. It provides info about used memory the partition.
 */
esp_err_t nvs_get_stats(const char* part_name, nvs_stats_t* nvs_stats)
{
    return 0;
}

/**
 * @brief      Calculate all entries in a namespace.
 */
esp_err_t nvs_get_used_entry_count(nvs_handle handle, size_t* used_entries)
{
    return 0;
}
