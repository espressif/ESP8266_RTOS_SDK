// Copyright 2019 Espressif Systems (Shanghai) PTE LTD
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

#include "utils/includes.h"
#include "utils/common.h"
#include "rsn_supp/wpa.h"
#include "rsn_supp/wpa_i.h"
#include "common/eapol_common.h"
#include "common/ieee802_11_defs.h"
#include "rsn_supp/wpa_ie.h"
#include "crypto/crypto.h"
#include "crypto/sha1.h"
#include "crypto/aes_wrap.h"
#include "esp_supplicant/esp_wpas_glue.h"

#include "esp_wifi_driver.h"
#define LOCAL static

extern struct ieee80211_cipher tkip;
extern struct ieee80211_cipher ccmp;
extern struct ieee80211_cipher wep;

extern bool dhcpc_flag;


void  wpa_install_key(enum wpa_alg alg, u8 *addr, int key_idx, int set_tx,
                      u8 *seq, size_t seq_len, u8 *key, size_t key_len, int key_entry_valid)
{
    esp_wifi_set_sta_key_internal(alg, addr, key_idx, set_tx, seq, seq_len, key, key_len, key_entry_valid);
}

int  wpa_get_key(uint8_t *ifx, int *alg, u8 *addr, int *key_idx,
                 u8 *key, size_t key_len, int key_entry_valid)
{
    return esp_wifi_get_sta_key_internal(ifx, alg, addr, key_idx, key, key_len, key_entry_valid);
}

/**
 * eapol_sm_notify_eap_success - Notification of external EAP success trigger
 * @sm: Pointer to EAPOL state machine allocated with eapol_sm_init()
 * @success: %TRUE = set success, %FALSE = clear success
 *
 * Notify the EAPOL state machine that external event has forced EAP state to
 * success (success = %TRUE). This can be cleared by setting success = %FALSE.
 *
 * This function is called to update EAP state when WPA-PSK key handshake has
 * been completed successfully since WPA-PSK does not use EAP state machine.
 */

#include "esp_aio.h"
#define  EP_OFFSET 36
int ieee80211_output_pbuf(esp_aio_t *aio);
/* fix buf for tx for now */
#define WPA_TX_MSG_BUFF_MAXLEN 200

LOCAL int ICACHE_FLASH_ATTR wpa_send_cb(esp_aio_t* aio)
{
    char* pb = (char*)aio->arg;

    os_free(pb);

    return 0;
}

void  wpa_sendto_wrapper(void* dataptr, u16 datalen)
{
#ifndef IOT_SIP_MODE
    esp_aio_t aio;

    aio.arg = dataptr - EP_OFFSET;
    aio.cb = wpa_send_cb;
    aio.fd = 0;
    aio.pbuf = (char *)dataptr;
    aio.len = datalen;
    aio.ret = 0;

    if (ieee80211_output_pbuf(&aio) != 0) {
        os_free(dataptr - EP_OFFSET);
    }

#else
    esf_buf* eb = NULL;
    uint8_t* frm;

    eb = ieee80211_getmgtframe(&frm, sizeof(struct ieee80211_frame), WPA_TX_MSG_BUFF_MAXLEN);
    os_memcpy(frm, msg, msg_len);
//    eb->hdr_len = sizeof(struct ieee80211_frame);
    eb->data_len = msg_len;
    EBUF_START(eb) = frm;
    ieee80211_output_pbuf(ic->ic_if0_conn, eb);
#endif
}

void  wpa_deauthenticate(u8 reason_code)
{
    esp_wifi_deauthenticate_internal(reason_code);
}

void  wpa_config_profile(void)
{
    if (esp_wifi_sta_prof_is_wpa_internal()) {
        wpa_set_profile(WPA_PROTO_WPA, esp_wifi_sta_get_prof_authmode_internal());
    } else if (esp_wifi_sta_prof_is_wpa2_internal() || esp_wifi_sta_prof_is_wpa3_internal()) {
        wpa_set_profile(WPA_PROTO_RSN, esp_wifi_sta_get_prof_authmode_internal());
    } else {
        WPA_ASSERT(0);
    }
}

void wpa_config_bss(uint8_t *bssid)
{
    u8 mac[6];
    struct wifi_ssid *ssid = esp_wifi_sta_get_prof_ssid_internal();
    esp_wifi_get_mac(WIFI_IF_STA, mac);

    wpa_set_bss((char *)mac, (char *)bssid, esp_wifi_sta_get_pairwise_cipher_internal(), esp_wifi_sta_get_group_cipher_internal(),
                (char *)esp_wifi_sta_get_prof_password_internal(), ssid->ssid, ssid->len);
}

void  wpa_config_assoc_ie(u8 proto, u8 *assoc_buf, u32 assoc_wpa_ie_len)
{
    esp_wifi_set_appie_internal(proto, assoc_buf, assoc_wpa_ie_len, 0);
}

void  wpa_neg_complete(void)
{
    esp_wifi_auth_done_internal();
}

void ICACHE_FLASH_ATTR wpa_sta_init()
{
    wpa_register(NULL, wpa_sendto_wrapper,
                 wpa_config_assoc_ie, wpa_install_key, wpa_get_key, wpa_deauthenticate, wpa_neg_complete);
}

void  wpa_sta_connect(uint8_t *bssid)
{
    wpa_config_profile();
    wpa_config_bss(bssid);
}

