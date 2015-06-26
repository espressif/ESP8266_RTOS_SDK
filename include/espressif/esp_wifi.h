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

typedef enum _auth_mode {
    AUTH_OPEN           = 0,
    AUTH_WEP,
    AUTH_WPA_PSK,
    AUTH_WPA2_PSK,
    AUTH_WPA_WPA2_PSK,
    AUTH_MAX
} AUTH_MODE;

uint8 wifi_get_opmode(void);
uint8 wifi_get_opmode_default(void);
bool wifi_set_opmode(uint8 opmode);
bool wifi_set_opmode_current(uint8 opmode);

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
void wifi_status_led_uninstall(void);

bool wifi_promiscuous_set_mac(const uint8_t *address);

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

enum {
    EVENT_STAMODE_SCAN_DONE = 0,
    EVENT_STAMODE_CONNECTED,
    EVENT_STAMODE_DISCONNECTED,
    EVENT_STAMODE_AUTHMODE_CHANGE,
    EVENT_STAMODE_GOT_IP,
    EVENT_SOFTAPMODE_STACONNECTED,
    EVENT_SOFTAPMODE_STADISCONNECTED,
    EVENT_MAX
};

enum {
	REASON_UNSPECIFIED              = 1,
	REASON_AUTH_EXPIRE              = 2,
	REASON_AUTH_LEAVE               = 3,
	REASON_ASSOC_EXPIRE             = 4,
	REASON_ASSOC_TOOMANY            = 5,
	REASON_NOT_AUTHED               = 6,
	REASON_NOT_ASSOCED              = 7,
	REASON_ASSOC_LEAVE              = 8,
	REASON_ASSOC_NOT_AUTHED         = 9,
	REASON_DISASSOC_PWRCAP_BAD      = 10,
	REASON_DISASSOC_SUPCHAN_BAD     = 11,
	REASON_IE_INVALID               = 13,
	REASON_MIC_FAILURE              = 14,
	REASON_4WAY_HANDSHAKE_TIMEOUT   = 15,
	REASON_GROUP_KEY_UPDATE_TIMEOUT = 16,
	REASON_IE_IN_4WAY_DIFFERS       = 17,
	REASON_GROUP_CIPHER_INVALID     = 18,
	REASON_PAIRWISE_CIPHER_INVALID  = 19,
	REASON_AKMP_INVALID             = 20,
	REASON_UNSUPP_RSN_IE_VERSION    = 21,
	REASON_INVALID_RSN_IE_CAP       = 22,
	REASON_802_1X_AUTH_FAILED       = 23,
	REASON_CIPHER_SUITE_REJECTED    = 24,

	REASON_BEACON_TIMEOUT           = 200,
	REASON_NO_AP_FOUND              = 201,
};

typedef struct {
	uint32 status;
	struct bss_info *bss;
} Event_StaMode_ScanDone_t;

typedef struct {
	uint8 ssid[32];
	uint8 ssid_len;
	uint8 bssid[6];
	uint8 channel;
} Event_StaMode_Connected_t;

typedef struct {
	uint8 ssid[32];
	uint8 ssid_len;
	uint8 bssid[6];
	uint8 reason;
} Event_StaMode_Disconnected;

typedef struct {
	uint8 old_mode;
	uint8 new_mode;
} Event_StaMode_AuthMode_Change_t;

typedef struct {
	struct ip_addr ip;
	struct ip_addr mask;
	struct ip_addr gw;
} Event_StaMode_Got_IP_t;

typedef struct {
	uint8 mac[6];
	uint8 aid;
} Event_SoftAPMode_StaConnected_t;

typedef struct {
	uint8 mac[6];
	uint8 aid;
} Event_SoftAPMode_StaDisconnected_t;

typedef union {
	Event_StaMode_ScanDone_t			scan_done;
	Event_StaMode_Connected_t			connected;
	Event_StaMode_Disconnected			disconnected;
	Event_StaMode_AuthMode_Change_t		auth_change;
	Event_StaMode_Got_IP_t				got_ip;
	Event_SoftAPMode_StaConnected_t		sta_connected;
	Event_SoftAPMode_StaDisconnected_t	sta_disconnected;
} Event_Info_u;

typedef struct _esp_event {
    uint32 event_id;
    Event_Info_u event_info;
} System_Event_t;

typedef void (* wifi_event_handler_cb_t)(System_Event_t *event);

void wifi_set_event_handler_cb(wifi_event_handler_cb_t cb);

#endif
