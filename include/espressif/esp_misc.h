/*
 *  Copyright (C) 2013 -2014  Espressif System
 *
 */

#ifndef __ESP_MISC_H__
#define __ESP_MISC_H__

#include "lwip/ip_addr.h"

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"

#define IP2STR(ipaddr) ip4_addr1_16(ipaddr), \
    ip4_addr2_16(ipaddr), \
    ip4_addr3_16(ipaddr), \
    ip4_addr4_16(ipaddr)

#define IPSTR "%d.%d.%d.%d"

void os_delay_us(uint16 us);

void os_install_putc1(void (*p)(char c));
void os_putc(char c);

enum dhcp_status{
	DHCP_STOPPED,
	DHCP_STARTED
};

struct dhcps_lease {
	struct ip_addr start_ip;
	struct ip_addr end_ip;
};

enum dhcps_offer_option{
	OFFER_START = 0x00,
	OFFER_ROUTER = 0x01,
	OFFER_END
};
#endif
