/* vi: set sw=4 ts=4: */
/*
 * udhcp server
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

//usage:#define udhcpd_trivial_usage
//usage:       "[-fS] [-I ADDR]" IF_FEATURE_UDHCP_PORT(" [-P N]") " [CONFFILE]"
//usage:#define udhcpd_full_usage "\n\n"
//usage:       "DHCP server\n"
//usage:     "\n	-f	Run in foreground"
//usage:     "\n	-S	Log to syslog too"
//usage:     "\n	-I ADDR	Local address"
//usage:	IF_FEATURE_UDHCP_PORT(
//usage:     "\n	-P N	Use port N (default 67)"
//usage:	)

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "udhcp/common.h"
#include "udhcp/dhcpc.h"
#include "udhcp/dhcpd.h"

int errno;
#define UDHCPD 0

extern struct sockaddr dest_sin;
extern socklen_t dest_length;
static struct dhcp_packet pdhcp_pkt;
static uint32 dhcp_pkt_len;

static xQueueHandle QueueStop = NULL;

enum logtype {
	LOGHEX,
	LOGSTR
};

void ICACHE_FLASH_ATTR log_info(uint8* infobuf, uint32 length, enum logtype log_type)
{
	uint32 i = 0;
	if (log_type == LOGHEX){
		for (i = 0; i < length; i ++){
			UDHCP_DEBUG("%2x ", infobuf[i]);
			if ((i+1)%16==0)
				UDHCP_DEBUG("\n");
		}
		UDHCP_DEBUG("\n");
	} else if (log_type == LOGSTR){
		UDHCP_DEBUG("%s\n", infobuf);
	}
}

/* Send a packet to a specific mac address and ip address by creating our own ip packet */
static void ICACHE_FLASH_ATTR send_packet_to_client(int s, struct dhcp_packet *dhcp_pkt, int force_broadcast)
{
	const uint8_t *chaddr;
	uint32_t ciaddr;
	unsigned padding;
	int fd;
	int result = -1;
	const char *msg;
	struct sockaddr_in dest_addr;

    bzero(&dest_addr, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(CLIENT_PORT);

	if (force_broadcast
	 || (dhcp_pkt->flags & htons(BROADCAST_FLAG))
	 || dhcp_pkt->ciaddr == 0
	) {
		UDHCP_DEBUG("Broadcasting packet to client\n");
		ciaddr = INADDR_BROADCAST;
		chaddr = MAC_BCAST_ADDR;
		dest_addr.sin_addr.s_addr = INADDR_BROADCAST;
	} else {
		UDHCP_DEBUG("Unicasting packet to client ciaddr\n");
		ciaddr = dhcp_pkt->ciaddr;
		chaddr = dhcp_pkt->chaddr;
		dest_addr.sin_addr.s_addr = htons(ciaddr);
	}

	memcpy(&dest_sin, &dest_addr, dest_length);
	padding = DHCP_OPTIONS_BUFSIZE - 1 - udhcp_end_option(dhcp_pkt->options);
	if (padding > DHCP_SIZE - 300)
		padding = DHCP_SIZE - 300;

	dhcp_pkt_len = DHCP_SIZE - padding;
	log_info((uint8*)dhcp_pkt, dhcp_pkt_len, LOGHEX);

	memset(&pdhcp_pkt, 0, DHCP_SIZE);
	memcpy(&pdhcp_pkt , dhcp_pkt, dhcp_pkt_len);
//	result = safe_write(s, dhcp_pkt, DHCP_SIZE - padding);
//	if (result < 0)
//		printf("safe_write function send data failed\n");
//	else
//		printf("safe_write function send data size %d\n", result);
}

/* Send a packet to gateway_nip using the kernel ip stack */
static void ICACHE_FLASH_ATTR send_packet_to_relay(struct dhcp_packet *dhcp_pkt)
{
	UDHCP_DEBUG("Forwarding packet to relay\n");

	udhcp_send_kernel_packet(dhcp_pkt,
			server_config.server_nip, SERVER_PORT,
			dhcp_pkt->gateway_nip, SERVER_PORT);
}

static void ICACHE_FLASH_ATTR send_packet(int s, struct dhcp_packet *dhcp_pkt, int force_broadcast)
{
	if (dhcp_pkt->gateway_nip)
		send_packet_to_relay(dhcp_pkt);
	else
		send_packet_to_client(s, dhcp_pkt, force_broadcast);
}

static void ICACHE_FLASH_ATTR init_packet(struct dhcp_packet *packet, struct dhcp_packet *oldpacket, char type)
{
	/* Sets op, htype, hlen, cookie fields
	 * and adds DHCP_MESSAGE_TYPE option */
	udhcp_init_header(packet, type);

	packet->xid = oldpacket->xid;
	memcpy(packet->chaddr, oldpacket->chaddr, sizeof(oldpacket->chaddr));
	packet->flags = oldpacket->flags;
	packet->gateway_nip = oldpacket->gateway_nip;
	packet->ciaddr = oldpacket->ciaddr;
	udhcp_add_simple_option(packet, DHCP_SERVER_ID, server_config.server_nip);
}

/* Fill options field, siaddr_nip, and sname and boot_file fields.
 * TODO: teach this code to use overload option.
 */
static void ICACHE_FLASH_ATTR add_server_options(struct dhcp_packet *packet)
{
	struct option_set *curr = server_config.options;

	while (curr) {
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
			udhcp_add_binary_option(packet, curr->data);
		curr = curr->next;
	}

	packet->siaddr_nip = server_config.siaddr_nip;

	if (server_config.sname)
		strncpy((char*)packet->sname, server_config.sname, sizeof(packet->sname) - 1);
	if (server_config.boot_file)
		strncpy((char*)packet->file, server_config.boot_file, sizeof(packet->file) - 1);
}

static uint32_t ICACHE_FLASH_ATTR select_lease_time(struct dhcp_packet *packet)
{
	uint32_t lease_time_sec = server_config.max_lease_sec;
	uint8_t *lease_time_opt = udhcp_get_option(packet, DHCP_LEASE_TIME);
	if (lease_time_opt) {
		move_from_unaligned32(lease_time_sec, lease_time_opt);
		lease_time_sec = ntohl(lease_time_sec);
		if (lease_time_sec > server_config.max_lease_sec)
			lease_time_sec = server_config.max_lease_sec;
		if (lease_time_sec < server_config.min_lease_sec)
			lease_time_sec = server_config.min_lease_sec;
	}
	return lease_time_sec;
}

/* We got a DHCP DISCOVER. Send an OFFER. */
/* NOINLINE: limit stack usage in caller */
static void ICACHE_FLASH_ATTR send_offer(int s, struct dhcp_packet *oldpacket,
		uint32_t static_lease_nip,
		struct dyn_lease *lease,
		uint8_t *requested_ip_opt)
{
	struct dhcp_packet packet;
	uint32_t lease_time_sec;
	struct in_addr addr;

	init_packet(&packet, oldpacket, DHCPOFFER);

	/* If it is a static lease, use its IP */
	packet.yiaddr = static_lease_nip;
	/* Else: */
	if (!static_lease_nip) {
		/* We have no static lease for client's chaddr */
		uint32_t req_nip;
		const char *p_host_name;

		if (lease) {
			/* We have a dynamic lease for client's chaddr.
			 * Reuse its IP (even if lease is expired).
			 * Note that we ignore requested IP in this case.
			 */
			packet.yiaddr = lease->lease_nip;
		}
		/* Or: if client has requested an IP */
		else if (requested_ip_opt != NULL
		 /* (read IP) */
		 && (move_from_unaligned32(req_nip, requested_ip_opt), 1)
		 /* and the IP is in the lease range */
		 && ntohl(req_nip) >= server_config.start_ip
		 && ntohl(req_nip) <= server_config.end_ip
		 /* and */
		 && (  !(lease = find_lease_by_nip(req_nip)) /* is not already taken */
		    || is_expired_lease(lease) /* or is taken, but expired */
		    )
		) {
			packet.yiaddr = req_nip;
		}
		else {
			/* Otherwise, find a free IP */
			packet.yiaddr = find_free_or_expired_nip(oldpacket->chaddr);
		}

		if (!packet.yiaddr) {
			UDHCP_DEBUG("no free IP addresses. OFFER abandoned\n");
			return;
		}
		/* Reserve the IP for a short time hoping to get DHCPREQUEST soon */
		p_host_name = (const char*) udhcp_get_option(oldpacket, DHCP_HOST_NAME);
		lease = add_lease(packet.chaddr, packet.yiaddr,
				server_config.offer_time,
				p_host_name,
				p_host_name ? (unsigned char)p_host_name[OPT_LEN - OPT_DATA] : 0
		);
		if (!lease) {
			UDHCP_DEBUG("no free IP addresses. OFFER abandoned\n");
			return;
		}
	}

	lease_time_sec = select_lease_time(oldpacket);
	udhcp_add_simple_option(&packet, DHCP_LEASE_TIME, htonl(lease_time_sec));
	add_server_options(&packet);

	addr.s_addr = packet.yiaddr;
	UDHCP_DEBUG("Sending OFFER of %s\n", inet_ntoa(addr));
	/* send_packet emits error message itself if it detects failure */
	send_packet(s, &packet, /*force_bcast:*/ 0);
}

/* NOINLINE: limit stack usage in caller */
static void ICACHE_FLASH_ATTR send_NAK(int s, struct dhcp_packet *oldpacket)
{
	struct dhcp_packet packet;

	init_packet(&packet, oldpacket, DHCPNAK);

	UDHCP_DEBUG("Sending NAK\n");
	send_packet(s, &packet, /*force_bcast:*/ 1);
}

/* NOINLINE: limit stack usage in caller */
static void ICACHE_FLASH_ATTR send_ACK(int s, struct dhcp_packet *oldpacket, uint32_t yiaddr)
{
	struct dhcp_packet packet;
	uint32_t lease_time_sec;
	struct in_addr addr;
	const char *p_host_name;

	init_packet(&packet, oldpacket, DHCPACK);
	packet.yiaddr = yiaddr;

	lease_time_sec = select_lease_time(oldpacket);
	udhcp_add_simple_option(&packet, DHCP_LEASE_TIME, htonl(lease_time_sec));

	add_server_options(&packet);

	addr.s_addr = yiaddr;
	UDHCP_DEBUG("Sending ACK to %s\n", inet_ntoa(addr));
	send_packet(s, &packet, /*force_bcast:*/ 0);

	p_host_name = (const char*) udhcp_get_option(oldpacket, DHCP_HOST_NAME);
	add_lease(packet.chaddr, packet.yiaddr,
		lease_time_sec,
		p_host_name,
		p_host_name ? (unsigned char)p_host_name[OPT_LEN - OPT_DATA] : 0
	);
	if (ENABLE_FEATURE_UDHCPD_WRITE_LEASES_EARLY) {
		/* rewrite the file with leases at every new acceptance */
		write_leases();
	}
}

/* NOINLINE: limit stack usage in caller */
static void ICACHE_FLASH_ATTR send_inform(int s, struct dhcp_packet *oldpacket)
{
	struct dhcp_packet packet;

	/* "If a client has obtained a network address through some other means
	 * (e.g., manual configuration), it may use a DHCPINFORM request message
	 * to obtain other local configuration parameters.  Servers receiving a
	 * DHCPINFORM message construct a DHCPACK message with any local
	 * configuration parameters appropriate for the client without:
	 * allocating a new address, checking for an existing binding, filling
	 * in 'yiaddr' or including lease time parameters.  The servers SHOULD
	 * unicast the DHCPACK reply to the address given in the 'ciaddr' field
	 * of the DHCPINFORM message.
	 * ...
	 * The server responds to a DHCPINFORM message by sending a DHCPACK
	 * message directly to the address given in the 'ciaddr' field
	 * of the DHCPINFORM message.  The server MUST NOT send a lease
	 * expiration time to the client and SHOULD NOT fill in 'yiaddr'."
	 */
//TODO: do a few sanity checks: is ciaddr set?
//Better yet: is ciaddr == IP source addr?
	init_packet(&packet, oldpacket, DHCPACK);
	add_server_options(&packet);

	send_packet(s, &packet, /*force_bcast:*/ 0);
}

/* globals */
struct dyn_lease *g_leases = NULL;
/* struct server_config_t server_config is in bb_common_bufsiz1 */

#if UDHCPD

static void ICACHE_FLASH_ATTR udhcpd_start_task(void *pvParameters)
{
	int server_socket = -1, retval, max_sock;
	uint8_t *state;
	unsigned timeout_end;
	unsigned num_ips;
	unsigned opt;
	struct option_set *option;
	char *str_I = str_I;
	char* config_file;
	char *str_P;
	struct dhcp_packet *packet;
	int bytes;
	struct timeval tv;
	uint8_t *server_id_opt;
	uint8_t *requested_ip_opt;
	uint32_t requested_nip = requested_nip; /* for compiler */
	uint32_t static_lease_nip;
	struct dyn_lease *lease, fake_lease;

	/* Would rather not do read_config before daemonization -
	 * otherwise NOMMU machines will parse config twice */
//	read_config(config_file);
	config_init(&server_config);

	option = udhcp_find_option(server_config.options, DHCP_LEASE_TIME);

	if (option) {
		UDHCP_DEBUG("udhcp_find_option function return value %x\n", option);
		move_from_unaligned32(server_config.max_lease_sec, option->data + OPT_DATA);
		server_config.max_lease_sec = ntohl(server_config.max_lease_sec);
	} else
		server_config.max_lease_sec = DEFAULT_LEASE_TIME;

	/* Sanity check */
	num_ips = server_config.end_ip - server_config.start_ip + 1;
	if (server_config.max_leases > num_ips) {
		UDHCP_DEBUG("max_leases=%u is too big, setting to %u",
			(unsigned)server_config.max_leases, num_ips);
		server_config.max_leases = num_ips;
	}

	g_leases = (struct dyn_lease *)zalloc(server_config.max_leases * sizeof(g_leases[0]));
//	read_leases(server_config.lease_file);

	if (udhcp_read_interface(server_config.interface,
			&server_config.ifindex,
			(server_config.server_nip == 0 ? &server_config.server_nip : NULL),
			server_config.server_mac)
	) {
		retval = 1;
		return;
	}

	/* Setup the socket */
	server_socket = udhcp_listen_socket(/*INADDR_ANY,*/ SERVER_PORT,
			server_config.interface);

	if (server_socket < 0) {
		printf("set up socket fail\n");
	} else {
		packet = (struct dhcp_packet *)zalloc(sizeof(struct dhcp_packet));
		while (1) { /* loop until universe collapses */

			UDHCP_DEBUG("dhcp handsahke start %d.\n", server_socket);
			printf("udhcpd_task %d\n", uxTaskGetStackHighWaterMark(NULL));
			bytes = udhcp_recv_kernel_packet(packet, server_socket);
			log_info((uint8*)packet, bytes, LOGHEX);

			if (bytes < 0) {
				/* bytes can also be -2 ("bad packet data") */
				if (bytes == -1 && errno != EINTR) {
	//				UDHCP_DEBUG("Read error: %s, reopening socket", strerror(errno));
					close(server_socket);
					server_socket = -1;
				}
			}
			if (packet->hlen != 6) {
				UDHCP_DEBUG("MAC length != 6, ignoring packet");
			}
			if (packet->op != BOOTREQUEST) {
				UDHCP_DEBUG("not a REQUEST, ignoring packet");
			}

			/* Get message type if present */
			state = udhcp_get_option(packet, DHCP_MESSAGE_TYPE);
			if (state == NULL || state[0] < DHCP_MINTYPE || state[0] > DHCP_MAXTYPE) {
				UDHCP_DEBUG("no or bad message type option, ignoring packet");
			}

			/* Get SERVER_ID if present */
			server_id_opt = udhcp_get_option(packet, DHCP_SERVER_ID);
			if (server_id_opt) {
				uint32_t server_id_network_order;
				move_from_unaligned32(server_id_network_order, server_id_opt);
				if (server_id_network_order != server_config.server_nip) {
					/* client talks to somebody else */
					UDHCP_DEBUG("server ID doesn't match, ignoring");
				}
			}

			/* Look for a static/dynamic lease */
			static_lease_nip = get_static_nip_by_mac(server_config.static_leases, packet->chaddr);
			if (static_lease_nip) {
				UDHCP_DEBUG("Found static lease: %x", static_lease_nip);
				memcpy(&fake_lease.lease_mac, packet->chaddr, 6);
				fake_lease.lease_nip = static_lease_nip;
				fake_lease.expires = 0;
				lease = &fake_lease;
			} else {
				UDHCP_DEBUG("static lease not found, Find the lease that matches MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
					packet->chaddr[0], packet->chaddr[1],packet->chaddr[2],
					packet->chaddr[3],packet->chaddr[4],packet->chaddr[5]);
				lease = find_lease_by_mac(packet->chaddr);
			}

			/* Get REQUESTED_IP if present */
			requested_ip_opt = udhcp_get_option(packet, DHCP_REQUESTED_IP);
			if (requested_ip_opt) {
				move_from_unaligned32(requested_nip, requested_ip_opt);
			}

			switch (state[0]) {

				case DHCPDISCOVER:
					UDHCP_DEBUG("Received DISCOVER\n");

					send_offer(server_socket, packet, static_lease_nip, lease, requested_ip_opt);
					break;

				case DHCPREQUEST:
					UDHCP_DEBUG("Received REQUEST\n");

					if (!requested_ip_opt) {
						requested_nip = packet->ciaddr;
						if (requested_nip == 0) {
							UDHCP_DEBUG("no requested IP and no ciaddr, ignoring\n");
							break;
						}
					}
					if (lease && requested_nip == lease->lease_nip) {
						/* client requested or configured IP matches the lease.
						 * ACK it, and bump lease expiration time. */
						send_ACK(server_socket, packet, lease->lease_nip);
						break;
					}
					/* No lease for this MAC, or lease IP != requested IP */

					if (server_id_opt    /* client is in SELECTING state */
					 || requested_ip_opt /* client is in INIT-REBOOT state */
					) {
						/* "No, we don't have this IP for you" */
						send_NAK(server_socket, packet);
					} /* else: client is in RENEWING or REBINDING, do not answer */

					break;

				case DHCPDECLINE:

					UDHCP_DEBUG("Received DECLINE\n");
					if (server_id_opt
					 && requested_ip_opt
					 && lease  /* chaddr matches this lease */
					 && requested_nip == lease->lease_nip
					) {
						memset(lease->lease_mac, 0, sizeof(lease->lease_mac));
						lease->expires = time(NULL) + server_config.decline_time;
					}
					break;

				case DHCPRELEASE:
					/* "Upon receipt of a DHCPRELEASE message, the server
					 * marks the network address as not allocated."
					 *
					 * SERVER_ID must be present,
					 * REQUESTED_IP must not be present (we do not check this),
					 * chaddr must be filled in,
					 * ciaddr must be filled in
					 */
					UDHCP_DEBUG("Received RELEASE\n");
					if (server_id_opt
					 && lease  /* chaddr matches this lease */
					 && packet->ciaddr == lease->lease_nip
					) {
						lease->expires = time(NULL);
					}
					break;

				case DHCPINFORM:
					UDHCP_DEBUG("Received INFORM\n");
					send_inform(server_socket, packet);
					break;
			}
			UDHCP_DEBUG("message processing fineshed.\n");
		}
		free(packet);
	}

}

#else

static void ICACHE_FLASH_ATTR udhcpd_start_task(void *pvParameters)
{
	bool ValueToSend = true;
	bool ValueFromReceive = false;
	portBASE_TYPE xStatus;
	const portTickType xTickToWait = 100 / portTICK_RATE_MS;
//	struct sockaddr_in server_addr;
	int send_size;
	int recv_size;
	struct option_set *option = NULL;
	unsigned num_ips;
	uint8_t *state = NULL;
	uint8_t *requested_ip_opt = NULL;
	uint32_t requested_nip = 0;
	uint8_t *server_id_opt = NULL;
	uint32_t static_lease_nip;
	struct dyn_lease *lease, fake_lease;

	config_init(&server_config);

	/*setup the maximum lease time*/
	option = udhcp_find_option(server_config.options, DHCP_LEASE_TIME);
	if (option) {
		UDHCP_DEBUG("udhcp_find_option function return value %x\n", option);
		move_from_unaligned32(server_config.max_lease_sec, option->data + OPT_DATA);
		server_config.max_lease_sec = ntohl(server_config.max_lease_sec);
	} else
		server_config.max_lease_sec = DEFAULT_LEASE_TIME;

	/* Sanity check */
	num_ips = server_config.end_ip - server_config.start_ip + 1;
	if (server_config.max_leases > num_ips) {
		UDHCP_DEBUG("max_leases=%u is too big, setting to %u",
			(unsigned)server_config.max_leases, num_ips);
		server_config.max_leases = num_ips;
	}

	/*setup the server_mac and server_ip*/
	if (udhcp_read_interface(server_config.interface,
				&server_config.ifindex,
				(server_config.server_nip == 0 ? &server_config.server_nip : NULL),
				server_config.server_mac)
	) {
		return;
	}

	g_leases = (struct dyn_lease *)zalloc(server_config.max_leases * sizeof(g_leases[0]));

	int server_socket = udhcp_listen_socket(SERVER_PORT,NULL);

	if (server_socket < 0){

	} else {
		struct dhcp_packet *packet = (struct dhcp_packet *)zalloc(sizeof(struct dhcp_packet));
		for(;;) {
			xStatus = xQueueReceive(QueueStop,&ValueFromReceive,0);
			if (xStatus != pdPASS){
				UDHCP_DEBUG("Could not receive from the queue!\n");
			} else if (ValueFromReceive){
				break;
			}

			recv_size = udhcp_recv_kernel_packet(packet, server_socket);
			if (recv_size < 0){
				UDHCP_DEBUG("Server Recieve Data Failed!\n");
//				break;
			} else {
				UDHCP_DEBUG("recvfrom function recv data size %d %d\n", recv_size, sizeof(struct dhcp_packet));
				if (packet->hlen != 6) {
					UDHCP_DEBUG("MAC length != 6, ignoring packet");
				}
				if (packet->op != BOOTREQUEST) {
					UDHCP_DEBUG("not a REQUEST, ignoring packet");
				}

				/* Get message type if present */
				state = udhcp_get_option(packet, DHCP_MESSAGE_TYPE);
				if (state == NULL || state[0] < DHCP_MINTYPE || state[0] > DHCP_MAXTYPE) {
					UDHCP_DEBUG("no or bad message type option, ignoring packet");
				}

				/* Get SERVER_ID if present */
				server_id_opt = udhcp_get_option(packet, DHCP_SERVER_ID);
				if (server_id_opt) {
					uint32_t server_id_network_order;
					move_from_unaligned32(server_id_network_order, server_id_opt);
					if (server_id_network_order != server_config.server_nip) {
						/* client talks to somebody else */
						UDHCP_DEBUG("server ID doesn't match, ignoring\n");
					}
				}

				/* Look for a static/dynamic lease */
				static_lease_nip = get_static_nip_by_mac(server_config.static_leases, packet->chaddr);
				if (static_lease_nip) {
					UDHCP_DEBUG("Found static lease: %x", static_lease_nip);
					memcpy(&fake_lease.lease_mac, packet->chaddr, 6);
					fake_lease.lease_nip = static_lease_nip;
					fake_lease.expires = 0;
					lease = &fake_lease;
				} else {
					UDHCP_DEBUG("static lease not found, Find the lease that matches MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
							packet->chaddr[0], packet->chaddr[1],packet->chaddr[2],
							packet->chaddr[3],packet->chaddr[4],packet->chaddr[5]);
					lease = find_lease_by_mac(packet->chaddr);
				}

				/* Get REQUESTED_IP if present */
				requested_ip_opt = udhcp_get_option(packet, DHCP_REQUESTED_IP);
				if (requested_ip_opt) {
					move_from_unaligned32(requested_nip, requested_ip_opt);
				}

				switch(state[0]){
					case DHCPDISCOVER:
						UDHCP_DEBUG("Received DISCOVER\n");
						send_offer(server_socket, packet, static_lease_nip, lease, requested_ip_opt);
						send_size = safe_write(server_socket, &pdhcp_pkt, dhcp_pkt_len);
						if (send_size < 0)
							UDHCP_DEBUG("safe_write function send data failed\n");
						else
							UDHCP_DEBUG("safe_write function send data size %d\n", send_size);
						break;

					case DHCPREQUEST:
						UDHCP_DEBUG("Received REQUEST\n");
						if (!requested_ip_opt) {
							requested_nip = packet->ciaddr;
							if (requested_nip == 0) {
								UDHCP_DEBUG("no requested IP and no ciaddr, ignoring\n");
								break;
							}
						}
						if (lease && requested_nip == lease->lease_nip) {
							/* client requested or configured IP matches the lease.
							 * ACK it, and bump lease expiration time. */
							send_ACK(server_socket, packet, lease->lease_nip);
							send_size = safe_write(server_socket, &pdhcp_pkt, dhcp_pkt_len);
							if (send_size < 0)
								UDHCP_DEBUG("safe_write function send data failed\n");
							else
								UDHCP_DEBUG("safe_write function send data size %d\n", send_size);
							break;
						}

						/* No lease for this MAC, or lease IP != requested IP */
						if (server_id_opt    /* client is in SELECTING state */
						 || requested_ip_opt /* client is in INIT-REBOOT state */
						) {
							/* "No, we don't have this IP for you" */
							send_NAK(server_socket, packet);
							send_size = safe_write(server_socket, &pdhcp_pkt, dhcp_pkt_len);
							if (send_size < 0)
								UDHCP_DEBUG("safe_write function send data failed\n");
							else
								UDHCP_DEBUG("safe_write function send data size %d\n", send_size);
						} /* else: client is in RENEWING or REBINDING, do not answer */

						break;

					case DHCPDECLINE:
						UDHCP_DEBUG("Received DECLINE\n");
						if (server_id_opt
								 && requested_ip_opt
								 && lease  /* chaddr matches this lease */
								 && requested_nip == lease->lease_nip
							) {
							memset(lease->lease_mac, 0, sizeof(lease->lease_mac));
							lease->expires = time(NULL) + server_config.decline_time;
						}

						break;

					case DHCPRELEASE:
						UDHCP_DEBUG("Received RELEASE\n");
						if (server_id_opt
								 && lease  /* chaddr matches this lease */
								 && packet->ciaddr == lease->lease_nip
							) {
							lease->expires = time(NULL);
						}

						break;

					case DHCPINFORM:
						UDHCP_DEBUG("Received INFORM\n");
						send_inform(server_socket, packet);
						send_size = safe_write(server_socket, &pdhcp_pkt, dhcp_pkt_len);
						if (send_size < 0)
							UDHCP_DEBUG("safe_write function send data failed\n");
						else
							UDHCP_DEBUG("safe_write function send data size %d\n", send_size);

						break;

					default :
						break;
				}

			}

		}
		free(packet);
	}

	free(g_leases);
	close(server_socket);
	vQueueDelete(QueueStop);
	QueueStop = NULL;
	vTaskDelete(NULL);
}
#endif

void ICACHE_FLASH_ATTR udhcpd_start(void)
{
	if (QueueStop == NULL)
		QueueStop = xQueueCreate(1,1);

	if (QueueStop != NULL)
		xTaskCreate(udhcpd_start_task, "udhcpd_start", 512, NULL, 2, NULL);
}

sint8 ICACHE_FLASH_ATTR udhcpd_stop(void)
{
	bool ValueToSend = true;
	portBASE_TYPE xStatus;
	if (QueueStop == NULL)
		return -1;

	xStatus = xQueueSend(QueueStop,&ValueToSend,0);
	if (xStatus != pdPASS){
		UDHCP_DEBUG("Could not send to the queue!\n");
		return -1;
	} else {
		taskYIELD();
		return pdPASS;
	}
}
