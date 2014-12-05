/* vi: set sw=4 ts=4: */
/*
 * Mostly stolen from: dhcpcd - DHCP client daemon
 * by Yoichi Hariguchi <yoichi@fore.com>
 *
 * Licensed under GPLv2, see file LICENSE in this source tree.
 */
#include "udhcp/common.h"
#include "udhcp/dhcpd.h"

#define HWTYPE_ETHERNET 1

struct arpMsg {
	/* Ethernet header */
	uint8_t  h_dest[6];     /* 00 destination ether addr */
	uint8_t  h_source[6];   /* 06 source ether addr */
	uint16_t h_proto;       /* 0c packet type ID field */

	/* ARP packet */
	uint16_t htype;         /* 0e hardware type (must be ARPHRD_ETHER) */
	uint16_t ptype;         /* 10 protocol type (must be ETH_P_IP) */
	uint8_t  hlen;          /* 12 hardware address length (must be 6) */
	uint8_t  plen;          /* 13 protocol address length (must be 4) */
	uint16_t operation;     /* 14 ARP opcode */
	uint8_t  sHaddr[6];     /* 16 sender's hardware address */
	uint8_t  sInaddr[4];    /* 1c sender's IP address */
	uint8_t  tHaddr[6];     /* 20 target's hardware address */
	uint8_t  tInaddr[4];    /* 26 target's IP address */
	uint8_t  pad[18];       /* 2a pad for min. ethernet payload (60 bytes) */
};

enum {
	ARP_MSG_SIZE = 0x2a
};

/* Returns 1 if no reply received */
int ICACHE_FLASH_ATTR arpping(uint32_t test_nip,
		const uint8_t *safe_mac,
		uint32_t from_ip,
		uint8_t *from_mac,
		const char *interface)
{
	return 1;
#if 0
	int timeout_ms;
	int s;

	int rv = 1;             /* "no reply received" yet */
	struct sockaddr addr;   /* for interface name */
	struct arpMsg arp;

	s = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
	if (s == -1) {
		UDHCP_DEBUG("can not create raw_socket\n");
		return -1;
	}

	if (setsockopt_broadcast(s) == -1) {
		UDHCP_DEBUG("can't enable bcast on raw socket\n");
		goto ret;
	}

	/* send arp request */
	memset(&arp, 0, sizeof(arp));
	memset(arp.h_dest, 0xff, 6);                    /* MAC DA */
	memcpy(arp.h_source, from_mac, 6);              /* MAC SA */
	arp.h_proto = htons(ETHTYPE_IP);                 /* protocol type (Ethernet) */
	arp.htype = htons(HWTYPE_ETHERNET);                /* hardware type */
	arp.ptype = htons(ETHTYPE_ARP);                    /* protocol type (ARP message) */
	arp.hlen = 6;                                   /* hardware address length */
	arp.plen = 4;                                   /* protocol address length */
	arp.operation = htons(ARP_REQUEST);           /* ARP op code */
	memcpy(arp.sHaddr, from_mac, 6);                /* source hardware address */
	memcpy(arp.sInaddr, &from_ip, sizeof(from_ip)); /* source IP address */
	/* tHaddr is zero-filled */                     /* target hardware address */
	memcpy(arp.tInaddr, &test_nip, sizeof(test_nip));/* target IP address */

	memset(&addr, 0, sizeof(addr));

	strncpy(addr.sa_data, interface, sizeof(addr.sa_data));
	if (sendto(s, &arp, sizeof(arp), 0, &addr, sizeof(addr)) < 0) {
		// TODO: error message? caller didn't expect us to fail,
		// just returning 1 "no reply received" misleads it.
		goto ret;
	}

	/* wait for arp reply, and check it */
	timeout_ms = 2000;
	do {
		typedef uint32_t aliased_uint32_t;
		int r;
		unsigned prevTime = monotonic_ms();

		r = safe_read(s, &arp, sizeof(arp));
		if (r < 0)
			break;

		UDHCP_DEBUG("sHaddr %02x:%02x:%02x:%02x:%02x:%02x\n",
			arp.sHaddr[0], arp.sHaddr[1], arp.sHaddr[2],
			arp.sHaddr[3], arp.sHaddr[4], arp.sHaddr[5]);

		if (r >= ARP_MSG_SIZE
			&& arp.operation == htons(ARP_REPLY)
			 /* don't check it: Linux doesn't return proper tHaddr (fixed in 2.6.24?) */
			 /* && memcmp(arp.tHaddr, from_mac, 6) == 0 */
			 && *(aliased_uint32_t*)arp.sInaddr == test_nip
		) {
				/* if ARP source MAC matches safe_mac
				 * (which is client's MAC), then it's not a conflict
				 * (client simply already has this IP and replies to ARPs!)
				 */
			if (!safe_mac || memcmp(safe_mac, arp.sHaddr, 6) != 0)
				rv = 0;
			else
				UDHCP_DEBUG("sHaddr == safe_mac\n");
			break;
		}

		timeout_ms -= (unsigned)monotonic_ms() - prevTime + 1;

		/* We used to check "timeout_ms > 0", but
		 * this is more under/overflow-resistant
		 * (people did see overflows here when system time jumps):
		 */
	} while ((unsigned)timeout_ms <= 2000);

 ret:
	close(s);
	UDHCP_DEBUG("%srp reply received for this address\n", rv ? "No a" : "A");
	return rv;
#endif
}
