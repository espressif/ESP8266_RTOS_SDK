/*
 * wpa_supplicant/hostapd / Debug prints
 * Copyright (c) 2002-2007, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */
#ifdef EMBEDDED_SUPP
#include "rom/ets_sys.h"
#include "utils/includes.h"
#include "utils/common.h"

#if 0//def DEBUG_PRINT
int ICACHE_FLASH_ATTR wpa_snprintf_hex(char* buf, size_t buf_size, const u8* data, size_t len)
{
    return 0;
}

void ICACHE_FLASH_ATTR wpa_debug_print_timestamp(void)
{
#ifdef DEBUG_PRINT
    struct os_time tv;
    os_get_time(&tv);
    wpa_printf(MSG_DEBUG, "%ld.%06u: ", (long) tv.sec, (unsigned int) tv.usec);
#endif
}

void ICACHE_FLASH_ATTR wpa_hexdump(int level, const char* title, const u8* buf, size_t len)
{
#ifdef DEBUG_PRINT
    size_t i;

    if (level < MSG_MSGDUMP) {
        return;
    }

    wpa_printf(MSG_DEBUG, "%s - hexdump(len=%lu):", title, (unsigned long) len);

    if (buf == NULL) {
        wpa_printf(MSG_DEBUG, " [NULL]");
    } else {
        for (i = 0; i < len; i++) {
            wpa_printf(MSG_DEBUG, " %02x", buf[i]);

            if ((i + 1) % 16 == 0) {
                wpa_printf(MSG_DEBUG, "\n");
            }
        }
    }

    wpa_printf(MSG_DEBUG, "");
#endif
}

void ICACHE_FLASH_ATTR wpa_hexdump_key(int level, const char* title, const u8* buf, size_t len)
{
    wpa_hexdump(level, title, buf, len);
}
#endif

int ICACHE_FLASH_ATTR eloop_cancel_timeout(eloop_timeout_handler handler,
        void* eloop_data, void* user_data)
{
    return 0;
}

int ICACHE_FLASH_ATTR eloop_register_timeout(unsigned int secs, unsigned int usecs,
        eloop_timeout_handler handler,
        void* eloop_data, void* user_data)
{
    return 0;
}
#endif // EMBEDDED_SUPP

