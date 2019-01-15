/* coap_io_lwip.h -- Network I/O functions for libcoap on lwIP
 *
 * Copyright (C) 2012,2014 Olaf Bergmann <bergmann@tzi.org>
 *               2014 chrysn <chrysn@fsfe.org>
 *
 * This file is part of the CoAP library libcoap. Please see
 * README for terms of use. 
 */

#include "coap_config.h"
#include "mem.h"
#include "coap_io.h"
#include <lwip/udp.h>

void coap_packet_populate_endpoint(coap_packet_t *packet, coap_endpoint_t *target)
{
	printf("FIXME no endpoint populated\n");
}
void coap_packet_copy_source(coap_packet_t *packet, coap_address_t *target)
{
	target->port = packet->srcport;
	memcpy(&target->addr, ip_current_src_addr(), sizeof(ip_addr_t));
}
void coap_packet_get_memmapped(coap_packet_t *packet, unsigned char **address, size_t *length)
{
	LWIP_ASSERT("Can only deal with contiguous PBUFs to read the initial details", packet->pbuf->tot_len == packet->pbuf->len);
	*address = packet->pbuf->payload;
	*length = packet->pbuf->tot_len;
}
void coap_free_packet(coap_packet_t *packet)
{
	if (packet->pbuf)
		pbuf_free(packet->pbuf);
	coap_free_type(COAP_PACKET, packet);
}

struct pbuf *coap_packet_extract_pbuf(coap_packet_t *packet)
{
	struct pbuf *ret = packet->pbuf;
	packet->pbuf = NULL;
	return ret;
}


/** Callback from lwIP when a package was received.
 *
 * The current implemntation deals this to coap_handle_message immedately, but
 * other mechanisms (as storing the package in a queue and later fetching it
 * when coap_read is called) can be envisioned.
 *
 * It handles everything coap_read does on other implementations.
 */
static void coap_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	coap_endpoint_t *ep = (coap_endpoint_t*)arg;
	coap_packet_t *packet = coap_malloc_type(COAP_PACKET, sizeof(coap_packet_t));
	/* this is fatal because due to the short life of the packet, never should there be more than one coap_packet_t required */
	LWIP_ASSERT("Insufficient coap_packet_t resources.", packet != NULL);
	packet->pbuf = p;
	packet->srcport = port;

	/** FIXME derive the context without changing endopint definition */
	coap_handle_message(ep->context, packet);

	coap_free_packet(packet);
}

coap_endpoint_t *coap_new_endpoint(const coap_address_t *addr, int flags) {
	coap_endpoint_t *result;
	err_t err;

	LWIP_ASSERT("Flags not supported for LWIP endpoints", flags == COAP_ENDPOINT_NOSEC);

	result = coap_malloc_type(COAP_ENDPOINT, sizeof(coap_endpoint_t));
	if (!result) return NULL;

	result->pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
	if (result->pcb == NULL) goto error;

	udp_recv(result->pcb, coap_recv, (void*)result);
	err = udp_bind(result->pcb, &addr->addr, addr->port);
	if (err) {
		udp_remove(result->pcb);
		goto error;
	}

	return result;

error:
	coap_free_type(COAP_ENDPOINT, result);
	return NULL;
}

void coap_free_endpoint(coap_endpoint_t *ep)
{
	udp_remove(ep->pcb);
	coap_free_type(COAP_ENDPOINT, ep);
}
