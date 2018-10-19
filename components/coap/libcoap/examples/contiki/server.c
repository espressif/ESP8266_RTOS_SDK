/* coap-server.c -- Example CoAP server using Contiki and libcoap
 *
 * Copyright (C) 2011 Olaf Bergmann <bergmann@tzi.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "coap_config.h"

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"
#include "net/net-debug.h"

#include <string.h>

#include "debug.h"
#include "coap.h"

static coap_context_t *coap_context;

static clock_time_t clock_offset;
/* changeable clock base (see handle_put_time()) */
static clock_time_t my_clock_base = 0;
static coap_resource_t *time_resource = NULL; /* just for testing */

PROCESS(coap_server_process, "CoAP server process");
AUTOSTART_PROCESSES(&coap_server_process);
/*---------------------------------------------------------------------------*/
void
init_coap_server(coap_context_t **ctx) {
  coap_address_t listen_addr;
  uip_ipaddr_t gw_addr;

  assert(ctx);

  coap_set_log_level(LOG_DEBUG);
  
  coap_address_init(&listen_addr);
  listen_addr.port = UIP_HTONS(COAP_DEFAULT_PORT);

  uip_ip6addr(&listen_addr.addr, 0xaaaa, 0, 0, 0, 0, 0, 0, NODE_ADDR);
#ifndef CONTIKI_TARGET_MINIMAL_NET
  uip_ds6_prefix_add(&listen_addr.addr, 64, 0, 0, 0, 0);
#endif /* not CONTIKI_TARGET_MINIMAL_NET */
  
  uip_ds6_addr_add(&listen_addr.addr, 0, ADDR_MANUAL);

  /* set default route to gateway aaaa::1 */
  uip_ip6addr(&gw_addr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0x0001);
  uip_ds6_defrt_add(&gw_addr, 0);

  PRINTLLADDR(&uip_lladdr);
  printf("\r\n");
  PRINT6ADDR(&listen_addr.addr);
  printf("\r\n");

  *ctx = coap_new_context(&listen_addr);

  if (!*ctx) {
    coap_log(LOG_CRIT, "cannot create CoAP context\r\n");
  }
}

/*---------------------------------------------------------------------------*/
#ifndef min
# define min(a,b) ((a) < (b) ? (a) : (b))
#endif

void 
hnd_get_time(coap_context_t  *ctx, struct coap_resource_t *resource,
	     const coap_endpoint_t *local_interface,
	     coap_address_t *peer, coap_pdu_t *request, str *token, 
	     coap_pdu_t *response) {
  coap_opt_iterator_t opt_iter;
  coap_opt_t *option;
  unsigned char buf[40];
  size_t len;
  coap_tick_t now;
  coap_tick_t t;

  /* FIXME: return time, e.g. in human-readable by default and ticks
   * when query ?ticks is given. */

  /* if my_clock_base was deleted, we pretend to have no such resource */
  response->hdr->code = 
    my_clock_base ? COAP_RESPONSE_CODE(205) : COAP_RESPONSE_CODE(404);

  if (coap_find_observer(resource, peer, token)) {
    /* FIXME: need to check for resource->dirty? */
    coap_add_option(response, COAP_OPTION_OBSERVE, 
		    coap_encode_var_bytes(buf, ctx->observe), buf);
  }

  if (my_clock_base)
    coap_add_option(response, COAP_OPTION_CONTENT_FORMAT,
		    coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);

  coap_add_option(response, COAP_OPTION_MAXAGE,
	  coap_encode_var_bytes(buf, 0x01), buf);

  if (my_clock_base) {

    /* calculate current time */
    coap_ticks(&t);
    now = my_clock_base + (t / COAP_TICKS_PER_SECOND);
    
    if (request != NULL
	&& (option = coap_check_option(request, COAP_OPTION_URI_QUERY, &opt_iter))
	&& memcmp(COAP_OPT_VALUE(option), "ticks",
		  min(5, COAP_OPT_LENGTH(option))) == 0) {
      /* output ticks */
      len = snprintf((char *)buf, 
	   min(sizeof(buf), response->max_size - response->length),
		     "%u", (unsigned int)now);
      coap_add_data(response, len, buf);

    }
  }
}

void
init_coap_resources(coap_context_t *ctx) {
  coap_resource_t *r;
#if 0
  r = coap_resource_init(NULL, 0, 0);
  coap_register_handler(r, COAP_REQUEST_GET, hnd_get_index);

  coap_add_attr(r, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  coap_add_attr(r, (unsigned char *)"title", 5, (unsigned char *)"\"General Info\"", 14, 0);
  coap_add_resource(ctx, r);
#endif
  /* store clock base to use in /time */
  my_clock_base = clock_offset;

  r = coap_resource_init((unsigned char *)"time", 4, 0);
  if (!r)
    goto error;

  r->observable = 1;
  time_resource = r;
  coap_register_handler(r, COAP_REQUEST_GET, hnd_get_time);
#if 0
  coap_register_handler(r, COAP_REQUEST_PUT, hnd_put_time);
  coap_register_handler(r, COAP_REQUEST_DELETE, hnd_delete_time);
#endif
  coap_add_attr(r, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  /* coap_add_attr(r, (unsigned char *)"title", 5, (unsigned char *)"\"Internal Clock\"", 16, 0); */
  coap_add_attr(r, (unsigned char *)"rt", 2, (unsigned char *)"\"Ticks\"", 7, 0);
  coap_add_attr(r, (unsigned char *)"if", 2, (unsigned char *)"\"clock\"", 7, 0);

  coap_add_resource(ctx, r);
#if 0
#ifndef WITHOUT_ASYNC
  r = coap_resource_init((unsigned char *)"async", 5, 0);
  coap_register_handler(r, COAP_REQUEST_GET, hnd_get_async);

  coap_add_attr(r, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  coap_add_resource(ctx, r);
#endif /* WITHOUT_ASYNC */
#endif

  return;
 error:
  coap_log(LOG_CRIT, "cannot create resource\n");
}

/* struct etimer notify_timer; */
struct etimer dirty_timer;

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(coap_server_process, ev, data)
{
  PROCESS_BEGIN();

  clock_offset = clock_time();
  init_coap_server(&coap_context);

  if (!coap_context) {
    coap_log(LOG_EMERG, "cannot create context\n");
    PROCESS_EXIT();
  }

  init_coap_resources(coap_context);

  if (!coap_context) {
    coap_log(LOG_EMERG, "cannot create context\n");
    PROCESS_EXIT();
  }

  /* etimer_set(&notify_timer, 5 * CLOCK_SECOND); */
  etimer_set(&dirty_timer, 30 * CLOCK_SECOND);

  while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
      coap_read(coap_context);	/* read received data */
      /* coap_dispatch(coap_context); /\* and dispatch PDUs from receivequeue *\/ */
    } else if (ev == PROCESS_EVENT_TIMER && etimer_expired(&dirty_timer)) {
      time_resource->dirty = 1;
      etimer_reset(&dirty_timer);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
