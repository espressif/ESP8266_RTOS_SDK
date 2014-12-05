/*
 *  Copyright (C) 2013 -2014  Espressif System
 *
 */

#ifndef __ESP_WIFI_H__
#define __ESP_WIFI_H__

enum {
	NULL_MODE = 0,
	STATION_MODE,
	SOFTAP_MODE,
	STATIONAP_MODE,
	MAX_MODE
};

uint8 wifi_get_opmode(void);
bool wifi_set_opmode(uint8 opmode);

enum {
	STATION_IF = 0,
	SOFTAP_IF,
	MAX_IF
};

struct ip_info {
    struct ip_addr ip;
    struct ip_addr netmask;
    struct ip_addr gw;
};

bool wifi_get_ip_info(uint8 if_index, struct ip_info *info);
bool wifi_set_ip_info(uint8 if_index, struct ip_info *info);
bool wifi_get_macaddr(uint8 if_index, uint8 *macaddr);
bool wifi_set_macaddr(uint8 if_index, uint8 *macaddr);

uint8 wifi_get_channel(void);
bool wifi_set_channel(uint8 channel);

void wifi_status_led_install(uint8 gpio_id, uint32 gpio_name, uint8 gpio_func);

void wifi_promiscuous_enable(uint8 promiscuous);

typedef void (* wifi_promiscuous_cb_t)(uint8 *buf, uint16 len);

void wifi_set_promiscuous_rx_cb(wifi_promiscuous_cb_t cb);

enum phy_mode {
	PHY_MODE_11B	= 1,
	PHY_MODE_11G	= 2,
	PHY_MODE_11N    = 3
};

enum phy_mode wifi_get_phy_mode(void);
bool wifi_set_phy_mode(enum phy_mode mode);

#endif
