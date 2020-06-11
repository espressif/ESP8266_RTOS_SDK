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

#ifndef _ESP_WIFI_DRIVER_H_
#define _ESP_WIFI_DRIVER_H_

#include "esp_err.h"
#include "esp_wifi.h"

enum {
    NONE_AUTH           = 0x01,
    WPA_AUTH_UNSPEC     = 0x02,
    WPA_AUTH_PSK        = 0x03,
    WPA2_AUTH_ENT       = 0x04,
    WPA2_AUTH_PSK       = 0x05,
    WPA_AUTH_CCKM       = 0x06,
    WPA2_AUTH_CCKM      = 0x07,
    WPA2_AUTH_PSK_SHA256= 0x08,
    WPA3_AUTH_PSK       = 0x09,
    WPA2_AUTH_INVALID   = 0x0a,
};

struct wifi_ssid {
    int len;
    uint8_t ssid[32];
};

#define WPA_IGTK_LEN 16
typedef struct {
    uint8_t keyid[2];
    uint8_t pn[6];
    uint8_t igtk[WPA_IGTK_LEN];
} wifi_wpa_igtk_t;

/*wpa_auth.c*/
uint8_t *esp_wifi_ap_get_prof_pmk_internal(void);
void *esp_wifi_get_hostap_private_internal(void);
int esp_wifi_set_ap_key_internal(int alg, const u8 *addr, int idx, u8 *key, size_t key_len);
bool esp_wifi_wpa_ptk_init_done_internal(uint8_t *mac);

/*wpa.c*/

bool esp_wifi_sta_is_ap_notify_completed_rsne_internal(void);
struct wifi_ssid *esp_wifi_sta_get_prof_ssid_internal(void);
uint8_t *esp_wifi_sta_get_prof_password_internal(void);
uint8_t *esp_wifi_sta_get_prof_pmk_internal(void);
uint8_t esp_wifi_sta_get_reset_param_internal(void);
uint8_t esp_wifi_sta_set_reset_param_internal(uint8_t reset_flag);
int esp_wifi_sta_update_ap_info_internal(void);
bool esp_wifi_sta_is_running_internal(void);

int esp_wifi_get_macaddr_internal(uint8_t if_index, uint8_t *macaddr);
uint8_t *esp_wifi_sta_get_ap_info_prof_pmk_internal(void);

/*wpa_main.c*/
int esp_wifi_set_sta_key_internal(int alg, u8 *addr, int key_idx, int set_tx,
                                  u8 *seq, size_t seq_len, u8 *key, size_t key_len, int key_entry_valid);
int  esp_wifi_get_sta_key_internal(uint8_t *ifx, int *alg, u8 *addr, int *key_idx,
                                   u8 *key, size_t key_len, int key_entry_valid);
uint8_t esp_wifi_sta_get_prof_authmode_internal(void);
bool esp_wifi_sta_prof_is_wpa_internal(void);
bool esp_wifi_sta_prof_is_wpa2_internal(void);
bool esp_wifi_sta_prof_is_wpa3_internal(void);
void esp_wifi_deauthenticate_internal(u8 reason_code);
uint8_t esp_wifi_sta_get_pairwise_cipher_internal(void);
uint8_t esp_wifi_sta_get_group_cipher_internal(void);
int esp_wifi_set_appie_internal(uint8_t type, uint8_t *ie, uint16_t len, uint8_t flag);
bool esp_wifi_auth_done_internal(void);
uint16_t esp_wifi_sta_pmf_enabled(void);
wifi_cipher_type_t esp_wifi_sta_get_mgmt_group_cipher(void);
int esp_wifi_set_igtk_internal(uint8_t if_index, const wifi_wpa_igtk_t *igtk);

#endif
