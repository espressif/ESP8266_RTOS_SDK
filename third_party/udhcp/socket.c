/* vi: set sw=4 ts=4: */
/*
 * DHCP server client/server socket creation
 *
 * udhcp client/server
 * Copyright (C) 1999 Matthew Ramsay <matthewr@moreton.com.au>
 *			Chris Trew <ctrew@moreton.com.au>
 *
 * Rewrite by Russ Dill <Russ.Dill@asu.edu> July 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "udhcp/common.h"

int ICACHE_FLASH_ATTR udhcp_read_interface(const char *interface, int *ifindex, uint32_t *nip, uint8_t *mac)
{
	struct ip_info info;
	if (wifi_get_opmode() == NULL_MODE) {
		UDHCP_DEBUG("wifi's opmode is invalid\n");
		return 1;
	}

	if (nip) {
		wifi_get_ip_info(SOFTAP_IF,&info);
		*nip = info.ip.addr;
		UDHCP_DEBUG("IP %s\n", inet_ntoa(info.ip));
	}

	if (ifindex) {

		if (wifi_get_opmode() == SOFTAP_MODE || wifi_get_opmode() == STATIONAP_MODE) {
			*ifindex = SOFTAP_IF;
		} else {
			*ifindex = STATION_IF;
		}
	}

	if (mac) {

		if (wifi_get_opmode() == SOFTAP_MODE || wifi_get_opmode() == STATIONAP_MODE) {
			wifi_get_macaddr(SOFTAP_IF, mac);
		}
		UDHCP_DEBUG("MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}

	return 0;
}

/* 1. None of the callers expects it to ever fail */
/* 2. ip was always INADDR_ANY */
int ICACHE_FLASH_ATTR udhcp_listen_socket(/*uint32_t ip,*/ int port, const char *inf)
{
	int fd;
	int recv_timeout = 10;
	struct sockaddr_in addr;

	UDHCP_DEBUG("Opening listen socket on *:%d %s\n", port, inf);

	fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htons(INADDR_ANY);
	addr.sin_port = htons(port);
	/* addr.sin_addr.s_addr = ip; - all-zeros is INADDR_ANY */
	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		UDHCP_DEBUG("bind port %d failed\n", port);

	setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,(void *)&recv_timeout,sizeof(recv_timeout));

	return fd;
}
