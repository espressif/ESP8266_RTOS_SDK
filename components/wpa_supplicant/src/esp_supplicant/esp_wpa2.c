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

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "esp_err.h"

#include "utils/includes.h"
#include "utils/common.h"
#include "utils/wpa_debug.h"
#include "common/wpa_ctrl.h"
#include "common/eapol_common.h"
#include "common/ieee802_11_defs.h"
#include "utils/state_machine.h"
#include "rsn_supp/wpa.h"

#include "crypto/crypto.h"

#include "utils/ext_password.h"
#include "tls/tls.h"
#include "eap_peer/eap_i.h"
#include "eap_peer/eap_config.h"
#include "eap_peer/eap.h"
#include "eap_peer/eap_tls.h"
#ifdef EAP_PEER_METHOD
#include "eap_peer/eap_methods.h"
#endif

#include "esp_wifi_driver.h"
#include "esp_wpa_err.h"

#define WPA2_VERSION    "v2.0"

static bool s_disable_time_check = true;

extern bool wifi_unregister_wpa2_cb(void);

esp_err_t esp_wifi_sta_wpa2_ent_set_cert_key(const unsigned char *client_cert, int client_cert_len, const unsigned char *private_key, int private_key_len, const unsigned char *private_key_passwd, int private_key_passwd_len)
{
    if (client_cert && client_cert_len > 0) {
        g_wpa_client_cert = client_cert;
        g_wpa_client_cert_len = client_cert_len;
    }
    if (private_key && private_key_len > 0) {
        g_wpa_private_key = private_key;
        g_wpa_private_key_len = private_key_len;
    }
    if (private_key_passwd && private_key_passwd_len > 0) {
        g_wpa_private_key_passwd = private_key_passwd;
        g_wpa_private_key_passwd_len = private_key_passwd_len;
    }

    return ESP_OK;
}

void esp_wifi_sta_wpa2_ent_clear_cert_key(void)
{
    wifi_unregister_wpa2_cb();

    g_wpa_client_cert = NULL;
    g_wpa_client_cert_len = 0;
    g_wpa_private_key = NULL;
    g_wpa_private_key_len = 0;
    g_wpa_private_key_passwd = NULL;
    g_wpa_private_key_passwd_len = 0;
}

esp_err_t esp_wifi_sta_wpa2_ent_set_ca_cert(const unsigned char *ca_cert, int ca_cert_len)
{
    if (ca_cert && ca_cert_len > 0) {
        g_wpa_ca_cert = ca_cert;
        g_wpa_ca_cert_len = ca_cert_len;
    }

    return ESP_OK;
}

void esp_wifi_sta_wpa2_ent_clear_ca_cert(void)
{
    g_wpa_ca_cert = NULL;
    g_wpa_ca_cert_len = 0;
}

#define ANONYMOUS_ID_LEN_MAX 128
esp_err_t esp_wifi_sta_wpa2_ent_set_identity(const unsigned char *identity, int len)
{
    if (len <= 0 || len > ANONYMOUS_ID_LEN_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    if (g_wpa_anonymous_identity) {
        os_free(g_wpa_anonymous_identity);
        g_wpa_anonymous_identity = NULL;
    }

    g_wpa_anonymous_identity = (u8 *)os_zalloc(len);
    if (g_wpa_anonymous_identity == NULL) {
        return ESP_ERR_NO_MEM;
    }

    os_memcpy(g_wpa_anonymous_identity, identity, len);
    g_wpa_anonymous_identity_len = len;

    return ESP_OK;
}

void esp_wifi_sta_wpa2_ent_clear_identity(void)
{
    if (g_wpa_anonymous_identity) {
        os_free(g_wpa_anonymous_identity);
    }

    g_wpa_anonymous_identity = NULL;
    g_wpa_anonymous_identity_len = 0;
}

#define USERNAME_LEN_MAX 128
esp_err_t esp_wifi_sta_wpa2_ent_set_username(const unsigned char *username, int len)
{
    if (len <= 0 || len > USERNAME_LEN_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    if (g_wpa_username) {
        os_free(g_wpa_username);
        g_wpa_username = NULL;
    }

    g_wpa_username = (u8 *)os_zalloc(len);
    if (g_wpa_username == NULL) {
        return ESP_ERR_NO_MEM;
    }

    os_memcpy(g_wpa_username, username, len);
    g_wpa_username_len = len;

    return ESP_OK;
}

void esp_wifi_sta_wpa2_ent_clear_username(void)
{
    if (g_wpa_username) {
        os_free(g_wpa_username);
    }

    g_wpa_username = NULL;
    g_wpa_username_len = 0;
}

esp_err_t esp_wifi_sta_wpa2_ent_set_password(const unsigned char *password, int len)
{
    if (len <= 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (g_wpa_password) {
        os_free(g_wpa_password);
        g_wpa_password = NULL;
    }

    g_wpa_password = (u8 *)os_zalloc(len);
    if (g_wpa_password == NULL) {
        return ESP_ERR_NO_MEM;
    }

    os_memcpy(g_wpa_password, password, len);
    g_wpa_password_len = len;

    return ESP_OK;
}

void esp_wifi_sta_wpa2_ent_clear_password(void)
{
    if (g_wpa_password) {
        os_free(g_wpa_password);
    }
    g_wpa_password = NULL;
    g_wpa_password_len = 0;
}

esp_err_t esp_wifi_sta_wpa2_ent_set_new_password(const unsigned char *new_password, int len)
{
    if (len <= 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (g_wpa_new_password) {
        os_free(g_wpa_new_password);
        g_wpa_new_password = NULL;
    }

    g_wpa_new_password = (u8 *)os_zalloc(len);
    if (g_wpa_new_password == NULL) {
        return ESP_ERR_NO_MEM;
    }

    os_memcpy(g_wpa_new_password, new_password, len);
    g_wpa_password_len = len;

    return ESP_OK;
}

void esp_wifi_sta_wpa2_ent_clear_new_password(void)
{
    if (g_wpa_new_password) {
        os_free(g_wpa_new_password);
    }
    g_wpa_new_password = NULL;
    g_wpa_new_password_len = 0;
}

esp_err_t esp_wifi_sta_wpa2_ent_set_disable_time_check(bool disable)
{
    s_disable_time_check = disable;
    return ESP_OK;
}

bool wifi_sta_get_enterprise_disable_time_check(void)
{
    return s_disable_time_check;
}

esp_err_t esp_wifi_sta_wpa2_ent_get_disable_time_check(bool *disable)
{
    *disable = wifi_sta_get_enterprise_disable_time_check();
    return ESP_OK;
}

