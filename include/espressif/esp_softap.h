/*
 *  Copyright (C) 2013 -2014  Espressif System
 *
 */

#ifndef __ESP_SOFTAP_H__
#define __ESP_SOFTAP_H__

#include "queue.h"

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

struct station_info {
    STAILQ_ENTRY(station_info)     next;

	u8 bssid[6];
	struct ip_addr ip;
};

bool wifi_softap_get_config(struct softap_config *config);
bool wifi_softap_get_config_default(struct softap_config *config);
bool wifi_softap_set_config(struct softap_config *config);
bool wifi_softap_set_config_current(struct softap_config *config);

uint8 wifi_softap_get_station_num(void);
struct station_info * wifi_softap_get_station_info(void);
void wifi_softap_free_station_info(void);

bool wifi_softap_dhcps_start(void);
bool wifi_softap_dhcps_stop(void);
enum dhcp_status wifi_softap_dhcps_status(void);
bool wifi_softap_set_dhcps_lease(struct dhcps_lease *please);
bool wifi_softap_set_dhcps_offer_option(uint8 level, void* optarg);

#endif
