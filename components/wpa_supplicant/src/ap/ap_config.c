/*
 * hostapd / Configuration helper functions
 * Copyright (c) 2003-2012, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "utils/includes.h"

#include "utils/common.h"
#include "crypto/sha1.h"
//#include "radius/radius_client.h"
#include "common/ieee802_11_defs.h"
#include "common/eapol_common.h"
//#include "eap_common/eap_wsc_common.h"
//#include "eap_server/eap.h"
#include "ap/wpa_auth.h"
//#include "sta_info.h"
#include "ap/ap_config.h"
#include "utils/wpa_debug.h"
#include "esp_supplicant/esp_wifi_driver.h"

#ifdef MEMLEAK_DEBUG
static const char mem_debug_file[] ICACHE_RODATA_ATTR = __FILE__;
#endif


static int ICACHE_FLASH_ATTR hostapd_derive_psk(struct hostapd_ssid* ssid)
{
    ssid->wpa_psk = (struct hostapd_wpa_psk*)os_zalloc(sizeof(struct hostapd_wpa_psk));

    if (ssid->wpa_psk == NULL) {
        wpa_printf(MSG_ERROR, "Unable to alloc space for PSK");
        return -1;
    }

    wpa_hexdump_ascii(MSG_DEBUG, "SSID",
                      (u8*) ssid->ssid, ssid->ssid_len);
    wpa_hexdump_ascii_key(MSG_DEBUG, "PSK (ASCII passphrase)",
                          (u8*) ssid->wpa_passphrase,
                          os_strlen(ssid->wpa_passphrase));
    /* It's too SLOW */
//	pbkdf2_sha1(ssid->wpa_passphrase,
//		    ssid->ssid, ssid->ssid_len,
//		    4096, ssid->wpa_psk->psk, PMK_LEN);
    os_memcpy(ssid->wpa_psk->psk, esp_wifi_ap_get_prof_pmk_internal(), PMK_LEN);
    wpa_hexdump_key(MSG_DEBUG, "PSK (from passphrase)",
                    ssid->wpa_psk->psk, PMK_LEN);
    return 0;
}


int ICACHE_FLASH_ATTR hostapd_setup_wpa_psk(struct hostapd_bss_config* conf)
{
    struct hostapd_ssid* ssid = &conf->ssid;

    if (ssid->wpa_passphrase != NULL) {
        if (ssid->wpa_psk != NULL) {
            wpa_printf(MSG_DEBUG, "Using pre-configured WPA PSK "
                       "instead of passphrase");
        } else {
            wpa_printf(MSG_DEBUG, "Deriving WPA PSK based on "
                       "passphrase");

            if (hostapd_derive_psk(ssid) < 0) {
                return -1;
            }
        }

        ssid->wpa_psk->group = 1;
    }

#if 0

    if (ssid->wpa_psk_file) {
        if (hostapd_config_read_wpa_psk(ssid->wpa_psk_file,
                                        &conf->ssid)) {
            return -1;
        }
    }

#endif

    return 0;
}

const u8* ICACHE_FLASH_ATTR hostapd_get_psk(const struct hostapd_bss_config* conf,
        const u8* addr, const u8* prev_psk)
{
    struct hostapd_wpa_psk* psk;
    int next_ok = prev_psk == NULL;

    for (psk = conf->ssid.wpa_psk; psk != NULL; psk = psk->next) {
        if (next_ok &&
                (psk->group || os_memcmp(psk->addr, addr, ETH_ALEN) == 0)) {
            return psk->psk;
        }

        if (psk->psk == prev_psk) {
            next_ok = 1;
        }
    }

    return NULL;
}
