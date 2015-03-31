/*
 *  Copyright (C) 2013 -2014  Espressif System
 *
 */

#ifndef __ESP_SOFTAP_H__
#define __ESP_SOFTAP_H__

struct softap_config {
    uint8 ssid[32];
    uint8 password[64];
    uint8 ssid_len;
    uint8 channel;
    AUTH_MODE authmode;
    uint8 ssid_hidden;
    uint8 max_connection;
    uint16 beacon_interval;
};

bool wifi_softap_get_config(struct softap_config *config);
bool wifi_softap_set_config(struct softap_config *config);

#endif
